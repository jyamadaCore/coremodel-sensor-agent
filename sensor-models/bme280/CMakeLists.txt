cmake_minimum_required(VERSION 3.16)
project(bme280_coremodel C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(COREMODEL_DIR "/Users/jasonyamada/Documents/GitHub/coremodel-sensor-agent/coremodel" CACHE PATH "CoreModel checkout")

add_library(bme280_model_obj OBJECT src/bme280_model.c)
target_include_directories(bme280_model_obj PUBLIC src ${COREMODEL_DIR})
target_compile_options(bme280_model_obj PRIVATE -Wall -Wextra -Werror -pedantic)

add_library(coremodel_obj OBJECT ${COREMODEL_DIR}/coremodel.c)
target_include_directories(coremodel_obj PUBLIC ${COREMODEL_DIR})
target_compile_options(coremodel_obj PRIVATE -Wall)

add_executable(test_bme280 tests/test_bme280.c $<TARGET_OBJECTS:bme280_model_obj>)
target_include_directories(test_bme280 PRIVATE src ${COREMODEL_DIR})
target_compile_options(test_bme280 PRIVATE -Wall -Wextra -Werror -pedantic)
target_link_libraries(test_bme280 PRIVATE pthread)

add_executable(bme280-coremodel src/bme280_coremodel_main.c $<TARGET_OBJECTS:bme280_model_obj> $<TARGET_OBJECTS:coremodel_obj>)
target_include_directories(bme280-coremodel PRIVATE src ${COREMODEL_DIR})
target_compile_options(bme280-coremodel PRIVATE -Wall -Wextra -Werror -pedantic)
target_link_libraries(bme280-coremodel PRIVATE pthread)

enable_testing()
add_test(NAME bme280_unit COMMAND test_bme280)
