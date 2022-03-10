#include "common/imgui.hpp"
#include "common/glfw.hpp"
#include "common/serial_lever.hpp"
#include "common/lever_system.hpp"
#include <imgui.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

struct App {
  std::vector<om::PortDescriptor> ports;
  om::lever::LeverSystem lever_system;
  om::lever::SerialLeverHandle lever0;
};

void render_gui(App& app) {
  ImGui::Begin("GUI");
  if (ImGui::Button("Refresh ports")) {
    app.ports = om::enumerate_ports();
  }

  for (int i = 0; i < int(app.ports.size()); i++) {
    const auto& port = app.ports[i];
    ImGui::Text("%s (%s)", port.port.c_str(), port.description.c_str());

    std::string button_label{"Connect"};
    button_label += std::to_string(i);

    if (ImGui::Button(button_label.c_str())) {
      om::lever::open_connection(&app.lever_system, app.lever0, port.port);
    }
  }

  if (auto state = om::lever::get_state(&app.lever_system, app.lever0)) {
    auto str_state = om::to_string(state.value());
    ImGui::Text("%s", str_state.c_str());
  } else {
    ImGui::Text("Invalid state.");
  }
  if (auto force = om::lever::get_canonical_force(&app.lever_system, app.lever0)) {
    ImGui::Text("Canonical force: %d", force.value());
  } else {
    ImGui::Text("Invalid force.");
  }

  int commanded_force = om::lever::get_commanded_force(&app.lever_system, app.lever0);
  ImGui::SliderInt("SetForce", &commanded_force, 1, 20);
  om::lever::set_force(&app.lever_system, app.lever0, commanded_force);

  ImGui::Text("Pending remote commands: %d", num_remote_commands(&app.lever_system));

  if (ImGui::Button("Terminate serial context")) {
    om::lever::close_connection(&app.lever_system, app.lever0);
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

  auto levers = om::lever::initialize(&app->lever_system, 2);
  app->lever0 = levers[0];

  // Main loop
  while (!glfwWindowShouldClose(gui_win.window) && !glfwWindowShouldClose(render_win.window)) {
    glfwPollEvents();

    om::lever::update(&app->lever_system);

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

  om::lever::terminate(&app->lever_system);
  om::destroy_imgui_context(&imgui_context);
  om::destroy_glfw_context(&gui_win);
  om::destroy_glfw_context(&render_win);
  om::terminate_glfw();
  return 0;
}

