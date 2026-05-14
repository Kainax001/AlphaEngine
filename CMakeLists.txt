# ==============================================================================
# [SECTION 1] Project Configuration
# ==============================================================================
cmake_minimum_required(VERSION 3.15)
project(AlphaEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==============================================================================
# [SECTION 2] Common Dependencies
# ==============================================================================
find_package(glfw3 CONFIG REQUIRED)

# ==============================================================================
# [SECTION 3] Subprojects
# ==============================================================================
add_subdirectory(AlphaGraphic)
add_subdirectory(AlphaWindow)
add_subdirectory(AlphaControler)
add_subdirectory(AlphaMessenger)
add_subdirectory(AlphaScene)
add_subdirectory(AlphaApplication)
add_subdirectory(Sandbox)