#include "imgui.hpp"
#include <imgui.h>
#include <implot.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <glad/glad.h>

namespace om {

void new_frame(ImguiContext* context, int fb_width, int fb_height) {
  assert(context->initialized);
  (void) context;
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  glViewport(0, 0, fb_width, fb_height);
  glClearColor(0.45f, 0.55f, 0.6f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
}

void render(ImguiContext* context) {
  assert(context->initialized);
  (void) context;
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

std::optional<ImguiContext> create_imgui_context(GLFWwindow* window) {
  const char* glsl_version = "#version 150";

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  //ImGui::StyleColorsClassic();

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  ImPlot::CreateContext();

  ImguiContext result{};
  result.initialized = true;
  return result;
}

void destroy_imgui_context(ImguiContext* context) {
  if (context->initialized) {
    ImPlot::DestroyContext();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    context->initialized = false;
  }
}

}
