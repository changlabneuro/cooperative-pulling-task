#include "gui.hpp"
#include "lever_system.hpp"
#include "serial.hpp"
#include <imgui.h>

namespace om {

gui::LeverGUIResult gui::render_lever_gui(const LeverGUIParams& params) {
  gui::LeverGUIResult result{};
  auto* lever_sys = params.lever_system;

  int force_lims[2]{params.force_limit0, params.force_limit1};
  if (ImGui::InputInt2("ForceLimits", force_lims, ImGuiInputTextFlags_EnterReturnsTrue)) {
    for (int li = 0; li < params.num_levers; li++) {
      auto lever = params.levers[li];
      int commanded_force = om::lever::get_commanded_force(lever_sys, lever);
      if (commanded_force < force_lims[0]) {
        om::lever::set_force(lever_sys, lever, force_lims[0]);
      } else if (commanded_force > force_lims[1]) {
        om::lever::set_force(lever_sys, lever, force_lims[1]);
      }
    }
    result.force_limit0 = force_lims[0];
    result.force_limit1 = force_lims[1];
  }

  for (int li = 0; li < params.num_levers; li++) {
    std::string tree_label{"Lever"};
    tree_label += std::to_string(li);
    const auto lever = params.levers[li];

    if (ImGui::TreeNode(tree_label.c_str())) {
      const bool pending_open = om::lever::is_pending_open(lever_sys, lever);
      const bool open = om::lever::is_open(lever_sys, lever);

      if (!pending_open && !open) {
        for (int i = 0; i < params.num_serial_ports; i++) {
          const auto& port = params.serial_ports[i];
          ImGui::Text("%s (%s)", port.port.c_str(), port.description.c_str());

          std::string button_label{"Connect"};
          button_label += std::to_string(i);

          if (ImGui::Button(button_label.c_str())) {
            om::lever::open_connection(lever_sys, lever, port.port);
          }
        }
      }

      if (open) {
        ImGui::Text("Connection is open.");
      } else {
        ImGui::Text("Connection is closed.");
      }

      if (auto state = om::lever::get_state(lever_sys, lever)) {
        auto str_state = om::to_string(state.value());
        ImGui::Text("%s", str_state.c_str());
      } else {
        ImGui::Text("Invalid state.");
      }

      if (auto force = om::lever::get_canonical_force(lever_sys, lever)) {
        ImGui::Text("Canonical force: %d", force.value());
      } else {
        ImGui::Text("Invalid force.");
      }

      int commanded_force = om::lever::get_commanded_force(lever_sys, lever);
      ImGui::SliderInt("SetForce", &commanded_force, force_lims[0], force_lims[1]);
      om::lever::set_force(lever_sys, lever, commanded_force);

      if (open) {
        if (ImGui::Button("Terminate serial context")) {
          om::lever::close_connection(lever_sys, lever);
        }
      }

      ImGui::TreePop();
    }
  }
  return result;
}

}
