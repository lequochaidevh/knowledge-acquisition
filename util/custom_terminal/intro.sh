#!/usr/bin/bash -e

INTRO_PATH=$(pwd)
INTRO_PATH=$(realpath "$INTRO_PATH")

cd $HARIS_UTILS/script_root
. "$(dirname "$0")/lib.sh"
cd $INTRO_PATH

echo -e "${WHITE} author: _devh_"
echo -ne "${RED}" 
printf "        ____   _____                  _             \n"
echo -ne "${YELLOW}" 
printf "       /   /  /   __\                (_)  ________  \n"
echo -ne "${GREEN}" 
printf "      /   /  /   /______ __  ______ ___  /  _____/  \n"
echo -ne "${CYAN}" 
printf "     /   /__/   /   ___ \` / /  ___//  /  \__  \    \n"
echo -ne "${BLUE}" 
printf "    /   ___    /   /__/  / /  /   /  / ____/  /     \n"
echo -ne "${PURPLE}" 
printf " __/   /  /   /\_____,___\/__/   /__/ /______/      \n"
echo -ne "${GREEN_SLIM}" 
printf "/_____/--/___/ ---------- "
echo -ne "${BLUE}"
printf "robotic"
echo -ne "${GREEN_SLIM}"
printf " ----------       \n"
echo -e "${RESET}"
        