cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 20)

get_filename_component(projectName ${CMAKE_CURRENT_SOURCE_DIR} NAME)
message("cmake for ${projectName}")
project(${projectName})

find_package(networkit CONFIG QUIET)
add_executable(${projectName} src/main.cpp
)
target_link_libraries(${projectName} LINK_PUBLIC EvalMaxSAT MaLib cadical bz2 networkit pblib)

target_include_directories(${projectName} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(GccmToMaxSAT PRIVATE "${CMAKE_SOURCE_DIR}/Grover/src")


install(TARGETS ${projectName}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
