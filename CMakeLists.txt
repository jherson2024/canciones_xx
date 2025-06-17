cmake_minimum_required(VERSION 3.10)
project(mi_backend_cpp)
set(CMAKE_CXX_STANDARD 20)
# Integración con vcpkg
if(DEFINED ENV{VCPKG_ROOT})
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()
# Buscar cpprestsdk
find_package(cpprestsdk CONFIG REQUIRED)
# Archivos fuente
add_executable(mi_backend_cpp
  src/main.cpp         
  src/recomendador.cpp
)
target_include_directories(mi_backend_cpp PRIVATE src)
target_link_libraries(mi_backend_cpp PRIVATE cpprestsdk::cpprest)
