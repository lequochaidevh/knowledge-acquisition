#!/bin/bash
echo "--> Starting compile process..."
if [ -f cloned_source.txt ]; then
    cat cloned_source.txt
    echo "Compilation complete! Binary output: app.bin"
    echo "101010" > app.bin
else
    echo "[ERROR] Source file missing inside build volume."
    exit 1
fi

if [[ $1 == "ROS2" ]]; then
    # setup
    source "/opt/ros/humble/setup.bash"
    if [[ $2 == "franka" ]]; then
        /workspace/scripts/franka/run.sh
    fi
    bash
fi
