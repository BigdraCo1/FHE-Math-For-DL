cmake_minimum_required(VERSION 3.10)
project(fhe_math_dl VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── C++ standard ──────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── OpenFHE ───────────────────────────────────────────────────────────────────
find_package(OpenFHE REQUIRED PATHS /usr/local/lib/OpenFHE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenFHE_CXX_FLAGS}")

# ── glaze (JSON) ──────────────────────────────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
  glaze
  GIT_REPOSITORY https://github.com/stephenberry/glaze.git
  GIT_TAG v2.3.1
)
FetchContent_MakeAvailable(glaze)

# ── Library target ────────────────────────────────────────────────────────────
add_library(fhe_math_dl STATIC
    src/mlp.cc
)

target_include_directories(fhe_math_dl
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    ${OpenFHE_INCLUDE}
    ${OpenFHE_INCLUDE}/core
    ${OpenFHE_INCLUDE}/pke
    ${OpenFHE_INCLUDE}/binfhe
    ${OPENMP_INCLUDES}
)

target_link_directories(fhe_math_dl PUBLIC
    ${OpenFHE_LIBDIR}
    ${OPENMP_LIBRARIES}
)

target_link_libraries(fhe_math_dl
  PUBLIC
    ${OpenFHE_LIBRARIES}
    glaze::glaze
)

# ── Example executable ────────────────────────────────────────────────────────
add_executable(iris_demo examples/iris_demo.cc)
target_link_libraries(iris_demo PRIVATE fhe_math_dl)

# ── Install rules ─────────────────────────────────────────────────────────────
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS fhe_math_dl
    EXPORT fhe_math_dlTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/fhe_math_dl
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT fhe_math_dlTargets
    FILE fhe_math_dlTargets.cmake
    NAMESPACE fhe_math_dl::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/fhe_math_dl
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/fhe_math_dlConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/fhe_math_dlConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/fhe_math_dl
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/fhe_math_dlConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/fhe_math_dlConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/fhe_math_dlConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/fhe_math_dl
)
