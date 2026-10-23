#include "mpi.hh"
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <vector>

#ifdef MPI_FOUND
#include <mpi/mpi.h>
#endif

auto MpiUtils::get_nslices_per_axis(int num_ranks)
    -> std::array<std::size_t, 3> {

  // Split the num ranks into its prime factosr
  auto prime_factors = std::vector<int>{};

  int modifiable_num_ranks = num_ranks;

  int current_factor = 1;

  while (modifiable_num_ranks != 1) {
    // Start at 2
    current_factor++;

    if (modifiable_num_ranks % current_factor == 0) {
      // Divisible by this number, add to list
      prime_factors.push_back(current_factor);

      modifiable_num_ranks = modifiable_num_ranks / current_factor;
      current_factor = 1;
    }
  }

  auto axis_slices = std::array<std::size_t, 3>{1, 1, 1};

  for (int i = 0; i < prime_factors.size(); i++) {
    int current_factor = prime_factors[i];

    if (i % 3 == 0) {
      axis_slices[0] *= current_factor;
    } else if (i % 3 == 1) {
      axis_slices[1] *= current_factor;
    } else /* if (i % 3 == 2) */ {
      axis_slices[2] *= current_factor;
    }
  }

  return axis_slices;
}

auto MpiUtils::get_range_from_rank(int rank,
                                   std::array<std::size_t, 3> axis_decomp,
                                   std::array<std::size_t, 3> dimensions)
    -> std::array<std::size_t, 6> {
  auto index_in_each_axis = std::array<int, 3>{};

  std::size_t rank_copy = rank;

  index_in_each_axis[2] = rank_copy % axis_decomp[2];
  rank_copy = rank_copy / axis_decomp[2];
  index_in_each_axis[1] = rank_copy % axis_decomp[1];
  index_in_each_axis[0] = rank_copy / axis_decomp[1];

  std::size_t dim_x_div = dimensions[0] / axis_decomp[0];
  std::size_t dim_y_div = dimensions[1] / axis_decomp[1];
  std::size_t dim_z_div = dimensions[2] / axis_decomp[2];

  // The last rank in each axis can get get the leftovers
  std::size_t dim_x_mod = dimensions[0] % axis_decomp[0];
  std::size_t dim_y_mod = dimensions[1] % axis_decomp[1];
  std::size_t dim_z_mod = dimensions[2] % axis_decomp[2];

  auto offsets_and_ranges = std::array<std::size_t, 6>{};

  // Offset/Starting point
  offsets_and_ranges[0] = index_in_each_axis[0] * dim_x_div;
  offsets_and_ranges[1] = index_in_each_axis[1] * dim_y_div;
  offsets_and_ranges[2] = index_in_each_axis[2] * dim_z_div;

  // Range
  offsets_and_ranges[3] = dim_x_div;
  offsets_and_ranges[4] = dim_y_div;
  offsets_and_ranges[5] = dim_z_div;

  // End rank gets the leftovers
  for (int i = 0; i < 3; i++) {
    std::size_t relevant_leftovers = (i == 0)   ? dim_x_mod
                                     : (i == 1) ? dim_y_mod
                                                : dim_z_mod;
    if (index_in_each_axis[i] == axis_decomp[i] - 1)
      offsets_and_ranges[3 + i] += relevant_leftovers;
  }

  return offsets_and_ranges;
}
