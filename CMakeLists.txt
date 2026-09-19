cmake_minimum_required(VERSION 3.26)
project(roll_and_sprint CXX)

# The Dusklight version to build against: a release tag (e.g. "v1.5.0") or a commit SHA.
if (NOT DUSKLIGHT_VERSION)
    set(DUSKLIGHT_VERSION "e9b120544cb75e81b5aa36777f1688fa61f2d9e8")
endif ()
# Fetches the Dusklight sources into ./dusklight and provides DUSKLIGHT_DIR.
# Use -DDUSKLIGHT_DIR= to point at an existing Dusklight checkout.
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/FetchDusklight.cmake")
add_subdirectory("${DUSKLIGHT_DIR}/sdk" dusklight-sdk EXCLUDE_FROM_ALL)

add_mod(roll_and_sprint
        FEATURES game
        SOURCES src/mod.cpp
        MOD_JSON mod.json
		RES_DIR res
)
