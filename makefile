
CC = gcc
CFLAGS = -Wall -Wextra -O2

PARSER_SRCS = src/http.c src/request.c src/dict.c
TEST_BIN = tests/rfc_request_parser_test.out
BENCH_BIN = bench/request_parse_bench.out

all:
	make -C src

test: $(TEST_BIN)
	./$(TEST_BIN)

benchmark: $(BENCH_BIN)
	./$(BENCH_BIN)

$(TEST_BIN): tests/rfc_request_parser_test.c $(PARSER_SRCS)
	$(CC) $(CFLAGS) -o $@ $^

$(BENCH_BIN): bench/request_parse_bench.c $(PARSER_SRCS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TEST_BIN) $(BENCH_BIN)
	make -C src clean
