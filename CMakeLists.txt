cmake_minimum_required(VERSION 3.25)
project(row-tracker LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE)
endif()

find_package(SQLite3 REQUIRED)
find_package(Theads REQUIRED)

# =======================================
# Core library
# =======================================
add_library(core
    src/domain/calendar.cpp
    src/db/database.cpp
    src/db/migrations.cpp
    src/web/routes.cpp
    src/web/views.cpp)
target_include_directories(core PUBLIC src third_party)