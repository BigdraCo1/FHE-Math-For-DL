# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/Users/bellian/CEPP/FHE-Math-For-DL/build/_deps/glaze-src")
  file(MAKE_DIRECTORY "/Users/bellian/CEPP/FHE-Math-For-DL/build/_deps/glaze-src")
endif()
file(MAKE_DIRECTORY
  "/Users/bellian/CEPP/FHE-Math-For-DL/build/_deps/glaze-build"
  "/Users/bellian/CEPP/FHE-Math-For-DL/build/_deps/glaze-subbuild/glaze-populate-prefix"
  "/Users/bellian/CEPP/FHE-Math-For-DL/build/_deps/glaze-subbuild/glaze-populate-prefix/tmp"
  "/Users/bellian/CEPP/FHE-Math-For-DL/build/_deps/glaze-subbuild/glaze-populate-prefix/src/glaze-populate-stamp"
  "/Users/bellian/CEPP/FHE-Math-For-DL/build/_deps/glaze-subbuild/glaze-populate-prefix/src"
  "/Users/bellian/CEPP/FHE-Math-For-DL/build/_deps/glaze-subbuild/glaze-populate-prefix/src/glaze-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/bellian/CEPP/FHE-Math-For-DL/build/_deps/glaze-subbuild/glaze-populate-prefix/src/glaze-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/bellian/CEPP/FHE-Math-For-DL/build/_deps/glaze-subbuild/glaze-populate-prefix/src/glaze-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
