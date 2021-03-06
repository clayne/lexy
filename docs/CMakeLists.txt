# Copyright (C) 2020-2021 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

find_program(HUGO_BINARY hugo)
if(NOT HUGO_BINARY)
    message(WARNING "hugo not found, won't be able to generate documentation")
endif()

add_custom_command(OUTPUT content/_index.adoc
        COMMAND ${CMAKE_COMMAND} -E copy ../README.adoc content/_index.adoc
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../README.adoc)

set(resolver "${CMAKE_CURRENT_SOURCE_DIR}/../support/resolve-lexy-headers.py")
add_custom_command(OUTPUT assets/cpp/playground_headers.single.hpp
        COMMAND ${resolver} assets/cpp/playground_headers.hpp assets/cpp/playground_headers.single.hpp
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS foonathan::lexy)

add_custom_target(lexy_docs
        COMMAND ${HUGO_BINARY} --minify
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS content/_index.adoc assets/cpp/playground_headers.single.hpp)

add_custom_target(lexy_docs_serve
        COMMAND ${HUGO_BINARY} serve
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        USES_TERMINAL
        DEPENDS content/_index.adoc assets/cpp/playground_headers.single.hpp)

