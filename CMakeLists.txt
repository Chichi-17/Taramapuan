cmake_minimum_required(VERSION 3.20)
project(TaraShuttle VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(wxWidgets CONFIG REQUIRED)

add_executable(TaraShuttle WIN32
    src/main.cpp
    src/MainFrame.cpp
    src/MainFramePages.cpp
    src/ReservationDialog.cpp
)
target_include_directories(TaraShuttle PRIVATE include)
target_link_libraries(TaraShuttle PRIVATE wx::core wx::base)

if(MSVC)
    target_compile_options(TaraShuttle PRIVATE /W4 /permissive-)
endif()
