add_library(lib SHARED snake.c snake.h)
target_sources(lib PRIVATE
	snake.c
	snake.h
)
target_include_directories(lib PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
) 
include(GenerateExportHeader)
generate_export_header(lib)