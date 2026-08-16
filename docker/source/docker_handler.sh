#!/bin/bash
source ./lib.sh
source ./configuration.sh

run_in_docker() {
    local folder=$1
    local extra_args=$2

    # Load image configuration
    configure_env "$folder"

    # Check if Docker image exists; build it using the static Dockerfile inside the folder if missing
    if [[ "$(docker images -q $IMAGE_NAME 2> /dev/null)" == "" ]]; then
        echo -e "${YELLOW}[INFO] Image $IMAGE_NAME not found. Building from $folder/Dockerfile...${NC}"
        docker build \
        --build-arg PROJECT_ASSET="$LOCAL_ASSET_PATH" \
        -t "$IMAGE_NAME" \
        -f "$(pwd)/$folder/Dockerfile" /
    else
        echo -e "${GREEN}[INFO] Image $IMAGE_NAME already exists.${NC}"
    fi

    # Define dynamic workspace variables
    local volume_name="$(pwd)/project_builder_mount"
    local host_script_dir="$(pwd)"

    echo -e "${BLUE}[INFO] Launching container with isolated build volume...${NC}"
    
    # - Mount host scripts folder to /workspace/scripts as Read-Only (:ro)
    # - Mount named volume to /workspace/project_builder_mount for isolated scratchpad
    docker run --rm -it --net=host \
        --cpus="2.0" \
        --memory="4g" \
        --memory-swap="4g" \
        -v "$host_script_dir:/workspace/scripts:ro" \
        -v "$volume_name:/workspace/builder" \
        -v /tmp/.X11-unix:/tmp/.X11-unix \
        -v /run/user/1000/gdm/Xauthority:/tmp/.Xauthority:ro \
        -e DISPLAY=$DISPLAY \
        -e XAUTHORITY=/tmp/.Xauthority \
        "$IMAGE_NAME" \
        /workspace/scripts/get_source_and_build.sh --folder="$folder" $extra_args --is-inside-docker=true
}

# 1. Replace line "*get_source_and_build.sh*"" by "bash" for debug

# 2. --memory-swap=: Sets the total amount of RAM + Swap memory combined. 
# (Set it equal to --memory if you want to disable swap completely for the container).