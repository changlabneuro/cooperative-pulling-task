#include "common/imgui.hpp"
#include "common/glfw.hpp"
#include "common/serial_lever.hpp"
#include <imgui.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

struct SerialData {
  std::vector<om::PortDescriptor> ports;
  om::SerialContext context;
};

struct App {
  SerialData serial_data;
  om::LeverContext lever_context;
  bool had_serial_connect_error{};
};

void render_gui(App& app) {
  ImGui::Begin("GUI");
  if (ImGui::Button("Refresh ports")) {
    app.serial_data.ports = om::enumerate_ports();
  }

  for (int i = 0; i < int(app.serial_data.ports.size()); i++) {
    const auto& port = app.serial_data.ports[i];
    ImGui::Text("%s (%s)", port.port.c_str(), port.description.c_str());

    if (!om::is_open(app.serial_data.context)) {
      std::string button_label{"Connect"};
      button_label += std::to_string(i);

      if (ImGui::Button(button_label.c_str())) {
        auto serial_res = om::make_context(
          port.port, om::default_baud_rate(), om::default_read_write_timeout());
        if (serial_res) {
          app.serial_data.context = std::move(serial_res.value());
        } else {
          app.had_serial_connect_error = true;
        }
      }
    }
  }

  if (app.had_serial_connect_error) {
    ImGui::Text("Failed to connect.");
    if (ImGui::Button("Clear error")) {
      app.had_serial_connect_error = false;
    }
  }

  if (om::is_open(app.serial_data.context)) {
#if 1
    if (auto state = om::read_state(app.serial_data.context)) {
      auto str_state = om::to_string(state.value());
      ImGui::Text("%s", str_state.c_str());
    } else {
      ImGui::Text("Failed to read state.");
    }
#endif
#if 1
    ImGui::SliderInt("Force", &app.lever_context.commanded_force_grams, 1, 20);
    auto resp = om::set_force_grams(
      app.serial_data.context, app.lever_context.commanded_force_grams);
    if (resp) {
      ImGui::Text("Force response: %s", resp.value().c_str());
    } else {
      ImGui::Text("Failed to read force response");
    }
#endif

    if (ImGui::Button("Terminate serial context")) {
      app.serial_data.context = {};
    }
  }

  ImGui::End();
}

int main(int, char**) {
  auto app = std::make_unique<App>();

  if (!om::initialize_glfw()) {
    printf("Failed to initialize glfw.\n");
    return 0;
  }

  auto gui_win_res = om::create_glfw_context();
  if (!gui_win_res) {
    return 0;
  }

  auto render_win_res = om::create_glfw_context();
  if (!render_win_res) {
    return 0;
  }

  auto gui_win = gui_win_res.value();
  auto render_win = render_win_res.value();

  auto gui_res = om::create_imgui_context(gui_win.window);
  if (!gui_res) {
    om::terminate_glfw();
    om::destroy_glfw_context(&gui_win);
    om::destroy_glfw_context(&render_win);
    return 0;
  }

  auto imgui_context = gui_res.value();
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

  // Main loop
  while (!glfwWindowShouldClose(gui_win.window) && !glfwWindowShouldClose(render_win.window)) {
    glfwPollEvents();

    {
      glfwMakeContextCurrent(gui_win.window);
      om::update_framebuffer_dimensions(&gui_win);
      om::new_frame(&imgui_context);

      glViewport(0, 0, gui_win.framebuffer_width, gui_win.framebuffer_height);
      glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w,
                   clear_color.z * clear_color.w, clear_color.w);
      glClear(GL_COLOR_BUFFER_BIT);

      render_gui(*app);

      om::render(&imgui_context);
      glfwSwapBuffers(gui_win.window);
    }

    {
      glfwMakeContextCurrent(render_win.window);
      om::update_framebuffer_dimensions(&render_win);
      glViewport(0, 0, render_win.framebuffer_width, render_win.framebuffer_height);
      glClearColor(1, 0, 0, 1);
      glClear(GL_COLOR_BUFFER_BIT);
      glfwSwapBuffers(render_win.window);
    }
  }

  om::destroy_imgui_context(&imgui_context);
  om::destroy_glfw_context(&gui_win);
  om::destroy_glfw_context(&render_win);
  om::terminate_glfw();
  return 0;
}

