#include "common/app.hpp"
#include "common/gui.hpp"
#include "training.hpp"
#include <imgui.h>

struct App;

void render_gui(App& app);
void state_tick(App& app);
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
    state_tick(*this);
  }

  int lever_force_limits[2]{0, 20};
  std::optional<om::audio::BufferHandle> debug_audio_buffer;
  om::Vec3f stim0_color{1.0f};
  om::Vec3f stim1_color{1.0f};
};

void setup(App& app) {
  auto buff_p = std::string{OM_RES_DIR} + "/sounds/piano-c.wav";
  app.debug_audio_buffer = om::audio::read_buffer(buff_p.c_str());
}

void render_gui(App& app) {
  ImGui::Begin("GUI");
  if (ImGui::Button("Refresh ports")) {
    app.ports = om::enumerate_ports();
  }

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

  ImGui::InputFloat3("Stim0Color", &app.stim0_color.x);
  ImGui::InputFloat3("Stim1Color", &app.stim1_color.x);

  ImGui::End();
}

void state_tick(App& app) {
  using namespace om;

  static int state{};
  static bool entry{true};
  static NewTrialState new_trial{};
  static DelayState delay{};

  switch (state) {
    case 0: {
      new_trial.play_sound_on_entry = app.debug_audio_buffer;
      new_trial.stim0_color = app.stim0_color;
      new_trial.stim1_color = app.stim1_color;
      auto nt_res = tick_new_trial(&new_trial, &entry);
      if (nt_res.finished) {
        state = 1;
        entry = true;
      }
      break;
    }
    case 1: {
      delay.total_time = 2.0f;
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

