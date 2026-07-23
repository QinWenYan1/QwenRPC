#项目的cmake版本和项目名称
cmake_minimum_required(VERSION 3.16)
project(KopiRPC)

#生成compile_commands.json(clangd 需要)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#设置可执行文件的输出路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

#设置项目库文件的输出路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

#设置编译头文件的搜索路径 -I
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/example)

#设置项目库文件的搜索路径 -L
link_directories(${PROJECT_SOURCE_DIR}/lib)

#进入子目录进行编译
#add_subdirectory(src)
add_subdirectory(example)