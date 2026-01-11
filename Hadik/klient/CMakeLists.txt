add_executable(Player "Player.c" "Player.h")
target_sources(Player PUBLIC
	Player.c
)
target_link_libraries(Player PRIVATE SnakeLib)
target_include_directories(Player PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)
if(WIN32)
add_custom_command(TARGET Player POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:Player> $<TARGET_RUNTIME_DLLS:Player>
    COMMAND_EXPAND_LISTS
)
endif()