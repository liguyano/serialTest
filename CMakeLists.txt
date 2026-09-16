cmake_minimum_required(VERSION 3.20)
project(SerialTest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

if(BUILD_TESTING)
    add_executable(SerialHelpersTests
        tests/SerialHelpersTests.cpp
    )
    target_include_directories(SerialHelpersTests PRIVATE src)
    add_test(NAME SerialHelpersTests COMMAND SerialHelpersTests)
endif()
