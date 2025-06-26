cmake_minimum_required(VERSION 3.5)
project(wuff)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# - - - YAML-CPP
FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG master
)
FetchContent_MakeAvailable(yaml-cpp)

# - - - Tree-sitter
FetchContent_Declare(
    tree_sitter
    GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter.git
    GIT_TAG v0.20.9
)
FetchContent_GetProperties(tree_sitter)
if (NOT tree_sitter_POPULATED)
    message(STATUS "Fetching tree-sitter...")
    FetchContent_Populate(tree_sitter)

    # Since Tree-sitter does not use CMake, we manually add its source and include directories
    set(TREE_SITTER_SRC ${tree_sitter_SOURCE_DIR}/lib/src/lib.c)
    set(TREE_SITTER_INCLUDE_DIRS
        ${tree_sitter_SOURCE_DIR}/lib/include
        ${tree_sitter_SOURCE_DIR}/lib/src
        ${tree_sitter_SOURCE_DIR}/lib/src/unicode
    )
endif()

# Tree-Sitter WooWoo
set(WOOWOO_PARSER_SRC tree-sitter/woowoo/parser.c)
set(WOOWOO_SCANNER_SRC tree-sitter/woowoo/scanner.cc)

# Tree-Sitter YAML
set(YAML_PARSER_SRC tree-sitter/yaml/parser.c)
set(YAML_SCANNER_SRC tree-sitter/yaml/scanner.cc)

# Tree-Sitter BibTeX
set(BIBTEX_PARSER_SRC tree-sitter/bibtex/parser.c)

# - - - Pybind11
FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG v2.11.1
)
FetchContent_MakeAvailable(pybind11)

# - - - Build main module
pybind11_add_module(${PROJECT_NAME} NO_EXTRAS
    ${TREE_SITTER_SRC}
    ${WOOWOO_PARSER_SRC}
    ${WOOWOO_SCANNER_SRC}
    ${YAML_PARSER_SRC}
    ${YAML_SCANNER_SRC}
    ${BIBTEX_PARSER_SRC}
    Bindings.cpp
    WooWooAnalyzer.cpp
    project/WooWooDocument.cpp
    project/WooWooProject.cpp
    project/DialectedWooWooDocument.cpp
    project/MetaContext.cpp
    project/CommentLine.cpp
    project/UTF8toUTF16Mapping.cpp
    project/Woofile.cpp
    dialect/DialectManager.cpp
    dialect/DocumentPart.cpp
    dialect/Dialect.cpp
    dialect/MetaBlock.cpp
    dialect/Field.cpp
    dialect/Shorthand.cpp
    dialect/Reference.cpp
    dialect/Environment.cpp
    dialect/Wobject.cpp
    components/Component.cpp
    components/Hoverer.cpp
    components/Highlighter.cpp
    components/Navigator.cpp
    components/Completer.cpp
    components/Linter.cpp
    components/Folder.cpp
    parser/Parser.cpp
    utils/utils.cpp
)
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${TREE_SITTER_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC yaml-cpp::yaml-cpp)

if (APPLE)
    target_compile_options(${PROJECT_NAME} PRIVATE "-mmacosx-version-min=10.15")
    target_link_options(${PROJECT_NAME} PRIVATE "-mmacosx-version-min=10.15")
endif()

# - - - WooWooTest executable
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
    add_executable(WooWooTest
        ${TREE_SITTER_SRC}
        ${WOOWOO_PARSER_SRC}
        ${WOOWOO_SCANNER_SRC}
        ${YAML_PARSER_SRC}
        ${YAML_SCANNER_SRC}
        ${BIBTEX_PARSER_SRC}
        main.cpp
        WooWooAnalyzer.cpp
        project/WooWooDocument.cpp
        project/WooWooProject.cpp
        project/DialectedWooWooDocument.cpp
        project/MetaContext.cpp
        project/CommentLine.cpp
        project/UTF8toUTF16Mapping.cpp
        project/Woofile.cpp
        dialect/DialectManager.cpp
        dialect/DocumentPart.cpp
        dialect/Dialect.cpp
        dialect/MetaBlock.cpp
        dialect/Field.cpp
        dialect/Shorthand.cpp
        dialect/Reference.cpp
        dialect/Environment.cpp
        dialect/Wobject.cpp
        components/Component.cpp
        components/Hoverer.cpp
        components/Highlighter.cpp
        components/Navigator.cpp
        components/Completer.cpp
        components/Linter.cpp
        components/Folder.cpp
        parser/Parser.cpp
        utils/utils.cpp
    )
    target_include_directories(WooWooTest SYSTEM PRIVATE ${TREE_SITTER_INCLUDE_DIRS})
    target_link_libraries(WooWooTest PUBLIC yaml-cpp::yaml-cpp pybind11::embed)

    if (MSVC)
        target_compile_options(WooWooTest PRIVATE /W4)
    else()
        target_compile_options(WooWooTest PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

