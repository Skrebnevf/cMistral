CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -O2 -Iinclude
LDFLAGS = -lcurl -lcjson -lm

# Debug build
debug: CFLAGS += -DDEBUG -g -O0
debug: clean all

SRC_DIR = src
INC_DIR = include
EXAMPLE_DIR = examples
TEST_DIR = tests

LIB_SOURCES = $(SRC_DIR)/mistral.c $(SRC_DIR)/http_client.c $(SRC_DIR)/mistral_utils.c $(SRC_DIR)/mistral_helpers.c
LIB_OBJECTS = $(LIB_SOURCES:.c=.o)
LIB_NAME = libmistral.a

TEST_SOURCES = $(TEST_DIR)/test_http_client.c $(TEST_DIR)/test_mistral.c
TEST_EXECUTABLES = $(TEST_SOURCES:.c=)

all: $(LIB_NAME)

$(LIB_NAME): $(LIB_OBJECTS)
	ar rcs $@ $^


%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


example: $(LIB_NAME)
	$(CC) $(CFLAGS) $(EXAMPLE_DIR)/chat_example.c -L. -lmistral $(LDFLAGS) -o chat_example
	$(CC) $(CFLAGS) $(EXAMPLE_DIR)/fim_example.c -L. -lmistral $(LDFLAGS) -o fim_example
	$(CC) $(CFLAGS) $(EXAMPLE_DIR)/embeddings_example.c -L. -lmistral $(LDFLAGS) -o embeddings_example


tests: $(LIB_NAME)
	@for test in $(TEST_SOURCES); do \
		output=$${test%.c}; \
		$(CC) $(CFLAGS) $$test -L. -lmistral $(LDFLAGS) -o $$output; \
	done


test: tests
	@echo "Running tests..."
	@for test in $(TEST_EXECUTABLES); do \
		echo "Running $$test..."; \
		./$$test || exit 1; \
	done

clean:
	rm -f $(LIB_OBJECTS) $(LIB_NAME) chat_example fim_example embeddings_example $(TEST_EXECUTABLES)

.PHONY: all clean example tests test
