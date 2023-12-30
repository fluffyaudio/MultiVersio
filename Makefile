
# Project Name
TARGET = MultiEffect
# Library Locations
LIBDAISY_DIR = ../libdaisy
DAISYSP_DIR = ../DaisySP
CMSIS_DIR = $(LIBDAISY_DIR)/Drivers/CMSIS
C_INCLUDES += -I../stmlib/fft -I../stmlib/dsp -I.. -I./fx -I./core

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

# Sources
CPP_SOURCES = $(call rwildcard,.,*.cpp)

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

docs:
	doxygen Doxyfile
