# Build all .c files in `src/` into `build/`
build:
    mkdir -p build
    gcc src/lexer.c -o build/lexer
    gcc src/parser.c -o build/parser

# Execute the lexer and parser on every file in `inputs/`
test-all: build
    ./scripts/run_tests.sh

# Get grammar pdf
grammar:
    typst compile docs/grammar.typ grammar.pdf
