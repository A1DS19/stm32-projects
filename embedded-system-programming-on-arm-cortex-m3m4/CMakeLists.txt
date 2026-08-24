# Set the minimum version of CMake that can be used
cmake_minimum_required(VERSION 3.20)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)

# Set target definition variables required by the toolchain file
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/target.cmake)

# Set the toolchain file for cross-compilation
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/gcc-arm-none-eabi.cmake)

# Project definition
project(embedded-system-programming-on-arm-cortex-m3m4)

# Enable CMake support for ASM and C/CPP languages
enable_language(C ASM)

set(PROJECT_NAME embedded-system-programming-on-arm-cortex-m3m4)

# Create an executable object type
add_executable(${PROJECT_NAME})

# Include additional CMake files for flags, components, and files
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/flags.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/components.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/files.cmake)

# Add sources to executable/library
target_sources(${PROJECT_NAME} PRIVATE
  # User defined sources
  ${CMAKE_CURRENT_SOURCE_DIR}/Src/system.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Src/uart2.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Src/modes.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Src/access.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Src/coreregs.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Src/inlineasm.c
  ${CMAKE_CURRENT_SOURCE_DIR}/Src/resetseq.c
)

# CMSIS headers from the installed Cube firmware pack — register structs, no HAL sources
set(CUBE_FW_L4 /home/dev/STM32Cube/Repository/STM32Cube_FW_L4_V1.18.2)

# Add include directories
target_include_directories(${PROJECT_NAME} PRIVATE
  # User defined include directories
  ${CMAKE_CURRENT_SOURCE_DIR}/Inc
  ${CUBE_FW_L4}/Drivers/CMSIS/Device/ST/STM32L4xx/Include
  ${CUBE_FW_L4}/Drivers/CMSIS/Include
)

# Compiler options
target_compile_options(${PROJECT_NAME} PRIVATE
  # User defined compiler options
)

# Add additional defined symbols
target_compile_definitions(${PROJECT_NAME} PRIVATE
  # Add additional defined symbols here
  STM32L476xx
)

# Linker options
target_link_options(${PROJECT_NAME} PRIVATE
  # User defined linker options
)

# Add additional linker search directories
target_link_directories(${PROJECT_NAME} PRIVATE
  # Add additional linker search directories here
)

# Add additional linker libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
  # Add additional linker libraries here
)
