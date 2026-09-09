CC = gcc

CFLAGS = \
	-std=c11 \
	-Wall \
	-Wextra \
	-Wpedantic \
	-g \
	-pthread \
	-Iinclude

BUILD_DIR = build

SERVER_SOURCES = \
	src/main.c \
	src/server.c \
	src/protocol.c \
	src/state.c

SERVER = $(BUILD_DIR)/cci-server
STATE_TEST = $(BUILD_DIR)/test-state
PROTOCOL_TEST = $(BUILD_DIR)/test-protocol

.PHONY: all clean test run

all: $(SERVER)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(SERVER): $(SERVER_SOURCES) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SERVER_SOURCES) -o $(SERVER)

$(STATE_TEST): tests/test_state.c src/state.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) tests/test_state.c src/state.c -o $(STATE_TEST)

$(PROTOCOL_TEST): tests/test_protocol.c src/protocol.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) tests/test_protocol.c src/protocol.c -o $(PROTOCOL_TEST)

test: $(SERVER) $(STATE_TEST) $(PROTOCOL_TEST)
	./$(STATE_TEST)
	./$(PROTOCOL_TEST)
	python3 tests/it_test.py

run: $(SERVER)
	./$(SERVER) 9000

clean:
	rm -rf $(BUILD_DIR)