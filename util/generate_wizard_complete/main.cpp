#include <fstream>
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: gen_auto_complete <cli_tree.json>\n";
        return 1;
    }

    std::ifstream f(argv[1]);
    if (!f) {
        std::cerr << "Cannot open json file\n";
        return 1;
    }

    json j;
    f >> j;

    std::string app      = j["app"];
    auto &      commands = j["root"]["children"];

    /* ---------- bash header ---------- */
    std::cout << "_" << app << "_completion() {\n";
    std::cout << "  local cur prev cmd used\n";
    std::cout << "  cur=\"${COMP_WORDS[COMP_CWORD]}\"\n";
    std::cout << "  prev=\"${COMP_WORDS[COMP_CWORD-1]}\"\n";
    std::cout << "  cmd=\"${COMP_WORDS[1]}\"\n";
    std::cout << "  used=\" ${COMP_WORDS[*]} \"\n\n";

    /* ---------- level 1: command ---------- */
    std::cout << "  if (( COMP_CWORD == 1 )); then\n";
    std::cout << "    COMPREPLY=( $(compgen -W \"";
    for (auto &[cmd, _] : commands.items()) std::cout << cmd << " ";
    std::cout << "\" -- \"$cur\") )\n";
    std::cout << "    return\n";
    std::cout << "  fi\n\n";

    /* ---------- per command ---------- */
    std::cout << "  case \"$cmd\" in\n";

    for (auto &[cmd_name, cmd_data] : commands.items()) {
        auto &flags = cmd_data["flags"];

        std::cout << "    " << cmd_name << ")\n";

        /* ---- flag completion ---- */
        std::cout << "      if [[ -z \"$cur\" || \"$cur\" == --* ]]; then\n";
        std::cout << "        local opts=\"\"\n";

        for (auto &[flag, fdata] : flags.items()) {
            std::string excl = fdata.value("exclusive_group", "");

            std::cout << "        if [[ \"$used\" != *\" " << flag << " \"* ]]; then\n";

            if (!excl.empty()) {
                std::cout << "          if ! [[ \"$used\" =~ (";
                bool first = true;
                for (auto &[f2, d2] : flags.items()) {
                    if (d2.value("exclusive_group", "") == excl) {
                        if (!first) std::cout << "|";
                        std::cout << f2;
                        first = false;
                    }
                }
                std::cout << ") ]]; then\n";
                std::cout << "            opts+=\"" << flag << " \"\n";
                std::cout << "          fi\n";
            } else {
                std::cout << "          opts+=\"" << flag << " \"\n";
            }

            std::cout << "        fi\n";
        }

        std::cout << "        COMPREPLY=( $(compgen -W \"$opts\" -- \"$cur\") )\n";
        std::cout << "        return\n";
        std::cout << "      fi\n";

        /* ---- value completion ---- */
        std::cout << "      case \"$prev\" in\n";

        for (auto &[flag, fdata] : flags.items()) {
            if (fdata.contains("value")) {
                auto &v = fdata["value"];
                if (v["type"] == "enum") {
                    std::cout << "        " << flag << ")\n";
                    std::cout << "          COMPREPLY=( $(compgen -W \"";
                    for (auto &e : v["values"]) std::cout << e.get<std::string>() << " ";
                    std::cout << "\" -- \"$cur\") )\n";
                    std::cout << "          return\n";
                    std::cout << "          ;;\n";
                }
            }
        }

        std::cout << "      esac\n";
        std::cout << "      ;;\n";
    }

    std::cout << "  esac\n";
    std::cout << "}\n\n";
    std::cout << "complete -F _" << app << "_completion " << app << "\n";

    return 0;
}
