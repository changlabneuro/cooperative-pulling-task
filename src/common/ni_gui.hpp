#pragma once

#include "sample_queue.hpp"
#include "ni.hpp"

namespace om::gui {

struct NIGUIData {
  SampleQueue<double> sample_history;
  float led_voltage{ 5.0f };
};

void render_ni_gui(NIGUIData* gui, const ni::SampleBuffer* buffs, int num_sample_buffs);

}