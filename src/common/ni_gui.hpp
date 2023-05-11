#pragma once

#include "sample_queue.hpp"
#include "ni.hpp"

namespace om::led {
struct LEDSync;
}

namespace om::gui {

struct NIGUIData {
  SampleQueue<double> sample_history;
};

void render_ni_gui(NIGUIData* gui, const ni::SampleBuffer* buffs, int num_sample_buffs, om::led::LEDSync* sync);

}