# Build all .c files in `src/` into `build/`
build:
    mkdir -p build && cd build && \
    gcc ../src/lexer.c -o lexer && \
    gcc ../src/parser.c -o parser

# Execute the lexer and parser on every file in `inputs/`
test-all:
    ./scripts/run_tests.sh
