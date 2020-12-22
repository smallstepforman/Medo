cmake_minimum_required(VERSION 3.9)

project(Equaliser)

add_library(${PROJECT_NAME} MODULE
#AddOn
	"Effect_Equaliser.cpp"
)

include_directories(/boot/system/develop/headers)
#target_include_directories(${PROJECT_NAME} SYSTEM INTERFACE /boot/system/develop/tools/lib/gcc/x86_64-unknown-haiku/8.3.0/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../../
	${CMAKE_CURRENT_SOURCE_DIR}/../../Yarra
)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a -g  -O0")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")

target_compile_options(${PROJECT_NAME} PUBLIC -std=c++2a -g -O0)
#target_link_libraries(${PROJECT_NAME} be stdc++ GL tracker translation media)

