#include "3D_slice_tracker.hh"
#include "implementation.hh"
#include "mpi.hh"
#include "slice_visualiser.hh"
#include <SDL.h>
#include <SDL_render.h>
#include <array>
#include <cstddef>
#include <random>
#include <vector>

#include <mpi/mpi.h>

auto new_image_surface(SDL_Renderer *renderer, std::string file_name)
    -> SDL_Texture * {
  SDL_Surface *surface = SDL_LoadBMP(file_name.c_str());

  SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

  SDL_FreeSurface(surface);

  return texture;
}

int main(int argc, char *argv[]) {
  MPI_Init(&argc, &argv);

  int num_ranks;
  int rank;

  MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  auto dimensions = std::array<std::size_t, 3>{};

  if (rank == 0) {
    auto rand_device = std::random_device{};
    auto rand_gen = std::mt19937{rand_device()};
    auto distribution = std::uniform_int_distribution<std::size_t>(500, 750);
    dimensions = {distribution(rand_gen), distribution(rand_gen),
                  distribution(rand_gen)};
  }

  MPI_Bcast(dimensions.data(), 3, MPI_INT64_T, 0, MPI_COMM_WORLD);

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

  if (rank == 0) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
      std::cerr << SDL_GetError() << "\n";
      return EXIT_FAILURE;
    }
  }

  SDL_Window *window = nullptr;

  if (rank == 0)
    window =
        SDL_CreateWindow("BMP Viewer", SDL_WINDOWPOS_CENTERED,
                         SDL_WINDOWPOS_CENTERED, 1000, 1000, SDL_WINDOW_SHOWN);
  SDL_Renderer *renderer = nullptr;

  if (rank == 0)
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  if (!renderer)
    std::cerr << SDL_GetError() << std::endl;

  SDL_Event event;
  bool running = true;
  SliceVisualiser::RunTimeVariantData run_time_data{};
  run_time_data.dimensions = my_details.full_domain;
  run_time_data.slice = chosen_slices;
  auto &data = my_details.data;

  auto tracker_x = Insitu::SliceTracker3D<double, 0>(
      dims, run_time_data.slice[0], "x_slice", 1, 1);
  auto tracker_y = Insitu::SliceTracker3D<double, 1>(
      dims, run_time_data.slice[1], "y_slice", 1, 1);
  auto tracker_z = Insitu::SliceTracker3D<double, 2>(
      dims, run_time_data.slice[2], "z_slice", 1, 1);

  Implementation::per_insitu_frame(
      0, data.data(), run_time_data.slice, dims, my_details.displs,
      my_details.full_domain, rank, num_ranks, tracker_x, tracker_y, tracker_z,
      {run_time_data.x, run_time_data.y, run_time_data.z});

  static const std::string bmp_file_name = "3d_cuboid_buffer.bmp";

  while (running) {
    if (rank == 0) {
      while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT: {
          running = false;
          break;
        }
        case SDL_KEYDOWN: {
          const auto key = event.key.keysym.sym;
          if (SliceVisualiser::InputToBehaviourMapper.count(key) > 0)
            SliceVisualiser::InputToBehaviourMapper.at(key)(run_time_data);

          break;
        }
        }
      }
    }

    MPI_Bcast(run_time_data.slice.data(), 3, MPI_INT64_T, 0, MPI_COMM_WORLD);
    MPI_Bcast(&running, 1, MPI_C_BOOL, 0, MPI_COMM_WORLD);

    tracker_x = Insitu::SliceTracker3D<double, 0>(dims, run_time_data.slice[0],
                                                  "x_slice", 1, 1);
    tracker_y = Insitu::SliceTracker3D<double, 1>(dims, run_time_data.slice[1],
                                                  "y_slice", 1, 1);
    tracker_z = Insitu::SliceTracker3D<double, 2>(dims, run_time_data.slice[2],
                                                  "z_slice", 1, 1);

    Implementation::per_insitu_frame(
        0, data.data(), run_time_data.slice, dims, my_details.displs,
        my_details.full_domain, rank, num_ranks, tracker_x, tracker_y,
        tracker_z, {run_time_data.x, run_time_data.y, run_time_data.z});

    if (run_time_data.state_changed) {
      if (rank == 0) {
        SDL_Texture *texture = new_image_surface(renderer, bmp_file_name);

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
        SDL_DestroyTexture(texture);
      }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    run_time_data.state_changed = false;
  }

  if (rank == 0) {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
  }

  SDL_Quit();

  MPI_Finalize();
}
