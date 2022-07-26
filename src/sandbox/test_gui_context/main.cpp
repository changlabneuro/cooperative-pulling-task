#include "common/app.hpp"
#include "common/lever_gui.hpp"
#include "common/juice_pump_gui.hpp"
#include "common/lever_pull.hpp"
#include "common/common.hpp"
#include "common/juice_pump.hpp"
#include "training.hpp"
#include <imgui.h>
#include <Windows.h>


struct App;

void render_gui(App& app);
void task_update(App& app);
void setup(App& app);

struct App : public om::App {
  ~App() override = default;
  void setup() override {
    ::setup(*this);
  }
  void gui_update() override {
    render_gui(*this);
  }
  void task_update() override {
    ::task_update(*this);
  }
  

  int lever_force_limits[2]{0, 250};
  om::lever::PullDetect detect_pull[2]{};
  // float lever_position_limits[2]{25e3f, 33e3f};
  float lever_position_limits[4]{ 64e3f, 65e3f, 14e2f, 55e2f}; // lever 1 and lever 2 have different potentiometer ranges - WS 
  bool invert_lever_position[2]{true, false};
  
  bool allow_automated_juice_delivery{};

  float new_total_time{10.0f};
  om::Vec2f stim0_size{0.15f};
  om::Vec2f stim0_offset{-0.4f, 0.0f};
  om::Vec3f stim0_color{1.0f};
  om::Vec2f stim1_size{0.15f};
  om::Vec2f stim1_offset{0.4f, 0.0f};
  om::Vec3f stim1_color{1.0f};

  int trialnumber{ 0 };

  std::optional<om::audio::BufferHandle> debug_audio_buffer;
};

void setup(App& app) {
  //auto buff_p = std::string{OM_RES_DIR} + "/sounds/piano-c.wav";
  auto buff_p = std::string{ OM_RES_DIR } + "/sounds/beep-09.wav";
  app.debug_audio_buffer = om::audio::read_buffer(buff_p.c_str());

  const float dflt_rising_edge = 0.6f;
  const float dflt_falling_edge = 0.25f;
  app.detect_pull[0].rising_edge = dflt_rising_edge;
  app.detect_pull[1].rising_edge = dflt_rising_edge;
  app.detect_pull[0].falling_edge = dflt_falling_edge;
  app.detect_pull[1].falling_edge = dflt_falling_edge;
}

void render_lever_gui(App& app) {
  om::gui::LeverGUIParams gui_params{};
  gui_params.force_limit0 = app.lever_force_limits[0];
  gui_params.force_limit1 = app.lever_force_limits[1];
  gui_params.serial_ports = app.ports.data();
  gui_params.num_serial_ports = int(app.ports.size());
  gui_params.num_levers = int(app.levers.size());
  gui_params.levers = app.levers.data();
  gui_params.lever_system = om::lever::get_global_lever_system();
  auto gui_res = om::gui::render_lever_gui(gui_params);
  if (gui_res.force_limit0) {
    app.lever_force_limits[0] = gui_res.force_limit0.value();
  }
  if (gui_res.force_limit1) {
    app.lever_force_limits[1] = gui_res.force_limit1.value();
  }
}

void render_juice_pump_gui(App& app) {
  om::gui::JuicePumpGUIParams gui_params{};
  gui_params.serial_ports = app.ports.data();
  gui_params.num_ports = int(app.ports.size());
  gui_params.num_pumps = 2;
  gui_params.allow_automated_run = app.allow_automated_juice_delivery;
  auto res = om::gui::render_juice_pump_gui(gui_params);

  if (res.allow_automated_run) {
    app.allow_automated_juice_delivery = res.allow_automated_run.value();
  }
}

void render_gui(App& app) {
  const auto enter_flag = ImGuiInputTextFlags_EnterReturnsTrue;


  ImGui::Begin("GUI");
  if (ImGui::Button("Refresh ports")) {
    app.ports = om::enumerate_ports();
  }

  if (ImGui::Button("start the trial")) {
    app.start_render = true;
  }

  if (ImGui::Button("pause the trial")) {
    app.start_render = false;
  }


  render_lever_gui(app);

  if (ImGui::TreeNode("PullDetect")) {
    ImGui::InputFloat2("PositionLimits", app.lever_position_limits);

    auto& detect = app.detect_pull;
    if (ImGui::InputFloat("RisingEdge", &detect[0].rising_edge, 0.0f, 0.0f, "%0.3f", enter_flag)) {
      detect[1].rising_edge = detect[0].rising_edge;
    }
    if (ImGui::InputFloat("FallingEdge", &detect[0].falling_edge, 0.0f, 0.0f, "%0.3f", enter_flag)) {
      detect[1].falling_edge = detect[1].rising_edge;
    }

    ImGui::TreePop();
  }


  if (ImGui::TreeNode("Stim0")) {
    ImGui::InputFloat3("Color", &app.stim0_color.x);
    ImGui::InputFloat2("Offset", &app.stim0_offset.x);
    ImGui::InputFloat2("Size", &app.stim0_size.x);
    ImGui::TreePop();
  }
  if (ImGui::TreeNode("Stim1")) {
    ImGui::InputFloat3("Color", &app.stim1_color.x);
    ImGui::InputFloat2("Offset", &app.stim1_offset.x);
    ImGui::InputFloat2("Size", &app.stim1_size.x);
    ImGui::TreePop();
  }
  
  ImGui::End();

  ImGui::Begin("JuicePump");
  render_juice_pump_gui(app);
  ImGui::End();
}

float to_normalized(float v, float min, float max, bool inv) {
  if (min == max) {
    v = 0.0f;
  } else {
    v = om::clamp(v, min, max);
    v = (v - min) / (max - min);
  }
  return inv ? 1.0f - v : v;
}


void task_update(App& app) {
  using namespace om;

  static int state{};
  static bool entry{true};
  static NewTrialState new_trial{};
  static DelayState delay{};
  static InnerDelayState innerdelay{};


  for (int i = 0; i < 2; i++) {
    const auto lh = app.levers[i];
    auto& pd = app.detect_pull[i];
    if (auto lever_state = om::lever::get_state(om::lever::get_global_lever_system(), lh)) {
      om::lever::PullDetectParams params{};
      params.current_position = to_normalized(
        lever_state.value().potentiometer_reading,
        app.lever_position_limits[2*i],
        app.lever_position_limits[2*i+1],
        app.invert_lever_position[i]);
      auto pull_res = om::lever::detect_pull(&pd, params);
      if (pull_res.pulled_lever && app.debug_audio_buffer) {
      // if (pull_res.pulled_lever && app.debug_audio_buffer && state == 0) { // only pull during the trial, not the ITI (state == 0)  -WS
        //om::audio::play_buffer(app.debug_audio_buffer.value(), 0.25f);

        auto pump_handle = om::pump::ith_pump(i); // pump id: 0 - pump 1; 1 - pump 2  -WS
        float vol{};
        //om::pump::set_dispensed_volume(pump_handle, vol, om::pump::VolumeUnits(0));
        om::pump::run_dispense_program(pump_handle);

        state = 1;
        entry = true; // end the trial when the one of the juice is delivered  - WS
        break;
      }
    }
  }

  switch (state) {
    case 0: {
      new_trial.play_sound_on_entry = app.debug_audio_buffer;
      new_trial.total_time = app.new_total_time;
      new_trial.stim0_color = app.stim0_color;
      new_trial.stim0_offset = app.stim0_offset;
      new_trial.stim0_size = app.stim0_size;
      new_trial.stim1_color = app.stim1_color;
      new_trial.stim1_offset = app.stim1_offset;
      new_trial.stim1_size = app.stim1_size;

      //if (entry && app.allow_automated_juice_delivery) {
      //  auto pump_handle = om::pump::ith_pump(1); // pump id: 0 - pump 1; 1 - pump 2
      //  om::pump::run_dispense_program(pump_handle);
      //}

      // deliver juice when task cues are on; only used for training
      if (entry) {
        float vol1{0.1f};
        auto pump_handle1 = om::pump::ith_pump(0); // pump id: 0 - pump 1; 1 - pump 2
        //if (app.trialnumber == 0 && vol1 > 0) {
          om::pump::set_dispensed_volume(pump_handle1, vol1, om::pump::VolumeUnits(0));
          //innerdelay.total_time = 0.5f;
          //innerdelay.t0 = now();
          //float ttt = elapsed_time(innerdelay.t0, now());
          //while (elapsed_time(innerdelay.t0, now()) <= innerdelay.total_time) { 
           // continue; 
          //}
        //}
        om::pump::run_dispense_program(pump_handle1);
        //
        float vol2{0.1f};
        auto pump_handle2 = om::pump::ith_pump(1); // pump id: 0 - pump 1; 1 - pump 2
        //if (app.trialnumber == 0 && vol1 > 0) {
        //  om::pump::set_dispensed_volume(pump_handle2, vol2, om::pump::VolumeUnits(0));
        //}
        om::pump::run_dispense_program(pump_handle2);
      }

      auto nt_res = tick_new_trial(&new_trial, &entry);
      if (nt_res.finished) {
        state = 1;
        entry = true;
        app.trialnumber = app.trialnumber + 1;
      }


      break;
    }
    case 1: {
      delay.total_time = 2.0f; // 2.0f
      if (tick_delay(&delay, &entry)) {
        state = 0;
        entry = true;
      }
      break;
    }
    default: {
      assert(false);
    }
  }
}

int main(int, char**) {
  auto app = std::make_unique<App>();
  return app->run();
}

