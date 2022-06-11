#include "common/imgui.hpp"
#include "common/glfw.hpp"
#include "common/serial_lever.hpp"
#include "common/lever_system.hpp"
#include "common/render.hpp"
#include <imgui.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

struct App {
  std::vector<om::PortDescriptor> ports;
  om::lever::LeverSystem lever_system;
  std::array<om::lever::SerialLeverHandle, 2> levers{};
  int lever_force_limits[2]{0, 20};
};

void render_gui(App& app) {
  ImGui::Begin("GUI");
  if (ImGui::Button("Refresh ports")) {
    app.ports = om::enumerate_ports();
  }

  auto& force_lims = app.lever_force_limits;
  if (ImGui::InputInt2("ForceLimits", force_lims, ImGuiInputTextFlags_EnterReturnsTrue)) {
    for (auto& lever : app.levers) {
      int commanded_force = om::lever::get_commanded_force(&app.lever_system, lever);
      if (commanded_force < force_lims[0]) {
        om::lever::set_force(&app.lever_system, lever, force_lims[0]);
      } else if (commanded_force > force_lims[1]) {
        om::lever::set_force(&app.lever_system, lever, force_lims[1]);
      }
    }
  }

  for (int li = 0; li < int(app.levers.size()); li++) {
    std::string tree_label{"Lever"};
    tree_label += std::to_string(li);
    const auto lever = app.levers[li];

    if (ImGui::TreeNode(tree_label.c_str())) {
      const bool pending_open = om::lever::is_pending_open(&app.lever_system, lever);
      const bool open = om::lever::is_open(&app.lever_system, lever);

      if (!pending_open && !open) {
        for (int i = 0; i < int(app.ports.size()); i++) {
          const auto& port = app.ports[i];
          ImGui::Text("%s (%s)", port.port.c_str(), port.description.c_str());

          std::string button_label{"Connect"};
          button_label += std::to_string(i);

          if (ImGui::Button(button_label.c_str())) {
            om::lever::open_connection(&app.lever_system, lever, port.port);
          }
        }
      }

      if (open) {
        ImGui::Text("Connection is open.");
      } else {
        ImGui::Text("Connection is closed.");
      }

      if (auto state = om::lever::get_state(&app.lever_system, lever)) {
        auto str_state = om::to_string(state.value());
        ImGui::Text("%s", str_state.c_str());
      } else {
        ImGui::Text("Invalid state.");
      }

      if (auto force = om::lever::get_canonical_force(&app.lever_system, lever)) {
        ImGui::Text("Canonical force: %d", force.value());
      } else {
        ImGui::Text("Invalid force.");
      }

      int commanded_force = om::lever::get_commanded_force(&app.lever_system, lever);
      ImGui::SliderInt("SetForce", &commanded_force, force_lims[0], force_lims[1]);
      om::lever::set_force(&app.lever_system, lever, commanded_force);

      if (open) {
        if (ImGui::Button("Terminate serial context")) {
          om::lever::close_connection(&app.lever_system, lever);
        }
      }

      ImGui::TreePop();
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

  auto levers = om::lever::initialize(&app->lever_system, 2);
  app->levers[0] = levers[0];
  app->levers[1] = levers[1];

  glfwMakeContextCurrent(render_win.window);
  om::gfx::init_rendering();

  const char* const im_p = "C:\\Users\\nick\\source\\grove\\playground\\res\\textures\\debug\\3xyhvXg.jpeg";
  //const char* const im_p = "C:\\Users\\nick\\source\\grove\\playground\\res\\textures\\experiment\\calla_leaves.png";
  const auto im = om::gfx::read_2d_image(im_p);

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
      om::gfx::new_frame(render_win.framebuffer_width, render_win.framebuffer_height);
      om::gfx::draw_quad(1.0f, 1.0f, 1.0f, om::Vec2f{1.0f}, om::Vec2f{});
      om::gfx::draw_quad(1.0f, 0.0f, 1.0f, om::Vec2f{0.25f}, om::Vec2f{0.1f});
      if (im) {
        om::gfx::draw_2d_image(im.value(), om::Vec2f{0.25f}, om::Vec2f{});
      }

      om::gfx::submit_frame();
      assert(glGetError() == GL_NO_ERROR);

      glfwSwapBuffers(render_win.window);
    }
  }

  om::gfx::terminate_rendering();
  om::lever::terminate(&app->lever_system);
  om::destroy_imgui_context(&imgui_context);
  om::destroy_glfw_context(&gui_win);
  om::destroy_glfw_context(&render_win);
  om::terminate_glfw();
  return 0;
}

