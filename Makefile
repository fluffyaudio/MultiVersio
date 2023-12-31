# Project Name
TARGET = MultiVersio

.SHELLFLAGS += -e

# set to 1
DEBUG ?= 0

# Library Locations
LIBDAISY_DIR = libDaisy
DAISYSP_DIR = DaisySP
CMSIS_DIR = $(LIBDAISY_DIR)/Drivers/CMSIS

# dependent libs must build first
.PHONY: project
project: submodules $(LIBDAISY_DIR) $(DAISYSP_DIR) all  ## all is resolved via later include

.PHONY: submodules
submodules:
	git submodule update --init --recursive

.PHONY: flash
flash: program-dfu

.PHONY: libDaisy
libDaisy:
	$(MAKE) -C $(LIBDAISY_DIR) -j

.PHONY: DaisySP
DaisySP: libDaisy
	$(MAKE) -C $(DAISYSP_DIR) -j

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))
CPP_SOURCES := MultiVersio.cpp $(call rwildcard, fx, *.cpp) $(call rwildcard, core, *.cpp)

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

# Additional Include Paths, e.g. ./stmlib resolution
C_INCLUDES += -I. -Icore/ -Ifx/
CPP_INCLUDES += -I. -Icore/ -Ifx/

lint:
	python ./libDaisy/ci/run-clang-format.py -r ./ \
		--extensions c,cpp,h

docs: submodules
	@which doxygen > /dev/null || (echo "Doxygen is not installed. Please visit https://www.doxygen.nl/manual/install.html for installation instructions." && exit 1)
	doxygen Doxyfile
