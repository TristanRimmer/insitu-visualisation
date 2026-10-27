#include "implementation.hh"
#include "3D_slice_tracker.hh"
#include "3d_cuboid_renderer.hh"

#include <mpi/mpi.h>

auto Implementation::per_insitu_frame(
    int simulation_iter, double *raw_data,
    const std::array<std::size_t, 3> &slice_indexes,
    const std::array<std::size_t, 3> &local_block_size,
    const std::array<std::size_t, 3> &displacement,
    const std::array<std::size_t, 3> &full_block_size, int rank, int num_ranks,
    Insitu::SliceTracker3D<double, 0> &x_tracker,
    Insitu::SliceTracker3D<double, 1> &y_tracker,
    Insitu::SliceTracker3D<double, 2> &z_tracker) -> void {

  auto x_domain_slicer = DomainSliceAccessor3D<double, 0>{
      x_tracker.get_2d_slice_iterator(raw_data)};
  auto y_domain_slicer = DomainSliceAccessor3D<double, 1>{
      y_tracker.get_2d_slice_iterator(raw_data)};
  auto z_domain_slicer = DomainSliceAccessor3D<double, 2>{
      z_tracker.get_2d_slice_iterator(raw_data)};
  static auto colour_range = Colours::ColourRange<double>(0.0, 100 * 100 * 100);

  // std::cerr << rank << ":::::::-> " << displacement[0] << ", "
  //           << displacement[1] << ", " << displacement[2] << std::endl;

  // if (rank == 0)
  //   std::cout << "Z_SLICE" << std::endl;
  auto slice_z = Implementation::communicate_slice_to_master_rank(
      z_domain_slicer, slice_indexes, local_block_size, displacement,
      full_block_size, rank, num_ranks);

  // MPI_Barrier(MPI_COMM_WORLD);
  // if (rank == 0)
  //   std::cout << "X_SLICE" << std::endl;
  auto slice_x = Implementation::communicate_slice_to_master_rank(
      x_domain_slicer, slice_indexes, local_block_size, displacement,
      full_block_size, rank, num_ranks);

  // MPI_Barrier(MPI_COMM_WORLD);
  // if (rank == 0)
  //   std::cout << "Y_SLICE" << std::endl;
  auto slice_y = Implementation::communicate_slice_to_master_rank(
      y_domain_slicer, slice_indexes, local_block_size, displacement,
      full_block_size, rank, num_ranks);
  // MPI_Barrier(MPI_COMM_WORLD);
  // if (rank == 0)
  //   std::cout << "BARRIER" << std::endl;
  //
  if (rank == 0) {
    //   std::cout << "CONVERITNG TO MESH" << std::endl;

    auto mesh_x = Implementation::convert_sliced_data_to_mesh(
        slice_x, {full_block_size[2], full_block_size[1]}, 500, colour_range,
        "x");
    auto mesh_y = Implementation::convert_sliced_data_to_mesh(
        slice_y, {full_block_size[0], full_block_size[2]}, 500, colour_range,
        "y");
    auto mesh_z = Implementation::convert_sliced_data_to_mesh(
        slice_z, {full_block_size[0], full_block_size[1]}, 500, colour_range,
        "z");

    constexpr auto CUBOID_RENDER_IMAGE_SIZE =
        std::pair<std::size_t, std::size_t>{1500, 1500};
    constexpr auto PERSPECTIVE = RasteriserComponents::CameraPerspective{
        RasteriserComponents::CameraPerspectiveX::RIGHT,
        RasteriserComponents::CameraPerspectiveY::UP,
        RasteriserComponents::CameraPerspectiveZ::FRONT};

    auto faces = std::array<Renderer::Face<double>, 3>{
        Renderer::get_faces_of_cuboid<double>(full_block_size, slice_indexes,
                                              PERSPECTIVE)};
    auto full_faces =
        Renderer::get_full_dims_as_faces<double>(full_block_size, PERSPECTIVE);

    auto camera =
        Renderer::get_position_camera<double>(full_block_size, PERSPECTIVE);

    auto cuboid_pixel_buffer = Renderer::get_pixel_buffer(
        {CUBOID_RENDER_IMAGE_SIZE.first, CUBOID_RENDER_IMAGE_SIZE.second},
        camera, faces, full_faces,
        {std::get<0>(mesh_z), std::get<0>(mesh_x), std::get<0>(mesh_y)},
        {std::get<1>(mesh_z), std::get<1>(mesh_x), std::get<1>(mesh_y)},
        std::pair{slice_indexes, full_block_size}, PERSPECTIVE);

    std::string file_name =
        "3d_render_mpi_style_" + std::to_string(num_ranks) + ".bmp";

    Writer::write_bmp(file_name, cuboid_pixel_buffer,
                      CUBOID_RENDER_IMAGE_SIZE.first,
                      CUBOID_RENDER_IMAGE_SIZE.second);
  }
}

auto Implementation::calculate_local_slice_index(std::size_t gbl_slice,
                                                 std::size_t local_axis_size)
    -> std::size_t {
  std::size_t cpy_gbl_slice = gbl_slice;

  while (cpy_gbl_slice >= local_axis_size)
    cpy_gbl_slice = cpy_gbl_slice - local_axis_size;

  return cpy_gbl_slice;
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

  // if (slice_dims.first * slice_dims.second == data.size()) {
  //   std::cout << "POOPOO" << std::endl;
  // } else {
  //   std::cout << data.size() << " with " << slice_dims.first << "x"
  //             << slice_dims.second << std::endl;
  // }

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
