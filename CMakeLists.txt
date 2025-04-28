cmake_minimum_required(VERSION 3.31)
project(rune C)

set(CMAKE_C_STANDARD 11)

# Find SDL3 package
find_package(SDL3 CONFIG REQUIRED)

# Find Vulkan
find_package(Vulkan REQUIRED)

# Add the executable
add_executable(rune main.c
        std.h
        std.c
        coll.h
        str.c
        str.h
)

# Link SDL3 to the executable
target_link_libraries(rune PRIVATE SDL3::SDL3 Vulkan::Vulkan)
