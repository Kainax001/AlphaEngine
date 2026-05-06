# ==============================================================================
# [SECTION 1] Build Target (Header-Only Library)
# ==============================================================================
add_library(AlphaMessenger INTERFACE)

# ==============================================================================
# [SECTION 2] Include Directories
# ==============================================================================
target_include_directories(AlphaMessenger
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include
)
