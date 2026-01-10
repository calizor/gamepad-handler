cmake_minimum_required(VERSION 3.11)
project("stickpad")

set(INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/padmain.cpp)



add_executable(${PROJECT_NAME} ${SOURCES})


target_include_directories(${PROJECT_NAME} PUBLIC ${INC_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE 
    ws2_32 
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/libViGEmClient.a
    setupapi
)