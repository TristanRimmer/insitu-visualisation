#include "implementation.hh"
#include "3D_slice_tracker.hh"
#include "3d_cuboid_renderer.hh"
#include "bmp.hh"

#include <mpi/mpi.h>

auto Implementation::per_insitu_frame(
    int simulation_iter, double *raw_data,
    const std::array<std::size_t, 3> &slice_indexes,
    const std::array<std::size_t, 3> &local_block_size,
    const std::array<std::size_t, 3> &displacement,
    const std::array<std::size_t, 3> &full_block_size, int rank, int num_ranks,
    Insitu::SliceTracker3D<double, 0> &x_tracker,
    Insitu::SliceTracker3D<double, 1> &y_tracker,
    Insitu::SliceTracker3D<double, 2> &z_tracker,
    RasteriserComponents::CameraPerspective perspective) -> void {

  auto x_domain_slicer = DomainSliceAccessor3D<double, 0>{
      x_tracker.get_2d_slice_iterator(raw_data)};
  auto y_domain_slicer = DomainSliceAccessor3D<double, 1>{
      y_tracker.get_2d_slice_iterator(raw_data)};
  auto z_domain_slicer = DomainSliceAccessor3D<double, 2>{
      z_tracker.get_2d_slice_iterator(raw_data)};

  auto slice_z = Implementation::communicate_slice_to_master_rank(
      z_domain_slicer, slice_indexes, local_block_size, displacement,
      full_block_size, rank, num_ranks);
  auto slice_x = Implementation::communicate_slice_to_master_rank(
      x_domain_slicer, slice_indexes, local_block_size, displacement,
      full_block_size, rank, num_ranks);
  auto slice_y = Implementation::communicate_slice_to_master_rank(
      y_domain_slicer, slice_indexes, local_block_size, displacement,
      full_block_size, rank, num_ranks);

  if (rank == 0) {
    constexpr auto CUBOID_RENDER_IMAGE_SIZE =
        std::pair<std::size_t, std::size_t>{1500, 1500};

    auto render_data = Implementation::CuboidRenderMetadata<double>(
        CUBOID_RENDER_IMAGE_SIZE.first, CUBOID_RENDER_IMAGE_SIZE.second,
        perspective, full_block_size, slice_indexes, "3d_cuboid_render", 0.0,
        full_block_size[0] * full_block_size[1] * full_block_size[2]);

    auto buffer = render_data.generate_buffer(
        slice_x, slice_y, slice_z,
        std::pair{full_block_size[2], full_block_size[1]},
        std::pair{full_block_size[0], full_block_size[2]},
        std::pair{full_block_size[0], full_block_size[1]}, true);

    Writer::write_bmp("3d_cuboid_buffer.bmp", buffer,
                      CUBOID_RENDER_IMAGE_SIZE.first,
                      CUBOID_RENDER_IMAGE_SIZE.second);
  }
}

auto Implementation::convert_sliced_data_to_mesh(
    const std::vector<double> &data,
    const std::pair<std::size_t, std::size_t> slice_dims,
    std::size_t hoz_output_slice,
    const Colours::ColourRange<double> &colour_range, std::string uuid)
    -> std::tuple<std::vector<std::uint8_t>,
                  std::pair<std::size_t, std::size_t>> {

  std::size_t pixel_index = 0;

  auto image_scaled = Scaler::ImageScaler(slice_dims, 1);

  for (auto value : data) {
    auto relevant_colour = colour_range.get_colour(value);

    image_scaled.assign_scaled_pixel_colour(
        pixel_index,
        {relevant_colour[2], relevant_colour[1], relevant_colour[0]});

    pixel_index++;
  }

  Writer::write_bmp("reconstructed_slice_" + uuid + ".bmp",
                    image_scaled.get_pixel_buffer(),
                    image_scaled.get_new_image_size().first,
                    image_scaled.get_new_image_size().second);

  return std::make_tuple(image_scaled.get_pixel_buffer(), slice_dims);
}

auto Implementation::print_rank_data(
    const std::array<std::size_t, 3> &slice_indexes,
    const std::array<std::size_t, 3> &local_block_size,
    const std::array<std::size_t, 3> &displacement,
    const std::array<std::size_t, 3> &full_block_size, int rank, int num_ranks,
    int target_rank) -> void {
  if (rank != target_rank)
    return;

  auto print_vec3 = [](const std::array<std::size_t, 3> &vec,
                       std::string tag) -> void {
    std::cout << "[" << tag << "]: {" << vec[0] << ", " << vec[1] << ", "
              << vec[2] << "}" << std::endl;
  };

  std::cout << "|=========| " << rank << " |=========|" << std::endl;
  print_vec3(slice_indexes, "Slice Indexes");
  print_vec3(local_block_size, "Local Block Size");
  print_vec3(displacement, "Displacement");
  print_vec3(full_block_size, "Global Block Size");
}
