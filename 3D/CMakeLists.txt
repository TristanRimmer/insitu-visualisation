cmake_minimum_required(VERSION 3.23)

project(InsituVisualisationWithMPI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(MPI REQUIRED COMPONENTS CXX)


add_executable(InsituVisualisationWithMPI
    src/main.cpp
    src/bmp.cc
    src/mpi.cc
)

target_include_directories(InsituVisualisationWithMPI
    PRIVATE
    src
)

target_link_libraries(InsituVisualisationWithMPI
    PRIVATE MPI::MPI_CXX
)

target_compile_definitions(InsituVisualisationWithMPI
    PRIVATE USE_MPI
)

if (MPI_FOUND)
  message("MPI Package Found")
  target_compile_definitions(InsituVisualisationWithMPI PRIVATE MPI_FOUND)
endif()
