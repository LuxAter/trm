#include "img.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "prof.hpp"

void write_file(const std::string &file_desc, const glm::uvec2 &res,
                const uint8_t *raw) {
  PROF_FUNC("renderer", "file_desc", file_desc, "width", res.x, "height",
            res.y);
  stbi_flip_vertically_on_write(true);
  if (file_desc.ends_with(".png")) {
    stbi_write_png(file_desc.c_str(), res.x, res.y, 3, raw,
                   res.x * 3 * sizeof(uint8_t));
  } else if (file_desc.ends_with(".bmp")) {
    stbi_write_bmp(file_desc.c_str(), res.x, res.y, 3, raw);
  } else if (file_desc.ends_with(".tga")) {
    stbi_write_tga(file_desc.c_str(), res.x, res.y, 3, raw);
  } else if (file_desc.ends_with(".jpg")) {
    stbi_write_jpg(file_desc.c_str(), res.x, res.y, 3, raw, 75);
  }
}