all: setup_build
	@cd ./build && cmake .. && make database

setup_build:

	@if [ -d "build" ]; then \
		echo "Removing existing build folder..."; \
		rm -rf build; \
	fi

	@echo "Creating build folder..."
	@mkdir build

	@if [ -d "myroot" ]; then \
		echo "Removing existing myroot folder..."; \
		rm -rf myroot; \
	fi
	@mkdir myroot
	@mkdir myroot/base
	@mkdir myroot/global



