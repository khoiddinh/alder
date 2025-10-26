# Gather source files
file(GLOB ALDER_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

# Build the library
add_library(alder_lib ${ALDER_SOURCES})

# Build the executable (main.cpp must exist in src/)
add_executable(alder main.cpp)

# Link the library into the executable
target_link_libraries(alder PRIVATE alder_lib)