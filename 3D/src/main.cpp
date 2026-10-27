#include "3D_slice_tracker.hh"
#include "3d_cuboid_renderer.hh"
#include "bmp.hh"
#include "implementation.hh"
#include "mpi.hh"
#include <array>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <ostream>
#include <random>
#include <vector>

#include <mpi/mpi.h>

int main(int argc, char *argv[]) {
  MPI_Init(&argc, &argv);

  int num_ranks;
  int rank;

  MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  constexpr auto dimensions = std::array<std::size_t, 3>{100, 100, 100};

  MpiUtils::PerRankDetails<double> my_details =
      MpiUtils::distribute_data_across_ranks<double>(rank, num_ranks,
                                                     dimensions);

  std::array<std::size_t, 3> dims = {
      static_cast<std::size_t>(my_details.size[0]),
      static_cast<std::size_t>(my_details.size[1]),
      static_cast<std::size_t>(my_details.size[2]),
  };

  std::array<std::size_t, 3> chosen_slices = {my_details.full_domain[0] - 1,
                                              my_details.full_domain[1] - 1,
                                              my_details.full_domain[2] - 1};

  auto tracker_x = Insitu::SliceTracker3D<double, 0>(dims, chosen_slices[0],
                                                     "x_slice", 1, 1);
  auto tracker_y = Insitu::SliceTracker3D<double, 1>(dims, chosen_slices[1],
                                                     "y_slice", 1, 1);
  auto tracker_z = Insitu::SliceTracker3D<double, 2>(dims, chosen_slices[2],
                                                     "z_slice", 1, 1);
  // auto colour_range = Colours::ColourRange<double>(
  //     0.0, my_details.full_domain[0] * my_details.full_domain[1] *
  //              my_details.full_domain[2]);
  //
  auto &data = my_details.data;
  //
  // auto x_image_and_x_dims =
  //     tracker_x.generate_graph(data.data(), colour_range).value();
  // auto y_image_and_y_dims =
  //     tracker_y.generate_graph(data.data(), colour_range).value();
  // auto z_image_and_z_dims =
  //     tracker_z.generate_graph(data.data(), colour_range).value();
  //
  // constexpr auto CUBOID_RENDER_IMAGE_SIZE =
  //     std::pair<std::size_t, std::size_t>{1500, 1500};
  // constexpr auto PERSPECTIVE = RasteriserComponents::CameraPerspective{
  //     RasteriserComponents::CameraPerspectiveX::RIGHT,
  //     RasteriserComponents::CameraPerspectiveY::DOWN,
  //     RasteriserComponents::CameraPerspectiveZ::FRONT};
  //
  // auto faces = std::array<Renderer::Face<double>, 3>{
  //     Renderer::get_faces_of_cuboid<double>(dims, chosen_slices,
  //     PERSPECTIVE)};
  // auto full_faces = Renderer::get_full_dims_as_faces<double>(dims,
  // PERSPECTIVE);
  //
  // auto camera = Renderer::get_position_camera<double>(dims, PERSPECTIVE);
  //
  // auto cuboid_pixel_buffer = Renderer::get_pixel_buffer(
  //     {CUBOID_RENDER_IMAGE_SIZE.first, CUBOID_RENDER_IMAGE_SIZE.second},
  //     camera, faces, full_faces, {std::get<0>(z_image_and_z_dims),
  //     std::get<0>(x_image_and_x_dims),
  //      std::get<0>(y_image_and_y_dims)},
  //     {std::get<1>(z_image_and_z_dims), std::get<1>(x_image_and_x_dims),
  //      std::get<1>(y_image_and_y_dims)},
  //     std::pair{chosen_slices, dims}, PERSPECTIVE);

  // std::string file_name = "3d_render_" + std::to_string(rank) + ".bmp";
  //
  // Writer::write_bmp(file_name, cuboid_pixel_buffer,
  //                   CUBOID_RENDER_IMAGE_SIZE.first,
  //                   CUBOID_RENDER_IMAGE_SIZE.second);

  // std::cout << "[DISPL DUMP] " << rank << " Displs is " <<
  // my_details.displs[0]
  //           << ", " << my_details.displs[1] << ", " << my_details.displs[2]
  //           << std::endl;

  auto per_insitu_frame_start = std::chrono::steady_clock::now();

  Implementation::per_insitu_frame(
      0, data.data(), chosen_slices, dims, my_details.displs,
      my_details.full_domain, rank, num_ranks, tracker_x, tracker_y, tracker_z);

  auto per_insitu_frame_end = std::chrono::steady_clock::now();

  std::cout << "[Insitu Step (ms) For Rank " << rank << "]: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(
                   per_insitu_frame_end - per_insitu_frame_start)
                   .count()
            << std::endl;

  MPI_Finalize();
}

// int actual_main() {
//
//   auto data_alloc_start = std::chrono::steady_clock::now();
//
//   auto rand_device = std::random_device{};
//   auto rand_gen = std::mt19937{rand_device()};
//   auto rand_distribution = std::uniform_int_distribution<std::size_t>(50,
//   1500);
//
//   std::vector<double> data;
//   std::array<std::size_t, 3> dims = {100, 100, 100};
//   // {rand_distribution(rand_gen),
//   //  rand_distribution(rand_gen),
//   //  rand_distribution(rand_gen)};
//   std::array<std::size_t, 3> chosen_slices = {dims[0] - 1, dims[1] / 2, 0};
//
//   for (int z = dims[2]; z > 0; z--) {
//     for (int y = 0; y < dims[1]; y++) {
//       for (int x = 0; x < dims[0]; x++) {
//         data.push_back(static_cast<double>(x * y * z));
//       }
//     }
//   }
//
//   auto data_alloc_end = std::chrono::steady_clock::now();
//
//   auto tracker_x = Insitu::SliceTracker3D<double, 0>(dims, chosen_slices[0],
//                                                      "x_slice", 1, 1);
//   auto tracker_y = Insitu::SliceTracker3D<double, 1>(dims, chosen_slices[1],
//                                                      "y_slice", 1, 1);
//   auto tracker_z = Insitu::SliceTracker3D<double, 2>(dims, chosen_slices[2],
//                                                      "z_slice", 1, 1);
//   auto colour_range =
//       Colours::ColourRange<double>(0.0, dims[0] * dims[1] * dims[2]);
//
//   auto generate_start = std::chrono::steady_clock::now();
//
//   auto x_image_and_x_dims =
//       tracker_x.generate_graph(data.data(), colour_range).value();
//   auto y_image_and_y_dims =
//       tracker_y.generate_graph(data.data(), colour_range).value();
//   auto z_image_and_z_dims =
//       tracker_z.generate_graph(data.data(), colour_range).value();
//
//   auto generate_end = std::chrono::steady_clock::now();
//
//   auto rasteriser_start = std::chrono::steady_clock::now();
//
//   constexpr auto CUBOID_RENDER_IMAGE_SIZE =
//       std::pair<std::size_t, std::size_t>{1500, 1500};
//   constexpr auto PERSPECTIVE = RasteriserComponents::CameraPerspective{
//       RasteriserComponents::CameraPerspectiveX::RIGHT,
//       RasteriserComponents::CameraPerspectiveY::DOWN,
//       RasteriserComponents::CameraPerspectiveZ::BACK};
//
//   auto faces = std::array<Renderer::Face<double>, 3>{
//       Renderer::get_faces_of_cuboid<double>(dims, chosen_slices,
//       PERSPECTIVE)};
//   auto full_faces = Renderer::get_full_dims_as_faces<double>(dims,
//   PERSPECTIVE);
//
//   auto camera = Renderer::get_position_camera<double>(dims, PERSPECTIVE);
//
//   auto cuboid_pixel_buffer = Renderer::get_pixel_buffer(
//       {CUBOID_RENDER_IMAGE_SIZE.first, CUBOID_RENDER_IMAGE_SIZE.second},
//       camera, faces, full_faces, {std::get<0>(z_image_and_z_dims),
//       std::get<0>(x_image_and_x_dims),
//        std::get<0>(y_image_and_y_dims)},
//       {std::get<1>(z_image_and_z_dims), std::get<1>(x_image_and_x_dims),
//        std::get<1>(y_image_and_y_dims)},
//       std::pair{chosen_slices, dims}, PERSPECTIVE);
//
//   Writer::write_bmp("3d_render.bmp", cuboid_pixel_buffer,
//                     CUBOID_RENDER_IMAGE_SIZE.first,
//                     CUBOID_RENDER_IMAGE_SIZE.second);
//
//   auto rasteriser_end = std::chrono::steady_clock::now();
//
//   std::cout << "Array Population (ms): "
//             << std::chrono::duration_cast<std::chrono::milliseconds>(
//                    data_alloc_end - data_alloc_start)
//                    .count()
//             << std::endl;
//
//   std::cout << "Graph Generation (ms): "
//             << std::chrono::duration_cast<std::chrono::milliseconds>(
//                    generate_end - generate_start)
//                    .count()
//             << std::endl;
//   std::cout << "Rasteriser Image Generation (ms): "
//             << std::chrono::duration_cast<std::chrono::milliseconds>(
//                    rasteriser_end - rasteriser_start)
//                    .count()
//             << std::endl;
// }
