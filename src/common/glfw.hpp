#pragma once

#include <optional>

struct GLFWwindow;

namespace om {

struct GLFWContext {
  GLFWwindow* window;
  int framebuffer_width;
  int framebuffer_height;
};

bool initialize_glfw();
void terminate_glfw();

std::optional<GLFWContext> create_glfw_context();
void destroy_glfw_context(GLFWContext* context);
void update_framebuffer_dimensions(GLFWContext* context);

}