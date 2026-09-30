# Simulador de substituição de páginas — build e testes (WSL Ubuntu, g++).

CXX      ?= g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Wpedantic
BUILD    := build

LIB_SRCS := $(filter-out src/main.cpp,$(wildcard src/*.cpp))
LIB_OBJS := $(LIB_SRCS:src/%.cpp=$(BUILD)/%.o)
TEST_SRCS := $(wildcard tests/*.cpp)
TEST_OBJS := $(TEST_SRCS:tests/%.cpp=$(BUILD)/tests/%.o)

.PHONY: all test clean

all: $(BUILD)/sim

$(BUILD)/sim: $(BUILD)/main.o $(LIB_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILD)/test_sim: $(TEST_OBJS) $(LIB_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILD)/%.o: src/%.cpp $(wildcard src/*.hpp) | $(BUILD)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILD)/tests/%.o: tests/%.cpp $(wildcard src/*.hpp) | $(BUILD)
	@mkdir -p $(BUILD)/tests
	$(CXX) $(CXXFLAGS) -Isrc -c -o $@ $<

$(BUILD):
	mkdir -p $(BUILD)

# O teste de ponta a ponta executa o binário, então ele precisa existir antes.
test: $(BUILD)/sim $(BUILD)/test_sim
	./$(BUILD)/test_sim

clean:
	rm -rf $(BUILD)
