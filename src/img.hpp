#include <glm/glm.hpp>
#include <string>

void write_file(const std::string &file_desc, const glm::uvec2 &res,
                const uint8_t *raw);
bool file_exists(const std::string &file);
