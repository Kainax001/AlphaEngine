# ==============================================================================
# [SECTION 1] Dependency Loading
# ==============================================================================
find_package(RapidJSON CONFIG REQUIRED)

# ==============================================================================
# [SECTION 2] Source Files Collection
# ==============================================================================
file(GLOB_RECURSE AU_SOURCES "src/*.cpp")

# ==============================================================================
# [SECTION 3] Build Target (Static Library)
# ==============================================================================
add_library(AlphaUtil STATIC ${AU_SOURCES})

# ==============================================================================
# [SECTION 4] Include Directories
# ==============================================================================
# RAPIDJSON_INCLUDE_DIRS is PUBLIC so any module linking AlphaUtil
# can include rapidjson headers without its own find_package call.
target_include_directories(AlphaUtil
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${RAPIDJSON_INCLUDE_DIRS}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)
