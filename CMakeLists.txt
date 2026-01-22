cmake_minimum_required(VERSION 3.10)
project(RetroBoy)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(SFML_DIR "C:/SFML-3.0.2")
include_directories(${SFML_DIR}/include)
link_directories(${SFML_DIR}/lib)
file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(RetroBoy ${SOURCES})
target_link_libraries(RetroBoy
    sfml-graphics
    sfml-window
    sfml-system
    sfml-audio
)
add_custom_command(TARGET RetroBoy POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/dll"
    "$<TARGET_FILE_DIR:RetroBoy>")
