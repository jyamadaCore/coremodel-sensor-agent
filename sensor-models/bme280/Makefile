COREMODEL_DIR ?= /Users/jasonyamada/Documents/GitHub/coremodel-sensor-agent/coremodel
CC ?= cc
GEN_CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic -I./src -I$(COREMODEL_DIR)
COREMODEL_CFLAGS := -std=c11 -Wall -I$(COREMODEL_DIR)
LDFLAGS += -pthread

.PHONY: all test clean
all: tests/test_bme280 bme280-coremodel

tests/test_bme280: tests/test_bme280.c src/bme280_model.c src/bme280_model.h
	$(CC) $(GEN_CFLAGS) tests/test_bme280.c src/bme280_model.c -o $@ $(LDFLAGS)

build/coremodel.o: $(COREMODEL_DIR)/coremodel.c $(COREMODEL_DIR)/coremodel.h
	mkdir -p build
	$(CC) $(COREMODEL_CFLAGS) -c $(COREMODEL_DIR)/coremodel.c -o $@

build/bme280_model.o: src/bme280_model.c src/bme280_model.h
	mkdir -p build
	$(CC) $(GEN_CFLAGS) -c src/bme280_model.c -o $@

build/bme280_coremodel_main.o: src/bme280_coremodel_main.c src/bme280_model.h
	mkdir -p build
	$(CC) $(GEN_CFLAGS) -c src/bme280_coremodel_main.c -o $@

bme280-coremodel: build/bme280_coremodel_main.o build/bme280_model.o build/coremodel.o
	$(CC) $^ -o $@ $(LDFLAGS)

test: all
	./tests/test_bme280

clean:
	rm -rf build tests/test_bme280 bme280-coremodel
