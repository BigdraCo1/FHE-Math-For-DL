cmake_minimum_required(VERSION 3.10)
project(ml_ckks_math)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 1. Set C++17 standard BEFORE finding OpenFHE
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 2. Find the OpenFHE package
find_package(OpenFHE REQUIRED PATHS /usr/local/lib/OpenFHE)

# 3. Set compiler flags from OpenFHE
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenFHE_CXX_FLAGS}")

# 4. Fetch glaze
include(FetchContent)

FetchContent_Declare(
  glaze
  GIT_REPOSITORY https://github.com/stephenberry/glaze.git
  GIT_TAG v2.3.1
)

FetchContent_MakeAvailable(glaze)

# 5. Add your executable
add_executable(ml_ckks_math
    main.cc
    mlp.cc
)

# 6. Include OpenFHE headers
target_include_directories(ml_ckks_math PRIVATE
    ${OpenFHE_INCLUDE}
    ${OpenFHE_INCLUDE}/core
    ${OpenFHE_INCLUDE}/pke
    ${OpenFHE_INCLUDE}/binfhe
    ${OPENMP_INCLUDES}
)

# 7. Link libraries
target_link_directories(ml_ckks_math PRIVATE
    ${OpenFHE_LIBDIR}
    ${OPENMP_LIBRARIES}
)

target_link_libraries(ml_ckks_math
    ${OpenFHE_LIBRARIES}
    glaze::glaze
)
