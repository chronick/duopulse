###############################################################################
# Makefile for DaisySP Patch.Init Eurorack Firmware
###############################################################################

# Project Configuration
PROJECT_NAME := patch-init-firmware
TARGET := patch
DAISYSP_PATH ?= ./DaisySP
LIBDAISY_PATH ?= ./libDaisy

# Build Configuration
BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
SRC_DIR := src
INC_DIR := inc
TEST_DIR := tests

# Toolchain
# Try to locate the official Arm toolchain on macOS if GCC_PATH is not set
ifndef GCC_PATH
    # Check for the specific version found on this machine
    ARM_TOOLCHAIN_DIR := /Applications/ArmGNUToolchain/14.3.rel1/arm-none-eabi/bin
    ifneq ($(wildcard $(ARM_TOOLCHAIN_DIR)/arm-none-eabi-gcc),)
        GCC_PATH := $(ARM_TOOLCHAIN_DIR)
    endif
endif

# If GCC_PATH is set, use it; otherwise use system PATH
ifdef GCC_PATH
PREFIX := $(GCC_PATH)/arm-none-eabi-
else
PREFIX := arm-none-eabi-
endif
CC := $(PREFIX)gcc
CXX := $(PREFIX)g++
AS := $(PREFIX)as
AR := $(PREFIX)ar
OBJCOPY := $(PREFIX)objcopy
OBJDUMP := $(PREFIX)objdump
SIZE := $(PREFIX)size

# Compiler Flags
CXXFLAGS := -std=gnu++14
CXXFLAGS += -Wall -Wextra -Wno-unused-parameter
CXXFLAGS += -Wno-missing-attributes -Wno-stringop-overflow
CXXFLAGS += -fno-exceptions -fno-rtti
CXXFLAGS += -ffunction-sections -fdata-sections
CXXFLAGS += -fno-builtin -fno-common
CXXFLAGS += -fasm -finline -finline-functions-called-once
CXXFLAGS += -fshort-enums -fno-move-loop-invariants
CXXFLAGS += -fno-unwind-tables -fno-rtti -Wno-register
CXXFLAGS += -mcpu=cortex-m7 -mthumb
CXXFLAGS += -mfloat-abi=hard -mfpu=fpv5-d16
CXXFLAGS += -O2 -g -ggdb

# Compiler Defines
CXXFLAGS += -DCORE_CM7
CXXFLAGS += -DSTM32H750xx
CXXFLAGS += -DSTM32H750IB
CXXFLAGS += -DARM_MATH_CM7
CXXFLAGS += -DUSE_HAL_DRIVER
CXXFLAGS += -DUSE_FULL_LL_DRIVER
CXXFLAGS += -DHSE_VALUE=16000000
CXXFLAGS += -DUSE_FATFS
CXXFLAGS += -DFILEIO_ENABLE_FATFS_READER

# Logging Configuration
# LOG_COMPILETIME_LEVEL: Minimum log level to compile (0=TRACE, 1=DEBUG, 2=INFO, 3=WARN, 4=ERROR, 5=OFF)
#   - Logs below this level are stripped at compile time (zero binary size cost)
# LOG_DEFAULT_LEVEL: Default runtime filter level (same values as above)
#   - Can be changed at runtime with logging::SetLevel()
#
# Development build (keep DEBUG+ logs, default to INFO at runtime):
LOG_COMPILETIME_LEVEL ?= 1
LOG_DEFAULT_LEVEL ?= 2
CXXFLAGS += -DLOG_COMPILETIME_LEVEL=$(LOG_COMPILETIME_LEVEL)
CXXFLAGS += -DLOG_DEFAULT_LEVEL=$(LOG_DEFAULT_LEVEL)
#
# Release build (only WARN/ERROR, quiet by default):
# LOG_COMPILETIME_LEVEL=3 LOG_DEFAULT_LEVEL=3 make

# Include Directories
# Use -I for our own headers, -isystem for external libs (suppresses warnings)
INCLUDES := -I$(INC_DIR)
INCLUDES += -isystem $(DAISYSP_PATH)/Source
INCLUDES += -isystem $(DAISYSP_PATH)/Source/daisysp
INCLUDES += -isystem $(LIBDAISY_PATH)
INCLUDES += -isystem $(LIBDAISY_PATH)/src
INCLUDES += -isystem $(LIBDAISY_PATH)/src/sys
INCLUDES += -isystem $(LIBDAISY_PATH)/src/usbd
INCLUDES += -isystem $(LIBDAISY_PATH)/src/usbh
INCLUDES += -isystem $(LIBDAISY_PATH)/Drivers/CMSIS_5/CMSIS/Core/Include
INCLUDES += -isystem $(LIBDAISY_PATH)/Drivers/CMSIS-DSP/Include
INCLUDES += -isystem $(LIBDAISY_PATH)/Drivers/CMSIS-Device/ST/STM32H7xx/Include
INCLUDES += -isystem $(LIBDAISY_PATH)/Drivers/STM32H7xx_HAL_Driver/Inc
INCLUDES += -isystem $(LIBDAISY_PATH)/Drivers/STM32H7xx_HAL_Driver/Inc/Legacy
INCLUDES += -isystem $(LIBDAISY_PATH)/Middlewares/ST/STM32_USB_Device_Library/Core/Inc
INCLUDES += -isystem $(LIBDAISY_PATH)/Middlewares/ST/STM32_USB_Host_Library/Core/Inc
INCLUDES += -isystem $(LIBDAISY_PATH)/Middlewares/ST/STM32_USB_Host_Library/Class/MSC/Inc
INCLUDES += -isystem $(LIBDAISY_PATH)/Middlewares/ST/STM32_USB_Host_Library/Class/MIDI/Inc
INCLUDES += -isystem $(LIBDAISY_PATH)/Middlewares/Third_Party/FatFs/src

# Source Files
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
SRCS += $(wildcard $(SRC_DIR)/*.c)
SRCS += $(wildcard $(SRC_DIR)/*/*.cpp)
SRCS += $(wildcard $(SRC_DIR)/*/*.c)

# Object Files
OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
OBJS := $(OBJS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Libraries
DAISYSP_LIB := $(DAISYSP_PATH)/build/libdaisysp.a
LIBDAISY_LIB := $(LIBDAISY_PATH)/build/libdaisy.a

# Linker script (using libDaisy's linker script)
LDSCRIPT := $(LIBDAISY_PATH)/core/STM32H750IB_flash.lds

# Linker Flags
LDFLAGS := -T$(LDSCRIPT)
LDFLAGS += -mcpu=cortex-m7 -mthumb
LDFLAGS += -mfloat-abi=hard -mfpu=fpv5-d16
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Wl,-Map=$(BUILD_DIR)/$(PROJECT_NAME).map
LDFLAGS += -Wl,--print-memory-usage
LDFLAGS += -Wl,--start-group
LDFLAGS += -L$(DAISYSP_PATH)/build
LDFLAGS += -L$(LIBDAISY_PATH)/build
LDFLAGS += -ldaisysp
LDFLAGS += -ldaisy
LDFLAGS += -lm -lc -lnosys
LDFLAGS += -Wl,--end-group
LDFLAGS += --specs=nano.specs --specs=nosys.specs

# Debug Flags
ifeq ($(DEBUG),1)
    CXXFLAGS += -DDEBUG -g3 -O0
else ifeq ($(DEBUG_FLASH),1)
    # Debug build optimized for flash-constrained devices
    # Uses -Og (optimize for debugging) instead of -O0 to reduce code size
    CXXFLAGS += -DDEBUG -g3 -Og
else
    CXXFLAGS += -DNDEBUG
endif

# Hardware Debug Modes (QA testing)
ifeq ($(QA_SIMPLE),1)
    CXXFLAGS += -DDEBUG_BASELINE_SIMPLE=1
endif

ifeq ($(QA_LIVELY),1)
    CXXFLAGS += -DDEBUG_BASELINE_LIVELY=1
endif

ifdef QA_LEVEL
    CXXFLAGS += -DDEBUG_FEATURE_LEVEL=$(QA_LEVEL)
endif

# Output Files
ELF := $(BUILD_DIR)/$(PROJECT_NAME).elf
BIN := $(BUILD_DIR)/$(PROJECT_NAME).bin
HEX := $(BUILD_DIR)/$(PROJECT_NAME).hex

###############################################################################
# Serial Monitor Configuration
###############################################################################

BAUD ?= 115200
PORT ?= $(shell ls -t /dev/cu.usbmodem* 2>/dev/null | head -n 1)
LOGDIR ?= /tmp

###############################################################################
# Build Targets
###############################################################################

.PHONY: all clean rebuild daisy-build daisy-update libdaisy-build libdaisy-update program build-debug program-debug test test-coverage listen ports help pattern-viz run-pattern-viz pattern-sweep pattern-html expressiveness-report expressiveness-quick evals evals-generate evals-evaluate evals-serve evals-deploy weights-header baseline sensitivity-sweep sensitivity-matrix sensitivity-levers

# Default target
all: $(ELF) $(BIN) $(HEX)
	@echo "Build complete: $(ELF)"
	@$(SIZE) $(ELF)

# Build DaisySP library if needed
$(DAISYSP_LIB):
	@echo "Building DaisySP library..."
	@cd $(DAISYSP_PATH) && $(MAKE)

# Build libDaisy library if needed
$(LIBDAISY_LIB):
	@echo "Building libDaisy library..."
	@cd $(LIBDAISY_PATH) && $(MAKE)

# Link executable
$(ELF): $(OBJS) $(DAISYSP_LIB) $(LIBDAISY_LIB) | $(BUILD_DIR)
	@echo "Linking $(ELF)..."
	@$(CXX) $(OBJS) $(LDFLAGS) -o $@

# Create binary file
$(BIN): $(ELF)
	@echo "Creating binary $(BIN)..."
	@$(OBJCOPY) -O binary $< $@

# Create hex file
$(HEX): $(ELF)
	@echo "Creating hex $(HEX)..."
	@$(OBJCOPY) -O ihex $< $@

# Compile C++ source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	@echo "Compiling $<..."
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Compile C source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@echo "Compiling $<..."
	@mkdir -p $(dir $@)
	@$(CC) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Create directories
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(OBJ_DIR): | $(BUILD_DIR)
	@mkdir -p $(OBJ_DIR)

# Build DaisySP library
daisy-build: $(DAISYSP_LIB)

# Build libDaisy library
libdaisy-build: $(LIBDAISY_LIB)

# Update DaisySP submodule
daisy-update:
	@echo "Updating DaisySP submodule..."
	@cd $(DAISYSP_PATH) && git fetch origin && git checkout master && git pull origin master
	@echo "Rebuilding DaisySP..."
	@cd $(DAISYSP_PATH) && $(MAKE) clean && $(MAKE)

# Update libDaisy submodule
libdaisy-update:
	@echo "Updating libDaisy submodule..."
	@cd $(LIBDAISY_PATH) && git fetch origin && git checkout master && git pull origin master
	@cd $(LIBDAISY_PATH) && git submodule update --init --recursive
	@echo "Rebuilding libDaisy..."
	@cd $(LIBDAISY_PATH) && $(MAKE) clean && $(MAKE)

# Flash firmware to device (requires DFU mode)
program: $(BIN)
	@echo "Flashing firmware to Patch.Init module..."
	@dfu-util -a 0 -s 0x08000000:leave -D $(BIN)

# Build with DEBUG-level logging enabled at compile-time and runtime
# Sets LOG_COMPILETIME_LEVEL=1 (DEBUG) and LOG_DEFAULT_LEVEL=1 (DEBUG)
# Uses DEBUG_FLASH=1 for debug symbols with -Og optimization (fits in flash)
build-debug:
	@echo "Building firmware with DEBUG-level logging..."
	@$(MAKE) clean
	@$(MAKE) all DEBUG_FLASH=1 LOG_COMPILETIME_LEVEL=1 LOG_DEFAULT_LEVEL=1
	@echo "Debug build complete with DEBUG-level logging enabled"

# Build debug firmware and flash to device
program-debug: build-debug
	@echo "Flashing debug firmware to Patch.Init module..."
	@dfu-util -a 0 -s 0x08000000:leave -D $(BIN)

# Listen to serial output from device and log to temp file
# Uses 'cat' for logging (screen doesn't pipe to tee properly)
listen:
	@test -n "$(PORT)" || (echo "No /dev/cu.usbmodem* found. Is device connected?"; exit 1)
	@mkdir -p "$(LOGDIR)"
	@ts=$$(date +%Y%m%d_%H%M%S); \
	  logfile="$(LOGDIR)/daisy_$$ts.log"; \
	  echo "Listening on $(PORT) @ $(BAUD) baud"; \
	  echo "Log file: $$logfile"; \
	  echo ""; \
	  echo "Press Ctrl-C to quit"; \
	  echo ""; \
	  trap "echo ''; echo 'Log saved to: $$logfile'" EXIT; \
	  stty -f "$(PORT)" $(BAUD) cs8 -cstopb -parenb raw; \
	  cat "$(PORT)" | tee -a "$$logfile"

# List available serial ports
ports:
	@echo "Available USB serial ports:"
	@ls -1 /dev/cu.usbmodem* 2>/dev/null || echo "  (none found)"

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)
	@echo "Clean complete."

# Rebuild (clean + build)
rebuild: clean all

###############################################################################
# Weight Configuration
###############################################################################

# Default config file for weights-header target
CONFIG ?= config/weights/baseline.json
GENERATED_HEADER := inc/generated/weights_config.h

# Generate C++ header from JSON weight configuration
weights-header: $(GENERATED_HEADER)
	@echo "Weight configuration header up to date."

$(GENERATED_HEADER): $(CONFIG) scripts/generate-weights-header.py
	@echo "Generating weight configuration header from $(CONFIG)..."
	@python3 scripts/generate-weights-header.py $(CONFIG) $(GENERATED_HEADER)

###############################################################################
# Test Targets
###############################################################################

# Host compiler for tests (not ARM)
HOST_CXX := g++
HOST_CXXFLAGS := -std=c++17 -Wall -Wextra -g -O0
HOST_CXXFLAGS += -DHOST_BUILD  # Define for host-side builds
HOST_CXXFLAGS += -MMD -MP  # Generate dependency files for header tracking
HOST_CXXFLAGS += -I$(INC_DIR)
HOST_CXXFLAGS += -I$(SRC_DIR)
HOST_CXXFLAGS += -I$(DAISYSP_PATH)/Source
HOST_CXXFLAGS += -I$(DAISYSP_PATH)/Source/daisysp
HOST_CXXFLAGS += -I$(DAISYSP_PATH)/Source/Utility
HOST_CXXFLAGS += -I$(DAISYSP_PATH)/Source/Control
HOST_CXXFLAGS += -I$(DAISYSP_PATH)/Source/Synthesis
HOST_CXXFLAGS += -I$(DAISYSP_PATH)/Source/Effects
HOST_CXXFLAGS += -I$(DAISYSP_PATH)/Source/Filters
HOST_CXXFLAGS += -I$(DAISYSP_PATH)/Source/Noise
HOST_CXXFLAGS += -I$(DAISYSP_PATH)/Source/PhysicalModeling
HOST_CXXFLAGS += -I$(DAISYSP_PATH)/Source/Dynamics
HOST_CXXFLAGS += -I$(DAISYSP_PATH)/Source/Drums

# Test source files (host-compiled)
TEST_SRCS := $(wildcard $(TEST_DIR)/*.cpp)
TEST_OBJS := $(TEST_SRCS:$(TEST_DIR)/%.cpp=$(BUILD_DIR)/test_%.o)

# Project sources needed for host-side tests
TEST_APP_SRCS := $(wildcard $(SRC_DIR)/Engine/*.cpp)
TEST_APP_SRCS += $(SRC_DIR)/System/logging.cpp
TEST_APP_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/test_app_%.o,$(TEST_APP_SRCS))

# DaisySP sources needed for tests (compiled for host)
TEST_DAISYSP_SRCS := $(DAISYSP_PATH)/Source/Utility/metro.cpp \
                     $(DAISYSP_PATH)/Source/Control/adsr.cpp
TEST_DAISYSP_OBJS := $(TEST_DAISYSP_SRCS:$(DAISYSP_PATH)/%.cpp=$(BUILD_DIR)/daisysp_host/%.o)

TEST_RUNNER := $(BUILD_DIR)/test_runner

# Catch2 path (try common locations - parent directory for include)
CATCH2_INC := $(shell \
    if [ -d /opt/homebrew/include/catch2 ]; then \
        echo /opt/homebrew/include; \
    elif [ -d /usr/local/include/catch2 ]; then \
        echo /usr/local/include; \
    elif [ -d /usr/include/catch2 ]; then \
        echo /usr/include; \
    else \
        echo ""; \
    fi)

# Check if Catch2 is available
ifeq ($(CATCH2_INC),)
    $(warning Catch2 not found. Install Catch2 to run tests:)
    $(warning   git clone https://github.com/catchorg/Catch2.git && cd Catch2 && cmake -Bbuild -H. -DBUILD_TESTING=OFF && sudo cmake --build build/ --target install)
endif

# Build and run tests
test: $(TEST_RUNNER)
	@if [ -z "$(CATCH2_INC)" ]; then \
		echo "Error: Catch2 not found. Cannot run tests."; \
		echo "Install Catch2 or set CATCH2_INC environment variable."; \
		exit 1; \
	fi
	@echo "Running unit tests..."
	@$(TEST_RUNNER) --success

# Catch2 library path
CATCH2_LIB := $(shell \
    if [ -f /opt/homebrew/lib/libCatch2Main.a ]; then \
        echo /opt/homebrew/lib; \
    elif [ -f /usr/local/lib/libCatch2Main.a ]; then \
        echo /usr/local/lib; \
    elif [ -f /usr/lib/libCatch2Main.a ]; then \
        echo /usr/lib; \
    else \
        echo ""; \
    fi)

# Build test runner
$(TEST_RUNNER): $(TEST_OBJS) $(TEST_APP_OBJS) $(TEST_DAISYSP_OBJS) | $(BUILD_DIR)
	@if [ -z "$(CATCH2_INC)" ]; then \
		echo "Error: Catch2 not found. Cannot build tests."; \
		echo "Install Catch2 or set CATCH2_INC environment variable."; \
		exit 1; \
	fi
	@echo "Linking test runner..."
	@if [ -n "$(CATCH2_LIB)" ]; then \
		$(HOST_CXX) $(HOST_CXXFLAGS) $(TEST_OBJS) $(TEST_APP_OBJS) $(TEST_DAISYSP_OBJS) -L$(CATCH2_LIB) -lCatch2Main -lCatch2 -o $@; \
	else \
		$(HOST_CXX) $(HOST_CXXFLAGS) $(TEST_OBJS) $(TEST_APP_OBJS) $(TEST_DAISYSP_OBJS) -o $@; \
	fi

# Compile test files
$(BUILD_DIR)/test_%.o: $(TEST_DIR)/%.cpp | $(BUILD_DIR)
	@if [ -z "$(CATCH2_INC)" ]; then \
		echo "Error: Catch2 not found. Cannot compile tests."; \
		exit 1; \
	fi
	@echo "Compiling test $<..."
	@$(HOST_CXX) $(HOST_CXXFLAGS) -I$(CATCH2_INC) -c $< -o $@

# Compile DaisySP host files
$(BUILD_DIR)/daisysp_host/%.o: $(DAISYSP_PATH)/%.cpp | $(BUILD_DIR)
	@echo "Compiling host lib $<..."
	@mkdir -p $(dir $@)
	@$(HOST_CXX) $(HOST_CXXFLAGS) -c $< -o $@

# Compile project sources for host-side tests
$(BUILD_DIR)/test_app_%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@echo "Compiling app source $<..."
	@mkdir -p $(dir $@)
	@$(HOST_CXX) $(HOST_CXXFLAGS) -c $< -o $@

# Test coverage (requires gcov and lcov)
test-coverage: CXXFLAGS += --coverage
test-coverage: HOST_CXXFLAGS += --coverage
test-coverage: clean test
	@echo "Generating coverage report..."
	@lcov --capture --directory $(BUILD_DIR) --output-file $(BUILD_DIR)/coverage.info
	@genhtml $(BUILD_DIR)/coverage.info --output-directory $(BUILD_DIR)/coverage_html
	@echo "Coverage report: $(BUILD_DIR)/coverage_html/index.html"

###############################################################################
# Pattern Visualization Tool
###############################################################################

PATTERN_VIZ := $(BUILD_DIR)/pattern_viz
PATTERN_VIZ_SRC := tools/pattern_viz.cpp

# Build pattern visualization tool
pattern-viz: $(PATTERN_VIZ)
	@echo "Pattern visualization tool built: $(PATTERN_VIZ)"

$(PATTERN_VIZ): $(PATTERN_VIZ_SRC) $(TEST_APP_OBJS) $(TEST_DAISYSP_OBJS) | $(BUILD_DIR)
	@echo "Building pattern visualization tool..."
	@$(HOST_CXX) $(HOST_CXXFLAGS) $(PATTERN_VIZ_SRC) $(TEST_APP_OBJS) $(TEST_DAISYSP_OBJS) -o $@

# Run pattern visualization with default parameters
run-pattern-viz: $(PATTERN_VIZ)
	@$(PATTERN_VIZ)

# Output patterns to file with parameter sweep
pattern-sweep: $(PATTERN_VIZ)
	@mkdir -p $(BUILD_DIR)/patterns
	@echo "Generating SHAPE sweep..."
	@$(PATTERN_VIZ) --sweep=shape --output=$(BUILD_DIR)/patterns/shape_sweep.txt
	@echo "Generating ENERGY sweep..."
	@$(PATTERN_VIZ) --sweep=energy --output=$(BUILD_DIR)/patterns/energy_sweep.txt
	@echo "Generating CSV data..."
	@$(PATTERN_VIZ) --sweep=shape --format=csv --output=$(BUILD_DIR)/patterns/shape_sweep.csv
	@echo "Patterns written to $(BUILD_DIR)/patterns/"

# Generate HTML/SVG pattern visualization
pattern-html: $(PATTERN_VIZ)
	@echo "Generating HTML pattern visualization..."
	@python3 scripts/generate-pattern-html.py --pattern-viz=$(PATTERN_VIZ) --output=docs/patterns.html
	@echo "HTML visualization written to docs/patterns.html"

# Evaluate pattern expressiveness (feedback loop for algorithm development)
expressiveness-report: $(PATTERN_VIZ)
	@echo "Running expressiveness evaluation..."
	@python3 scripts/evaluate-expressiveness.py --pattern-viz=$(PATTERN_VIZ)

expressiveness-quick: $(PATTERN_VIZ)
	@echo "Running quick expressiveness evaluation..."
	@python3 scripts/evaluate-expressiveness.py --pattern-viz=$(PATTERN_VIZ) --quick

###############################################################################
# Pattern Evaluation Dashboard (tools/evals/)
###############################################################################

EVALS_DIR := tools/evals

# Build and generate all evaluation data
evals: $(PATTERN_VIZ)
	@echo "Setting up evaluation dashboard..."
	@cd $(EVALS_DIR) && npm install --silent
	@echo "Generating pattern data..."
	@cd $(EVALS_DIR) && PATTERN_VIZ=$(CURDIR)/$(PATTERN_VIZ) node generate-patterns.js
	@echo "Computing expressiveness metrics..."
	@cd $(EVALS_DIR) && node evaluate-expressiveness.js
	@echo ""
	@echo "Evaluation dashboard ready!"
	@echo "Run 'make evals-serve' to start local server"

# Generate pattern data only
evals-generate: $(PATTERN_VIZ)
	@cd $(EVALS_DIR) && npm install --silent
	@cd $(EVALS_DIR) && PATTERN_VIZ=$(CURDIR)/$(PATTERN_VIZ) node generate-patterns.js
	@cd $(EVALS_DIR) && node evaluate-expressiveness.js

# Evaluate expressiveness only (assumes patterns already generated)
evals-evaluate:
	@cd $(EVALS_DIR) && npm install --silent
	@cd $(EVALS_DIR) && node evaluate-expressiveness.js

# Clean evaluation data
evals-clean:
	@echo "Cleaning evaluation data..."
	@rm -f $(EVALS_DIR)/public/data/*.json
	@echo "Evaluation data cleaned"

# Serve evaluation dashboard locally
evals-serve: $(EVALS_DIR)/public/data/metadata.json
	@echo "Starting local server at http://localhost:3000"
	@cd $(EVALS_DIR) && npx serve public -l 3000

$(EVALS_DIR)/public/data/metadata.json: $(PATTERN_VIZ)
	@$(MAKE) evals-generate

# Deploy to GitHub Pages (builds to docs/evals/)
evals-deploy: evals-generate metrics-history
	@echo "Deploying evaluation dashboard to docs/evals/..."
	@mkdir -p docs/evals
	@cp -r $(EVALS_DIR)/public/* docs/evals/
	@echo "Dashboard deployed to docs/evals/"
	@echo "Commit and push to publish via GitHub Pages"

###############################################################################
# Baseline Metrics
###############################################################################

# Generate baseline metrics from current main branch
baseline: evals-generate
	@echo "Generating baseline metrics..."
	@python3 scripts/generate-baseline.py
	@echo "Baseline saved to metrics/baseline.json"

###############################################################################
# Sensitivity Analysis
###############################################################################

# Run parameter sweep and collect metrics
sensitivity-sweep: $(PATTERN_VIZ)
	@echo "Running parameter sensitivity sweep..."
	@node scripts/sensitivity/run-sweep.js --verbose --output metrics/sweep-data.json
	@echo "Sweep data saved to metrics/sweep-data.json"

# Compute sensitivity matrix from sweep data
sensitivity-matrix: $(PATTERN_VIZ)
	@echo "Computing sensitivity matrix..."
	@node scripts/sensitivity/compute-matrix.js --verbose
	@cp metrics/sensitivity-matrix.json $(EVALS_DIR)/public/data/sensitivity.json 2>/dev/null || true
	@echo "Sensitivity matrix saved to metrics/sensitivity-matrix.json"
	@echo "Dashboard data updated at $(EVALS_DIR)/public/data/sensitivity.json"

# Show lever recommendations for a specific metric
# Usage: make sensitivity-levers METRIC=syncopation
METRIC ?= syncopation
sensitivity-levers:
	@node scripts/sensitivity/identify-levers.js --target $(METRIC)

###############################################################################
# Metrics History
###############################################################################

# Extract metrics history from baseline tags for timeline chart
metrics-history:
	@echo "Extracting metrics history from baseline tags..."
	@node scripts/extract-metrics-history.js
	@echo "Metrics history saved to $(EVALS_DIR)/public/data/metrics-history.json"

###############################################################################
# Help Target
###############################################################################

help:
	@echo "DaisySP Patch.Init Firmware Makefile"
	@echo ""
	@echo "Build Targets:"
	@echo "  all              - Build firmware (default)"
	@echo "  clean            - Remove build artifacts"
	@echo "  rebuild          - Clean and rebuild"
	@echo "  build-debug      - Build with DEBUG-level logging (DEBUG_FLASH=1, log level=DEBUG)"
	@echo "  daisy-build      - Build DaisySP library"
	@echo "  daisy-update     - Update DaisySP to latest version"
	@echo "  libdaisy-build   - Build libDaisy library"
	@echo "  libdaisy-update  - Update libDaisy to latest version"
	@echo "  program          - Flash firmware to device (DFU mode)"
	@echo "  program-debug    - Build debug firmware and flash to device"
	@echo "  listen           - Monitor serial output and log to temp file"
	@echo "  ports            - List available USB serial ports"
	@echo "  test             - Build and run unit tests"
	@echo "  test-coverage    - Generate test coverage report"
	@echo "  pattern-viz      - Build pattern visualization tool"
	@echo "  run-pattern-viz  - Run pattern viz with default params"
	@echo "  pattern-sweep    - Generate pattern files for all param sweeps"
	@echo "  pattern-html     - Generate HTML/SVG pattern visualization (docs/patterns.html)"
	@echo "  expressiveness-report - Run full expressiveness evaluation"
	@echo "  expressiveness-quick  - Run quick expressiveness evaluation"
	@echo "  evals            - Build pattern evaluation dashboard (tools/evals/)"
	@echo "  evals-generate   - Generate evaluation data only"
	@echo "  evals-evaluate   - Evaluate expressiveness only (assumes patterns exist)"
	@echo "  evals-serve      - Serve evaluation dashboard locally (port 3000)"
	@echo "  evals-deploy     - Deploy dashboard to docs/evals/ for GitHub Pages"
	@echo "  sensitivity-sweep  - Run parameter sweep for sensitivity analysis"
	@echo "  sensitivity-matrix - Compute sensitivity matrix (weight -> metric impacts)"
	@echo "  sensitivity-levers - Show lever recommendations (METRIC=syncopation)"
	@echo "  help             - Show this help message"
	@echo ""
	@echo "Variables:"
	@echo "  DEBUG=1                   - Debug symbols + no optimization (-g3 -O0)"
	@echo "  DEBUG_FLASH=1             - Debug symbols + -Og optimization (fits in flash)"
	@echo "  LOG_COMPILETIME_LEVEL=N   - Min compile-time log level (default: 1=DEBUG)"
	@echo "                              0=TRACE, 1=DEBUG, 2=INFO, 3=WARN, 4=ERROR, 5=OFF"
	@echo "  LOG_DEFAULT_LEVEL=N       - Default runtime log level (default: 2=INFO)"
	@echo "  BUILD_DIR=dir             - Set build directory (default: build)"
	@echo "  DAISYSP_PATH=dir          - Set DaisySP path (default: ./DaisySP)"
	@echo "  GCC_PATH=dir              - Set ARM GCC toolchain path (default: use PATH)"
	@echo "  BAUD=rate                 - Set serial baud rate (default: 115200)"
	@echo "  PORT=device               - Set serial port (default: auto-detect)"
	@echo "  LOGDIR=dir                - Set log directory (default: /tmp)"
	@echo ""
	@echo "Examples:"
	@echo "  make                              - Build firmware"
	@echo "  make DEBUG=1                      - Build with debug symbols"
	@echo "  make build-debug                  - Build with DEBUG-level logging"
	@echo "  make program-debug                - Build debug and flash to device"
	@echo "  make test                         - Run tests"
	@echo "  make daisy-update                 - Update DaisySP"
	@echo "  make program                      - Flash firmware"
	@echo "  make listen                       - Monitor serial output (Ctrl-A \\ y to quit)"
	@echo "  make ports                        - Check available serial ports"
	@echo "  LOG_COMPILETIME_LEVEL=0 make     - Build with TRACE-level logging"

###############################################################################
# Dependency Tracking
###############################################################################

# Include firmware build dependencies
-include $(OBJS:.o=.d)

# Include host build dependencies (for tests and pattern_viz)
-include $(TEST_APP_OBJS:.o=.d)
-include $(TEST_OBJS:.o=.d)

$(OBJ_DIR)/%.d: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -MM -MT $(@:.d=.o) $< > $@

$(OBJ_DIR)/%.d: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	@$(CC) $(CXXFLAGS) $(INCLUDES) -MM -MT $(@:.d=.o) $< > $@
