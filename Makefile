# Project Name
TARGET = MultiEffect

# Library Locations
LIBDAISY_DIR = ../../libdaisy
DAISYSP_DIR = ../../DaisySP
CMSIS_DIR = $(LIBDAISY_DIR)/Drivers/CMSIS

# Sources
CPP_SOURCES = MultiEffect.cpp

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile