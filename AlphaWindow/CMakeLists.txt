# ==============================================================================
# [SECTION 1] Dependency Loading
# ==============================================================================
find_package(glfw3      CONFIG REQUIRED)
find_package(glad       CONFIG REQUIRED)

# ==============================================================================
# [SECTION 2] Source Files Collection
# ==============================================================================
file(GLOB_RECURSE AW_SOURCES "src/*.cpp")

# ==============================================================================
# [SECTION 3] Build Target (Static Library)
# ==============================================================================
add_library(AlphaWindow STATIC ${AW_SOURCES})

# ==============================================================================
# [SECTION 4] Include Directories
# ==============================================================================
target_include_directories(AlphaWindow
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ==============================================================================
# [SECTION 5] Library Linking
# ==============================================================================
target_link_libraries(AlphaWindow
    PUBLIC
        glfw
        glad::glad      # Window.cpp에서 glViewport 사용
        AlphaUtil       # rapidjson 헤더 포함
)
