#include "console.h"

#include <SDL2/SDL.h>
#include <algorithm>
#include <cctype>
#include <sstream>

namespace {
void addLogLine(ConsoleState& console, const std::string& line) {
    console.log.push_back(line);
    const size_t maxLines = 200;
    if (console.log.size() > maxLines) {
        console.log.erase(console.log.begin(), console.log.begin() + (console.log.size() - maxLines));
    }
}

std::string trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
        ++start;
    }
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
        --end;
    }
    return s.substr(start, end - start);
}

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::vector<std::string> tokenize(const std::string& cmd) {
    std::istringstream iss(cmd);
    std::vector<std::string> tokens;
    std::string tok;
    while (iss >> tok) {
        tokens.push_back(tok);
    }
    return tokens;
}

void printHelp(ConsoleState& console) {
    addLogLine(console, "Commands:");
    addLogLine(console, "  help               - Show this help");
    addLogLine(console, "  clear              - Clear console output");
    addLogLine(console, "  pos                - Print player position");
    addLogLine(console, "  speed              - Show movement speeds");
    addLogLine(console, "  set_speed <v>      - Set walk speed");
    addLogLine(console, "  set_sprint <v>     - Set sprint speed");
    addLogLine(console, "  wall_height <v>    - Set wall height scale");
    addLogLine(console, "  show_fps           - Toggle FPS counter");
    addLogLine(console, "  quit/exit          - Quit the game");
}

bool parseDouble(const std::string& s, double& out) {
    std::istringstream iss(s);
    iss >> out;
    return !iss.fail() && iss.eof();
}

void handleCommand(ConsoleState& console, const std::string& rawCmd, Config& cfg, Player& player, bool& running) {
    std::string cmd = trim(rawCmd);
    if (cmd.empty()) {
        return;
    }
    addLogLine(console, "> " + cmd);
    std::vector<std::string> tokens = tokenize(toLower(cmd));
    if (tokens.empty()) {
        return;
    }
    const std::string& name = tokens[0];
    if (name == "help") {
        printHelp(console);
    } else if (name == "clear") {
        console.log.clear();
    } else if (name == "pos") {
        std::ostringstream oss;
        oss.precision(2);
        oss << std::fixed << "pos: (" << player.x << ", " << player.y << ")";
        addLogLine(console, oss.str());
    } else if (name == "speed") {
        std::ostringstream oss;
        oss.precision(2);
        oss << std::fixed << "walk=" << cfg.moveSpeed << " sprint=" << cfg.moveSpeedSprint;
        addLogLine(console, oss.str());
    } else if (name == "set_speed" && tokens.size() >= 2) {
        double v = 0.0;
        if (parseDouble(tokens[1], v) && v > 0.0) {
            cfg.moveSpeed = v;
            std::ostringstream oss;
            oss.precision(2);
            oss << std::fixed << "walk speed set to " << v;
            addLogLine(console, oss.str());
        } else {
            addLogLine(console, "Invalid speed value");
        }
    } else if (name == "set_sprint" && tokens.size() >= 2) {
        double v = 0.0;
        if (parseDouble(tokens[1], v) && v > 0.0) {
            cfg.moveSpeedSprint = v;
            std::ostringstream oss;
            oss.precision(2);
            oss << std::fixed << "sprint speed set to " << v;
            addLogLine(console, oss.str());
        } else {
            addLogLine(console, "Invalid sprint value");
        }
    } else if (name == "wall_height" && tokens.size() >= 2) {
        double v = 0.0;
        if (parseDouble(tokens[1], v) && v > 0.1) {
            cfg.wallHeight = v;
            std::ostringstream oss;
            oss.precision(2);
            oss << std::fixed << "wall height set to " << v;
            addLogLine(console, oss.str());
        } else {
            addLogLine(console, "Invalid wall height value");
        }
    } else if (name == "show_fps") {
        console.showFPS = !console.showFPS;
        addLogLine(console, std::string("FPS display ") + (console.showFPS ? "enabled" : "disabled"));
    } else if (name == "quit" || name == "exit") {
        running = false;
    } else {
        addLogLine(console, "Unknown command: " + name);
    }
}

void submitInput(ConsoleState& console, Config& cfg, Player& player, bool& running) {
    std::string trimmed = trim(console.input);
    if (!trimmed.empty()) {
        console.history.push_back(trimmed);
        const size_t maxHist = 50;
        if (console.history.size() > maxHist) {
            console.history.erase(console.history.begin(), console.history.begin() + (console.history.size() - maxHist));
        }
    }
    console.historyIndex = -1;
    handleCommand(console, console.input, cfg, player, running);
    console.input.clear();
}
} // namespace

void setConsoleOpen(ConsoleState& console, bool open) {
    if (console.open == open) {
        return;
    }
    console.open = open;
    console.historyIndex = -1;
    if (open) {
        SDL_StartTextInput();
    } else {
        SDL_StopTextInput();
    }
}

void handleConsoleEvent(ConsoleState& console, const SDL_Event& e, Config& cfg, Player& player, bool& running) {
    if (!console.open) {
        return;
    }
    if (e.type == SDL_TEXTINPUT) {
        console.input += e.text.text;
    } else if (e.type == SDL_KEYDOWN) {
        SDL_Keycode key = e.key.keysym.sym;
        if (key == SDLK_BACKSPACE) {
            if (!console.input.empty()) {
                console.input.pop_back();
            }
        } else if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
            submitInput(console, cfg, player, running);
        } else if (key == SDLK_UP) {
            if (!console.history.empty()) {
                if (console.historyIndex == -1) {
                    console.historyIndex = static_cast<int>(console.history.size() - 1);
                } else if (console.historyIndex > 0) {
                    console.historyIndex--;
                }
                console.input = console.history[console.historyIndex];
            }
        } else if (key == SDLK_DOWN) {
            if (console.historyIndex != -1) {
                console.historyIndex++;
                if (console.historyIndex >= static_cast<int>(console.history.size())) {
                    console.historyIndex = -1;
                    console.input.clear();
                } else {
                    console.input = console.history[console.historyIndex];
                }
            }
        }
    }
}
