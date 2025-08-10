cmake -S . -B build
cmake --build build --config Release
./build/triadc samples/demo.triad
./build/triad_tests
