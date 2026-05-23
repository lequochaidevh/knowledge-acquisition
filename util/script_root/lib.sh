#!/bin/bash

# Export color variables for terminal output
export RED='\033[0;31m'
export GREEN='\033[0;32m'
export YELLOW='\033[0;33m'
export BLUE='\033[0;34m'
export PURPLE='\033[0;35m'
export CYAN='\033[0;36m'
export NC='\033[0m' # No Color
export BOLD='\033[1m'
export UNDERLINE='\033[4m'
export ENDLINE='\033[0m'

# Helper to ensure a message was passed to the logger
_validate_require_param() {
    if [[ -z "$1" ]]; then
        echo -e "${RED}${BOLD}INTERNAL ERROR: No message provided to function${NC}"
        return 1
    fi
}

# Core Logger: Handles all colors and tagging logic
_logger_handler() {
    _validate_require_param "$1" || return 1
    local color="$1"
    local tag="$2"
    local msg="$3"
    local no_tag=false

    # Check if the first argument of the message part is "-nt"
    if [[ "$msg" == "-nt" ]]; then
        no_tag=true
        msg="$4" # Move to the next argument for the actual message
    fi

    if [ "$no_tag" = true ]; then
        printf "${color}${BOLD}${msg}${NC}"
        echo
    else
        printf "${color}${BOLD}${tag}: ${msg}${NC}"
        echo
    fi
}

# Wrapper Functions
LOG_INFO()    { _logger_handler "${CYAN}"    "INFO"    "$@"; }
LOG_WARN()    { _logger_handler "${YELLOW}"  "WARNING" "$@"; }
LOG_ERROR()   { _logger_handler "${RED}"     "ERROR"   "$@"; }
LOG_SUCCESS() { _logger_handler "${GREEN}"   "SUCCESS" "$@"; }
LOG_DEBUG()   { _logger_handler "${PURPLE}"  "DEBUG"   "$@"; }

printf_var() {
    _validate_require_param "$1" || return 1
    local var_name="$1"
    local var_value="${!var_name}"
    if [ -z "$var_value" ]; then
        LOG_WARN "Variable '$var_name' is not set."
    else
        printf "${GREEN}${BOLD}%-50s${CYAN}${BOLD}%s${ENDLINE}${NC}\n" "$var_name" "$var_value"
    fi
}

delete_if_exist() {
    _validate_require_param "$1" || return 1
    local file_name="$1"
    if [ -e "$file_name" ]; then
        echo "$file_name exist. Remove it ..."
        rm -rf $file_name
    fi
}

set_default_if_not_exist() {
    _validate_require_param "$1" || return 1
    local var_name=$1
    local default_val=$2

    if [ -z "${!var_name}" ]; then
        export "$var_name"="$default_val"
        LOG_DEBUG "➜ $var_name set to default: ${!var_name}"
    else
        LOG_DEBUG "➜ $var_name existing: ${!var_name}"
    fi
}

check_file_exists() {
    # expect : file exist before
    _validate_require_param "$1" || return 1
    local FILE_PATH="$1"

    if [[ -f "$FILE_PATH" ]]; then
        LOG_SUCCESS "Successfully located: $FILE_PATH"
    else
        LOG_ERROR "'$FILE_PATH' not found."
    fi
}

have_file_exists_unexpected() {
    _validate_require_param "$1" || return 1
    local FILE_PATH="$1"

    if [[ -f "$FILE_PATH" ]]; then
        LOG_ERROR "File exist at located: $FILE_PATH"
        exit 0;
    else
        LOG_INFO "No '$FILE_PATH' be installed before"
        LOG_INFO "--- Install process continue ---"
    fi
}

find_once_file_contain() {
    # NOTE: not print anything in func return string
    _validate_require_param $1 || return 1
    local KEYWORK_FILE_NAME="$1"
    
    echo "$(ls | grep "$KEYWORK_FILE_NAME" | head -n 1)"
}

# TODO:
# create_if_not_exist()