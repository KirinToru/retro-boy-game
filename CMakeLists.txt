cmake_minimum_required(VERSION 4.2.1)
project(JourneyToTheClouds)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

if(DEFINED ENV{SFML_DIR})
    set(SFML_DIR "$ENV{SFML_DIR}")
else()
    set(SFML_DIR "C:/SFML-3.0.2")
endif()

message(STATUS "Using SFML from: ${SFML_DIR}")
include_directories(${SFML_DIR}/include)
link_directories(${SFML_DIR}/lib)
set(SOURCES
    "src/main.cpp"
    "src/Game.cpp"
    "src/Entities/Player.cpp"
    "src/World/Map.cpp"
)
add_executable(JourneyToTheClouds ${SOURCES})
target_link_libraries(JourneyToTheClouds
    debug sfml-graphics-d
    optimized sfml-graphics
    debug sfml-window-d
    optimized sfml-window
    debug sfml-system-d
    optimized sfml-system
    debug sfml-audio-d
    optimized sfml-audio
)
add_custom_command(TARGET JourneyToTheClouds POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/dll"
    "$<TARGET_FILE_DIR:JourneyToTheClouds>"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/assets"
    "$<TARGET_FILE_DIR:JourneyToTheClouds>/assets")
