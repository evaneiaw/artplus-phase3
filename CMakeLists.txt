cmake_minimum_required(VERSION 3.22)
project(ArtPlus VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Test ZlibPrivate)

# O Qt 6.8.3 mingw_64 pré-compilado usa a CRT MSVCRT (msvcrt.dll).
# Misturar executáveis compilados contra UCRT com DLLs MSVCRT do Qt
# corrompe o heap. Use o preset "mingw-msvcrt" (CMakePresets.json).

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Source files
set(DOCUMENT_SOURCES
    src/document/document_model.cpp
    src/document/document_model.h
    src/document/document_serializer.cpp
    src/document/document_serializer.h
)

set(PDF_SOURCES
    src/pdf/pdf_types.cpp
    src/pdf/pdf_types.h
    src/pdf/pdf_lexer.cpp
    src/pdf/pdf_lexer.h
    src/pdf/pdf_parser.h
    src/pdf/pdf_document.cpp
    src/pdf/pdf_document.h
    src/pdf/pdf_content.cpp
    src/pdf/pdf_content.h
    src/pdf/pdf_importer.cpp
    src/pdf/pdf_importer.h
    src/pdf/pdf_exporter.cpp
    src/pdf/pdf_exporter.h
)

set(UI_SOURCES
    src/ui/main_window.cpp
    src/ui/main_window.h
)

add_executable(pdf_probe
    tests/integration/pdf_probe.cpp
    ${DOCUMENT_SOURCES}
    ${PDF_SOURCES}
)
target_link_libraries(pdf_probe PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::ZlibPrivate)

add_executable(pdf_smoke
    tests/integration/pdf_smoke.cpp
    ${DOCUMENT_SOURCES}
    ${PDF_SOURCES}
)
target_link_libraries(pdf_smoke PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::ZlibPrivate)

add_executable(ArtPlus
    src/app/main.cpp
    ${DOCUMENT_SOURCES}
    ${PDF_SOURCES}
    ${UI_SOURCES}
)

target_link_libraries(ArtPlus PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::ZlibPrivate)

enable_testing()
add_executable(unit_tests 
    tests/unit/test_main.cpp
    ${DOCUMENT_SOURCES}
    ${PDF_SOURCES}
)
target_link_libraries(unit_tests PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test Qt6::ZlibPrivate)
add_test(NAME UnitTests COMMAND unit_tests)

add_executable(pdf_roundtrip_test
    tests/unit/test_pdf_roundtrip.cpp
    ${DOCUMENT_SOURCES}
    ${PDF_SOURCES}
)
target_link_libraries(pdf_roundtrip_test PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test Qt6::ZlibPrivate)
add_test(NAME PdfRoundTrip COMMAND pdf_roundtrip_test)
set_tests_properties(UnitTests PROPERTIES ENVIRONMENT "PATH=C:/Qt/6.8.3/mingw_64/bin")
set_tests_properties(PdfRoundTrip PROPERTIES ENVIRONMENT "PATH=C:/Qt/6.8.3/mingw_64/bin")
