CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic -Iinclude -pthread
LDFLAGS := -pthread

SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin

COMMON_OBJ := $(OBJ_DIR)/common.o
CLIENT_OBJ := $(OBJ_DIR)/client.o
SERVER_OBJ := $(OBJ_DIR)/server.o

CLIENT := $(BIN_DIR)/client
SERVER := $(BIN_DIR)/server

.PHONY: all clean

all: $(CLIENT) $(SERVER)

$(OBJ_DIR) $(BIN_DIR):
	mkdir -p $@

$(COMMON_OBJ): $(SRC_DIR)/common.c include/common.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(CLIENT_OBJ): $(SRC_DIR)/client.c include/common.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(SERVER_OBJ): $(SRC_DIR)/server.c include/common.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(CLIENT): $(CLIENT_OBJ) $(COMMON_OBJ) | $(BIN_DIR)
	$(CC) $^ -o $@ $(LDFLAGS)

$(SERVER): $(SERVER_OBJ) $(COMMON_OBJ) | $(BIN_DIR)
	$(CC) $^ -o $@ $(LDFLAGS)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
