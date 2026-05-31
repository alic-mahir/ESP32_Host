#!/bin/bash

# Check if the correct number of arguments is provided
if [ "$#" -lt 2 ]; then
    echo "Usage: $0 <SRC_DIR> <HEADER_FILE> [<EXCLUDE_FILES>]"
    exit 1
fi

# Define the source directory and the header file from the arguments
SRC_DIR=$1
HEADER_FILE=$2
EXCLUDE_FILES=$3

# Check if the source directory exists
if [ ! -d "$SRC_DIR" ]; then
    echo "Directory $SRC_DIR does not exist."
    exit 1
fi

# Remove the header file if it exists
if [ -f "$HEADER_FILE" ]; then
    rm "$HEADER_FILE"
fi

# Extract the base name of the header file for the guard
HEADER_GUARD=$(basename "$HEADER_FILE" | tr '[:lower:]' '[:upper:]' | tr '.' '_' | tr '-' '_')
HEADER_GUARD="__${HEADER_GUARD}"

# Create a new header file with the specified format
{
    echo "/**"
    echo "* @file        $(basename "$HEADER_FILE")"
    echo "* @author      semir-t"
    echo "* @date        January 2023"
    echo "* @version     1.0.0"
    echo "*/"
    echo ""
    echo "/* Define to prevent recursive inclusion *********************************** */"
    echo "#ifndef $HEADER_GUARD"
    echo "#define $HEADER_GUARD"
    echo ""
    echo "/* Includes **************************************************************** */"
} > "$HEADER_FILE"

# Prepare the exclude pattern
if [ -n "$EXCLUDE_FILES" ]; then
    IFS=', ' read -r -a EXCLUDE_ARRAY <<< "$EXCLUDE_FILES"
    EXCLUDE_PATTERN=""
    for file in "${EXCLUDE_ARRAY[@]}"; do
        EXCLUDE_PATTERN="$EXCLUDE_PATTERN ! -name \"$file\""
    done
fi

# Find all .h files in the source directory (not recursively), excluding the specified files and the destination header file
if [ -n "$EXCLUDE_PATTERN" ]; then
    H_FILES=$(eval find "$SRC_DIR" -maxdepth 1 -type f -name "*.h" ! -name "$(basename "$HEADER_FILE")" $EXCLUDE_PATTERN)
else
    H_FILES=$(find "$SRC_DIR" -maxdepth 1 -type f -name "*.h" ! -name "$(basename "$HEADER_FILE")")
fi

# Include each .h file in the header file
for file in $H_FILES; do
    echo "#include \"$(basename "$file")\"" >> "$HEADER_FILE"
done

# Add the remaining sections to the header file
{
    echo ""
    echo "/* Module configuration **************************************************** */"
    echo ""
    echo "/* Exported constants ****************************************************** */"
    echo ""
    echo "/* Exported macros ********************************************************* */"
    echo ""
    echo "/* Exported types ********************************************************** */"
    echo ""
    echo "/* Exported variables ****************************************************** */"
    echo ""
    echo "/* Exported functions ****************************************************** */"
    echo ""
    echo "#endif // $HEADER_GUARD"
} >> "$HEADER_FILE"

echo "Header file $HEADER_FILE has been created with the following includes:"
echo "$H_FILES"

