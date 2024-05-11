cmake_minimum_required(VERSION 3.27)
project(MutilThreadSignal)

set(CMAKE_CXX_STANDARD 26)

add_executable(MutilThreadSignal main.cpp
        Delegate.h
        EventManager.h
        EventManager.cpp
        Object.h
        TimeTool.h
        Delegate.cpp)