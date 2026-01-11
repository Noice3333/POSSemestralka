add_executable(client "client.c" "client.h")
target_sources(client PUBLIC
	client.c
)
target_link_libraries(client PRIVATE lib)
target_include_directories(client PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)
if(WIN32)
add_custom_command(TARGET client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:client> $<TARGET_RUNTIME_DLLS:client>
    COMMAND_EXPAND_LISTS
)
endif()