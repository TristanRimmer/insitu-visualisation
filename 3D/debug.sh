cmake -S . -B build  -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-O3 -march=native -ffast-math"

bear -- cmake --build build
