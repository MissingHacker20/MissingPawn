cmake_minimum_required(VERSION 3.20)

project(MissingPawn
    VERSION 0.1.0
    DESCRIPTION "Missing Pawn Chess Engine"
    LANGUAGES CXX
)

# =========================
# Standard C++
# =========================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# =========================
# Kompilator
# =========================
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -O3
    )
endif()

# =========================
# Wszystkie pliki źródłowe
# =========================
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
    "*.cpp"
)

# =========================
# Plik wykonywalny
# =========================
add_executable(MissingPawn
    ${ENGINE_SOURCES}
)

# =========================
# Katalogi nagłówków
# =========================
target_include_directories(MissingPawn PRIVATE
    .
    Engine
    Foundation
    Game
    Move
    Others
    Others/UCI
    Rate
)

# =========================
# Definicje
# =========================
target_compile_definitions(MissingPawn PRIVATE
    _CRT_SECURE_NO_WARNINGS
)

# =========================
# Windows
# =========================
if(WIN32)
    target_compile_definitions(MissingPawn PRIVATE NOMINMAX)
endif()