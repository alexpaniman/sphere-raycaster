add_executable(sphere-raycaster sphere-raycaster.cpp)

target_include_directories(sphere-raycaster PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(sphere-raycaster PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR})

target_link_libraries(sphere-raycaster gl)
