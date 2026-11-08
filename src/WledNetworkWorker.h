#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include "RegistryPolicy.h"
#include "WorkerPolicy.h"
#include "WledClient.h"

// One HTTP worker task owns all periodic GETs and POSTs.
// HomeSpan callbacks only enqueue fixed-size commands and consume snapshots;
// they never wait for a WLED controller to respond.
class WledNetworkWorker {
public:
  struct Snapshot {
    WledState state;
    bool valid = false;
    bool online = false;
    bool lastCommandOK = true;
    uint32_t generation = 0;
    uint32_t lastSeenMs = 0;
    uint32_t lastCommandMs = 0;
    uint32_t consecutiveFailures = 0;
    uint32_t totalFailures = 0;
    uint32_t queueDrops = 0;
    uint16_t pendingCommands = 0;
  };

  WledNetworkWorker() = default;
  bool start();
  bool ready() const { return started_; }
  bool setHost(unsigned slot, const String &host);
  bool queueWrite(unsigned slot, bool changePower, bool power,
                  bool changeBrightness, int brightness);
  bool queuePreset(unsigned slot, uint8_t presetId);
  bool snapshot(unsigned slot, Snapshot &out) const;

private:
  enum class Kind : uint8_t { Write = 0, Preset = 1 };
  struct Command {
    uint8_t slot = 0;
    Kind kind = Kind::Write;
    bool changePower = false;
    bool power = false;
    bool changeBrightness = false;
    uint8_t brightness = 0;
    uint8_t preset = 0;
    uint32_t hostVersion = 0;
  };
  struct Slot {
    char host[48] = {};
    uint32_t hostVersion = 0;
    uint32_t commandEpoch = 0;
    uint32_t dueAtMs = 0;
    Snapshot snapshot;
  };

  mutable portMUX_TYPE guard_ = portMUX_INITIALIZER_UNLOCKED;
  Slot slots_[bridge::kMaxDevices];
  QueueHandle_t commands_ = nullptr;
  TaskHandle_t task_ = nullptr;
  bool started_ = false;

  bool enqueue(Command command);
  static void run(void *context);
  void taskLoop();
  void runCommand(const Command &command, WledClient &transport);
  bool pollSlot(unsigned slot, WledClient &transport);
};
