#include "common/imgui.hpp"
#include "common/glfw.hpp"
#include "common/serial_lever.hpp"
#include "common/lever_system.hpp"
#include "common/render.hpp"
#include "common/audio.hpp"
#include "common/gui.hpp"
#include <imgui.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <array>

struct App {
  std::vector<om::PortDescriptor> ports;
  std::array<om::lever::SerialLeverHandle, 2> levers{};
  int lever_force_limits[2]{0, 20};

  std::optional<om::audio::BufferHandle> debug_audio_buffer;
};

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

  if (app.debug_audio_buffer) {
    if (ImGui::Button("PlaySound")) {
      om::audio::play_buffer(app.debug_audio_buffer.value(), 0.25f);
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

  auto* lever_sys = om::lever::get_global_lever_system();
  om::lever::initialize(lever_sys, 2, app->levers.data());

  glfwMakeContextCurrent(render_win.window);
  om::gfx::init_rendering();
  om::audio::init_audio();

  app->debug_audio_buffer = om::audio::read_buffer("C:\\Users\\nick\\source\\grove\\playground\\res\\audio\\choir-c.wav");
  const char* const im_p = "C:\\Users\\nick\\source\\grove\\playground\\res\\textures\\debug\\3xyhvXg.jpeg";
  //const char* const im_p = "C:\\Users\\nick\\source\\grove\\playground\\res\\textures\\experiment\\calla_leaves.png";
  const auto im = om::gfx::read_2d_image(im_p);

  while (!glfwWindowShouldClose(gui_win.window) && !glfwWindowShouldClose(render_win.window)) {
    glfwPollEvents();

    om::lever::update(lever_sys);
    {
      glfwMakeContextCurrent(gui_win.window);
      om::update_framebuffer_dimensions(&gui_win);
      om::new_frame(&imgui_context, gui_win.framebuffer_width, gui_win.framebuffer_height);
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

  om::audio::terminate_audio();
  om::gfx::terminate_rendering();
  om::lever::terminate(lever_sys);
  om::destroy_imgui_context(&imgui_context);
  om::destroy_glfw_context(&gui_win);
  om::destroy_glfw_context(&render_win);
  om::terminate_glfw();
  return 0;
}

