add_library(snakelib SHARED snake.c snake.h)
target_sources(snakelib PRIVATE
	snake.c
	snake.h
)
target_include_directories(snakelib PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
) 
include(GenerateExportHeader)
generate_export_header(snakelib)