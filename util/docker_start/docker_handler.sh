#!/bin/bash
source $SRIPT_ROOT/lib.sh
source ./configuration.sh

run_in_docker() {
    local folder=$1
    local extra_args=$2

    # Load image configuration
    configure_env "$folder"

    # Check if Docker image exists; build it using the static Dockerfile inside the folder if missing
    if [[ "$(docker images -q $IMAGE_NAME 2> /dev/null)" == "" ]]; then
        echo -e "${YELLOW}[INFO] Image $IMAGE_NAME not found. \
         Building from $DOCKER_WORKER/source/$folder/Dockerfile ...${NC}"
        docker build \
        --build-arg PROJECT_ASSET="$LOCAL_ASSET_PATH" \
        -t "$IMAGE_NAME" \
        -f "$DOCKER_WORKER/source/$folder/Dockerfile" /
    else
        echo -e "${GREEN}[INFO] Image $IMAGE_NAME already exists.${NC}"
    fi

    # Define dynamic workspace variables
    local volume_name="$DOCKER_WORKER/project_builder_mount"
    local host_script_dir="$DOCKER_WORKER/docker_share/"

    # Collect all script
    mkdir -p $host_script_dir
    cp "$DOCKER_SCRIPT"/*.sh $host_script_dir
    cp "$SRIPT_ROOT"/*.sh $host_script_dir

    # exit 0
    echo -e "${BLUE}[INFO] Launching container with isolated build volume...${NC}"

    # - Mount host scripts folder to /workspace/scripts as Read-Only (:ro)
    # - Mount named volume to /workspace/build for isolated scratchpad
    docker run --rm -it \
        -v "$host_script_dir:/workspace/scripts:ro" \
        -v "$volume_name:/workspace/project_builder_mount" \
        "$IMAGE_NAME" \
        /workspace/scripts/get_source_and_build.sh --folder="$folder" $extra_args --is-inside-docker=true

        # bash

    rm -rf $host_script_dir
}

# Todo: restructure
run_in_docker_compose() {
    xhost +local:root

    local folder=$1
    local extra_args=$2

    # Collect all script
    local host_script_dir="$DOCKER_WORKER/docker_share/"
    mkdir -p $host_script_dir
    cp "$DOCKER_SCRIPT"/*.sh $host_script_dir
    cp "$SRIPT_ROOT"/*.sh $host_script_dir

    # Load image configuration
    configure_env "$folder"

    # Check if Docker image exists; build it using the static Dockerfile inside the folder if missing
    if [[ "$(docker images -a | grep $IMAGE_NAME 2> /dev/null)" != "" ]]; then
        echo -e "${YELLOW}[INFO] Image $IMAGE_NAME not found. \
         Building from $DOCKER_WORKER/runtime/ros2 ...${NC}"
        local back_work=$(pwd)
        cd $DOCKER_WORKER/runtime/ros2
        docker compose up -d
        cd $back_work
    else
        echo -e "${GREEN}[INFO] Image $IMAGE_NAME already exists.${NC}"
    fi

    docker exec -it ros2_humble_container \
    /workspace/scripts/get_source_and_build.sh \
    --folder="$folder" $extra_args --is-inside-docker=true

}