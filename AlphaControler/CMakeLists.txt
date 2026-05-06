# ==============================================================================
# [SECTION 1] Dependency Loading
# ==============================================================================
find_package(glfw3 CONFIG REQUIRED)

# ==============================================================================
# [SECTION 2] Source Files Collection
# ==============================================================================
file(GLOB_RECURSE AC_SOURCES "src/*.cpp")

# ==============================================================================
# [SECTION 3] Build Target (Static Library)
# ==============================================================================
add_library(AlphaControler STATIC ${AC_SOURCES})

# ==============================================================================
# [SECTION 4] Include Directories
# ==============================================================================
target_include_directories(AlphaControler
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ==============================================================================
# [SECTION 5] Library Linking
# ==============================================================================
target_link_libraries(AlphaControler
    PUBLIC
        glfw
)
