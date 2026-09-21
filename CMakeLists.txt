cmake_minimum_required(VERSION 3.16)
project(Sidecar LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_path(PORTAUDIO_INCLUDE_DIR portaudio.h REQUIRED)
find_library(PORTAUDIO_LIBRARY portaudio REQUIRED)

add_executable(sidecar
	src/main.cpp
	src/audio.cpp
)
target_include_directories(sidecar PRIVATE
	${PORTAUDIO_INCLUDE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(sidecar PRIVATE ${PORTAUDIO_LIBRARY})