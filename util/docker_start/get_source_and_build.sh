#!/bin/bash

# Parse incoming arguments
FOLDER=""
IS_INSIDE_DOCKER=false
BRANCH="main"

mkdir $DOCKER_WORKER/project_builder_mount
set -e

for arg in "$@"; do
    case $arg in
        --folder=*)
            FOLDER="${arg#*=}"
            shift
            ;;
        --branch=*)
            BRANCH="${arg#*=}"
            shift
            ;;
        --is-inside-docker=*)
            IS_INSIDE_DOCKER="${arg#*=}"
            shift
            ;;
    esac
done

# Check required argument
if [ -z "$FOLDER" ]; then
    echo "[ERROR] Missing mandatory flag: --folder=<folder_name>"
    echo "Usage: ./get_source_and_build.sh --folder=X11Ubuntu22"
    exit 1
fi

# Main logic flow split between Host environment and Docker environment
if [ "$IS_INSIDE_DOCKER" = "false" ]; then
    # Host execution context
    source $SRIPT_ROOT/lib.sh
    echo -e "${BLUE}[HOST] Routing tasks to Docker Handler...${NC}"
    source ./docker_handler.sh
    if [ $FOLDER == "ROS2" ]; then
        run_in_docker_compose "$FOLDER" "--branch=$BRANCH"
        exit 0
    fi
    run_in_docker "$FOLDER" "--branch=$BRANCH"

else
    # Docker execution context
    source /workspace/scripts/lib.sh
    echo -e "${GREEN}[DOCKER] Isolated sandbox active.${NC}"
    echo -e "${GREEN}[DOCKER] Target Folder: $FOLDER | Branch: $BRANCH${NC}"
    
    # Navigate to the writable workspace build directory
    # cd /workspace/project_builder_mount || exit 1

    # Execute build lifecycle scripts from the mapped scripts directory
    echo -e "${YELLOW}[DOCKER] Step 1: Running get_source.sh...${NC}"
    /workspace/scripts/get_source.sh "$BRANCH"

    echo -e "${YELLOW}[DOCKER] Step 2: Running build.sh...${NC}"
    /workspace/scripts/build.sh "$FOLDER"
fi