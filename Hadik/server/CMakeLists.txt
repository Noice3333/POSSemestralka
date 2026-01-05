add_executable(Server)
target_sources(Server PRIVATE
	Server.c
)
target_include_directories(Server PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
)