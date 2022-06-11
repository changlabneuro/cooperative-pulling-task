#pragma once

#include "identifier.hpp"
#include "vector.hpp"
#include <cstdint>
#include <memory>
#include <optional>

namespace om::gfx {

struct TextureHandle {
  OM_INTEGER_IDENTIFIER_EQUALITY(TextureHandle, id)
  uint32_t id;
};

void init_rendering();
void terminate_rendering();
void new_frame(int fb_width, int fb_height);
void submit_frame();

TextureHandle create_2d_image(const void* data, int w, int h, int nc);
void draw_2d_image(TextureHandle tex, const Vec2f& scale, const Vec2f& offset);
void draw_quad(const Vec3f& color, const Vec2f& scale, const Vec2f& offset);

std::unique_ptr<unsigned char[]> read_image(const char* filepath, int* w, int* h, int* nc);
std::optional<TextureHandle> read_2d_image(const char* filepath);

}