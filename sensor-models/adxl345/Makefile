CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -O2 -g
.PHONY: all test clean app
all: test
adxl345-test: tests/test_adxl345.c adxl345_model.c adxl345_model.h
	$(CC) $(CFLAGS) -I. -o $@ tests/test_adxl345.c adxl345_model.c
test: adxl345-test
	./adxl345-test
app: adxl345-coremodel
adxl345-coremodel: adxl345_coremodel.c adxl345_model.c adxl345_model.h coremodel.o
ifndef COREMODEL_DIR
	$(error COREMODEL_DIR must be set to build the CoreModel adapter executable)
endif
	$(CC) $(CFLAGS) -I. -I$(COREMODEL_DIR) -o $@ adxl345_coremodel.c adxl345_model.c coremodel.o -lpthread
# The upstream inspected CoreModel implementation is compiled without this
# generated model's -Werror policy; the local library intentionally uses C
# extensions (e.g. zero-length arrays) and is treated as authoritative, not
# something this project corrects.
coremodel.o: $(COREMODEL_DIR)/coremodel.c $(COREMODEL_DIR)/coremodel.h
	$(CC) -std=c11 -Wall -I$(COREMODEL_DIR) -c -o $@ $(COREMODEL_DIR)/coremodel.c
clean:
	rm -f adxl345-test adxl345-coremodel coremodel.o *.o
	rm -rf build CMakeFiles CMakeCache.txt Testing cmake_install.cmake CTestTestfile.cmake
