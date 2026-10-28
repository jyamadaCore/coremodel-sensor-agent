cmake_minimum_required(VERSION 3.16)
project(example_spi_dac_actuator C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(example_spi_dac_actuator_model STATIC
    src/example_spi_dac_actuator.c
)
target_include_directories(example_spi_dac_actuator_model PUBLIC include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(example_spi_dac_actuator_model PRIVATE -Wall -Wextra -Werror -pedantic)
endif()

add_executable(test_example_spi_dac_actuator
    tests/test_example_spi_dac_actuator.c
)
target_link_libraries(test_example_spi_dac_actuator PRIVATE example_spi_dac_actuator_model)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(test_example_spi_dac_actuator PRIVATE -Wall -Wextra -Werror -pedantic)
endif()

enable_testing()
add_test(NAME example_spi_dac_actuator_unit COMMAND test_example_spi_dac_actuator)

set(COREMODEL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../coremodel" CACHE PATH "Path to inspected CoreModel checkout")
if(EXISTS "${COREMODEL_DIR}/coremodel.h" AND EXISTS "${COREMODEL_DIR}/coremodel.c")
    add_executable(run-example-spi-dac-actuator
        src/example_spi_dac_actuator_main.c
        ${COREMODEL_DIR}/coremodel.c
    )
    target_include_directories(run-example-spi-dac-actuator PRIVATE include ${COREMODEL_DIR})
    target_link_libraries(run-example-spi-dac-actuator PRIVATE example_spi_dac_actuator_model pthread)
    # Do not compile the upstream inspected CoreModel implementation with this
    # generated model's -Werror policy; the local library intentionally uses C
    # extensions such as zero-length arrays.
    if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
        set_source_files_properties(src/example_spi_dac_actuator_main.c PROPERTIES
            COMPILE_OPTIONS "-Wall;-Wextra;-Werror;-pedantic")
    endif()
endif()
