#!/bin/bash

configure_env() {
    local folder=$1
    export TARGET_FOLDER="$folder"

    case "$folder" in
        "X11Ubuntu22")
            export IMAGE_NAME="builder:ubuntu22"
            export PROJECT_ASSET="builder:ubuntu22"
            ;;
        "TempUbuntu24")
            export IMAGE_NAME="builder:ubuntu24"
            ;;
        *)
            echo -e "${RED}[ERROR] Unsupported folder configuration: $folder${NC}"
            exit 1
            ;;
    esac
}
