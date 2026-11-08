#include "WledNetworkWorker.h"
#include <WiFi.h>
#include <cstring>
#include "PresetPolicy.h"

namespace {
constexpr uint32_t kIdleSleepMs = 25;
constexpr uint32_t kPollSpreadMs = 850;
}

bool WledNetworkWorker::setHost(unsigned slot, const String &host) {
  if (slot >= bridge::kMaxDevices || host.length() >= sizeof(slots_[slot].host))
    return false;
  portENTER_CRITICAL(&guard_);
  Slot &s = slots_[slot];
  if (strcmp(s.host, host.c_str()) != 0) {
    memcpy(s.host, host.c_str(), host.length() + 1);
    ++s.hostVersion;
    ++s.commandEpoch;   // A previous in-flight GET is now stale.
    s.snapshot.online = false;
    s.snapshot.valid = false;
    s.dueAtMs = millis() + 350 + slot * kPollSpreadMs;
  }
  portEXIT_CRITICAL(&guard_);
  return true;
}

bool WledNetworkWorker::start() {
  if (started_) return true;
  commands_ = xQueueCreate(bridge::kWorkerQueueLength, sizeof(Command));
  if (!commands_) {
    Serial.println("[Network] Cannot allocate WLED command queue");
    return false;
  }
  // Arduino on classic ESP32 runs HomeSpan in loopTask on core 1.
  // This worker uses core 0 and performs all routine blocking HTTP calls there.
  const BaseType_t outcome = xTaskCreatePinnedToCore(
      &WledNetworkWorker::run, "wled-http", 8192, this, 1, &task_, 0);
  if (outcome != pdPASS) {
    vQueueDelete(commands_);
    commands_ = nullptr;
    Serial.println("[Network] Cannot start WLED HTTP worker");
    return false;
  }
  started_ = true;
  Serial.println("[Network] Background WLED HTTP task started (core 0, 8 slots)");
  return true;
}

bool WledNetworkWorker::enqueue(Command command) {
  if (!started_ || !commands_ || command.slot >= bridge::kMaxDevices) return false;
  const unsigned slot = command.slot;
  portENTER_CRITICAL(&guard_);
  Slot &s = slots_[slot];
  if (!*s.host) {
    portEXIT_CRITICAL(&guard_);
    return false;
  }
  command.hostVersion = s.hostVersion;
  ++s.commandEpoch;
  ++s.snapshot.pendingCommands;
  portEXIT_CRITICAL(&guard_);

  if (xQueueSend(commands_, &command, 0) == pdTRUE) return true;
  portENTER_CRITICAL(&guard_);
  if (s.snapshot.pendingCommands) --s.snapshot.pendingCommands;
  ++s.snapshot.queueDrops;
  portEXIT_CRITICAL(&guard_);
  Serial.printf("[Network] Queue full: WLED slot %u; rejected control\n", slot);
  return false;
}

bool WledNetworkWorker::queueWrite(unsigned slot, bool changePower, bool power,
                                    bool changeBrightness, int brightness,
                                    bool changeColor, float hue, float saturation) {
  if (slot >= bridge::kMaxDevices || brightness < 0 || brightness > 100 ||
      !(hue >= 0.0f && hue <= 360.0f) ||
      !(saturation >= 0.0f && saturation <= 100.0f)) return false;
  if (!changePower && !changeBrightness && !changeColor) return true;
  Command cmd;
  cmd.slot = static_cast<uint8_t>(slot);
  cmd.kind = Kind::Write;
  cmd.changePower = changePower;
  cmd.power = power;
  cmd.changeBrightness = changeBrightness;
  cmd.brightness = static_cast<uint8_t>(brightness);
  cmd.changeColor = changeColor;
  cmd.hue = hue;
  cmd.saturation = saturation;
  return enqueue(cmd);
}

bool WledNetworkWorker::queuePreset(unsigned slot, uint8_t presetId) {
  if (slot >= bridge::kMaxDevices || !bridge::validPresetId(presetId))
    return false;
  Command cmd;
  cmd.slot = static_cast<uint8_t>(slot);
  cmd.kind = Kind::Preset;
  cmd.preset = presetId;
  return enqueue(cmd);
}

bool WledNetworkWorker::snapshot(unsigned slot, Snapshot &out) const {
  if (slot >= bridge::kMaxDevices) return false;
  portENTER_CRITICAL(&guard_);
  out = slots_[slot].snapshot;
  portEXIT_CRITICAL(&guard_);
  return true;
}

void WledNetworkWorker::run(void *context) {
  static_cast<WledNetworkWorker *>(context)->taskLoop();
}

void WledNetworkWorker::runCommand(const Command &cmd, WledClient &transport) {
  char host[48] = {};
  bool sameHost = false;
  portENTER_CRITICAL(&guard_);
  Slot &s = slots_[cmd.slot];
  sameHost = cmd.hostVersion == s.hostVersion && *s.host;
  if (sameHost) memcpy(host, s.host, sizeof(host));
  portEXIT_CRITICAL(&guard_);

  bool succeeded = false;
  if (sameHost && WiFi.status() == WL_CONNECTED) {
    transport.setHost(host);
    if (cmd.kind == Kind::Preset)
      succeeded = transport.applyPreset(cmd.preset);
    else {
      int mainSegment = -1;
      if (cmd.changeColor) {
        portENTER_CRITICAL(&guard_);
        // Host changes clear this snapshot; never reuse another host's ID.
        if (s.snapshot.valid) mainSegment = s.snapshot.state.mainSegmentId;
        portEXIT_CRITICAL(&guard_);
      }
      succeeded = transport.writeState(cmd.changePower, cmd.power,
                                      cmd.changeBrightness, cmd.brightness,
                                      cmd.changeColor, cmd.hue, cmd.saturation,
                                      mainSegment);
    }
  }
  const uint32_t now = millis();
  portENTER_CRITICAL(&guard_);
  if (s.snapshot.pendingCommands) --s.snapshot.pendingCommands;
  if (cmd.hostVersion == s.hostVersion) {
    s.snapshot.lastCommandOK = succeeded;
    s.snapshot.lastCommandMs = now;
    if (!succeeded) {
      s.snapshot.online = false;
      ++s.snapshot.totalFailures;
      ++s.snapshot.consecutiveFailures;
    }
    // After a successful POST let WLED settle, then read authoritative state.
    // After an error back off, but new Home commands still have priority.
    s.dueAtMs = now + (succeeded ? bridge::kWorkerWriteSettleMs
                                : bridge::kWorkerOfflineRetryMs);
  }
  portEXIT_CRITICAL(&guard_);
  Serial.printf("[Network] POST slot=%u kind=%s result=%s\n",
                static_cast<unsigned>(cmd.slot),
                cmd.kind == Kind::Preset ? "preset" : "state",
                succeeded ? "HTTP OK" : "HTTP FAILED");
}

bool WledNetworkWorker::pollSlot(unsigned slot, WledClient &transport) {
  char host[48] = {};
  uint32_t startedEpoch = 0;
  uint32_t startedHostVersion = 0;
  portENTER_CRITICAL(&guard_);
  Slot &s = slots_[slot];
  memcpy(host, s.host, sizeof(host));
  startedEpoch = s.commandEpoch;
  startedHostVersion = s.hostVersion;
  portEXIT_CRITICAL(&guard_);

  if (!*host) return false;
  transport.setHost(host);
  WledState observed;
  const bool success = WiFi.status() == WL_CONNECTED && transport.readState(observed);
  const uint32_t now = millis();

  portENTER_CRITICAL(&guard_);
  const bool fresh = bridge::isFreshRead(
      startedEpoch, s.commandEpoch, startedHostVersion,
      s.hostVersion, s.snapshot.pendingCommands);
  if (fresh) {
    s.snapshot.online = success;
    if (success) {
      s.snapshot.valid = true;
      s.snapshot.state = observed;
      s.snapshot.lastSeenMs = now;
      s.snapshot.consecutiveFailures = 0;
      ++s.snapshot.generation;
    } else {
      ++s.snapshot.totalFailures;
      ++s.snapshot.consecutiveFailures;
    }
  }
  // Stale GETs do not publish; after the queued write, we reconcile again.
  if (startedHostVersion == s.hostVersion) {
    s.dueAtMs = now + (!fresh ? bridge::kWorkerWriteSettleMs
                             : bridge::workerRetryInterval(success));
  }
  portEXIT_CRITICAL(&guard_);
  return success;
}

void WledNetworkWorker::taskLoop() {
  WledClient transport; // Owned exclusively by this task, never by HomeSpan.
  for (;;) {
    Command cmd;
    if (xQueueReceive(commands_, &cmd, 0) == pdTRUE) {
      runCommand(cmd, transport);
      continue;
    }
    if (WiFi.status() != WL_CONNECTED) {
      portENTER_CRITICAL(&guard_);
      for (auto &s : slots_) s.snapshot.online = false;
      portEXIT_CRITICAL(&guard_);
      vTaskDelay(pdMS_TO_TICKS(200));
      continue;
    }

    int readySlot = -1;
    const uint32_t now = millis();
    portENTER_CRITICAL(&guard_);
    for (unsigned i = 0; i < bridge::kMaxDevices; ++i) {
      const Slot &s = slots_[i];
      if (*s.host && s.snapshot.pendingCommands == 0
          && bridge::timeReached(now, s.dueAtMs)) {
        readySlot = static_cast<int>(i);
        break;
      }
    }
    portEXIT_CRITICAL(&guard_);
    if (readySlot >= 0) {
      pollSlot(static_cast<unsigned>(readySlot), transport);
    } else {
      vTaskDelay(pdMS_TO_TICKS(kIdleSleepMs));
    }
  }
}
