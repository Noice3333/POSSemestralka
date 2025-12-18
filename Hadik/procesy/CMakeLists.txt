add_executable(Snake Hadik.c Hadik.h "client.c")
target_sources(Snake PUBLIC
	Hadik.c
)
target_include_directories(Snake PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)