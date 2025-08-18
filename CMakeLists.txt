cmake_minimum_required(VERSION 3.10)
project(chip8_emulator)

set(CMAKE_CXX_STANDARD 17)

# SDL2 include and library paths
include_directories("D:/Libraries/SDL2-2.32.6/x86_64-w64-mingw32/include")
link_directories("D:/Libraries/SDL2-2.32.6/x86_64-w64-mingw32/lib")

# Add src/ as an include directory so all headers are accessible
include_directories("${CMAKE_SOURCE_DIR}/src")

add_executable(chip8_emulator src/main.cpp
        src/chip8.cpp)

# Link against SDL2
target_link_libraries(chip8_emulator SDL2main SDL2)