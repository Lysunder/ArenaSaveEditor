cmake_minimum_required(VERSION 3.15)
project(ArenaSaveEditor CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Link the C++ runtime statically so the .exe runs without the VC++ redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_executable(ArenaSaveEditor
	src/main.cpp
	src/SaveEngine.cpp
)

if(MSVC)
	target_compile_options(ArenaSaveEditor PRIVATE /W4)
else()
	target_compile_options(ArenaSaveEditor PRIVATE -Wall -Wextra)
endif()
