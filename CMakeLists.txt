cmake_minimum_required(VERSION 3.22)
project(ArtPlus VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Test)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Source files
set(DOCUMENT_SOURCES
    src/document/document_model.cpp
    src/document/document_model.h
)

add_executable(ArtPlus
    src/app/main.cpp
    ${DOCUMENT_SOURCES}
)

target_link_libraries(ArtPlus PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)

enable_testing()
add_executable(unit_tests 
    tests/unit/test_main.cpp
    ${DOCUMENT_SOURCES}
)
target_link_libraries(unit_tests PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test)
add_test(NAME UnitTests COMMAND unit_tests)

