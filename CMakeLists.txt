cmake_minimum_required(VERSION 2.8)
project(app)

set(CMAKE_BUILD_TYPE Release)

add_subdirectory(libuv)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -O0 -ggdb -DNDEBUG")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wshadow -Wno-unused-parameter ")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -MMD -fPIC -O0 -g -ggdb  -Wall -std=c++11")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Map=app.map")

add_executable(app
    main.c
)

target_include_directories(app 
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:
			${CMAKE_CURRENT_SOURCE_DIR}/libuv/include/
    PRIVATE
)

target_link_libraries(app uv_a)

install(TARGETS app DESTINATION bin)
install(FILES app.map DESTINATION bin)

add_executable(app-server
    main-server.c
    list.c
)

target_include_directories(app-server
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:
			${CMAKE_CURRENT_SOURCE_DIR}/libuv/include/
    PRIVATE
)

target_link_libraries(app-server uv_a)

add_executable(app-client
    main-client.c
)
target_include_directories(app-client
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:
			${CMAKE_CURRENT_SOURCE_DIR}/libuv/include/
    PRIVATE
)

target_link_libraries(app-client uv_a)


