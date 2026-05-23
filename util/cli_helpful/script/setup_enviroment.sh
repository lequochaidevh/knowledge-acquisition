#!/bin/bash -e

. "$(dirname "$0")/lib.sh"

if [ ! -z "$RESET_ENV_EXIST" ]; then
    # TODO add feature reset set_env.sh
    echo "Reset env"
    echo "$RESET_ENV_EXIST"
elif [ ! -z "$SET_ENV_EXIST" ]; then
    # echo "set_env have been exist"
    return 0
else
    SET_ENV_EXIST="EXISTED"
fi

# This script sets up the environment for the project.
export SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
