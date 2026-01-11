add_executable(server "server.c" "server.h")
target_sources(server PUBLIC
	server.c
)
target_link_libraries(server PRIVATE lib)
target_include_directories(server PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)
if(WIN32)
add_custom_command(TARGET server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:server> $<TARGET_RUNTIME_DLLS:server>
    COMMAND_EXPAND_LISTS
)
endif()