CXX := g++
CC  := gcc

FLAGS  := -Wall -Wextra -std=c++20 -pedantic-errors -Iinclude -I../utils -Ithird_party/Inotify_cpp -Ithird_party/Inotify_cpp/inotify \
	-Idesign_patterns/command/include \
	-Idesign_patterns/observer/include \
	-Idesign_patterns/factory/include \
	-Idesign_patterns/singleton/include \
	-Idesign_patterns/reactor/include \
	-Iplugins/include \
	-Iservices/communication_protocols/nbd/include \
	-Iservices/local_storage/include \
	-Iutilities/logger/include \
	-Iutilities/threading/thread_pool/include \
	-Iutilities/thread_safe_data_structures/priority_queue/include
DFLAGS := -g
PIC_FLAG := -fPIC

SRC_DIR := src
TEST_DIR := test
APP_DIR := app
DEBUG_DIR := debug
LIB_DIR := lib
BIN_DIR := bin
UTILS_DIR := utils
THIRD_PARTY_DIR := third_party/*

LIBRARY_NAME := foo
LIBRARY_DEBUG := $(LIB_DIR)/lib$(LIBRARY_NAME)-debug.so

# Find all C++ source files from components (*/src/*.cpp recursively)
SRC_CPP := $(shell find design_patterns utilities services plugins -path "*/src/*.cpp" -type f 2>/dev/null) $(wildcard $(SRC_DIR)/*.cpp)
UTILS_C := $(wildcard $(UTILS_DIR)/*.c)
THIRD_PARTY_CPP := $(wildcard $(THIRD_PARTY_DIR)/*.cpp) $(wildcard third_party/Inotify_cpp/inotify/*.cpp)
TEST_CPP := $(shell find test/unit -name "*.cpp" -type f) $(wildcard $(TEST_DIR)/*.cpp)
APP_CPP := $(wildcard $(APP_DIR)/*.cpp)

# Object file rules with proper path preservation
DEBUG_OBJECTS := \
	$(patsubst design_patterns/%.cpp,$(DEBUG_DIR)/%.o,$(filter design_patterns/%,$(SRC_CPP))) \
	$(patsubst utilities/%.cpp,$(DEBUG_DIR)/%.o,$(filter utilities/%,$(SRC_CPP))) \
	$(patsubst services/%.cpp,$(DEBUG_DIR)/%.o,$(filter services/%,$(SRC_CPP))) \
	$(patsubst plugins/%.cpp,$(DEBUG_DIR)/%.o,$(filter plugins/%,$(SRC_CPP))) \
	$(patsubst $(SRC_DIR)/%.cpp,$(DEBUG_DIR)/%.o,$(filter $(SRC_DIR)/%.cpp,$(SRC_CPP))) \
	$(patsubst $(UTILS_DIR)/%.c,$(DEBUG_DIR)/%.o,$(UTILS_C)) \
	$(patsubst third_party/Inotify_cpp/inotify/%.cpp,$(DEBUG_DIR)/inotify_%.o,$(filter third_party/Inotify_cpp/inotify/%.cpp,$(THIRD_PARTY_CPP))) \
	$(patsubst $(THIRD_PARTY_DIR)/%.cpp,$(DEBUG_DIR)/third_party_%.o,$(filter-out third_party/Inotify_cpp/inotify/%.cpp,$(THIRD_PARTY_CPP)))

# Explicit test binaries from test/unit/
TEST_BINARIES := \
	$(BIN_DIR)/test_command_demo \
	$(BIN_DIR)/test_singelton \
	$(BIN_DIR)/test_dir_monitor \
	$(BIN_DIR)/test_inotify_debug \
	$(BIN_DIR)/test_msg_broker \
	$(BIN_DIR)/test_plugin_load \
	$(BIN_DIR)/test_pnp_main \
	$(BIN_DIR)/test_logger \
	$(BIN_DIR)/test_thread_pool \
	$(BIN_DIR)/test_wpq
APP_BINARIES := $(patsubst $(APP_DIR)/%.cpp,$(BIN_DIR)/%,$(APP_CPP))

# =========================
# Compile C++ source files (from */src/ and src/)
# =========================
$(DEBUG_DIR)/%.o: design_patterns/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) -c $(FLAGS) $(DFLAGS) $(PIC_FLAG) $< -o $@

$(DEBUG_DIR)/%.o: utilities/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) -c $(FLAGS) $(DFLAGS) $(PIC_FLAG) $< -o $@

$(DEBUG_DIR)/%.o: services/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) -c $(FLAGS) $(DFLAGS) $(PIC_FLAG) $< -o $@

$(DEBUG_DIR)/%.o: plugins/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) -c $(FLAGS) $(DFLAGS) $(PIC_FLAG) $< -o $@

$(DEBUG_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(DEBUG_DIR)
	$(CXX) -c $(FLAGS) $(DFLAGS) $(PIC_FLAG) $< -o $@

# =========================
# Compile C utils
# =========================
$(DEBUG_DIR)/%.o: $(UTILS_DIR)/%.c
	@mkdir -p $(DEBUG_DIR)
	$(CC) -c $(DFLAGS) $(PIC_FLAG) $< -o $@

# =========================
# Compile inotify
# =========================
$(DEBUG_DIR)/inotify_%.o: third_party/Inotify_cpp/inotify/%.cpp
	@mkdir -p $(DEBUG_DIR)
	$(CXX) -c $(FLAGS) $(DFLAGS) $(PIC_FLAG) $< -o $@

# =========================
# Compile third party
# =========================
$(DEBUG_DIR)/third_party_%.o: $(THIRD_PARTY_DIR)/%.cpp
	@mkdir -p $(DEBUG_DIR)
	$(CXX) -c $(FLAGS) $(DFLAGS) $(PIC_FLAG) $< -o $@

# =========================
# Build shared library
# =========================
$(LIBRARY_DEBUG): $(DEBUG_OBJECTS)
	@mkdir -p $(LIB_DIR)
	$(CXX) -shared -o $@ $^

# =========================
# Build test binaries
# =========================
$(BIN_DIR)/test_%: $(TEST_DIR)/%.cpp $(LIBRARY_DEBUG)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(FLAGS) $(DFLAGS) \
		-L$(LIB_DIR) -Wl,-rpath,$(LIB_DIR) \
		$< -o $@ -l$(LIBRARY_NAME)-debug -pthread

# Build tests - single rule for flattened test/unit/
$(BIN_DIR)/test_%: test/unit/test_%.cpp $(LIBRARY_DEBUG)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(FLAGS) $(DFLAGS) -L$(LIB_DIR) -Wl,-rpath,$(LIB_DIR) $< -o $@ -l$(LIBRARY_NAME)-debug -pthread

$(BIN_DIR)/test_%: $(TEST_DIR)/test_%.cpp $(LIBRARY_DEBUG)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(FLAGS) $(DFLAGS) -L$(LIB_DIR) -Wl,-rpath,$(LIB_DIR) $< -o $@ -l$(LIBRARY_NAME)-debug -pthread

# =========================
# Build app binaries
# =========================
$(BIN_DIR)/%: $(APP_DIR)/%.cpp $(LIBRARY_DEBUG)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(FLAGS) $(DFLAGS) \
		-L$(LIB_DIR) -Wl,-rpath,$(LIB_DIR) \
		$< -o $@ -l$(LIBRARY_NAME)-debug -pthread

# =========================
# Build plugins (only sample_plugin)
# =========================
PLUGIN_DIR := plugins/src
PLUGIN_CPP := $(PLUGIN_DIR)/sample_plugin.cpp
PLUGINS_SO := $(BIN_DIR)/sample_plugin.so
PLUGINS := $(PLUGINS_SO)

# Compile as shared object (.so)
$(BIN_DIR)/sample_plugin.so: $(PLUGIN_CPP)
	@mkdir -p $(BIN_DIR)
	$(CXX) -shared $(FLAGS) $(DFLAGS) $(PIC_FLAG) $< -o $@

# =========================
# Targets
# =========================
all: $(LIBRARY_DEBUG) $(PLUGINS) $(APP_BINARIES) $(TEST_BINARIES)

plugins: $(PLUGINS)

test: all $(TEST_BINARIES)

app: all $(APP_BINARIES)

test_pnp: plugins app
	./$(BIN_DIR)/test_pnp_main

run_tests: test
	@for t in $(TEST_BINARIES); do \
		echo "Running $$t"; \
		$$t; \
	done

test_nbd: app
	@PATH=.:$$PATH sudo -E bash test/test_readwrite.sh
	@PATH=.:$$PATH sudo -E bash test/test_signals.sh

clean:
	rm -rf $(DEBUG_DIR) $(LIB_DIR) $(BIN_DIR)

.PHONY: all plugins test app run_tests test_nbd clean