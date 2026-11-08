#include <cassert>
#include <cstdio>
#include "ColorMath.h"

bool near(float a, float b, float tolerance = 0.6f) {
  return a > b - tolerance && a < b + tolerance;
}
void equals(bridge::RgbColor x, int r, int g, int b) {
  assert(x.r == r && x.g == g && x.b == b);
}

int main() {
  equals(bridge::homeColorToRgb(0, 100), 255, 0, 0);
  equals(bridge::homeColorToRgb(60, 100), 255, 255, 0);
  equals(bridge::homeColorToRgb(120, 100), 0, 255, 0);
  equals(bridge::homeColorToRgb(180, 100), 0, 255, 255);
  equals(bridge::homeColorToRgb(240, 100), 0, 0, 255);
  equals(bridge::homeColorToRgb(300, 100), 255, 0, 255);
  equals(bridge::homeColorToRgb(360, 100), 255, 0, 0);
  equals(bridge::homeColorToRgb(120, 0), 255, 255, 255);
  equals(bridge::homeColorToRgb(0, 50), 255, 128, 128);
  equals(bridge::homeColorToRgb(-100, 300), 255, 0, 0);
  equals(bridge::homeColorToRgb(800, -100), 255, 255, 255);
  auto yellow = bridge::wledRgbToHomeColor({255,255,0});
  assert(near(yellow.hue, 60) && near(yellow.saturation, 100));
  auto red = bridge::wledRgbToHomeColor({255,0,0});
  assert(near(red.hue,0) && near(red.saturation,100));
  auto green = bridge::wledRgbToHomeColor({0,255,0});
  assert(near(green.hue,120) && near(green.saturation,100));
  auto blue = bridge::wledRgbToHomeColor({0,0,255});
  assert(near(blue.hue,240) && near(blue.saturation,100));
  auto magenta = bridge::wledRgbToHomeColor({255,0,255});
  assert(near(magenta.hue,300) && near(magenta.saturation,100));
  auto warm = bridge::wledRgbToHomeColor({255,128,0});
  assert(near(warm.hue,30.1f,0.6f) && near(warm.saturation,100));
  assert(near(bridge::wledRgbToHomeColor({0,0,0}).saturation,0));
  assert(near(bridge::wledRgbToHomeColor({125,125,125}).saturation,0));
  for (float h=0;h<360;h+=13.5f) {
    for (float s=0;s<=100;s+=17) {
      auto rgb=bridge::homeColorToRgb(h,s);
      auto back=bridge::wledRgbToHomeColor(rgb);
      assert(near(back.saturation,s,0.6f));
      if (s > 0) {
        float difference=back.hue-h;
        if (difference > 180) difference -= 360;
        if (difference < -180) difference += 360;
        // 8-bit RGB quantization shifts a low-saturation hue by up to ~1.1 degrees.
        assert(near(difference,0,1.2f));
      }
    }
  }
  std::puts("HomeKit RGB/HSV color conversion passed");
}
