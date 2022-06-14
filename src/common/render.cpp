#include "render.hpp"
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <cassert>
#include <unordered_map>
#include <glad/glad.h>
#include <optional>
#include <iostream>

namespace om::gfx {

namespace {

struct Buffer {
  unsigned int handle{};
};

struct Vao {
  unsigned int handle{};
};

struct Texture {
  unsigned int handle{};
};

struct Program {
  unsigned int handle{};
};

struct ProgramHandle {
  uint32_t id;
};

struct VaoHandle {
  uint32_t id;
};

struct BufferHandle {
  uint32_t id;
};

struct ImageDrawable {
  TextureHandle texture;
  Vec2f scale;
  Vec2f offset;
};

struct QuadDrawable {
  Vec3f color;
  Vec2f scale;
  Vec2f offset;
};

struct {
  uint32_t next_buffer_handle_id{1};
  uint32_t next_vao_handle_id{1};
  uint32_t next_texture_handle_id{1};
  uint32_t next_program_handle_id{1};

  std::unordered_map<uint32_t, Buffer> buffers;
  std::unordered_map<uint32_t, Texture> textures;
  std::unordered_map<uint32_t, Vao> vaos;
  std::unordered_map<uint32_t, Program> programs;

  VaoHandle quad_vao{};
  ProgramHandle image_program{};
  ProgramHandle colored_quad_program{};

  std::vector<ImageDrawable> image_drawables;
  std::vector<QuadDrawable> quad_drawables;

  int framebuffer_width{};
  int framebuffer_height{};

  bool rendering_initialized{};
} globals;

unsigned int create_shader(GLenum type, const char* source) {
  auto shader = glCreateShader(type);

  glShaderSource(shader, 1, &source, nullptr);
  glCompileShader(shader);

  int success = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

  if (!success) {
    char info_log[1024];
    glGetShaderInfoLog(shader, 1024, nullptr, info_log);
    std::cerr << info_log << std::endl;
    assert(false);
    return 0;
  } else {
    return shader;
  }
}

ProgramHandle create_program(const char* vert, const char* frag) {
  Program prog{};
  prog.handle = glCreateProgram();

  auto vert_shader = create_shader(GL_VERTEX_SHADER, vert);
  auto frag_shader = create_shader(GL_FRAGMENT_SHADER, frag);

  glAttachShader(prog.handle, vert_shader);
  glAttachShader(prog.handle, frag_shader);
  glLinkProgram(prog.handle);

  {
    int success = 0;
    glGetProgramiv(prog.handle, GL_LINK_STATUS, &success);

    if (!success) {
      char info_log[4096];
      glGetProgramInfoLog(prog.handle, 4096, nullptr, info_log);
      std::cerr << info_log << std::endl;
      assert(false);
    }
  }

  glDeleteShader(vert_shader);
  glDeleteShader(frag_shader);

  ProgramHandle result{globals.next_program_handle_id++};
  globals.programs[result.id] = prog;
  return result;
}

VaoHandle create_vao() {
  Vao vao{};
  glGenVertexArrays(1, &vao.handle);

  VaoHandle result{globals.next_vao_handle_id++};
  globals.vaos[result.id] = vao;
  return result;
}

const char* get_quad_vert_source() {
  static const char* const vert = R"(
    #version 330 core
    layout (location = 0) in vec2 position;

    out vec2 v_uv;

    uniform vec2 u_scale;
    uniform vec2 u_offset;
    uniform float u_aspect;
    
    void main() {
      v_uv = position * 0.5 + 0.5;
      gl_Position = vec4(position / vec2(u_aspect, 1.0) * u_scale + u_offset, 0.0, 1.0);
    }
)";
  return vert;
}

ProgramHandle create_image_program() {
  const char* const frag = R"(
  #version 330 core
  in vec2 v_uv;

  out vec4 frag_color;

  uniform sampler2D u_image;
  uniform int u_flip;

  float linear_to_srgb(float c) {
    if (c <= 0.0031308) {
      return c * 12.92;
    } else {
      return 1.055 * pow(c, 1.0 / 2.4) - 0.055;
    }
  }

  vec3 linear_to_srgb(vec3 c) {
    c.x = linear_to_srgb(c.x);
    c.y = linear_to_srgb(c.y);
    c.z = linear_to_srgb(c.z);
    return c;
  }

  void main() {
   vec2 uv = v_uv;
    if (u_flip == 1) {
      uv.y = 1.0 - uv.y;
    }

    frag_color = vec4(linear_to_srgb(texture(u_image, uv).rgb), 1.0);
  }
)";

  return create_program(get_quad_vert_source(), frag);
}

ProgramHandle create_colored_quad_program() {
  const char* const frag = R"(
  #version 330 core
  in vec2 v_uv;

  out vec4 frag_color;

  uniform vec3 u_color;

  void main() {
    frag_color = vec4(u_color, 1.0);
  }
)";

  return create_program(get_quad_vert_source(), frag);
}

BufferHandle create_buffer() {
  Buffer buff{};
  glGenBuffers(1, &buff.handle);

  BufferHandle result{globals.next_buffer_handle_id++};
  globals.buffers[result.id] = buff;
  return result;
}

VaoHandle create_2d_quad() {
  const float data[12] = {
    -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 
    1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f
  };

  auto buff_handle = create_buffer();
  auto& buff = globals.buffers.at(buff_handle.id);

  auto vao_handle = create_vao();
  auto& vao = globals.vaos.at(vao_handle.id);

  glBindBuffer(GL_ARRAY_BUFFER, buff.handle);
  glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), data, GL_STATIC_DRAW);

  glBindVertexArray(vao.handle);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (void*) 0);
  glEnableVertexAttribArray(0);
  glBindVertexArray(0);

  return vao_handle;
}

Program* get_program(ProgramHandle handle) {
  return &globals.programs.at(handle.id);
}

Vao* get_vao(VaoHandle handle) {
  return &globals.vaos.at(handle.id);
}

Texture* get_texture(TextureHandle handle) {
  return &globals.textures.at(handle.id);
}

template <typename Drawable>
void set_scale_offset_uniforms(unsigned int handle, const Drawable& drawable) {
  auto scale_loc = glGetUniformLocation(handle, "u_scale");
  assert(scale_loc >= 0);
  glUniform2f(scale_loc, drawable.scale.x, drawable.scale.y);

  auto offset_loc = glGetUniformLocation(handle, "u_offset");
  assert(offset_loc >= 0);
  glUniform2f(offset_loc, drawable.offset.x, drawable.offset.y);
}

} //  anon

void init_rendering() {
  globals.quad_vao = create_2d_quad();
  globals.image_program = create_image_program();
  globals.colored_quad_program = create_colored_quad_program();
  globals.rendering_initialized = true;
}

void new_frame(int fb_width, int fb_height) {
  globals.framebuffer_width = fb_width;
  globals.framebuffer_height = fb_height;

  glViewport(0, 0, fb_width, fb_height);
  glClearColor(0, 0, 0, 1);
  glClearDepth(0.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  globals.image_drawables.clear();
  globals.quad_drawables.clear();
}

void submit_frame() {
  const float ar = float(globals.framebuffer_width) / float(globals.framebuffer_height);

  if (!globals.quad_drawables.empty()) {
    auto* prog = get_program(globals.colored_quad_program);
    glUseProgram(prog->handle);
    glBindVertexArray(get_vao(globals.quad_vao)->handle);
    glUniform1f(glGetUniformLocation(prog->handle, "u_aspect"), ar);

    for (auto& drawable : globals.quad_drawables) {
      set_scale_offset_uniforms(prog->handle, drawable);

      auto color_loc = glGetUniformLocation(prog->handle, "u_color");
      assert(color_loc >= 0);
      glUniform3f(color_loc, drawable.color.x, drawable.color.y, drawable.color.z);
      glDrawArrays(GL_TRIANGLES, 0, 6);
    }
  }

  if (!globals.image_drawables.empty()) {
    auto* prog = get_program(globals.image_program);
    glUseProgram(prog->handle);
    glBindVertexArray(get_vao(globals.quad_vao)->handle);
    glUniform1f(glGetUniformLocation(prog->handle, "u_aspect"), ar);
    glUniform1i(glGetUniformLocation(prog->handle, "u_flip"), 1);

    for (auto& drawable : globals.image_drawables) {
      set_scale_offset_uniforms(prog->handle, drawable);

      auto* tex = get_texture(drawable.texture);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, tex->handle);
      glUniform1i(glGetUniformLocation(prog->handle, "u_image"), 0);
      glDrawArrays(GL_TRIANGLES, 0, 6);
    }
  }

  assert(glGetError() == GL_NO_ERROR);
}

void terminate_rendering() {
  for (auto& [_, vao] : globals.vaos) {
    glDeleteVertexArrays(1, &vao.handle);
  }
  for (auto& [_, buff] : globals.buffers) {
    glDeleteBuffers(1, &buff.handle);
  }
  for (auto& [_, texture] : globals.textures) {
    glDeleteTextures(1, &texture.handle);
  }
  for (auto& [_, prog] : globals.programs) {
    glDeleteProgram(prog.handle);
  }

  globals.buffers.clear();
  globals.textures.clear();
  globals.vaos.clear();
  globals.programs.clear();
  globals.rendering_initialized = false;
}

TextureHandle create_2d_image(const void* data, int w, int h, int nc) {
  assert(globals.rendering_initialized);
  assert(nc == 3 || nc == 4);

  Texture tex{};
  glGenTextures(1, &tex.handle);

  TextureHandle result{globals.next_texture_handle_id++};
  globals.textures[result.id] = tex;

  glBindTexture(GL_TEXTURE_2D, tex.handle);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  auto interal_format = nc == 4 ? GL_SRGB8_ALPHA8 : GL_SRGB8;
  auto src_format = nc == 4 ? GL_RGBA : GL_RGB;

  if (nc == 3) {
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  }

  glTexImage2D(GL_TEXTURE_2D, 0, interal_format, w, h, 0, src_format, GL_UNSIGNED_BYTE, data);
  return result;
}

void draw_2d_image(TextureHandle tex, const Vec2f& scale, const Vec2f& offset) {
  ImageDrawable drawable{};
  drawable.texture = tex;
  drawable.offset = offset;
  drawable.scale = scale;
  globals.image_drawables.push_back(drawable);
}

void draw_quad(const Vec3f& color, const Vec2f& scale, const Vec2f& offset) {
  QuadDrawable drawable{};
  drawable.color = color;
  drawable.offset = offset;
  drawable.scale = scale;
  globals.quad_drawables.push_back(drawable);
}

std::optional<TextureHandle> read_2d_image(const char* filepath) {
  int w;
  int h;
  int nc;
  auto data = read_image(filepath, &w, &h, &nc);
  if (!data || (nc != 3 && nc != 4)) {
    return std::nullopt;
  } else {
    return create_2d_image(data.get(), w, h, nc);
  }
}

std::unique_ptr<unsigned char[]> read_image(const char* filepath, int* w, int* h, int* nc) {
  unsigned char* data = stbi_load(filepath, w, h, nc, 0);
  if (!data) {
    return nullptr;
  }

  const size_t data_size = size_t(*w * *h * *nc);
  auto data_copy = std::make_unique<unsigned char[]>(data_size);
  std::memcpy(data_copy.get(), data, data_size);
  stbi_image_free(data);
  return data_copy;
}

}