#pragma once

#include "glint/glint_core.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <vector>

// ---------------------------------------------------------------------------
// GPVuMeterSegmentColors / GPVuMeterConfig
// ---------------------------------------------------------------------------
struct GPVuMeterSegmentColors
{
  sk_color active;   // lit segment
  sk_color dim;      // unlit segment
  sk_color hold;     // peak-hold marker
};

struct GPVuMeterConfig
{
  int   numSegments   = 10;
  float segGap        = 1.f;   // px gap between segments
  float chGap         = 3.0f;   // px gap between L and R bars
  float minDb         = -40.f;  // dBFS at bottom of segment 0
  float holdDuration      = 1.5f;   // seconds before peak hold decays
  float decayDbPerSec     = 20.f;   // dB/s fall rate after hold expires
  float levelDecayDbPerSec = 20.f;  // dB/s fall rate of the main level bar

  // Per-segment colors, index 0 = bottom-most segment.
  // If fewer entries than numSegments, the last entry is repeated.
  std::vector<GPVuMeterSegmentColors> segments;

  // Default: 10-segment green / yellow / red
  static GPVuMeterConfig Default()
  {
    GPVuMeterConfig c;
    c.segments = {
      { "#2ecc4a", "#1e5c30", "#66ff99" },  // 0
      { "#2ecc4a", "#1e5c30", "#66ff99" },  // 1
      { "#2ecc4a", "#1e5c30", "#66ff99" },  // 2
      { "#2ecc4a", "#1e5c30", "#66ff99" },  // 3
      { "#f0c020", "#5a4a00", "#ffe066" },  // 4
      { "#f0c020", "#5a4a00", "#ffe066" },  // 5
      { "#f0c020", "#5a4a00", "#ffe066" },  // 6
      { "#f0c020", "#5a4a00", "#ffe066" },  // 7
      { "#ff3322", "#6b1a10", "#ff7766" },  // 8
      { "#ff3322", "#6b1a10", "#ff7766" },  // 9
    };
    return c;
  }
};

// ---------------------------------------------------------------------------
// GPVuMeter
// Stereo segmented VU meter with peak hold.
// ---------------------------------------------------------------------------
class GPVuMeter
{
public:
  explicit GPVuMeter(GPVuMeterConfig config = GPVuMeterConfig::Default())
    : mConfig(std::move(config))
    , mLevelL(0.f), mLevelR(0.f)
    , mHoldL(0.f),  mHoldR(0.f)
  {
    const auto now = std::chrono::steady_clock::now();
    mHoldTimeL    = now;
    mHoldTimeR    = now;
    mLastUpdate   = now;
  }

  // Call from the UI thread (e.g. 30 Hz timer).
  // peakL / peakR: linear amplitudes (0.0 = silence, 1.0 = 0 dBFS).
  void SetLevel(float peakL, float peakR)
  {
    const auto now = std::chrono::steady_clock::now();
    const float dt = std::chrono::duration<float>(now - mLastUpdate).count();
    mLastUpdate = now;

    const float rawL = std::max(0.f, peakL);
    const float rawR = std::max(0.f, peakR);

    mLevelL = smoothedLevel(mLevelL, rawL, dt);
    mLevelR = smoothedLevel(mLevelR, rawR, dt);

    // Hold tracks the raw (unsmoothed) peak for accuracy.
    updateHold(rawL, mHoldL, mHoldTimeL, now);
    updateHold(rawR, mHoldR, mHoldTimeR, now);
  }

  void Draw(glint_canvas& g, const glint_rect& rect) const
  {
    const int   n      = mConfig.numSegments;
    const float segGap = mConfig.segGap;
    const float chGap  = mConfig.chGap;

    //g.FillRect(glint_color(255, 12, 12, 12), rect);

    const float chW  = (rect.W() - chGap) * 0.5f;
    const float segH = (rect.H() - static_cast<float>(n - 1) * segGap) / static_cast<float>(n);

    const float levelLSeg = levelToSegment(mLevelL);
    const float levelRSeg = levelToSegment(mLevelR);
    const int   holdLIdx  = levelToHoldIdx(mHoldL);
    const int   holdRIdx  = levelToHoldIdx(mHoldR);

    for (int ch = 0; ch < 2; ++ch)
    {
      const float barL     = rect.L + static_cast<float>(ch) * (chW + chGap);
      const float barR     = barL + chW;
      const float levelSeg = (ch == 0) ? levelLSeg : levelRSeg;
      const int   holdIdx  = (ch == 0) ? holdLIdx  : holdRIdx;

      for (int seg = 0; seg < n; ++seg)
      {
        const float segT = rect.T + static_cast<float>(n - 1 - seg) * (segH + segGap);
        const glint_rect segRect(barL, segT, barR, segT + segH);

        const bool active = static_cast<float>(seg) < levelSeg;
        const GPVuMeterSegmentColors& col = segColors(seg);

        if (holdIdx > 0 && seg == holdIdx - 1 && !active)
          g.FillRect(col.hold, segRect);
        else if (active)
          g.FillRect(col.active, segRect);
        else
          g.FillRect(col.dim, segRect);
      }
    }
  }

  const GPVuMeterConfig& config() const { return mConfig; }
  GPVuMeterConfig&       config()       { return mConfig; }

private:
  GPVuMeterConfig mConfig;
  float mLevelL, mLevelR;
  float mHoldL,  mHoldR;
  std::chrono::steady_clock::time_point mHoldTimeL, mHoldTimeR;
  std::chrono::steady_clock::time_point mLastUpdate;

  const GPVuMeterSegmentColors& segColors(int seg) const
  {
    const auto& s = mConfig.segments;
    if (s.empty())
    {
      static const GPVuMeterSegmentColors kFallback{ "#2ecc4a", "#1e5c30", "#66ff99" };
      return kFallback;
    }
    const int idx = std::min(seg, static_cast<int>(s.size()) - 1);
    return s[idx];
  }

  // Returns the smoothed display level: instant attack, rate-limited decay.
  float smoothedLevel(float current, float target, float dt) const
  {
    if (target >= current)
      return target;  // instant attack
    if (current <= 0.f)
      return 0.f;
    const float curDb    = linearToDb(current);
    const float decayedDb = curDb - mConfig.levelDecayDbPerSec * dt;
    if (decayedDb <= mConfig.minDb)
      return 0.f;
    return std::max(target, dbToLinear(decayedDb));
  }

  float linearToDb(float linear) const
  {
    if (linear < 1e-6f) return mConfig.minDb - 10.f;
    return 20.f * std::log10(linear);
  }

  float dbToLinear(float db) const
  {
    return std::pow(10.f, db / 20.f);
  }

  float levelToSegment(float linear) const
  {
    if (linear <= 0.f) return 0.f;
    const float db = linearToDb(linear);
    if (db <= mConfig.minDb) return 0.f;
    if (db >= 0.f)           return static_cast<float>(mConfig.numSegments);
    return (db - mConfig.minDb) / (-mConfig.minDb) * static_cast<float>(mConfig.numSegments);
  }

  int levelToHoldIdx(float linear) const
  {
    return std::min(static_cast<int>(levelToSegment(linear)), mConfig.numSegments);
  }

  void updateHold(float newLevel,
                  float& hold,
                  std::chrono::steady_clock::time_point& holdTime,
                  const std::chrono::steady_clock::time_point& now) const
  {
    if (newLevel >= hold)
    {
      hold     = newLevel;
      holdTime = now;
    }
    else
    {
      const float elapsed = std::chrono::duration<float>(now - holdTime).count();
      if (elapsed > mConfig.holdDuration)
      {
        const float decayedDb = linearToDb(hold) - (elapsed - mConfig.holdDuration) * mConfig.decayDbPerSec;
        hold = (decayedDb < mConfig.minDb) ? 0.f : dbToLinear(decayedDb);
      }
    }
  }
};
