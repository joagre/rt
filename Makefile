# Compiler and flags
CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 -g
CPPFLAGS := -Iinclude -D_POSIX_C_SOURCE=200809L
DEPFLAGS = -MMD -MP
LDFLAGS :=
LDLIBS :=

# Feature toggles (set to 0 to disable)
ENABLE_NET ?= 1
ENABLE_FILE ?= 1

# Add feature flags to compiler
ifeq ($(ENABLE_NET),1)
  CPPFLAGS += -DHIVE_ENABLE_NET=1
else
  CPPFLAGS += -DHIVE_ENABLE_NET=0
endif

ifeq ($(ENABLE_FILE),1)
  CPPFLAGS += -DHIVE_ENABLE_FILE=1
else
  CPPFLAGS += -DHIVE_ENABLE_FILE=0
endif

# Directories
SRC_DIR := src
INC_DIR := include
BUILD_DIR := build
EXAMPLES_DIR := examples
MAN_DIR := man

# Installation directories
PREFIX ?= /usr/local
MANPREFIX ?= $(PREFIX)/share/man

# Source files (core, always compiled)
SRCS := $(wildcard $(SRC_DIR)/*.c)
ASM_SRCS := $(wildcard $(SRC_DIR)/*.S)

# Exclude optional subsystems if disabled
ifneq ($(ENABLE_NET),1)
  SRCS := $(filter-out $(SRC_DIR)/hive_net.c,$(SRCS))
endif
ifneq ($(ENABLE_FILE),1)
  SRCS := $(filter-out $(SRC_DIR)/hive_file.c,$(SRCS))
endif

OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o) $(ASM_SRCS:$(SRC_DIR)/%.S=$(BUILD_DIR)/%.o)
DEPS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.d)

# Library
LIB := $(BUILD_DIR)/libhive.a

# Examples
EXAMPLE_SRCS := $(wildcard $(EXAMPLES_DIR)/*.c)

# Exclude examples that depend on disabled features
ifneq ($(ENABLE_NET),1)
  EXAMPLE_SRCS := $(filter-out $(EXAMPLES_DIR)/echo.c,$(EXAMPLE_SRCS))
endif
ifneq ($(ENABLE_FILE),1)
  EXAMPLE_SRCS := $(filter-out $(EXAMPLES_DIR)/fileio.c,$(EXAMPLE_SRCS))
endif

EXAMPLES := $(EXAMPLE_SRCS:$(EXAMPLES_DIR)/%.c=$(BUILD_DIR)/%)

# Benchmarks
BENCHMARKS_DIR := benchmarks
BENCHMARK_SRCS := $(wildcard $(BENCHMARKS_DIR)/*.c)
BENCHMARKS := $(BENCHMARK_SRCS:$(BENCHMARKS_DIR)/%.c=$(BUILD_DIR)/%)

# Tests
TESTS_DIR := tests
TEST_SRCS := $(wildcard $(TESTS_DIR)/*.c)

# Exclude tests that depend on disabled features
ifneq ($(ENABLE_NET),1)
  TEST_SRCS := $(filter-out $(TESTS_DIR)/net_test.c,$(TEST_SRCS))
endif
ifneq ($(ENABLE_FILE),1)
  TEST_SRCS := $(filter-out $(TESTS_DIR)/file_test.c,$(TEST_SRCS))
endif

TESTS := $(TEST_SRCS:$(TESTS_DIR)/%.c=$(BUILD_DIR)/%)

# Default target
.PHONY: all
all: $(LIB) $(EXAMPLES) $(BENCHMARKS)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compile source files with dependency generation
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(DEPFLAGS) -c $< -o $@

# Compile assembly files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.S | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) -c $< -o $@

# Create static library
$(LIB): $(OBJS)
	ar rcs $@ $^

# Build examples
$(BUILD_DIR)/%: $(EXAMPLES_DIR)/%.c $(LIB)
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -o $@ -L$(BUILD_DIR) -lhive $(LDLIBS)

# Build benchmarks
$(BUILD_DIR)/%: $(BENCHMARKS_DIR)/%.c $(LIB)
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -o $@ -L$(BUILD_DIR) -lhive $(LDLIBS)

# Build tests
$(BUILD_DIR)/%: $(TESTS_DIR)/%.c $(LIB)
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -o $@ -L$(BUILD_DIR) -lhive $(LDLIBS)

# Run benchmarks
.PHONY: bench
bench: $(BUILD_DIR)/bench
	./$(BUILD_DIR)/bench

# Run tests
.PHONY: test
test: $(TESTS)
	@echo "Running tests..."
	@for test in $(TESTS); do \
		echo ""; \
		echo "=== Running $$test ==="; \
		timeout 5 $$test || exit 1; \
	done
	@echo ""
	@echo "All tests passed!"

# Clean
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

# Clean Emacs backup/auto-save files recursively
.PHONY: clean-emacs
clean-emacs:
	@echo "Removing Emacs backup and auto-save files..."
	find . -name '*~' -delete
	find . -name '#*#' -delete
	find . -name '.#*' -delete

# Run ping-pong example
.PHONY: run-pingpong
run-pingpong: $(BUILD_DIR)/pingpong
	./$(BUILD_DIR)/pingpong

# Run file I/O example
.PHONY: run-fileio
run-fileio: $(BUILD_DIR)/fileio
	./$(BUILD_DIR)/fileio

# Run echo server/client example
.PHONY: run-echo
run-echo: $(BUILD_DIR)/echo
	./$(BUILD_DIR)/echo

# Install man pages
.PHONY: install-man
install-man:
	@echo "Installing man pages to $(MANPREFIX)/man3/"
	install -d $(MANPREFIX)/man3
	install -m 644 $(MAN_DIR)/man3/*.3 $(MANPREFIX)/man3/

# Uninstall man pages
.PHONY: uninstall-man
uninstall-man:
	@echo "Removing man pages from $(MANPREFIX)/man3/"
	rm -f $(MANPREFIX)/man3/hive_*.3

# Help
.PHONY: help
help:
	@echo "Available targets:"
	@echo "  all               - Build library, examples, and benchmarks (default)"
	@echo "  clean             - Remove build artifacts"
	@echo "  clean-emacs       - Remove Emacs backup files (*~, #*#, .#*)"
	@echo "  test              - Build and run all tests"
	@echo "  bench             - Build and run benchmark suite"
	@echo "  install-man       - Install man pages to $(MANPREFIX)/man3"
	@echo "  uninstall-man     - Remove installed man pages"
	@echo "  run-pingpong      - Build and run ping-pong example"
	@echo "  run-fileio        - Build and run file I/O example"
	@echo "  run-echo          - Build and run echo server/client example"
	@echo "  help              - Show this help message"
	@echo ""
	@echo "Feature toggles (set to 0 to disable):"
	@echo "  ENABLE_NET=1      - Network I/O subsystem (default: 1)"
	@echo "  ENABLE_FILE=1     - File I/O subsystem (default: 1)"
	@echo ""
	@echo "Example: make ENABLE_NET=0 ENABLE_FILE=0"

# Dependencies
.PHONY: deps
deps:
	@echo "No external dependencies required"

# Print variables for debugging
.PHONY: print-%
print-%:
	@echo '$*=$($*)'

# Include automatically generated dependencies
-include $(DEPS)
