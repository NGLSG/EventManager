cmake_minimum_required(VERSION 3.27)
project(MutilThreadSignal)

set(CMAKE_CXX_STANDARD 23)

add_executable(MutilThreadSignal main.cpp
        Delegate.h)
