cmake_minimum_required(VERSION 3.10)
project(chip8_emulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SDL2 paths (your install)
set(SDL2_INCLUDE_DIR "D:/Libraries/SDL2-2.32.6/x86_64-w64-mingw32/include")
set(SDL2_LIB_DIR "D:/Libraries/SDL2-2.32.6/x86_64-w64-mingw32/lib")

include_directories(${SDL2_INCLUDE_DIR})
link_directories(${SDL2_LIB_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src)

add_executable(chip8_emulator
        src/main.cpp
        src/chip8.cpp
)

# FORCE console app (this kills WinMain forever)
target_link_options(chip8_emulator PRIVATE -mconsole)

# LINK SDL2 ONLY — ABSOLUTELY NO SDL2main
target_link_libraries(chip8_emulator SDL2)
