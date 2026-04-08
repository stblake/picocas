
CC = gcc
CFLAGS = -O3 -std=c99 -Wall -Wextra -g -I./src -I/opt/homebrew/include
LDFLAGS = -lreadline -L/opt/homebrew/lib -lgmp
SRC_DIR = src
SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(SRC:.c=.o)
TARGET = picocas

TEST_BINARIES = eval_tests expr_tests parse_tests test_ld test_ops test_pattern list_tests stats_tests expand_tests
TEST_DIR = tests
CMAKE_TEST_BINARIES = comparisons_tests eval_tests expr_tests match_tests match_extensive_tests parse_tests regression_tests symtab_tests list_tests trig_tests hyperbolic_tests logexp_tests piecewise_tests purefunc_tests stats_tests expand_tests

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(SRC_DIR)/boolean.o: $(SRC_DIR)/boolean.c
	$(CC) $(CFLAGS) -c $< -o $@

$(SRC_DIR)/list.o: $(SRC_DIR)/list.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
	rm -rf *.dSYM
	rm -f $(TEST_BINARIES)
	rm -f *~
	rm -f $(SRC_DIR)/*~
	rm -f $(TEST_DIR)/*~
	rm -f *.o
	if [ -f $(TEST_DIR)/Makefile ]; then $(MAKE) -C $(TEST_DIR) clean; fi
	rm -f $(addprefix $(TEST_DIR)/, $(CMAKE_TEST_BINARIES))

.PHONY: all clean
