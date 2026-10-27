#include "3D_slice_tracker.hh"
#include "implementation.hh"
#include "mpi.hh"
#include <array>
#include <chrono>
#include <cstddef>
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
  auto &data = my_details.data;

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
