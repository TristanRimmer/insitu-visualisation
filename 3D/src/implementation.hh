#ifndef INSITU_VISUALISATION_3D_IMPLEMENTATION_HH
#define INSITU_VISUALISATION_3D_IMPLEMENTATION_HH

#include "3D_slice_tracker.hh"
#include "3d_cuboid_renderer.hh"
#include "bmp.hh"
#include <array>
#include <cstdint>
#include <mpi/mpi.h>
#include <string>

namespace Implementation {

template <typename T> class CuboidRenderMetadata {
public:
  CuboidRenderMetadata(std::size_t output_x, std::size_t output_y,
                       RasteriserComponents::CameraPerspective perspective,
                       std::array<std::size_t, 3> full_block_size,
                       std::array<std::size_t, 3> slice_indexes,
                       std::string file_name, T minimum_colour_value,
                       T maximum_colour_value)
      : image_dimensions(output_x, output_y), camera_perspective(perspective),
        file_name(file_name), slice_indexes(slice_indexes),
        full_block_dims(full_block_size),
        sliced_faces(Renderer::get_faces_of_cuboid<T>(
            full_block_size, slice_indexes, perspective)),
        full_dim_faces(
            Renderer::get_full_dims_as_faces<T>(full_block_size, perspective)),
        camera(Renderer::get_position_camera<T>(full_block_size, perspective)),
        colour_range(Colours::ColourRange<T>(minimum_colour_value,
                                             maximum_colour_value)) {}

  auto generate_buffer(const std::vector<T> &x_slice,
                       const std::vector<T> &y_slice,
                       const std::vector<T> &z_slice,
                       std::pair<std::size_t, std::size_t> x_dims,
                       std::pair<std::size_t, std::size_t> y_dims,
                       std::pair<std::size_t, std::size_t> z_dims,
                       bool print_2d_images) -> std::vector<uint8_t> {
    // Convert slices to meshes
    auto mesh_x =
        this->convert_slice_to_mesh(x_slice, x_dims, print_2d_images, "x");
    auto mesh_y =
        this->convert_slice_to_mesh(y_slice, y_dims, print_2d_images, "y");
    auto mesh_z =
        this->convert_slice_to_mesh(z_slice, z_dims, print_2d_images, "z");

    // Generate the pixel buffer for the cuboid
    auto cuboid_buffer = Renderer::get_pixel_buffer(
        this->image_dimensions, this->camera, this->sliced_faces,
        this->full_dim_faces,
        // WARN: Not 100% sure why its ZXY rather than XYZ, should probably
        // remind myself
        {std::get<0>(mesh_z), std::get<0>(mesh_x), std::get<0>(mesh_y)},
        {std::get<1>(mesh_z), std::get<1>(mesh_x), std::get<1>(mesh_y)},
        {this->slice_indexes, this->full_block_dims}, this->camera_perspective);

    return cuboid_buffer;
  }

private:
  auto
  convert_slice_to_mesh(const std::vector<T> &data,
                        std::pair<std::size_t, std::size_t> slice_dimensions,
                        bool print_reconstructed_slice = false,
                        std::string uuid = "")
      -> std::tuple<std::vector<std::uint8_t>,
                    std::pair<std::size_t, std::size_t>> {
    std::size_t pixel_index = 0;
    auto image_scaled = Scaler::ImageScaler(slice_dimensions, 1);

    for (auto v : data) {
      auto relevant_colour = colour_range.get_colour(v);
      image_scaled.assign_scaled_pixel_colour(
          pixel_index,
          // BMP takes BGR
          {relevant_colour[2], relevant_colour[1], relevant_colour[0]});

      pixel_index++;
    }

    if (print_reconstructed_slice) {
      Writer::write_bmp("reconstructed_slice_" + uuid + ".bmp",
                        image_scaled.get_pixel_buffer(),
                        image_scaled.get_new_image_size().first,
                        image_scaled.get_new_image_size().second);
    }

    return std::make_tuple(image_scaled.get_pixel_buffer(), slice_dimensions);
  }

private:
  std::pair<std::size_t, std::size_t> image_dimensions;
  RasteriserComponents::CameraPerspective camera_perspective;
  std::string file_name;
  std::array<std::size_t, 3> slice_indexes;
  std::array<std::size_t, 3> full_block_dims;
  std::array<Renderer::Face<T>, 3> sliced_faces;
  std::array<Renderer::Face<T>, 3> full_dim_faces;
  RasteriserComponents::Camera<T> camera;
  Colours::ColourRange<T> colour_range;
};

/**
 * "per_insitu_frame"
 *
 * - This function tries to imitate the actions necessary in an OPS codebase to
 *   create a cuboid rendering of the whole block when using domain
 *   decomposition with MPI.
 *
 * - Gets the X/Y/Z Slice onto root rank via 3 calls of
 *   "communicate_slice_to_master_rank"
 *
 * - Gathers additional data about the slice and the preferred perspective setup
 *
 * - Renders the cuboid as one image
 */
auto per_insitu_frame(int simulation_iter, double *raw_data,
                      const std::array<std::size_t, 3> &slice_indexes,
                      const std::array<std::size_t, 3> &local_block_size,
                      const std::array<std::size_t, 3> &displacement,
                      const std::array<std::size_t, 3> &full_block_size,
                      int rank, int num_ranks,
                      Insitu::SliceTracker3D<double, 0> &x_tracker,
                      Insitu::SliceTracker3D<double, 1> &y_tracker,
                      Insitu::SliceTracker3D<double, 2> &z_tracker,
                      RasteriserComponents::CameraPerspective perspective)
    -> void;
/**
 *
 * "communicate_slice_to_master_rank"
 *
 * - The transfering of the full slice data to the master rank is done on a
 *   row-by-row basis due to the contiguous memory contraints that MPI_Gatherv
 *   has
 *
 * - An initial AllGatherv is done so every rank can figure out how many
 *   Gatherv calls the master rank is expecting
 *
 * - Then, per row, it calculates if it has any more data to contribute, and:
 *   - Sends to the root how many items it is contributing this row
 *   - Sends the displacement of this ranks data
 *   - With the above info, the row of data
 */
template <std::uint8_t Axis>
auto communicate_slice_to_master_rank(
    DomainSliceAccessor3D<double, Axis> &accessor,
    const std::array<std::size_t, 3> &slice_indexes,
    const std::array<std::size_t, 3> &local_block_size,
    const std::array<std::size_t, 3> &displacement,
    const std::array<std::size_t, 3> &full_block_size, int rank, int num_ranks)
    -> std::vector<double>;

/**
 * "convert_sliced_data_to_mesh"
 *
 * - Given the raw slice data, this function:
 *
 * - Scales the data by a decimal multiplier if the data is too big/small
 *
 * - Writes a "reconstructed_slice_*.bmp" file to demonstrate what the full,
 *   rebuilt slice looks like
 *
 * - Returns the data in the format which the `3d_cuboid_renderer` API expects
 */
auto convert_sliced_data_to_mesh(
    const std::vector<double> &data,
    const std::pair<std::size_t, std::size_t> slice_dims,
    std::size_t hoz_output_slice,
    const Colours::ColourRange<double> &colour_range, std::string uuid)
    -> std::tuple<std::vector<std::uint8_t>,
                  std::pair<std::size_t, std::size_t>>;

auto print_rank_data(const std::array<std::size_t, 3> &slice_indexes,
                     const std::array<std::size_t, 3> &local_block_size,
                     const std::array<std::size_t, 3> &displacement,
                     const std::array<std::size_t, 3> &full_block_size,
                     int rank, int num_ranks, int target_rank) -> void;

} // namespace Implementation

template <std::uint8_t Axis>
auto Implementation::communicate_slice_to_master_rank(
    DomainSliceAccessor3D<double, Axis> &accessor,
    const std::array<std::size_t, 3> &slice_indexes,
    const std::array<std::size_t, 3> &local_block_size,
    const std::array<std::size_t, 3> &displacement,
    const std::array<std::size_t, 3> &full_block_size, int rank, int num_ranks)
    -> std::vector<double> {
  static_assert(Axis == 0 || Axis == 1 || Axis == 2);

  // Some reusable components for gathers where each rank sends 1 item to index
  // [rank]
  auto slice_dimensions = std::pair{accessor.get_slice_dims()};
  const auto ranks_sends_one_item = std::vector<int>(num_ranks, 1);
  auto ranks_send_to_itself_index = std::vector<int>(num_ranks, 0);
  for (int r_i = 0; r_i < num_ranks; r_i++)
    ranks_send_to_itself_index[r_i] = r_i;
  auto row_counts_from_all_ranks = std::vector<int>(num_ranks, 0);

  const auto local_row_count = static_cast<int>(slice_dimensions.second);

  // All ranks now know the max amount of row gathers to do
  MPI_Allgatherv(&local_row_count, 1, MPI_INT, row_counts_from_all_ranks.data(),
                 ranks_sends_one_item.data(), ranks_send_to_itself_index.data(),
                 MPI_INT, MPI_COMM_WORLD);

  int number_of_row_gathers = 0;
  for (auto num_rows : row_counts_from_all_ranks)
    number_of_row_gathers =
        num_rows > number_of_row_gathers ? num_rows : number_of_row_gathers;

  // Check once if the slice index is out of its displacement
  const bool slice_lies_in_my_domain =
      (displacement[Axis] <= slice_indexes[Axis] &&
       slice_indexes[Axis] < displacement[Axis] + local_block_size[Axis]);

  // Treat the 'x' and 'y' index/axis internally as the X/Y of the 2D slice, not
  // referring to the block
  auto x_axis_index = (Axis == 0) ? 2 : 0;
  auto y_axis_index = (Axis == 0 || Axis == 2) ? 1 : 2;

  auto full_slice_x_axis = full_block_size[x_axis_index];
  auto full_slice_y_axis = full_block_size[y_axis_index];
  auto slice_displacement_x = displacement[x_axis_index];
  auto slice_displacement_y = displacement[y_axis_index];

  auto receive_buffer = std::vector<double>{};
  auto reusable_recvcounts = std::vector<int>{};
  auto reusable_displs = std::vector<int>{};

  // Only the master rank needs to have the memory capacity for these buffers
  if (rank == 0) {
    receive_buffer.resize(full_slice_x_axis * full_slice_y_axis, 0.0);
    reusable_recvcounts.resize(num_ranks, 0);
    reusable_displs.resize(num_ranks, 0);
  }
  auto *receive_buffer_ptr = (rank != 0) ? nullptr : receive_buffer.data();
  auto *recvcounts_ptr = (rank != 0) ? nullptr : reusable_recvcounts.data();
  auto *displs_ptr = (rank != 0) ? nullptr : reusable_displs.data();

  // Get the slice in the current block rather than the full block dimensions
  const auto local_slice_index = slice_indexes[Axis] - displacement[Axis];

  for (int row_index = 0; row_index < number_of_row_gathers; row_index++) {
    int current_displacement = 0;
    auto send_buffer = std::vector<double>{};
    bool row_index_has_exceeded_num_rows = row_index >= slice_dimensions.second;

    // Only needs to add row data to the send buffer if it contains the global
    // slice and it still has rows to submit
    if (slice_lies_in_my_domain and not row_index_has_exceeded_num_rows) {
      current_displacement =
          (slice_displacement_y + row_index) * full_slice_x_axis +
          slice_displacement_x;

      auto row_begin_and_end = accessor.get_row(row_index, local_slice_index);

      for (auto it = row_begin_and_end.first; it != row_begin_and_end.second;
           ++it) {
        send_buffer.push_back(*it);
      }
    }

    int size_as_primitive_int = static_cast<int>(send_buffer.size());

    // Gather the contributions
    MPI_Gatherv(&size_as_primitive_int, 1, MPI_INT, recvcounts_ptr,
                ranks_sends_one_item.data(), ranks_send_to_itself_index.data(),
                MPI_INT, 0, MPI_COMM_WORLD);
    // And the displacements
    MPI_Gatherv(&current_displacement, 1, MPI_INT, displs_ptr,
                ranks_sends_one_item.data(), ranks_send_to_itself_index.data(),
                MPI_INT, 0, MPI_COMM_WORLD);
    // Buffer
    MPI_Gatherv(send_buffer.data(), size_as_primitive_int, MPI_DOUBLE,
                receive_buffer_ptr, recvcounts_ptr, displs_ptr, MPI_DOUBLE, 0,
                MPI_COMM_WORLD);
  }

  return receive_buffer;
}
#endif
