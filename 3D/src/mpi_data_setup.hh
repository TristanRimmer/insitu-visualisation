#ifndef INSITU_VISUALISATION_WITH_MPI_HH
#define INSITU_VISUALISATION_WITH_MPI_HH

#include <array>
#include <cstdlib>
#include <iostream>
#include <vector>

#ifdef MPI_FOUND
#include <mpi/mpi.h>
#else
#define MPI_comm bool
#endif

namespace MpiUtils {

// This struct aims to mimic what data the ops_dat function stores

template <typename T> struct PerRankDetails {
  // The ops_dat definitely contains these
  std::vector<T> data;
  std::array<std::size_t, 3> size;
  std::array<std::size_t, 3> displs;
  // // A rank may have duplicate data
  std::array<int, 6> halos;

  // This may be stored internally but in OPS may need to be stored elsewhere
  std::array<std::size_t, 3> full_domain;
  int num_ranks;
};

// Given the number of ranks, it (reasonably evenly) splits each axis
auto get_nslices_per_axis(int num_ranks) -> std::array<std::size_t, 3>;

template <typename T>
auto distribute_data_across_ranks(int rank, int num_ranks,
                                  std::array<std::size_t, 3> dimensions)
    -> MpiUtils::PerRankDetails<T>;

auto get_range_from_rank(int rank, std::array<std::size_t, 3> axis_decomp,
                         std::array<std::size_t, 3> dimensions)
    -> std::array<std::size_t, 6>;

auto get_halos_from_rank(int rank,
                         const std::array<std::size_t, 3> &axis_decomp)
    -> std::array<int, 6>;

namespace detail {
auto get_index_in_each_axis(int rank,
                            const std::array<std::size_t, 3> &axis_decomp)
    -> std::array<int, 3>;
}

} // namespace MpiUtils

template <typename T>
auto MpiUtils::distribute_data_across_ranks(
    int rank, int num_ranks, std::array<std::size_t, 3> dimensions)
    -> PerRankDetails<T> {
  auto axis_decomp = MpiUtils::get_nslices_per_axis(num_ranks);

  auto offset_and_range =
      MpiUtils::get_range_from_rank(rank, axis_decomp, dimensions);

  auto halos = MpiUtils::get_halos_from_rank(rank, axis_decomp);

  std::vector<T> my_data{};

  for (int z = static_cast<int>(offset_and_range[2]) + halos[4];
       z < static_cast<int>(offset_and_range[2]) +
               static_cast<int>(offset_and_range[5]) + halos[5];
       z++) {

    for (int y = static_cast<int>(offset_and_range[1]) + halos[2];
         y < static_cast<int>(offset_and_range[1]) +
                 static_cast<int>(offset_and_range[4]) + halos[3];
         y++) {

      for (int x = static_cast<int>(offset_and_range[0]) + halos[0];
           x < static_cast<int>(offset_and_range[0]) +
                   static_cast<int>(offset_and_range[3]) + halos[1];
           x++) {

        my_data.push_back(static_cast<T>(x * y * z));
      }
    }
  }

  auto my_details = PerRankDetails<T>{};

  my_details.data = my_data;
  my_details.size = {offset_and_range[3] + halos[1] - halos[0],
                     offset_and_range[4] + halos[3] - halos[2],
                     offset_and_range[5] + halos[5] - halos[4]};
  auto size_dims = my_details.size[0] * my_details.size[1] * my_details.size[2];

  my_details.displs = {offset_and_range[0], offset_and_range[1],
                       offset_and_range[2]};

  my_details.full_domain = dimensions;
  my_details.num_ranks = num_ranks;
  my_details.halos = halos;

  return my_details;
}
#endif
