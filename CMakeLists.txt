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
    src/LandingPage.cpp
    src/ReservationDialog.cpp
)
target_include_directories(TaraShuttle PRIVATE include)
target_link_libraries(TaraShuttle PRIVATE wx::core wx::base)

add_custom_command(TARGET TaraShuttle POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:TaraShuttle>/assets"
    COMMENT "Copying TaraMapuan landing page assets"
)

if(MSVC)
    target_compile_options(TaraShuttle PRIVATE /W4 /permissive- /utf-8)
endif()
