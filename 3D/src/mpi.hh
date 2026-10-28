#ifndef INSITU_VISUALISATION_WITH_MPI_HH
#define INSITU_VISUALISATION_WITH_MPI_HH

#include <array>
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
  // std::array<int, 6> halos;

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

} // namespace MpiUtils

template <typename T>
auto MpiUtils::distribute_data_across_ranks(
    int rank, int num_ranks, std::array<std::size_t, 3> dimensions)
    -> PerRankDetails<T> {
  auto axis_decomp = MpiUtils::get_nslices_per_axis(num_ranks);

  auto offset_and_range =
      MpiUtils::get_range_from_rank(rank, axis_decomp, dimensions);

  std::vector<T> my_data{};

  for (std::size_t z = offset_and_range[2];
       z < offset_and_range[2] + offset_and_range[5]; z++) {
    for (std::size_t y = offset_and_range[1];
         y < offset_and_range[1] + offset_and_range[4]; y++) {
      for (std::size_t x = offset_and_range[0];
           x < offset_and_range[0] + offset_and_range[3]; x++) {
        my_data.push_back(static_cast<T>(x * y * z));
      }
    }
  }

  auto my_details = PerRankDetails<T>{};

  my_details.data = my_data;
  my_details.size = {offset_and_range[3], offset_and_range[4],
                     offset_and_range[5]};
  my_details.displs = {offset_and_range[0], offset_and_range[1],
                       offset_and_range[2]};
  my_details.full_domain = dimensions;
  my_details.num_ranks = num_ranks;

  return my_details;
}
#endif
