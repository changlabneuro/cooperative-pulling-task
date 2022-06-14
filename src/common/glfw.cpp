#include "glfw.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace om {

bool initialize_glfw() {
  return glfwInit();
}

void terminate_glfw() {
  glfwTerminate();
}

void destroy_glfw_context(GLFWContext* context) {
  if (context->window) {
    glfwDestroyWindow(context->window);
    context->window = nullptr;
  }
}

std::optional<GLFWContext> create_glfw_context() {
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only

  GLFWwindow* window = glfwCreateWindow(1280, 720, "", NULL, NULL);
  if (window == NULL) {
    return std::nullopt;
  }

  glfwMakeContextCurrent(window);
  gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
  glfwSwapInterval(1);

  GLFWContext result{};
  result.window = window;
  update_framebuffer_dimensions(&result);
  return result;
}

void update_framebuffer_dimensions(GLFWContext* context) {
  glfwGetFramebufferSize(
    context->window, &context->framebuffer_width, &context->framebuffer_height);
}

}
