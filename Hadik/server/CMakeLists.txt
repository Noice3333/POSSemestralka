add_executable(Server Server.c)
target_sources(Server PUBLIC
	Server.c
)
target_include_directories(Server PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)