# Copyright Lewis Baker, Corentin Jabot
# Licensed under Boost Software License 1.0

cmake_minimum_required(VERSION 3.12)

project(stdgenerator LANGUAGES CXX
                     VERSION 0.1)

add_library(stdgenerator INTERFACE)

target_include_directories(stdgenerator
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>)

target_compile_features(stdgenerator INTERFACE cxx_std_20)

enable_testing()
include(CTest)

add_subdirectory("tests")
