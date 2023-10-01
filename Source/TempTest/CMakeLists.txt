cmake_minimum_required(VERSION 3.12)

add_executable(TempTest "Main.cpp")

set_property(TARGET TempTest PROPERTY CXX_STANDARD 23)

target_link_libraries(TempTest PUBLIC ShaderCompiler RtrcPreprocessedShader)
