file(GLOB_RECURSE AS_SOURCES "src/*.cpp")
add_library(AlphaScene STATIC ${AS_SOURCES})

target_include_directories(AlphaScene
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(AlphaScene
    PUBLIC
        AlphaGraphic
        AlphaControler
        AlphaUtil
)
