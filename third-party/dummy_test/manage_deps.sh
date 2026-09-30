#!/bin/bash
set -e

# 1. Validate custom installation directory argument
if [ -z "$1" ]; then
    echo "Error: Please specify a custom install directory."
    echo "Usage: $0 <path_to_install_dir>"
    exit 1
fi

# Convert installation path to an absolute path
INSTALL_DIR=$(realpath "$1")
_SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE}")" && pwd)"
ROOT_DIR=$(realpath "$_SCRIPT_DIR/..")

# Define the builder directory inside third-party
THIRD_PARTY_BUILDER="$_SCRIPT_DIR/builders"

echo "[+] Target Custom Install Directory: $INSTALL_DIR"
echo "[+] Builder Directory: $THIRD_PARTY_BUILDER"
mkdir -p "$INSTALL_DIR"

# 2. Automatically initialize and update Git Submodules first
echo "[+] Syncing and updating Git submodules..."
cd "$ROOT_DIR"
git submodule update --init --recursive
cd "$_SCRIPT_DIR"

# 3. Read .gitmodules dynamically to find third-party repositories
echo "=================================================="
echo "[+] Scanning .gitmodules for third-party dependencies..."

# Parse the '.gitmodules' file to extract all 'path' values
# 'git config -f' is the safest industry standard way to read .gitmodules
SUBMODULE_PATHS=$(git config -f "$ROOT_DIR/.gitmodules" --get-regexp '^submodule\..*\.path$' | awk '{print $2}')

for sub_path in $SUBMODULE_PATHS; do
    # Check if the submodule is located inside the 'third-party' directory
    if [[ "$sub_path" == third-party/* ]]; then
        
        # Extract the repository folder name (e.g., third-party/googletest -> googletest)
        repo_folder=$(basename "$sub_path")
        
        echo "--------------------------------------------------"
        echo "[+] Found Submodule Repo: $repo_folder"

        # Dynamically define the specific build script name
        # Format: ${repo_prefix}_build_and_install.sh
        SPECIFIC_BUILDER_SCRIPT="$THIRD_PARTY_BUILDER/${repo_folder}_build_and_install.sh"

        # Check if the custom builder script exists for this specific repo
        if [ -f "$SPECIFIC_BUILDER_SCRIPT" ]; then
            echo "[+] Executing: $(basename "$SPECIFIC_BUILDER_SCRIPT")"
            chmod +x "$SPECIFIC_BUILDER_SCRIPT"
            
            # Call the specific builder and pass the custom absolute install directory
            "$SPECIFIC_BUILDER_SCRIPT" "$INSTALL_DIR"
        else
            echo "[] Warning: No builder script found at $SPECIFIC_BUILDER_SCRIPT"
            echo "    Skipping automatic build for $repo_folder."
        fi
    fi
done

echo "=================================================="
echo " Dynamic pipeline complete! All verified assets ready at: $INSTALL_DIR"
