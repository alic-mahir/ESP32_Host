#!/bin/bash

# Check if the correct number of arguments is provided
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <SRC_DIR> <MAKEFILE>"
    exit 1
fi

# Define the source directory and the Makefile from the arguments
SRC_DIR=$1
MAKEFILE=$2

# Check if the source directory exists
if [ ! -d "$SRC_DIR" ]; then
    echo "Directory $SRC_DIR does not exist."
    exit 1
fi

# Check if the directory for the Makefile exists; if not, create it
MAKEFILE_DIR=$(dirname "$MAKEFILE")
if [ ! -d "$MAKEFILE_DIR" ]; then
    echo "Directory $MAKEFILE_DIR does not exist. Creating it..."
    mkdir -p "$MAKEFILE_DIR"
fi

# Remove the Makefile if it exists
if [ -f "$MAKEFILE" ]; then
    rm "$MAKEFILE"
fi

# Create a new Makefile with a header indicating it's automatically generated
{
    echo "# Automatically generated Makefile"
} > "$MAKEFILE"

# Find all .c files in the source directory (not recursively)
C_FILES=$(find "$SRC_DIR" -maxdepth 1 -type f -name "*.c")

# Prepare the new C_SRCS lines
NEW_C_SRCS=""
for file in $C_FILES; do
    NEW_C_SRCS="$NEW_C_SRCS\nC_SRCS += $file"
done

# Prepare the new INC_PATHS line
NEW_INC_PATHS="\nINC_PATHS += -I$SRC_DIR"

# Create a temporary file to hold the new Makefile contents
TMP_FILE=$(mktemp)

# Update the Makefile with the new C_SRCS and INC_PATHS lines
awk -v new_c_srcs="$NEW_C_SRCS" -v new_inc_paths="$NEW_INC_PATHS" '
BEGIN {
    print "# Automatically generated Makefile"
    updated_csrcs = 0
    updated_incpaths = 0
}
{
    if ($1 == "C_SRCS" && $2 == "+=") {
        if (new_c_srcs != "") {
            print new_c_srcs
        } else {
            print $0
        }
        updated_csrcs = 1
    } else if ($1 == "INC_PATHS" && $2 == "+=") {
        if (new_inc_paths != "") {
            print ""
            print new_inc_paths
        } else {
            print $0
        }
        updated_incpaths = 1
    } else {
        print
    }
}
END {
    if (updated_csrcs == 0 && new_c_srcs != "") {
        print new_c_srcs
    }
    if (updated_incpaths == 0 && new_inc_paths != "") {
        print ""
        print new_inc_paths
    }
}
' "$MAKEFILE" > "$TMP_FILE"

# Move the temporary file to the original Makefile
mv "$TMP_FILE" "$MAKEFILE"

echo -e "Makefile $MAKEFILE has been updated with the following C sources and include paths:"
echo -e "$NEW_C_SRCS"
echo -e "$NEW_INC_PATHS"

