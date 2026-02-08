.PHONY: all clean test help

CXX = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra -Iinclude
LLVM_CONFIG = llvm-config-20

LLVM_CXXFLAGS = $(shell $(LLVM_CONFIG) --cxxflags)
LLVM_LDFLAGS = $(shell $(LLVM_CONFIG) --ldflags --system-libs --libs support)

CLANG_LIBS = -lclang-cpp

TARGET = safecpp
SRC_DIR = src
BUILD_DIR = build
TEST_DIR = examples

SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LLVM_LDFLAGS) $(CLANG_LIBS) -o $(TARGET)
	@echo "✓ Built $(TARGET)"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(LLVM_CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
	@echo "✓ Cleaned build artifacts"

test: $(TARGET)
	@echo "Running SafeCpp analyzer on test examples..."
	@echo ""
	./$(TARGET) $(TEST_DIR)/use_after_free.cpp --
	@echo ""
	./$(TARGET) $(TEST_DIR)/double_free.cpp --
	@echo ""
	./$(TARGET) $(TEST_DIR)/memory_leak.cpp --

help:
	@echo "SafeCpp - Memory Safety Analyzer"
	@echo ""
	@echo "Targets:"
	@echo "  make           - Build the analyzer"
	@echo "  make clean     - Remove build artifacts"
	@echo "  make test      - Run on example files"
	@echo "  make help      - Show this help"
