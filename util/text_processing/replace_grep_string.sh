#!/bin/bash

# ex: ./replace_grep_string.sh "_domain" "_address_families" path/to/unix/socket/

# Define your preferred editor tool here
EDITOR_TOOL="code" # Examples: "code" (VS Code), "subl" (Sublime), "nano", "vim"

# Ensure correct number of arguments
if [ "$#" -lt 3 ]; then
    echo "Usage: $0 <search_string> <replace_string> <file_or_folder1> [file_or_folder2 ...]"
    exit 1
fi

SEARCH_STR="$1"
REPLACE_STR="$2"
shift 2

# ANSI Color Codes
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' 
BOLD='\033[1m'

# Array to store modified files
MODIFIED_FILES=()

# Loop through all provided targets
for TARGET in "$@"; do
    if [ -d "$TARGET" ]; then
        FILES=$(find "$TARGET" -type f)
    else
        FILES="$TARGET"
    fi

    for FILE in $FILES; do
        [ -f "$FILE" ] || continue

        if grep -q "$SEARCH_STR" "$FILE"; then
            echo -e "${BOLD}Modifying file:${NC} $FILE"
            
            # Save the modified file to our array
            MODIFIED_FILES+=("$FILE")
            
            # Read file line by line for debug preview
            while IFS= read -r line || [ -n "$line" ]; do
                if [[ "$line" == *"$SEARCH_STR"* ]]; then
                    BEFORE="${line//$SEARCH_STR/${RED}${SEARCH_STR}${NC}}"
                    NEW_LINE="${line//$SEARCH_STR/$REPLACE_STR}"
                    AFTER="${NEW_LINE//$REPLACE_STR/${GREEN}${REPLACE_STR}${NC}}"
                    
                    echo -e "  ${RED}-${NC} $BEFORE"
                    echo -e "  ${GREEN}+${NC} $AFTER"
                fi
            done < "$FILE"

            # In-place replacement
            sed "s/$SEARCH_STR/$REPLACE_STR/g" "$FILE" > "$FILE.tmp" && mv "$FILE.tmp" "$FILE"
            echo "----------------------------------------"
        fi
    done
done

# Open all modified files with the hardcoded tool if any changes were made
if [ ${#MODIFIED_FILES[@]} -gt 0 ]; then
    echo -e "${BOLD}Opening ${#MODIFIED_FILES[@]} modified file(s) with $EDITOR_TOOL...${NC}"
    "$EDITOR_TOOL" "${MODIFIED_FILES[@]}"
else
    echo "No matching strings found. No files modified."
fi
