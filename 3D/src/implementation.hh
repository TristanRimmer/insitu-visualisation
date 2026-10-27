#ifndef INSITU_VISUALISATION_3D_IMPLEMENTATION_HH
#define INSITU_VISUALISATION_3D_IMPLEMENTATION_HH

#include "3D_slice_tracker.hh"
#include "bmp.hh"
#include <array>
#include <cstdint>
#include <mpi/mpi.h>
#include <string>

namespace Implementation {

auto per_insitu_frame(int simulation_iter, double *raw_data,
                      const std::array<std::size_t, 3> &slice_indexes,
                      const std::array<std::size_t, 3> &local_block_size,
                      const std::array<std::size_t, 3> &displacement,
                      const std::array<std::size_t, 3> &full_block_size,
                      int rank, int num_ranks,
                      Insitu::SliceTracker3D<double, 0> &x_tracker,
                      Insitu::SliceTracker3D<double, 1> &y_tracker,
                      Insitu::SliceTracker3D<double, 2> &z_tracker) -> void;

template <std::uint8_t Axis>
auto communicate_slice_to_master_rank(
    DomainSliceAccessor3D<double, Axis> &accessor,
    const std::array<std::size_t, 3> &slice_indexes,
    const std::array<std::size_t, 3> &local_block_size,
    const std::array<std::size_t, 3> &displacement,
    const std::array<std::size_t, 3> &full_block_size, int rank, int num_ranks)
    -> std::vector<double>;

auto calculate_local_slice_index(std::size_t gbl_slice,
                                 std::size_t local_axis_size) -> std::size_t;

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
/**
 * Before anything happens:
 * - Every rank needs to know how many times a row broadcast needs to happen
 * - This is done via a Gatherv to rank 0 of the number of rows followed by a
 * broadcast of the max of these values
 * - Each rank is then responsible for correctly swapping to 0 passed parameters
 * at the correct point.
 * - Therefore, recvcounts needs to be updated correctly
 *
 * PSUEDOCODE(ISH):
 *
 * First: MPI_Allgatherv(Num Rows to Broadcast) - Each rank calculates the max
 *
 * for (row = 0; row < max_rows_to_be_sent; row++) {
 *    MPI_Gatherv(to rank 0, send counts for each rank)
 *    (rank 0 preps recvcounts)
 *    MPI_Gatherv(to rank 0, displs)
 *    (rank 0 preps displs)
 *    MPI_Gatherv(to rank 0, the row)
 *
 *    Ranks which are not sending data/have sent all data send 0 data to displ 0
 *
 *    Ranks which do not lie inside of the slice index just send 0 every time
 * }
 */

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
  auto slice_dimensions = std::pair{accessor.get_slice_dims()};

  const auto ranks_sends_one_item = std::vector<int>(num_ranks, 1);
  auto ranks_send_to_itself_index = std::vector<int>(num_ranks, 0);
  for (int r_i = 0; r_i < num_ranks; r_i++)
    ranks_send_to_itself_index[r_i] = r_i;

  auto row_counts_from_all_ranks = std::vector<int>(num_ranks, 0);
  const auto local_row_count = static_cast<int>(slice_dimensions.second);

  // All ranks now know the max amount of row bcasts to do
  MPI_Allgatherv(&local_row_count, 1, MPI_INT, row_counts_from_all_ranks.data(),
                 ranks_sends_one_item.data(), ranks_send_to_itself_index.data(),
                 MPI_INT, MPI_COMM_WORLD);

  int NUMBER_OF_ROW_BROADCASTS = 0;
  for (auto num_rows : row_counts_from_all_ranks)
    NUMBER_OF_ROW_BROADCASTS = num_rows > NUMBER_OF_ROW_BROADCASTS
                                   ? num_rows
                                   : NUMBER_OF_ROW_BROADCASTS;

  // auto local_slice_index = Implementation::calculate_local_slice_index(
  //     slice_indexes[Axis], local_block_size[Axis]);

  // Check once if the slice index is out of its displacement
  bool slice_lies_in_my_domain =
      (displacement[Axis] <= slice_indexes[Axis] &&
       slice_indexes[Axis] < displacement[Axis] + local_block_size[Axis]);

  // std::cout << "[slice_lies_in_my_domain] " << rank << ": "
  //           << slice_lies_in_my_domain << std::endl;

  auto x_axis_index = (Axis == 0) ? 2 : 0;
  auto y_axis_index = (Axis == 0 || Axis == 2) ? 1 : 2;

  auto local_full_x_axis = full_block_size[x_axis_index];
  auto local_full_y_axis = full_block_size[y_axis_index];
  auto local_displacement_x = displacement[x_axis_index];
  auto local_displacement_y = displacement[y_axis_index];

  auto receive_buffer = std::vector<double>{};
  auto reusable_recvcounts = std::vector<int>{};
  auto reusable_displs = std::vector<int>{};

  if (rank == 0) {
    receive_buffer.resize(local_full_x_axis * local_full_y_axis, 0.0);
    reusable_recvcounts.resize(num_ranks, 0);
    reusable_displs.resize(num_ranks, 0);
  }

  // if (rank == 0)
  //   std::cout << "HERE 2" << std::endl;

  auto *receive_buffer_ptr = (rank != 0) ? nullptr : receive_buffer.data();
  auto *reusable_recvcounts_ptr =
      (rank != 0) ? nullptr : reusable_recvcounts.data();
  auto *reusable_displs_ptr = (rank != 0) ? nullptr : reusable_displs.data();

  // MPI_Barrier(MPI_COMM_WORLD);
  //
  // if (rank == 0)
  //   std::cout << "HERE 3" << std::endl;
  //
  // if (Axis == 0) {
  //   std::cout << "[PRE-ROW-BCAST-DATA-DUMP] " << rank << ": Local Disp: ["
  //             << local_displacement_x << ", " << local_displacement_y
  //             << "], Full: [" << local_full_x_axis << ", " <<
  //             local_full_y_axis
  //             << "]" << std::endl;
  // }

  auto local_slice_index = slice_indexes[Axis] - displacement[Axis];

  for (int row_index = 0; row_index < NUMBER_OF_ROW_BROADCASTS; row_index++) {
    // if (rank == 0)
    //   std::cout << "[SIZE_OF_RECV_BUFFER] " << receive_buffer.size()
    //             << std::endl;

    int current_displacement = 0;

    auto send_buffer = std::vector<double>{};

    bool row_index_has_exceeded_num_rows = row_index >= slice_dimensions.second;

    if (slice_lies_in_my_domain and not row_index_has_exceeded_num_rows) {
      // if (rank == 0)
      //   std::cout << "Calcualting local slice " << row_index << std::endl;

      // if (rank == 0)
      // std::cout << "Calcualting local slice completed " << row_index
      //           << std::endl;
      // local_displacement_y + row_index gets to the right y index on the 2d
      // slice recbuf. local_displacement_x gets you to the row
      // WARN: I think this is majorly wrong
      current_displacement =
          (local_displacement_y + row_index) * local_full_x_axis +
          local_displacement_x;

      // if (rank == 1 && Axis == 1)
      //   std::cout << "FLPFLPFLPFLPFLFP: " << current_displacement << ", "
      //             << local_slice_index << ": " << row_index << std::endl;
      auto row_begin_and_end = accessor.get_row(row_index, local_slice_index);
      // if (rank == 1 && Axis == 1)
      //   std::cout << "Iterating over row items " << row_index << std::endl;
      for (auto it = row_begin_and_end.first; it != row_begin_and_end.second;
           ++it) {
        send_buffer.push_back(*it);
      }
    }

    int size_as_primitive_int = static_cast<int>(send_buffer.size());

    // if (rank == 0)
    // std::cout << "GATHERS BEGIN ON ROW " << row_index << std::endl;

    // Gather the contributions
    MPI_Gatherv(&size_as_primitive_int, 1, MPI_INT, reusable_recvcounts_ptr,
                ranks_sends_one_item.data(), ranks_send_to_itself_index.data(),
                MPI_INT, 0, MPI_COMM_WORLD);
    // And the displacements
    MPI_Gatherv(&current_displacement, 1, MPI_INT, reusable_displs_ptr,
                ranks_sends_one_item.data(), ranks_send_to_itself_index.data(),
                MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
      auto sum = 0;

      std::string recvcounts = "[";
      std::string displs = "[";
      for (int i = 0; i < reusable_recvcounts.size(); i++) {
        recvcounts = recvcounts + std::to_string(reusable_recvcounts[i]) + ", ";
        displs = displs + std::to_string(reusable_displs[i]) + ", ";
        sum += reusable_recvcounts[i];
      }
      // std::cout << "Recv counts : " + recvcounts << std::endl;
      // std::cout << "Displs: " + displs << std::endl;
      // std::cout << sum << "values into " << receive_buffer.size() <<
      // std::endl;
    }

    MPI_Gatherv(send_buffer.data(), size_as_primitive_int, MPI_DOUBLE,
                receive_buffer_ptr, reusable_recvcounts_ptr,
                reusable_displs_ptr, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  }

  return receive_buffer;
}
#endif
