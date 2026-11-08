#pragma once
#include <cstdint>

// Pure, platform-independent color math. HomeKit Hue is 0..360 degrees,
// Saturation 0..100 percent. WLED segment primary RGB uses 0..255.
namespace bridge {
struct RgbColor { uint8_t r = 255, g = 255, b = 255; };
struct HsvColor { float hue = 0.0f, saturation = 0.0f; };

inline float clampFloat(float v, float low, float high) {
  return v < low ? low : (v > high ? high : v);
}

inline uint8_t colorByte(float v) {
  if (!(v > 0.0f)) return 0;
  if (v >= 255.0f) return 255;
  return static_cast<uint8_t>(v + 0.5f);
}

inline RgbColor homeColorToRgb(float hue, float saturation) {
  // HomeKit sends bounded values, but clamp external/out-of-range inputs too.
  hue = clampFloat(hue, 0.0f, 360.0f);
  if (hue >= 360.0f) hue = 0.0f;
  const float chroma = 255.0f * clampFloat(saturation, 0.0f, 100.0f) / 100.0f;
  const float minimum = 255.0f - chroma;
  const float scaledHue = hue / 60.0f;
  const unsigned sector = static_cast<unsigned>(scaledHue);
  const float fractional = scaledHue - static_cast<float>(sector);
  const float secondary = chroma * ((sector % 2) ? (1.0f - fractional) : fractional);
  float r = 0.0f, g = 0.0f, b = 0.0f;
  switch (sector) {
    case 0: r = chroma; g = secondary; break;
    case 1: r = secondary; g = chroma; break;
    case 2: g = chroma; b = secondary; break;
    case 3: g = secondary; b = chroma; break;
    case 4: r = secondary; b = chroma; break;
    default: r = chroma; b = secondary; break;
  }
  return {colorByte(r + minimum), colorByte(g + minimum), colorByte(b + minimum)};
}

inline HsvColor wledRgbToHomeColor(RgbColor rgb) {
  int high = rgb.r, low = rgb.r;
  if (rgb.g > high) high = rgb.g;
  if (rgb.b > high) high = rgb.b;
  if (rgb.g < low) low = rgb.g;
  if (rgb.b < low) low = rgb.b;
  const int diff = high - low;
  if (high == 0 || diff == 0) return {0.0f, 0.0f};
  const float sat = static_cast<float>(diff) * 100.0f / high;
  float hue;
  if (high == rgb.r)
    hue = 60.0f * (static_cast<float>(static_cast<int>(rgb.g) - rgb.b) / diff);
  else if (high == rgb.g)
    hue = 60.0f * (static_cast<float>(static_cast<int>(rgb.b) - rgb.r) / diff + 2.0f);
  else
    hue = 60.0f * (static_cast<float>(static_cast<int>(rgb.r) - rgb.g) / diff + 4.0f);
  if (hue < 0.0f) hue += 360.0f;
  if (hue >= 360.0f) hue -= 360.0f;
  return {hue, sat};
}
} // namespace bridge
