#!/bin/bash -e

UTILS_PATH=$(pwd)
DOCS_DIR=$(realpath "$UTILS_PATH")
# Define the list of files
FILES_TO_LINK=("autocmake")
CONFIG_FILE_TXT="wizard_tool_configuration.txt"

cd ${HARIS_UTILS}/script_root/

. "$(dirname "$0")/lib.sh"

export PROJECT_TYPE="TEST FUNCTION"

source setup_enviroment.sh

# export SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
export DOCS_DIR=$(realpath "$HARIS_UTILS/fast_helper/")
LOG_INFO "\tParam Supported:"
ls $DOCS_DIR
LOG_SUCCESS "Declare docs path: $DOCS_DIR"
echo ""

# create file log tag
export INSTALL_DIR=$(realpath "$HARIS_UTILS/fast_helper/")
echo "$DOCS_DIR" > $INSTALL_DIR/$CONFIG_FILE_TXT
cd $INSTALL_DIR
sudo ln -sfv $(pwd)/$CONFIG_FILE_TXT /usr/local/bin/

# Loop through the array
for file in "${FILES_TO_LINK[@]}"; do
    # Get the absolute path of the source file
    SRC=$(realpath "$UTILS_PATH/$file")
    DEST="/usr/local/bin/$file"
    
    # -s: symbolic, -f: force (overwrite), -v: verbose
    sudo ln -sfv "$SRC" "$DEST"

    # Create an absolute path symlink to /usr/local/bin
    echo ""
    LOG_INFO "install symlink $SRC to $DEST"

    if [ -L "$DEST" ]; then
        LOG_SUCCESS "\t\t\t Application is ready to use."
    fi
done