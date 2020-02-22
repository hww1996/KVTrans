cmake_minimum_required(VERSION 3.15)
project(KVTrans)

set(CMAKE_CXX_STANDARD 17)

include_directories("include")

aux_source_directory(src src_list)

add_library(KVTrans ${src_list})