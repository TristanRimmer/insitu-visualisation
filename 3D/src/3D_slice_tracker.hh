#ifndef INSITU_VISUALISATION_3D_SLICE_TRACKER_HH
#define INSITU_VISUALISATION_3D_SLICE_TRACKER_HH

#include "bmp.hh"
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>

namespace Colours {
using FixedColourPoints = std::array<std::array<std::uint8_t, 3>, 5>;
using AllColours = std::array<std::array<std::uint8_t, 3>, 255 * (4)>;

constexpr FixedColourPoints fixed_colour_points{
    {{0, 0, 0}, {0, 0, 255}, {0, 255, 0}, {255, 0, 0}, {255, 255, 255}}};

constexpr auto populate_colours_array(const FixedColourPoints &points)
    -> AllColours {
  AllColours colour_array{};

  for (int i = 0; i < points.size() - 1; i++) {
    // Ref to current points
    std::pair<const std::array<std::uint8_t, 3> &,
              const std::array<std::uint8_t, 3> &>
        curr_pair = {points[i], points[i + 1]};

    auto increment = std::array<int, 3>{
        static_cast<int>(curr_pair.second[0] - curr_pair.first[0]) / 255,
        static_cast<int>(curr_pair.second[1] - curr_pair.first[1]) / 255,
        static_cast<int>(curr_pair.second[2] - curr_pair.first[2]) / 255,
    };

    for (auto x = 0; x < 255; x++) {
      colour_array[(i * 255) + x] = {
          static_cast<std::uint8_t>(static_cast<int>(curr_pair.first[0]) +
                                    increment[0] * x),
          static_cast<std::uint8_t>(static_cast<int>(curr_pair.first[1]) +
                                    increment[1] * x),
          static_cast<std::uint8_t>(static_cast<int>(curr_pair.first[2]) +
                                    increment[2] * x),
      };
    }
  }

  return colour_array;
}

constexpr inline AllColours all_colours =
    populate_colours_array(fixed_colour_points);

template <typename T> class ColourRange {
  static_assert(std::is_arithmetic<T>::value,
                "Template argument T to SliceTracker3D must pass "
                "std::is_arithmetic_v<T>");

public:
  ColourRange(T lower_bound, T upper_bound)
      : lower(lower_bound), upper(upper_bound) {}

  auto get_colour(T &value) const -> std::array<std::uint8_t, 3> {
    auto divisor = std::max(T{1}, upper - lower);
    auto perc_through_range = value / divisor;

    perc_through_range = value < lower   ? 0
                         : value > upper ? 1
                                         : perc_through_range;

    auto relevant_colour =
        Colours::all_colours[perc_through_range * Colours::all_colours.size()];

    return relevant_colour;
  }

private:
  T lower;
  T upper;
};

} // namespace Colours
/*
// clang-format off
 * WARN: The axis is making the assumption that, on a cuboid shape where the
 * (0,0,0) coordinate is the left most bottom point.
 * From there, it assumes that the points go like this (assume a 3x3x3)
 * (0,0,0)
 * (1,0,0)
 * (2,0,0)
 * (3,0,0)
 * (0,1,0)
 * (1,1,0)
 * (2,1,0)
 * (3,1,0)
 * ...
 * (0,0,1)
 * (1,0,1)
 * (2,0,1)
 * (3,0,1)
//clang-format on
 */

// INFO: The template Axis parameter indicates that all points in that plane
// have the Same Index in the (Axis) direction
// (.., .., X)
// (.., .., X)
// (.., .., X)
// (.., .., X)
// (.., .., X)
// eg for fixed Z axis
template <typename T, std::uint8_t Axis> class CustomIterator {
  static_assert(std::is_arithmetic<T>::value,
                "Template argument T to SliceTracker3D must pass "
                "std::is_arithmetic_v<T>");
  static_assert(Axis == 0 || Axis == 1 || Axis == 2,
                "Axis Slice template parameter must be 0, 1 or 2");

public:
  CustomIterator(std::shared_ptr<T> data, std::array<std::size_t, 3> dimensions,
                 std::size_t slice_index, std::size_t local_x_start,
                 std::size_t local_y_start, std::size_t degrees_of_freedom = 1)
      : data(data), dimensions(dimensions), slice_index(slice_index),
        local_x(local_x_start), local_y(local_y_start),
        degrees_of_freedom(degrees_of_freedom) {};

  auto operator++() -> CustomIterator<T, Axis> & {
    // the Y index is never the local x
    auto this_local_x = (Axis == 0) ? 2 : 0;

    if (local_x + 1 >= dimensions[this_local_x]) {
      local_x = 0;
      local_y++;
    } else {
      local_x++;
    }
    return *this;
  };
  auto operator++(int) -> CustomIterator<T, Axis> {
    auto old = *this;
    ++(*this);
    return old;
  }
  auto operator*() const -> T & {
    return data.get()[this->get_flattened_index()];
  }
  auto operator!=(const CustomIterator<T, Axis> &oth) const -> bool {
    return (this->iter_match_data() != oth.iter_match_data());
  }
  auto operator==(const CustomIterator<T, Axis> &oth) const -> bool {
    return (this->iter_match_data() == oth.iter_match_data());
  };

  auto iter_match_data() const
      -> std::tuple<T const *, std::size_t, std::size_t, std::size_t> {
    return {data.get(), this->get_flattened_index(), local_x, local_y};
  }

private:
  auto get_flattened_index() const -> std::size_t {
    if constexpr (Axis == 0) {
      // The Local X will indicate the real Z component -> The correct plane
      // The Local Y will indicate the real Y component -> The correct line in
      //                                                   the plane
      // X is fixed -> Slice index gets correct point
      return degrees_of_freedom * ((local_x * (dimensions[0] * dimensions[1])) +
                                   (local_y * dimensions[0]) + slice_index);
    } else if constexpr (Axis == 1) {
      // The Local Y will indicate the real Z component -> The correct plane
      // Y is fixed -> Slice index gets the correct line in the plane
      // The Local X will indicate the real X component -> the correct point
      return degrees_of_freedom * ((local_y * (dimensions[0] * dimensions[1])) +
                                   (slice_index * dimensions[0]) + local_x);
    } else if constexpr (Axis == 2) {
      // The Local X will indicate the X component -> The point on line
      // The Local Y will indicate the Y component -> The line in the plane
      // Z is fixed -> Slice Index gets you to the correct plane
      return degrees_of_freedom *
             ((slice_index * (dimensions[0] * dimensions[1])) +
              (local_y * dimensions[0]) + local_x);
    }
  }

  const std::shared_ptr<T> data;
  const std::array<std::size_t, 3> dimensions;
  const std::size_t slice_index;
  // Local referring to the plane in which it is being sliced
  std::size_t local_x;
  std::size_t local_y;
  // These types have 'degrees of freedom'. For EG, a BMP's dofs is 4 (RGBA)
  std::size_t degrees_of_freedom;
};

/*
 * - Class will contain a iterator given a flattened 3D set
 * of points
 *
 * - The Axis template parameter specifies the axis which is being
 * sliced through
 *
 * for (auto value : DomainSliceAccessorObject1) {
 *    a routine can be called such as .get_local_coord() which will return a
 *    (relative) [x, y]
 * }￼
 */
template <typename T, std::uint8_t Axis> class DomainSliceAccessor3D {
  static_assert(std::is_arithmetic<T>::value,
                "Template argument T to SliceTracker3D must pass "
                "std::is_arithmetic_v<T>");
  static_assert(Axis == 0 || Axis == 1 || Axis == 2,
                "Axis Slice template parameter must be 0, 1 or 2");

public:
  DomainSliceAccessor3D(std::array<std::size_t, 3> block_size,
                        std::shared_ptr<T> flattened_data,
                        std::size_t target_slice)
      : block_size(block_size), flattened_data(flattened_data),
        target_slice(target_slice) {};

  auto begin() const -> const CustomIterator<T, Axis> {
    return CustomIterator<T, Axis>(flattened_data, block_size, target_slice, 0,
                                   0);
  }
  auto end() const -> const CustomIterator<T, Axis> {
    auto end_coord = std::pair{0, 0};
    if constexpr (Axis == 0) {
      end_coord = {0, block_size[1]};
    } else if constexpr (Axis == 1) {
      end_coord = {0, block_size[2]};
    } else if constexpr (Axis == 2) {
      end_coord = {0, block_size[1]};
    }
    return CustomIterator<T, Axis>(flattened_data, block_size, target_slice,
                                   end_coord.first, end_coord.second);
  }

  auto get_row(std::size_t slice_y, std::size_t slice)
      -> std::pair<CustomIterator<T, Axis>, CustomIterator<T, Axis>> {
    auto row_width = 0;
    if constexpr (Axis == 0) {
      row_width = block_size[0];
    } else if constexpr (Axis == 1) {
      row_width = block_size[2];
    } else if constexpr (Axis == 2) {
      row_width = block_size[0];
    }
    // 500|500|500 -> 500, 499,0
    std::cout << block_size[0] << "|" << block_size[1] << "|" << block_size[2]
              << " -> " << row_width << ", " << slice << "," << slice_y
              << std::endl;

    return std::pair{
        CustomIterator<T, Axis>(flattened_data, block_size, slice, 0, slice_y),
        CustomIterator<T, Axis>(flattened_data, block_size, slice, 0,
                                slice_y + 1)};
  }

  auto get_slice_dims() const -> std::pair<std::size_t, std::size_t> {
    if constexpr (Axis == 0) {
      return {block_size[2], block_size[1]};
    } else if constexpr (Axis == 1) {
      return {block_size[0], block_size[2]};
    } else if constexpr (Axis == 2) {
      return {block_size[0], block_size[1]};
    }
  }

private:
  std::array<std::size_t, 3> block_size;
  std::shared_ptr<T> flattened_data;
  std::size_t target_slice;
};

namespace Insitu {

template <typename T, std::uint8_t Axis> class SliceTracker3D {
  static_assert(std::is_arithmetic<T>::value,
                "Template argument T to SliceTracker3D must pass "
                "std::is_arithmetic_v<T>");
  static_assert(Axis == 0 || Axis == 1 || Axis == 2,
                "Axis Slice template parameter must be 0, 1 or 2");

public:
  SliceTracker3D(std::array<std::size_t, 3> block_size, std::size_t slice,
                 std::string output_files_header, std::size_t hoz_output_size,
                 std::size_t graph_every_n)
      : block_size(block_size), slice(slice),
        output_files_header(output_files_header),
        hoz_output_size(hoz_output_size), graph_every_n(graph_every_n),
        num_calls(0) {};

  auto generate_graph(T *flattened_data,
                      const Colours::ColourRange<T> &colour_range)
      -> std::optional<std::tuple<std::vector<std::uint8_t>,
                                  std::pair<std::size_t, std::size_t>>>;

  auto get_2d_slice_iterator(T *flattened_data)
      -> DomainSliceAccessor3D<T, Axis> {
    auto shared_data = std::shared_ptr<T>(flattened_data, [](T *) {});

    return DomainSliceAccessor3D<T, Axis>(block_size, shared_data, slice);
  }

private:
  std::array<std::size_t, 3> block_size;
  std::size_t slice;
  std::string output_files_header;
  std::size_t hoz_output_size;
  std::size_t graph_every_n;
  std::size_t num_calls;
};

template <typename T, std::uint8_t Axis>
auto SliceTracker3D<T, Axis>::generate_graph(
    T *flattened_block, const Colours::ColourRange<T> &colour_range)
    -> std::optional<std::tuple<std::vector<std::uint8_t>,
                                std::pair<std::size_t, std::size_t>>> {
  auto shared_data = std::shared_ptr<T>(flattened_block, [](T *) {});
  auto slice_accessor =
      DomainSliceAccessor3D<T, Axis>(block_size, shared_data, slice);
  auto slice_dims = slice_accessor.get_slice_dims();
  auto min_and_max = std::pair<T, T>{T{0}, T{0}};

  for (auto value : slice_accessor) {
    if (value < min_and_max.first)
      min_and_max.first = value;

    if (value > min_and_max.second)
      min_and_max.second = value;
  }

  std::size_t pixel_index = 0;

  auto image_scaled = Scaler::ImageScaler(slice_dims, hoz_output_size);

  for (auto value : slice_accessor) {
    auto relevant_colour = colour_range.get_colour(value);

    image_scaled.assign_scaled_pixel_colour(
        pixel_index,
        {relevant_colour[2], relevant_colour[1], relevant_colour[0]});

    pixel_index++;
  }

  Writer::write_bmp(output_files_header + std::to_string(num_calls) + ".bmp",
                    image_scaled.get_pixel_buffer(),
                    image_scaled.get_new_image_size().first,
                    image_scaled.get_new_image_size().second);

  std::cout << "Wrote state to "
            << output_files_header + std::to_string(num_calls) + ".bmp"
            << std::endl;

  num_calls++;
  return std::make_tuple(image_scaled.get_pixel_buffer(), slice_dims);
}

} // namespace Insitu
#endif
