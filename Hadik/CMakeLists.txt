cmake_minimum_required(VERSION 3.22)
project(
	Snake
	LANGUAGES C
)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_subdirectory(lib)
add_subdirectory(klient)
add_subdirectory(server)
