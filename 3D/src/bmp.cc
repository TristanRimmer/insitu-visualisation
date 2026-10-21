#include "bmp.hh"
#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

auto Writer::little_endian_32_to_8(const std::uint32_t &value)
    -> std::vector<std::uint8_t> {
  auto returnValue = std::vector<std::uint8_t>{};

  returnValue.push_back(static_cast<std::uint8_t>((value) & 0xFF));
  returnValue.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
  returnValue.push_back(static_cast<std::uint8_t>((value >> 16) & 0xFF));
  returnValue.push_back(static_cast<std::uint8_t>((value >> 24) & 0xFF));

  return returnValue;
}
auto Writer::little_endian_16_to_8(const std::uint16_t &value)
    -> std::vector<std::uint8_t> {
  auto returnValue = std::vector<std::uint8_t>{};

  returnValue.push_back(static_cast<std::uint8_t>((value) & 0xFF));
  returnValue.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));

  return returnValue;
}

auto Writer::write_bmp(const std::string &file_name,
                       const std::vector<std::uint8_t> &pixel_buffer,
                       const std::size_t &width, const std::size_t &height)
    -> bool {
  auto file_bytes = std::vector<std::uint8_t>{};

  //==// Shorthands for adding 16 and 32 bit numbers to an 8 bit array
  auto add_32_to_array = [&](std::uint32_t value32) {
    auto vec = little_endian_32_to_8(value32);

    for (auto v : vec) {
      file_bytes.push_back(v);
    }
  };
  auto add_16_to_array = [&](std::uint16_t value16) {
    auto vec = little_endian_16_to_8(value16);

    for (auto v : vec) {
      file_bytes.push_back(v);
    }
  };
  // std::cout << " ADDING STUFF" << std::endl;

  //==// BM Header //==//
  file_bytes.push_back(0x42);                             // B
  file_bytes.push_back(0x4D);                             // M
  add_32_to_array(std::uint32_t{14} + std::uint32_t{40} + // File size
                  static_cast<std::uint32_t>(pixel_buffer.size()) * 4);
  add_32_to_array(std::uint32_t{0}); // The 4 bytes of reserved
  add_32_to_array(std::uint32_t{14} + std::uint32_t{40}); // Data offset

  //==// DIP Header //==//
  add_32_to_array(std::uint32_t{40}); // DIP header size
  add_32_to_array(static_cast<std::uint32_t>(
      static_cast<std::int32_t>(width))); // Image width (pixels)
  add_32_to_array(static_cast<std::uint32_t>(
      static_cast<std::int32_t>(-height))); // Image Height (pixels)
  add_16_to_array(std::uint16_t{1});        // Colour Planes
  add_16_to_array(std::uint16_t{32}); // Bits per Pixel: 3x8 BGR, 1x8 Padding
  add_32_to_array(std::uint32_t{0});  // Compression
  add_32_to_array(std::uint32_t{0});  // Raw bimap data size: No compression: 0
  add_32_to_array(std::uint32_t{0});  // Print resolution horiz
  add_32_to_array(std::uint32_t{0});  // Print resolution vert
  add_32_to_array(std::uint32_t{0});  // Pallete info (num colours)
  add_32_to_array(std::uint32_t{0});  // Pallete info (important colours)

  //==// Pixel data in BGR(P) Format //==//

  for (auto row = 0; row < height; row++) {
    auto inverted_row = height - (1 + row);
    for (auto col = std::size_t{0}; col < width; col++) {
      // R G B A -> B G R A
      // always (pixel*4) + 0, 1, 2, 3
      file_bytes.push_back(
          pixel_buffer[(inverted_row * 3 * width) + (col * 3) + 0]);
      file_bytes.push_back(
          pixel_buffer[(inverted_row * 3 * width) + (col * 3) + 1]);
      file_bytes.push_back(
          pixel_buffer[(inverted_row * 3 * width) + (col * 3) + 2]);
      file_bytes.push_back(255);
    }
  }

  //==// Open the file in binary-write and save //==//
  auto file = std::ofstream(file_name, std::ios::binary);
  if (!file)
    return false;

  // Interpret the data in memory as a list of chars, not a cast
  file.write(reinterpret_cast<const char *>(file_bytes.data()),
             file_bytes.size());
  file.close();

  return true;
}

namespace Scaler {

auto ImageScaler::get_new_image_size() -> std::pair<std::size_t, std::size_t> {
  return {new_hoz_dim, new_vert_dim};
}

auto ImageScaler::get_pixel_buffer() -> std::vector<std::uint8_t> {
  return alloced_pixel_buffer;
}

auto ImageScaler::assign_scaled_pixel_colour(std::size_t pixel_num,
                                             std::array<std::uint8_t, 3> rgb)
    -> void {
  // First establish the x and y component of pixel_num
  auto pixel_x = pixel_num % original_dimensions.first;
  auto pixel_y = pixel_num / original_dimensions.first;

  // Establish the base index in the new buffer
  auto base_index =
      pixel_y * (new_hoz_dim * 3 * image_scale) + (pixel_x * 3 * image_scale);

  for (int y = 0; y < image_scale; y++) {
    for (int x = 0; x < image_scale; x++) {
      alloced_pixel_buffer[(base_index + (y * 3 * new_hoz_dim) + (x * 3)) + 0] =
          rgb[0];
      alloced_pixel_buffer[(base_index + (y * 3 * new_hoz_dim) + (x * 3)) + 1] =
          rgb[1];
      alloced_pixel_buffer[(base_index + (y * 3 * new_hoz_dim) + (x * 3)) + 2] =
          rgb[2];
    }
  }
}

} // namespace Scaler
