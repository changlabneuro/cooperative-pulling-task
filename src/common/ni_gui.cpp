#include "ni_gui.hpp"
#include <imgui.h>
#include <implot.h>

void om::gui::render_ni_gui(NIGUIData* gui, const ni::SampleBuffer* buffs, int num_sample_buffs) {
  constexpr int sample_history_size = 5000;
  gui->sample_history.reserve(sample_history_size);
  for (int i = 0; i < num_sample_buffs; i++) {
    gui->sample_history.push(buffs[i].data, buffs[i].num_samples_per_channel);
  }

  ImGui::Begin("NI");
  ImPlot::BeginPlot("TriggerChannel");

  ImPlot::PlotLine("Trigger", gui->sample_history.data.data(), gui->sample_history.size);

  ImPlot::EndPlot();
  ImGui::End();
}
