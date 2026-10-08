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
    enable_language(RC)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/wxwidget/CMakeLists.txt")
        message(FATAL_ERROR "wxWidgets is missing. Run setup_wxwidgets.bat first.")
    endif()

    set(wxBUILD_SHARED OFF CACHE BOOL "" FORCE)
    set(wxBUILD_SAMPLES OFF CACHE BOOL "" FORCE)
    set(wxBUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(wxBUILD_DEMOS OFF CACHE BOOL "" FORCE)

    # The serial terminal does not embed a browser or require the WebView2 SDK.
    set(wxUSE_WEBVIEW OFF CACHE BOOL "" FORCE)

    add_subdirectory(wxwidget wxwidgets-build EXCLUDE_FROM_ALL)

    add_executable(SerialTest WIN32
        src/App.cpp
        src/MainFrame.cpp
        src/SerialPort.cpp
        src/SerialTest.rc
    )
    target_include_directories(SerialTest PRIVATE src)
    target_compile_definitions(SerialTest PRIVATE UNICODE _UNICODE)
    target_link_libraries(SerialTest PRIVATE SerialHelpers wx::core wx::base)
    if(MINGW)
        # Some GCC distributions inject a default manifest even when an RC
        # file supplies one. Preserve startup objects, omitting only that manifest.
        execute_process(COMMAND "${CMAKE_CXX_COMPILER}" -dumpspecs
            OUTPUT_VARIABLE gcc_specs RESULT_VARIABLE gcc_specs_result)
        if(gcc_specs_result EQUAL 0)
            string(REGEX MATCH "\\*endfile:[\r\n]+([^\r\n]+)" gcc_endfile_match "${gcc_specs}")
            set(gcc_endfile "${CMAKE_MATCH_1}")
            set(default_manifest_spec "%{!shared:%:if-exists(default-manifest.o%s)}")
            string(FIND "${gcc_endfile}" "${default_manifest_spec}" manifest_position)
            if(NOT manifest_position EQUAL -1)
                string(REPLACE "${default_manifest_spec}" "" gcc_endfile "${gcc_endfile}")
                set(manifest_specs "${CMAKE_CURRENT_BINARY_DIR}/serialtest-manifest.specs")
                file(WRITE "${manifest_specs}" "*endfile:\n${gcc_endfile}\n")
                target_link_options(SerialTest PRIVATE "-specs=${manifest_specs}")
            endif()
        endif()
    endif()
    if(MSVC)
        # The application manifest is embedded by SerialTest.rc.
        target_link_options(SerialTest PRIVATE /MANIFEST:NO)
    endif()
endif()
