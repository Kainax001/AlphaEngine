# ==============================================================================
# [SECTION 1] Source Files Collection
# ==============================================================================
file(GLOB_RECURSE SANDBOX_SOURCES "src/*.cpp")

# ==============================================================================
# [SECTION 2] Build Target (Executable)
# ==============================================================================
add_executable(Sandbox ${SANDBOX_SOURCES})

# ==============================================================================
# [SECTION 3] Library Linking
# ==============================================================================
target_link_libraries(Sandbox
    PRIVATE
        AlphaApplication
        AlphaGraphic
        AlphaWindow
        AlphaControler
)

# ==============================================================================
# [SECTION 4] Asset Directory Definition
# ==============================================================================
set(SANDBOX_ASSET_DIR_ABS "${CMAKE_CURRENT_SOURCE_DIR}/assets/")

message(STATUS "[Sandbox] Asset directory: ${SANDBOX_ASSET_DIR_ABS}")

target_compile_definitions(Sandbox PRIVATE
    SANDBOX_ASSET_DIR="${SANDBOX_ASSET_DIR_ABS}"
)