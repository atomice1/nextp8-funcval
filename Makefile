# nextp8 Functional Validation Makefile
# Top-level Makefile for building and running all tests

.PHONY: all clean help
.PHONY: lib lib-clean
.PHONY: base base-clean base-model base-sim
.PHONY: main main-clean main-model main-sim
.PHONY: test-all test-clean test-model test-sim

# Main test suite directories (relative to main/)
MAIN_TEST_DIRS = 01_version_info \
                 03_joystick \
                 04_mouse \
                 05a_built_in_keyboard \
                 05b_external_keyboard \
                 06_sdspi \
                 07_i2c \
                 09_uart_esp \
                 10_digital_audio \
                 12_palette \
                 13_p8audio \
                 14_overlay \
                 00_menu

# Base test directories
BASE_TEST_DIRS = 01_post_code \
                 02_uart_output \
                 03_sram_read \
                 04_sram_write \
                 05_vram_access \
                 06_timers \
                 07_screen_output \
                 08_interrupts

# Default target: build everything
all: lib main base
	@echo ""
	@echo "=== Build Complete ==="
	@echo "Library: libfuncval.a"
	@echo "Main tests: $(words $(MAIN_TEST_DIRS)) suites (including interactive menu at 00_menu)"
	@echo "Base tests: $(words $(BASE_TEST_DIRS)) suites"

# Help target
help:
	@echo "nextp8 Functional Validation Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all          - Build library and all tests (default)"
	@echo "  clean        - Clean library and all tests"
	@echo ""
	@echo "Library targets:"
	@echo "  lib          - Build common library (libfuncval.a)"
	@echo "  lib-clean    - Clean library"
	@echo ""
	@echo "Main test suite targets:"
	@echo "  main         - Build all main test suites (01-14)"
	@echo "  main-clean   - Clean all main test suites"
	@echo "  main-model   - Run all main tests on model (sQLux)"
	@echo "  main-sim     - Run all main tests in simulator (Vivado)"
	@echo ""
	@echo "Base test targets:"
	@echo "  base         - Build all base tests (01-08)"
	@echo "  base-clean   - Clean all base tests"
	@echo "  base-model   - Run all base tests on model (sQLux)"
	@echo "  base-sim     - Run all base tests in simulator (Vivado)"
	@echo ""
	@echo "Combined test targets:"
	@echo "  test-all     - Build main + base tests"
	@echo "  test-clean   - Clean main + base tests"
	@echo "  test-model   - Run main + base tests on model"
	@echo "  test-sim     - Run main + base tests in simulator"

# Clean everything
clean: lib-clean main-clean base-clean
	@echo ""
	@echo "=== Clean Complete ==="

# ============================================================================
# Common Library Targets
# ============================================================================

lib:
	@echo "=== Building Common Library ==="
	@$(MAKE) -C common
	@echo ""

lib-clean:
	@echo "=== Cleaning Common Library ==="
	@$(MAKE) -C common clean
	@echo ""

# ============================================================================
# Main Test Suite Targets
# ============================================================================

main: lib
	@echo "=== Building Main Test Suites ==="
	@for dir in $(MAIN_TEST_DIRS); do \
		echo "Building main/$$dir..."; \
		$(MAKE) -C main/$$dir || exit 1; \
	done
	@echo "All main test suites built successfully!"
	@echo ""

main-clean:
	@echo "=== Cleaning Main Test Suites ==="
	@for dir in $(MAIN_TEST_DIRS); do \
		echo "Cleaning main/$$dir..."; \
		$(MAKE) -C main/$$dir clean; \
	done
	@echo "All main test suites cleaned!"
	@echo ""

main-model: main
	@echo "=== Running Main Test Suites on Model ==="
	@for dir in $(MAIN_TEST_DIRS); do \
		echo "Running main/$$dir (model)..."; \
		$(MAKE) -C main/$$dir model || exit 1; \
	done
	@echo "All main test model runs complete!"
	@echo ""

main-sim: main
	@echo "=== Running Main Test Suites in Simulator ==="
	@for dir in $(MAIN_TEST_DIRS); do \
		echo "Running main/$$dir (sim)..."; \
		$(MAKE) -C main/$$dir sim || exit 1; \
	done
	@echo "All main test simulations complete!"
	@echo ""

# ============================================================================
# Base Test Targets
# ============================================================================

base:
	@echo "=== Building Base Tests ==="
	@for dir in $(BASE_TEST_DIRS); do \
		echo "Building base/$$dir..."; \
		$(MAKE) -C base/$$dir || exit 1; \
	done
	@echo "All base tests built successfully!"
	@echo ""

base-clean:
	@echo "=== Cleaning Base Tests ==="
	@for dir in $(BASE_TEST_DIRS); do \
		echo "Cleaning base/$$dir..."; \
		$(MAKE) -C base/$$dir clean; \
	done
	@echo "All base tests cleaned!"
	@echo ""

base-model: base
	@echo "=== Running Base Tests on Model ==="
	@for dir in $(BASE_TEST_DIRS); do \
		echo "Running base/$$dir (model)..."; \
		$(MAKE) -C base/$$dir model || exit 1; \
	done
	@echo "All base test model runs complete!"
	@echo ""

base-sim: base
	@echo "=== Running Base Tests in Simulator ==="
	@for dir in $(BASE_TEST_DIRS); do \
		echo "Running base/$$dir (sim)..."; \
		$(MAKE) -C base/$$dir sim || exit 1; \
	done
	@echo "All base test simulations complete!"
	@echo ""

# ============================================================================
# Combined Test Targets
# ============================================================================

test-all: main base
	@echo ""
	@echo "=== All Tests Built Successfully ==="

test-clean: main-clean base-clean
	@echo ""
	@echo "=== All Tests Cleaned ==="

test-model: main-model base-model
	@echo ""
	@echo "=== All Tests on Model Complete ==="

test-sim: main-sim base-sim
	@echo ""
	@echo "=== All Tests in Simulator Complete ==="
