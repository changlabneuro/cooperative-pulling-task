#include "training.hpp"
#include "common/render.hpp"

namespace om {

NewTrialResult tick_new_trial(NewTrialState* state, bool* entry) {
  NewTrialResult result{};
  if (*entry) {
    if (state->play_sound_on_entry) {
      audio::play_buffer(state->play_sound_on_entry.value(), 0.25f);
    }

    state->t0 = now();
    *entry = false;
  }

  gfx::draw_quad(state->stim0_color, state->stim0_size, state->stim0_offset);
  gfx::draw_quad(state->stim1_color, state->stim1_size, state->stim1_offset);
  if (elapsed_time(state->t0, now()) >= state->total_time) {
    result.finished = true;
  }

  return result;
}

bool tick_delay(DelayState* state, bool* entry) {
  if (*entry) {
    state->t0 = now();
    *entry = false;
  }
  return elapsed_time(state->t0, now()) >= state->total_time;
}

}
