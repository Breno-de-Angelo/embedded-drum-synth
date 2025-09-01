##
#  @file      Makefile
#  @brief     General makefile for EMF32GG
#  @version   v2.1 (Fixed dependency cycle)
#  @date      02/08/2025

#
#  @note      CMSIS library used
#
#  @note Options:
#  @param all          generate image file for microcontroller
#  @param host_tools   generate executable(s) for the host machine
#  @param build        generate the axf file
#  @param flash        write image file into microcontroller
#  @param clean        delete all generated files
#  (see 'make help' for more)
#

###############################################################################
# Main parameters
###############################################################################

# Program name
PROGNAME=embedded-drum-synth

# Verbose output for Make (Comment to have verbose output)
#MAKEFLAGS+= --silent --no-print-directory
.SILENT:

# Compatibility Windows/Linux
ifeq (${OS},Windows_NT)
	HOSTOS := Windows
else
	HOSTOS := $(shell uname -s)
endif

# Defines the part (device) that this project uses
PART=EFM32GG990F1024

###############################################################################
# Host Tools (for build machine)
###############################################################################

# Host compiler
HOST_CC = gcc
HOST_SCRIPT_SRC = scripts/Wave2C.c
HOST_SCRIPT_EXE = scripts/Wave2C

###############################################################################
# Project Directories and Files
###############################################################################

# Directories
BUILD_DIR   = bin
SOFTWARE_DIR = software
FIRMWARE_DIR= firmware
STARTUP_DIR = startup
SCRIPTS_DIR = scripts

# --- Sound File Conversion ---
SOUNDS_DIR    = sounds
WAVE2C_EXE    = ./scripts/Wave2C
WAVE2C_FLAGS  = -v -c -s

# Lista de arquivos .wav encontrados no diretório de sons
WAV_FILES       = $(wildcard $(SOUNDS_DIR)/*.wav)
# Lista dos arquivos .c que serão gerados a partir dos .wav
C_SOUND_FILES   = $(WAV_FILES:.wav=.h)

# VPATH tells make where to look for files
VPATH = $(SOFTWARE_DIR) $(FIRMWARE_DIR) $(STARTUP_DIR) $(SOUNDS_DIR)

# Source files for the TARGET (Microcontroller)
C_SOURCES = \
    $(wildcard $(SOFTWARE_DIR)/*.c) \
    $(wildcard $(FIRMWARE_DIR)/*.c) \
    $(wildcard $(STARTUP_DIR)/*.c)

# Object files are placed flat in the BUILD_DIR
OBJFILES = $(addprefix $(BUILD_DIR)/, $(notdir $(C_SOURCES:.c=.o)))

# Linker script
LINKERSCRIPT = $(STARTUP_DIR)/efm32gg.ld

# Entry Point
ENTRY=Reset_Handler

###############################################################################
# Toolchain Configuration
###############################################################################

# Toolchain prefix and debug mode
PREFIX:=arm-none-eabi
DEBUG=y

# Gecko_SDK Dir
GECKOSDKDIR=../Gecko_SDK

# CMSIS header files folders (ARM and manufacturer specific)
CMSISDIR=${GECKOSDKDIR}/platform/CMSIS/Core
CMSISDEVINCDIR=${GECKOSDKDIR}/platform/Device/SiliconLabs/EFM32GG/Include
CMSISINCDIR= ${CMSISDIR}/Include

# Include path
INCLUDEPATH = $(SOUNDS_DIR) $(SOFTWARE_DIR) $(FIRMWARE_DIR) $(STARTUP_DIR) ${CMSISDEVINCDIR} ${CMSISINCDIR}

# Commands Linux/Windows
ifeq (${HOSTOS},Windows)
	RM=rmdir /s /q
	MKDIR=mkdir
else
	RM=rm -rf
	MKDIR=mkdir -p
endif

###############################################################################
# Compilation parameters
###############################################################################

# Compiler parameters for Cortex-M3
CPUFLAGS = -mcpu=cortex-m3 -mthumb
FPUFLAGS = -mfloat-abi=soft
SPECFLAGS= --specs=nano.specs

# Section flags for code optimization
CSECTIONS = -ffunction-sections -fdata-sections
LSECTIONS = -Wl,--gc-sections

# C Error and Warning Messages Flags
CERRORFLAGS = -Wall -std=c11 -pedantic

# The flags passed to the C compiler.
CFLAGS = \
	${CPUFLAGS} \
	${FPUFLAGS} \
	${addprefix -I,${INCLUDEPATH}} \
	${CSECTIONS} \
	-D${PART} \
	${CERRORFLAGS}

# The flags passed to the linker.
LDFLAGS = \
	${LSECTIONS} \
	-Wl,-Map,${BUILD_DIR}/${PROGNAME}.map \
	-Wl,--print-memory-usage \
	--entry '${ENTRY}'

# Generate debug version if DEBUG is set
ifneq (${DEBUG},)
	CFLAGS+=-g -D DEBUG -O0
else
	CFLAGS+=-O3
endif

# Additional Flags
CFLAGS+= -Wuninitialized

# Controlling dependencies on header files
DEPFLAGS = -MT $@ -MMD -MP -MF $(patsubst %.o,%.d,$(@))

# Libraries linked
LIBS=

###############################################################################
# Commands
###############################################################################

# Toolchain commands
CC      = ${PREFIX}-gcc
LD      = ${PREFIX}-gcc
OBJCOPY = ${PREFIX}-objcopy
OBJDUMP = ${PREFIX}-objdump
OBJSIZE = ${PREFIX}-size
GDB     = ${PREFIX}-gdb

# JLink config (copied from your original Makefile)
USE_JLINK=yes
ifeq (${USE_JLINK},yes)
ifeq (${HOSTOS},Windows)
JLINK=JLink
GDBPROXY=JLinkGDBServer
else
JLINK=JLinkExe
GDBPROXY=JLinkGDBServer
endif
JLINKPARMS= -Device ${PART} -If SWD -Speed 4000
JLINKFLASHSCRIPT=${BUILD_DIR}/flash.jlink
endif

###############################################################################
# Rules
###############################################################################

# The rule for building object files from C source files.
# The '|' creates an order-only prerequisite, fixing the circular dependency.
${BUILD_DIR}/%.o: %.c | ${BUILD_DIR}
	@echo "  CC       $<"
	${CC} -c ${CFLAGS} ${SPECFLAGS} ${DEPFLAGS} -o $@ $<

# The rule for linking the application.
${BUILD_DIR}/${PROGNAME}.axf: ${OBJFILES}
	@echo "  LD       $@"
	${LD} ${CFLAGS} ${SPECFLAGS} -T'${LINKERSCRIPT}' ${LDFLAGS} -o $@ ${OBJFILES} ${LIBS}

# The rule for creating the final binary image.
${BUILD_DIR}/${PROGNAME}.bin: ${BUILD_DIR}/${PROGNAME}.axf
	@echo "  OBJCOPY  $@"
	${OBJCOPY} -O binary ${^} ${@}

# Rule to build the host script(s).
$(HOST_SCRIPT_EXE): $(HOST_SCRIPT_SRC)
	@echo "  HOST CC  $@"
	$(HOST_CC) -o $@ $<

# Rule to create the build directory.
${BUILD_DIR}:
	@echo "  MKDIR    $@"
	${MKDIR} ${BUILD_DIR}

###############################################################################
# Targets
###############################################################################

# Default target
default: build

# Build the final binary for the microcontroller
all: $(C_SOUND_FILES) ${BUILD_DIR}/${PROGNAME}.bin
	@echo "Firmware build complete: $@"

# Build the .axf file for the microcontroller
build: $(C_SOUND_FILES) ${BUILD_DIR}/${PROGNAME}.axf
	@echo "Firmware build complete: $@"

# Build the tools for the host machine
host_tools: $(HOST_SCRIPT_EXE)
	@echo "Host tools build complete."

# Transfer binary to board
flash: deploy
burn: deploy
deploy: all ${JLINKFLASHSCRIPT}
	@echo "Flashing binary to the board..."
	@${JLINK} ${JLINKPARMS} -CommanderScript ${JLINKFLASHSCRIPT}

${JLINKFLASHSCRIPT}: FORCE
	@echo r > ${JLINKFLASHSCRIPT}
	@echo h >> ${JLINKFLASHSCRIPT}
	@echo loadbin ${BUILD_DIR}/${PROGNAME}.bin,0 >> ${JLINKFLASHSCRIPT}
	@echo exit >> ${JLINKFLASHSCRIPT}
FORCE:

# Clean out all generated files
clean: docs-clean
	-$(RM) ${BUILD_DIR} $(HOST_SCRIPT_EXE) *~ $(C_SOUND_FILES)
	@echo "Clean complete."

# Show code size
size: all
	@${OBJSIZE} -A -x ${BUILD_DIR}/${PROGNAME}.axf

# Disassemble output file
dis: all
	@echo "Disassembling ${BUILD_DIR}/${PROGNAME}.axf -> ${BUILD_DIR}/${PROGNAME}.S"
	@${OBJDUMP} -x -S -D ${BUILD_DIR}/${PROGNAME}.axf > ${BUILD_DIR}/${PROGNAME}.S

# GDB initialization file
GDBINIT=${BUILD_DIR}/gdbinit

# Debug with gdb
gdb: all ${GDBINIT}
	${GDB} -x ${GDBINIT} -n ${BUILD_DIR}/${PROGNAME}.axf

# Debug using gdb with tui (text user interface}
tui: all ${GDBINIT}
	${GDB} --tui -x ${GDBINIT} -n ${BUILD_DIR}/${PROGNAME}.axf

# Create gdbinit
${GDBINIT}: FORCE
	@echo # Run this script using gdb source command > ${GDBINIT}
	@echo target remote localhost:2331 >> ${GDBINIT}
	@echo monitor halt >> ${GDBINIT}
	@echo load >> ${GDBINIT}
	@echo monitor reset >> ${GDBINIT}
	@echo break main >> ${GDBINIT}
	@echo continue >> ${GDBINIT}

# Target to start the GDB Server in a new terminal
gdb-server:
	@echo "==> Starting J-Link GDB Server on localhost:2331..."
	@echo "    (Close the new terminal to stop the server)"
	@gnome-terminal -- $(GDBPROXY) $(JLINKPARMS) &

# Help message
help:
	@echo "Usage: make [target]"
	@echo "------------------------------------------------------------"
	@echo "Primary Targets:"
	@echo "  all          - Build firmware & sounds, create final .bin file."
	@echo "  build        - Build firmware & sounds, create .axf executable."
	@echo "  flash        - Flash the final binary to the microcontroller."
	@echo "  clean        - Remove all generated files."
	@echo ""
	@echo "Utility Targets:"
	@echo "  host_tools   - Build executable scripts for the host machine."
	@echo "  docs         - Generate project documentation using Doxygen."
	@echo ""
	@echo "Analysis:"
	@echo "  size         - Show code, data, and bss size."
	@echo "  dis          - Disassemble the executable."
	@echo ""
	@echo "Debug:"
	@echo "  gdb-server   - Starts the J-Link GDB server in a new terminal."
	@echo "  debug        - Creates debug session with gdb"
	@echo "  gdb          - Creates debug session with gdb (same as debug)"
	@echo "  tui       	  - Creates debug session with gdb text user interface"

DOXYGEN = doxygen

# Target to generate documentation from Doxyfile
docs: Doxyfile
	@echo "==> Generating documentation with Doxygen..."
	@$(DOXYGEN) Doxyfile
	@echo "Documentation generated in 'html' directory. ✨"

# Target to clean generated documentation
docs-clean:
	@echo "==> Cleaning Doxygen documentation..."
	-$(RM) html latex

# Regra de Padrão para converter um arquivo .wav em .c
$(SOUNDS_DIR)/%.h: $(SOUNDS_DIR)/%.wav $(HOST_SCRIPT_EXE)
	@echo "  WAVE2C   $<"
	@./$(HOST_SCRIPT_EXE) $(WAVE2C_FLAGS) $<
	@echo "  MV       $(notdir $@) -> $(dir $@)"
	@mv $(notdir $@) $(dir $@)

# Adicione 'sounds' à lista .PHONY
.PHONY: all build host_tools sounds flash clean size dis help default FORCE burn deploy gdb docs docs-clean

# Include dependency files generated by the compiler.
-include $(OBJFILES:.o=.d)
