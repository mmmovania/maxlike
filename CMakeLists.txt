cmake_minimum_required(VERSION 3.20)
project(maxlike VERSION 0.0.1 LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT WIN32)
    message(FATAL_ERROR "maxlike is a Windows application")
endif()

add_executable(maxlike WIN32
    src/main.cpp
    src/win/App.cpp
    src/win/App.h
    src/win/Window.cpp
    src/win/Window.h
    src/ui/Theme.cpp
    src/ui/Theme.h
    src/ui/MainFrame.cpp
    src/ui/MainFrame.h
    src/ui/Metrics.h
    src/ui/FrameLayout.cpp
    src/ui/FrameLayout.h
    src/ui/Bevel.cpp
    src/ui/Bevel.h
    src/ui/Placeholder.cpp
    src/ui/Placeholder.h
    src/ui/CommandTable.cpp
    src/ui/CommandTable.h
    src/ui/MenuSpec.cpp
    src/ui/MenuSpec.h
    src/ui/MenuBuilder.cpp
    src/ui/MenuBuilder.h
    res/maxlike.rc
)

target_include_directories(maxlike PRIVATE src)

target_compile_definitions(maxlike PRIVATE
    UNICODE _UNICODE
    WIN32_LEAN_AND_MEAN NOMINMAX
    WINVER=0x0A00 _WIN32_WINNT=0x0A00
    MAXLIKE_VERSION="${PROJECT_VERSION}"
)

if(MSVC)
    target_compile_options(maxlike PRIVATE /W4 /permissive- /utf-8)
    # Our own manifest comes from res/maxlike.rc; stop the linker adding a second one.
    target_link_options(maxlike PRIVATE /MANIFEST:NO)
else()
    target_compile_options(maxlike PRIVATE -Wall -Wextra)
    target_link_options(maxlike PRIVATE -municode -static)
endif()

target_link_libraries(maxlike PRIVATE user32 gdi32)

# ---- Unit tests (console program; run with: ctest --test-dir build -C Debug) ----
enable_testing()

# Each test is its own small console program; CTest runs them all.
function(maxlike_add_test name)
    add_executable(${name} ${ARGN})
    target_include_directories(${name} PRIVATE src)
    if(MSVC)
        target_compile_options(${name} PRIVATE /W4 /permissive- /utf-8)
    else()
        target_compile_options(${name} PRIVATE -Wall -Wextra)
        target_link_options(${name} PRIVATE -static)
    endif()
    add_test(NAME ${name} COMMAND ${name})
endfunction()

maxlike_add_test(test_frame_layout tests/test_frame_layout.cpp src/ui/FrameLayout.cpp)
maxlike_add_test(test_menu_spec
    tests/test_menu_spec.cpp src/ui/MenuSpec.cpp src/ui/CommandTable.cpp)
