#!/bin/bash

# Create test-outputs directory
mkdir -p test-outputs

# Change to build directory where lexer and parser executables are located
cd build

# Process each .x25a file in the inputs directory
for input_file in ../inputs/*.x25a; do
    # Extract just the filename without path and extension
    input_name=$(basename "$input_file" .x25a)

    echo "Processing $input_name..."

    # Run lexer: ./lexer ../inputs/<input_name>.x25a > ../test-outputs/<input_name>.lex
    ./lexer "../inputs/${input_name}.x25a" > "../test-outputs/${input_name}.lex"

    # Check if lexer succeeded
    if [ $? -eq 0 ]; then
        echo "  Lexer completed for $input_name"

        # Run parser: ./parser ../test-outputs/<input_name>.lex
        ./parser "../test-outputs/${input_name}.lex"

        # Check if parser succeeded
        if [ $? -eq 0 ]; then
            echo "  Parser completed for $input_name"
        else
            echo "  Parser failed for $input_name"
        fi
    else
        echo "  Lexer failed for $input_name"
    fi

    echo ""
done

echo "Testing complete. Results are in test-outputs/"
