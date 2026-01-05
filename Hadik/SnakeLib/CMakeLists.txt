add_library(SnakeLib SHARED Hadik.c Hadik.h)
target_sources(SnakeLib PRIVATE
	Hadik.c
	Hadik.h
)
target_include_directories(SnakeLib PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
) 
include(GenerateExportHeader)
generate_export_header(SnakeLib)