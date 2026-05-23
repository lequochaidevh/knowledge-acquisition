#!/usr/bin/bash -e

AT_HERE_PATH=$(pwd)
AT_HERE_PATH=$(realpath "$AT_HERE_PATH")

cd ../../script_root
. "$(dirname "$0")/lib.sh"
cd $AT_HERE_PATH

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)

echo ""

cd $SCRIPT_DIR/../
source intro.sh
cd $AT_HERE_PATH

git_branch() {
    local branch
    branch=$(git rev-parse --abbrev-ref HEAD 2>/dev/null)
    if [[ -n "$branch" ]]; then
        echo "$branch"
    fi
}

export PS1_FIXED=1

set_prompt() {
    local cols=$(tput cols)
    local pwd_len=${#PWD}
    local endcmd="${YELLOW_SLIM}($(git_branch)) ${CYAN_SLIM}$(date +%D-%T)${RESET}"

    clean_color_endcmd=$(echo -e "$endcmd" | perl -pe 's/\e\[[0-9;]*[mK]//g')
    local endcmd_len=${#clean_color_endcmd}

    # space 15
    endcmd_len=$((endcmd_len + 15))

    local path_wrk="\w"
    local path_full_wrk=""

    # Short path if path_working long more than cols
    if ((cols < (pwd_len + endcmd_len))); then
        path_wrk="\W"
        pwd_len=$(basename "$PWD")
        pwd_len=${#pwd_len}
        local path_full_wrk="\n\w"
    fi

    local padding_len=$((cols - pwd_len - endcmd_len))

    # TODO no tuning - no hardcode
    # if ((padding_len < 10 )) && (( padding_len > 1 )); then
    #     padding_len=6
    # else

    if ((padding_len < 1)); then
        # padding_len=$((cols - pwd_len - 3))
        endcmd="\n│ ${YELLOW_SLIM}($(git_branch)) ${CYAN_SLIM}$(date +%D-%T)${RESET}"
    fi

    local padding=$(printf "%*s" "$padding_len" "" | tr " " " ")

    if [ ${PS1_FIXED} -ne 0 ]; then
	    PS1="${path_full_wrk}\n┌[${BOLD}${GREEN}\u@ ${BLUE}${path_wrk}${RESET}] ${WHITE}${UNDERLINE}${padding}${RESET} ${endcmd}\n│\n╰──> "
    fi
}

PROMPT_COMMAND=set_prompt
