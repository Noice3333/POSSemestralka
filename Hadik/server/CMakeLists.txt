add_executable(Server "Server.c" "Server.h")
target_sources(Server PUBLIC
	Server.c
)
target_link_libraries(Server PRIVATE SnakeLib)
target_include_directories(Server PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)
if(WIN32)
add_custom_command(TARGET Server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:Server> $<TARGET_RUNTIME_DLLS:Server>
    COMMAND_EXPAND_LISTS
)
endif()