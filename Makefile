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
project: $(LIBDAISY_DIR) $(DAISYSP_DIR) all  ## all is resolved via later include

.PHONY: libDaisy
libDaisy:
	$(MAKE) -C $(LIBDAISY_DIR) -j

.PHONY: DaisySP
DaisySP: libDaisy
	$(MAKE) -C $(DAISYSP_DIR) -j

# Sources for this project
CPP_SOURCES = MultiVersio.cpp

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

# Additional Include Paths, e.g. ./stmlib resolution
CFLAGS += -I.
CPPFLAGS += -I.

lint:
	python ./libDaisy/ci/run-clang-format.py -r ./ \
		--extensions c,cpp,h
