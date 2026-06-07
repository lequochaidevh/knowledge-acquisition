#!/usr/bin/bash -e

INTRO_PATH=$(pwd)
INTRO_PATH=$(realpath "$INTRO_PATH")

cd $HARIS_UTILS/script_root
. "$(dirname "$0")/lib.sh"
cd $INTRO_PATH


BG_BLACK="\e[40m"
BG_BRIGHT_CYAN="\e[106m"
BG_BRIGHT_BLUE="\e[104m"
BG_DARK_GRAY="\e[48;2;40;40;40m"
BG_NAVY_BLUE="\e[48;2;20;35;60m"
BG_PURPLE="\e[48;2;90;20;90m"
BG_ORANGE="\e[48;2;200;100;0m"

BACKGROUND="${BG_NAVY_BLUE}"

echo -e ""
echo -e "   ${WHITE} author: _devh_"
echo -e ""

echo -ne "  ${RED}${BACKGROUND}" 
printf "          ____   _____                  _            ${RESET}\n"
echo -ne "  ${YELLOW}${BACKGROUND}" 
printf "         /   /  /   __\                (_) ________  ${RESET}\n"
echo -ne "  ${GREEN}${BACKGROUND}" 
printf "        /   /  /   /______ __  ______ ___ /  _____/  ${RESET}\n"
echo -ne "  ${CYAN}${BACKGROUND}" 
printf "       /   /__/   /   ___ \` / /  ___//  / \__  \     ${RESET}\n"
echo -ne "  ${BLUE}${BACKGROUND}" 
printf "      /   ___    /   /__/  / /  /   /  /____/  /     ${RESET}\n"
echo -ne "  ${PURPLE}${BACKGROUND}" 
printf "   __/   /  /   /\_____,___\/__/   /__//______/      ${RESET}\n"
echo -ne "  ${GREEN_SLIM}${BACKGROUND}" 
printf "  /_____/--/___/----------- "
echo -ne "${NC}${BACKGROUND}"
printf "solutions"
echo -ne "${GREEN_SLIM}${BACKGROUND}"
printf " -----------    ${RESET}\n"
echo -ne "  ${BACKGROUND}"
printf "                                                     ${RESET}\n"