cmake_minimum_required(VERSION 3.25)
project(row-tracker LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# =======================================
# Dependancies
# =======================================
find_package(SQLite3 REQUIRED)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM REQUIRED IMPORTED_TARGET libsodium)

# =======================================
# Core library
# =======================================
add_library(core
    src/domain/calendar.cpp
    src/db/database.cpp
    src/db/migrations.cpp
    src/web/routes.cpp
    src/web/views.cpp
    src/auth/auth.cpp
)

target_include_directories(core PUBLIC src)
target_compile_options(core PUBLIC -Wall -Wextra -Wpedantic)
target_link_libraries(core PUBLIC 
    SQLite3::SQLite3 
    Threads::Threads 
    PkgConfig::SODIUM
)

# =======================================
# App
# =======================================
add_executable(row-tracker src/main.cpp)
target_link_libraries(row-tracker PRIVATE core)

# =======================================
# Tests
# =======================================
enable_testing()
add_executable(calendar_test tests/calendar_test.cpp)
target_link_libraries(calendar_test PRIVATE core)
add_test(NAME calendar_test COMMAND calendar_test)