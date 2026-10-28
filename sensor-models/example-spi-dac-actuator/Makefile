CC ?= cc
COREMODEL_DIR ?= ../../coremodel
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic -O2 -g
CPPFLAGS += -Iinclude

MODEL_SRC := src/example_spi_dac_actuator.c
TEST_SRC := tests/test_example_spi_dac_actuator.c

.PHONY: all test clean

ifneq ($(wildcard $(COREMODEL_DIR)/coremodel.h),)
COREMODEL_AVAILABLE := 1
endif

all: test
ifeq ($(COREMODEL_AVAILABLE),1)
all: run-example-spi-dac-actuator
endif

test: test-example-spi-dac-actuator
	./test-example-spi-dac-actuator

test-example-spi-dac-actuator: $(MODEL_SRC) $(TEST_SRC) include/example_spi_dac_actuator.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(MODEL_SRC) $(TEST_SRC)

ifeq ($(COREMODEL_AVAILABLE),1)
# The upstream inspected CoreModel implementation is compiled without this
# generated model's -Werror policy; the local library intentionally uses C
# extensions (e.g. zero-length arrays) and is treated as authoritative, not
# something this project corrects.
run-example-spi-dac-actuator: $(MODEL_SRC) src/example_spi_dac_actuator_main.c include/example_spi_dac_actuator.h coremodel.o
	$(CC) $(CPPFLAGS) -I$(COREMODEL_DIR) $(CFLAGS) -o $@ $(MODEL_SRC) src/example_spi_dac_actuator_main.c coremodel.o -lpthread

coremodel.o: $(COREMODEL_DIR)/coremodel.c $(COREMODEL_DIR)/coremodel.h
	$(CC) -std=c11 -Wall -I$(COREMODEL_DIR) -c -o $@ $(COREMODEL_DIR)/coremodel.c
endif

clean:
	rm -f test-example-spi-dac-actuator run-example-spi-dac-actuator coremodel.o
