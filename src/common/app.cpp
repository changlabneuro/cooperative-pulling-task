#include "app.hpp"
#include "common/om.hpp"
#include <GLFW/glfw3.h>

namespace om {

int App::run() {
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
  om::lever::initialize(lever_sys, 2, levers.data());

  glfwMakeContextCurrent(render_win.window);
  om::gfx::init_rendering();
  om::audio::init_audio();

  setup();

  while (!glfwWindowShouldClose(gui_win.window) && !glfwWindowShouldClose(render_win.window)) {
    glfwPollEvents();

    om::lever::update(lever_sys);
    om::pump::submit_commands();
    {
      glfwMakeContextCurrent(gui_win.window);
      om::update_framebuffer_dimensions(&gui_win);
      om::new_frame(&imgui_context, gui_win.framebuffer_width, gui_win.framebuffer_height);

      gui_update();

      om::render(&imgui_context);
      glfwSwapBuffers(gui_win.window);
    }

    {
      glfwMakeContextCurrent(render_win.window);
      om::update_framebuffer_dimensions(&render_win);
      om::gfx::new_frame(render_win.framebuffer_width, render_win.framebuffer_height);

      task_update();

      om::gfx::submit_frame();
      glfwSwapBuffers(render_win.window);
    }
  }

  om::audio::terminate_audio();
  om::gfx::terminate_rendering();
  om::lever::terminate(lever_sys);
  om::pump::terminate_pump_system();
  om::destroy_imgui_context(&imgui_context);
  om::destroy_glfw_context(&gui_win);
  om::destroy_glfw_context(&render_win);
  om::terminate_glfw();
  return 0;
}

}
