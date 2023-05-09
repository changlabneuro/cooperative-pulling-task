#include "ni_gui.hpp"
#include "ni.hpp"
#include <imgui.h>
#include <implot.h>

void om::gui::render_ni_gui(NIGUIData* gui, const ni::SampleBuffer* buffs, int num_sample_buffs) {
  constexpr int sample_history_size = 5000;
  gui->sample_history.reserve(sample_history_size);
  for (int i = 0; i < num_sample_buffs; i++) {
    gui->sample_history.push(buffs[i].data, buffs[i].num_samples_per_channel);
  }

  auto trigger_time_points = ni::read_trigger_time_points();

  ImGui::Begin("NI");

  if (ImGui::Button("TriggerPulse")) {
    om::ni::write_analog_pulse(0, 0.5f);
  }

  if (ImGui::TreeNode("StartTriggerTimePoints")) {
    for (int i = 0; i < std::min(16, int(trigger_time_points.size())); i++) {
      auto& tp = trigger_time_points[i];
      ImGui::Text("TriggerTimePoint: %0.3f (s) | %d (sample)", float(tp.elapsed_time), int(tp.sample_index));
    }
    ImGui::TreePop();
  }

  if (ImGui::TreeNode("VoltagePlot")) {
    ImPlot::BeginPlot("TriggerChannel");
    ImPlot::PlotLine("Trigger", gui->sample_history.data.data(), gui->sample_history.size);
    ImPlot::EndPlot();
    ImGui::TreePop();
  }

  ImGui::End();
}
