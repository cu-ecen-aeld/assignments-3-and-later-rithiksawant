#!/bin/bash

# Define color codes
RED='\033[0;31m'
DODGER_BLUE='\033[1;34m'
NC='\033[0m' # No Color

# Check if exactly two arguments are provided
if [ $# -ne 2 ]; then
    echo -e "${RED}Error: Two arguments required${NC}"
    echo -e "${DODGER_BLUE}Usage: $0 <filesdir> <searchstr>${NC}"
    exit 1
fi

# Assign arguments to variables
filesdir="$1"
searchstr="$2"

# Check if filesdir is a directory
if [ ! -d "$filesdir" ]; then
    echo -e "${RED}Error: '$filesdir' is not a directory${NC}"
    exit 1
fi

# Count the number of files in the directory and subdirectories
file_count=$(find "$filesdir" -type f | wc -l)

# Count the number of matching lines in all files
line_count=$(grep -r "$searchstr" "$filesdir" 2>/dev/null | wc -l)

# Print the result
echo "The number of files are $file_count and the number of matching lines are $line_count"
