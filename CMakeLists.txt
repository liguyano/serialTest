cmake_minimum_required(VERSION 3.20)
project(SerialTest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

add_library(SerialHelpers STATIC
    src/SerialHelpers.cpp
)
target_include_directories(SerialHelpers PUBLIC src)

if(BUILD_TESTING)
    add_executable(SerialHelpersTests
        tests/SerialHelpersTests.cpp
    )
    target_link_libraries(SerialHelpersTests PRIVATE SerialHelpers)
    add_test(NAME SerialHelpersTests COMMAND SerialHelpersTests)
endif()

if(WIN32)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/wxWidgets/CMakeLists.txt")
        message(FATAL_ERROR "wxWidgets is missing. Run setup_wxwidgets.bat first.")
    endif()

    set(wxBUILD_SHARED OFF CACHE BOOL "" FORCE)
    set(wxBUILD_SAMPLES OFF CACHE BOOL "" FORCE)
    set(wxBUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(wxBUILD_DEMOS OFF CACHE BOOL "" FORCE)

    add_subdirectory(wxWidgets wxwidgets-build EXCLUDE_FROM_ALL)

    add_executable(SerialTest WIN32
        src/App.cpp
        src/MainFrame.cpp
        src/SerialPort.cpp
    )
    target_include_directories(SerialTest PRIVATE src)
    target_compile_definitions(SerialTest PRIVATE UNICODE _UNICODE)
    target_link_libraries(SerialTest PRIVATE SerialHelpers wx::core wx::base)
endif()
