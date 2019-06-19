aux_source_directory(. source_list)



include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(/usr/include/)

add_executable(echo ${source_list})
target_link_libraries(echo pthread rt)