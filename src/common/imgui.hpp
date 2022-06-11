#pragma once

#include <optional>

struct GLFWwindow;

namespace om {

struct ImguiContext {
  bool initialized;
};

std::optional<ImguiContext> create_imgui_context(GLFWwindow* window);
void new_frame(ImguiContext* context, int fb_width, int fb_height);
void render(ImguiContext* context);
void destroy_imgui_context(ImguiContext* context);

}