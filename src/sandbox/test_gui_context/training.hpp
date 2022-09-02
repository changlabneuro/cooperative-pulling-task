#pragma once

#include "common/vector.hpp"
#include "common/time.hpp"
#include "common/audio.hpp"
#include "common/render.hpp"

namespace om {

struct NewTrialState {
  TimePoint t0;
  float total_time{20.0f};
  Vec2f stim0_size{0.25f};
  Vec2f stim0_offset{-0.25f, 0.0f};
  Vec3f stim0_color{1.0f};
  std::optional<gfx::TextureHandle> stim0_image;
  Vec2f stim1_size{0.25f};
  Vec2f stim1_offset{0.25f, 0.0f};
  Vec3f stim1_color{1.0f};
  std::optional<gfx::TextureHandle> stim1_image;
  std::optional<audio::BufferHandle> play_sound_on_entry;
};

struct NewTrialResult {
  bool finished;
};

struct DelayState {
  TimePoint t0;
  float total_time;
};

struct InnerDelayState {
  TimePoint t0;
  float total_time;
};

NewTrialResult tick_new_trial(NewTrialState* state, bool* entry);
bool tick_delay(DelayState* state, bool* entry);

}