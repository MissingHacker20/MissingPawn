cmake_minimum_required(VERSION 3.20)

project(MissingPawn
    VERSION 2
    DESCRIPTION "MissingPawn Chess Engine"
    LANGUAGES CXX RC
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


if(MSVC)
    add_compile_options(/W4)
    add_compile_options(/O2 /arch:AVX2 /fp:fast /DNDEBUG)
else()
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -O3
        -funroll-loops
        -fomit-frame-pointer
    )
endif()

file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "${CMAKE_CURRENT_SOURCE_DIR}/Tests/.*")

set(ENGINE_CORE_SOURCES ${ENGINE_SOURCES})
list(FILTER ENGINE_CORE_SOURCES EXCLUDE REGEX ".*/main\\.cpp$")

add_executable(MissingPawn
    ${ENGINE_SOURCES}
    app.rc
)

target_include_directories(MissingPawn PRIVATE
    .
    Engine
    Foundation
    Game
    Move
    Others
    Others/UCI
    Rate
    OpeningBooks
)

target_compile_definitions(MissingPawn PRIVATE
    _CRT_SECURE_NO_WARNINGS
)

if(WIN32)
    target_compile_definitions(MissingPawn PRIVATE NOMINMAX)

    set_target_properties(MissingPawn PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
endif()

# Ustaw folder wyjściowy dla plików wykonywalnych na główny katalog projektu
set_target_properties(MissingPawn PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
    # Wszystkie inne pliki budowy (obiekty, biblioteki itp.) będą w folderze build
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

# MissingPawn.exe nie wymaga środowiska MinGW ani jego plików DLL.
if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_options(MissingPawn PRIVATE
        -static
        -static-libgcc
        -static-libstdc++
    )
endif()
