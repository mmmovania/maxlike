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
