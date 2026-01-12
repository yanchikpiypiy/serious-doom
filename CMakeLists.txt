cmake_minimum_required(VERSION 3.10)
project(DoomClone LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find SDL2 using pkg-config (Linux way)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

# Add all source files
add_executable(game 
    main.cpp
    player.cpp
    map.cpp
    sprite.cpp
    gun.cpp
    renderer.cpp
)

# Include directories
target_include_directories(game PRIVATE 
    ${SDL2_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}  # For finding header files
)

# Link libraries
target_link_libraries(game PRIVATE 
    ${SDL2_LIBRARIES}
    m  # Math library for cos, sin, etc.
)
