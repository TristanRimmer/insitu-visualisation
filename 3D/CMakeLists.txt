cmake_minimum_required(VERSION 3.23)

project(InsituVisualisationWithMPI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(MPI COMPONENTS CXX)
find_package(SDL2 REQUIRED)

add_executable(InsituVisualisationWithMPI
    src/main.cpp
    src/bmp.cc
    src/mpi.cc
    src/implementation.cc
)

target_include_directories(InsituVisualisationWithMPI
    PRIVATE
    src
)

target_link_libraries(InsituVisualisationWithMPI
    PRIVATE MPI::MPI_CXX
)
target_link_libraries(InsituVisualisationWithMPI PRIVATE SDL2::SDL2)

target_compile_definitions(InsituVisualisationWithMPI
    PRIVATE USE_MPI
)

if (MPI_FOUND)
  message("MPI Package Found")
  target_compile_definitions(InsituVisualisationWithMPI PRIVATE MPI_FOUND)
endif()
