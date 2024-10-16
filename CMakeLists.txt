cmake_minimum_required(VERSION 3.19)

project(StudioBrightness LANGUAGES CXX)

add_executable(studio-brightness WIN32 src/main.cpp src/hid.cpp studio-brightness.rc)

target_include_directories(studio-brightness PRIVATE include)

target_link_libraries(studio-brightness PRIVATE hid setupapi wbemuuid comctl32 User32 Shell32 Gdi32)

target_compile_options(studio-brightness PRIVATE /W4)

install(TARGETS studio-brightness)
