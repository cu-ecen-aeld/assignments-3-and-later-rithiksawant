#!/bin/bash

# Define color codes
RED='\033[0;31m'
DODGER_BLUE='\033[1;34m'
NC='\033[0m' # No Color

# Check if exactly two arguments are provided
if [ $# -ne 2 ]; then
    echo -e "${RED}Error: Two arguments required${NC}"
    echo -e "${DODGER_BLUE}Usage: $0 <writefile> <writestr>${NC}"
    exit 1
fi

# Assign arguments to variables
writefile="$1"
writestr="$2"

# Create the directory path if it doesn't exist
dir=$(dirname "$writefile")
if [ ! -d "$dir" ]; then
    mkdir -p "$dir"
    if [ $? -ne 0 ]; then
        echo -e "${RED}Error: Could not create directory $dir${NC}"
        exit 1
    fi
fi

# Write the content to the file
echo "$writestr" > "$writefile"
if [ $? -ne 0 ]; then
    echo -e "${RED}Error: Could not create file $writefile${NC}"
    exit 1
fi
