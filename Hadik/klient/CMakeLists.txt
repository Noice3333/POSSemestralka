add_executable(Player "Player.c")
target_sources(Player PUBLIC
	Player.c
)
target_include_directories(Player PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)