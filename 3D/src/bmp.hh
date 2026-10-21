#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Writer {

//==// Converting a 16 and 32 bit number into a set of 8 bits in little endians
auto little_endian_32_to_8(const std::uint32_t &value)
    -> std::vector<std::uint8_t>;
auto little_endian_16_to_8(const std::uint16_t &value)
    -> std::vector<std::uint8_t>;

auto write_bmp(const std::string &file_name,
               const std::vector<std::uint8_t> &pixel_buffer,
               const std::size_t &width, const std::size_t &height) -> bool;
} // namespace Writer

namespace Scaler {
class ImageScaler {
public:
  ImageScaler(std::pair<std::size_t, std::size_t> original_dimensions,
              std::size_t image_scale)
      : original_dimensions(original_dimensions), image_scale(image_scale),
        new_hoz_dim(original_dimensions.first * image_scale),
        new_vert_dim(original_dimensions.second * image_scale) {
    // The * 3 is specific to this implementation of Writer::write_bmp, which
    // expects an r,g,b separately for each pixel. Values are initialised to
    // zero
    alloced_pixel_buffer.resize(new_hoz_dim * new_vert_dim * 3, 0);
  }

  auto get_new_image_size() -> std::pair<std::size_t, std::size_t>;

  auto get_pixel_buffer() -> std::vector<std::uint8_t>;

  auto assign_scaled_pixel_colour(std::size_t pixel_num,
                                  std::array<std::uint8_t, 3> rgb) -> void;

private:
  std::pair<std::size_t, std::size_t> original_dimensions;
  std::size_t image_scale;
  std::size_t new_hoz_dim;
  std::size_t new_vert_dim;
  std::vector<std::uint8_t> alloced_pixel_buffer;
};
} // namespace Scaler
