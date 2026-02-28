# Base Functionality Tests Makefile (run from base/)

.PHONY: all clean model-all sim-all

# All test directories (relative to base/)
TEST_DIRS = 01_post_code \
            02_uart_output \
            03_sram_read \
            04_sram_write \
            05_vram_access \
            06_timers \
            07_screen_output \
            08_interrupts

all:
	@echo "Building all base functionality tests..."
	@for dir in $(TEST_DIRS); do \
		echo "Building $$dir..."; \
		$(MAKE) -C $$dir || exit 1; \
	done
	@echo "All tests built successfully!"

clean:
	@echo "Cleaning all base functionality tests..."
	@for dir in $(TEST_DIRS); do \
		echo "Cleaning $$dir..."; \
		$(MAKE) -C $$dir clean; \
	done
	@echo "All tests cleaned!"

model-all: all
	@echo "Running all tests on the model..."
	@for dir in $(TEST_DIRS); do \
		echo "Running $$dir (model)..."; \
		$(MAKE) -C $$dir model || exit 1; \
	done
	@echo "All model runs complete!"

sim-all: all
	@echo "Running all tests in simulation..."
	@for dir in $(TEST_DIRS); do \
		echo "Running $$dir (sim)..."; \
		$(MAKE) -C $$dir sim || exit 1; \
	done
	@echo "All simulations complete!"
