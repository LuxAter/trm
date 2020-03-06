#include "img.hpp"

#define PATH_MAX_STRING_SIZE 256
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <dirent.h>
#include <errno.h>
#include <unistd.h>

#include "prof.hpp"

#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

bool mkdir_p(const char *dir, const mode_t mode) {
  char tmp[PATH_MAX_STRING_SIZE];
  char *p = NULL;
  struct stat sb;
  size_t len;

  len = strnlen(dir, PATH_MAX_STRING_SIZE);
  if (len == 0 || len == PATH_MAX_STRING_SIZE) {
    return false;
  }
  memcpy(tmp, dir, len);
  tmp[len] = '\0';

  if (tmp[len - 1] == '/') {
    tmp[len - 1] = '\0';
  }

  if (stat(tmp, &sb) == 0) {
    if (S_ISDIR(sb.st_mode)) {
      return true;
    }
  }

  for (p = tmp + 1; *p; p++) {
    if (*p == '/') {
      *p = 0;
      if (stat(tmp, &sb) != 0) {
        if (mkdir(tmp, mode) < 0) {
          return false;
        }
      } else if (!S_ISDIR(sb.st_mode)) {
        return false;
      }
      *p = '/';
    }
  }
  if (stat(tmp, &sb) != 0) {
    if (mkdir(tmp, mode) < 0) {
      return false;
    }
  } else if (!S_ISDIR(sb.st_mode)) {
    return false;
  }
  return true;
}

bool ends_with(const std::string &str, const std::string &suffix) {
  return str.size() >= suffix.size() &&
         str.compare(str.size() - suffix.size(), std::string::npos, suffix) ==
             0;
}

void write_file(const std::string &file_desc, const glm::uvec2 &res,
                const uint8_t *raw) {
  PROF_FUNC("renderer", "file_desc", file_desc, "width", res.x, "height",
            res.y);
  std::size_t dir_sep_pos = 0;
  if ((dir_sep_pos = file_desc.rfind('/')) != std::string::npos) {
    mkdir_p(file_desc.substr(0, dir_sep_pos).c_str(), 0777);
  }
  stbi_flip_vertically_on_write(true);
  if (ends_with(file_desc, "png")) {
    stbi_write_png(file_desc.c_str(), res.x, res.y, 3, raw,
                   res.x * 3 * sizeof(uint8_t));
  } else if (ends_with(file_desc, ".bmp")) {
    stbi_write_bmp(file_desc.c_str(), res.x, res.y, 3, raw);
  } else if (ends_with(file_desc, ".tga")) {
    stbi_write_tga(file_desc.c_str(), res.x, res.y, 3, raw);
  } else if (ends_with(file_desc, ".jpg")) {
    stbi_write_jpg(file_desc.c_str(), res.x, res.y, 3, raw, 75);
  }
}
bool file_exists(const std::string &file) {
  return access(file.c_str(), F_OK) != -1;
}
