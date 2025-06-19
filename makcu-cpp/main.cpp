#include "include/makcu.h"
#include <atomic>
#include <thread>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <map>
#include <cmath>
#include <Windows.h>

// Spray patterns database ( Ill adjust them more when I get more freetime ) 
std::map<std::string, std::vector<std::pair<int, int>>> WEAPON_PATTERNS = {
    {"AK-47", {
        {-4,7}, {4,19}, {-3,29}, {-1,31}, {13,31},
        {8,28}, {13,21}, {-17,12}, {-42,-3}, {-21,2},
        {12,11}, {-15,7}, {-26,-8}, {-3,4}, {40,1},
        {19,7}, {14,10}, {27,0}, {33,-10}, {-21,-2},
        {7,3}, {-7,9}, {-8,4}, {19,-3}, {5,6},
        {-20,-1}, {-33,-4}, {-45,-21}, {-14,1}
    }},
    {"M4A1-S", {
        {1,6}, {0,4}, {-4,14}, {4,18}, {-6,21},
        {-4,24}, {14,14}, {8,12}, {18,5}, {-4,10},
        {-14,5}, {-25,-3}, {-19,0}, {-22,-3}, {1,3},
        {8,3}, {-9,1}, {-13,-2}, {3,2}, {1,1}
    }},
    {"M4A4", {
        {2,7}, {0,9}, {-6,16}, {7,21}, {-9,23},
        {-5,27}, {16,15}, {11,13}, {22,5}, {-4,11},
        {-18,6}, {-30,-4}, {-24,0}, {-25,-6}, {0,4},
        {8,4}, {-11,1}, {-13,-2}, {2,2}, {33,-1},
        {10,6}, {27,3}, {10,2}, {11,0}, {-12,0},
        {6,5}, {4,5}, {3,1}, {4,-1}
    }},
    {"FAMAS", {
        {-4,5}, {1,4}, {-6,10}, {-1,17}, {0,20},
        {14,18}, {16,12}, {-6,12}, {-20,8}, {-16,5},
        {-13,2}, {4,5}, {23,4}, {12,6}, {20,-3},
        {5,0}, {15,0}, {3,5}, {-4,3}, {-25,-1},
        {-3,2}, {11,0}, {15,-7}, {15,-10}
    }},
    {"GALIL AR", {
        {4,4}, {-2,5}, {6,10}, {12,15}, {-1,21},
        {2,24}, {6,16}, {11,10}, {-4,14}, {-22,8},
        {-30,-3}, {-29,-13}, {-9,8}, {-12,2}, {-7,1},
        {0,1}, {4,7}, {25,7}, {14,4}, {25,-3},
        {31,-9}, {6,3}, {-12,3}, {13,-1}, {10,-1},
        {16,-4}, {-9,5}, {-32,-5}, {-24,-3}, {-15,5},
        {6,8}, {-14,-3}, {-24,-14}, {-13,-1}
    }},
    {"UMP-45", {
        {-1,6}, {-4,8}, {-2,18}, {-4,23}, {-9,23},
        {-3,26}, {11,17}, {-4,12}, {9,13}, {18,8},
        {15,5}, {-1,3}, {5,6}, {0,6}, {9,-3},
        {5,-1}, {-12,4}, {-19,1}, {-1,-2}, {15,-5},
        {17,-2}, {-6,3}, {-20,-2}, {-3,-1}
    }},
    {"AUG", {
        {5,6}, {0,13}, {-5,22}, {-7,26}, {5,29},
        {9,30}, {14,21}, {6,15}, {14,13}, {-16,11},
        {-5,6}, {13,0}, {1,6}, {-22,5}, {-38,-11},
        {-31,-13}, {-3,6}, {-5,5}, {-9,0}, {24,1},
        {32,3}, {15,6}, {-5,1}
    }},
    {"SG 553", {
        {-4,9}, {-13,15}, {-9,25}, {-6,29}, {-8,31},
        {-7,36}, {-20,14}, {14,17}, {-8,12}, {-15,8},
        {-5,5}, {6,5}, {-8,6}, {2,11}, {-14,-6},
        {-20,-17}, {-18,-9}, {-8,-2}, {41,3}, {56,-5},
        {43,-1}, {18,9}, {14,9}, {6,7}, {21,-3},
        {29,-4}, {-6,8}, {-15,5}, {-38,-5}
    }}
};

class RecoilControl {
public:
    RecoilControl(makcu::Device& device)
        : device(device), isActive(false), currentBullet(0) {
        setWeapon("AK-47");
    }

    void start() {
        if (isActive) return;
        isActive = true;
        currentBullet = 0;
        if (controlThread.joinable()) {
            controlThread.join();
        }
        controlThread = std::thread(&RecoilControl::controlLoop, this);
    }

    void stop() {
        isActive = false;
        if (controlThread.joinable()) {
            controlThread.join();
        }
    }

    void configure(float dpi, float sens, float scaleX = 1.0f, float scaleY = 1.0f) {
        this->dpi = dpi;
        this->sens = sens;
        conversionFactor = (2.54f) / sens;
        this->scaleX = scaleX;
        this->scaleY = scaleY;
    }

    void setWeapon(const std::string& weaponName) {
        if (WEAPON_PATTERNS.find(weaponName) == WEAPON_PATTERNS.end()) {
            std::cerr << "Weapon not found! Using AK-47\n";
            currentPattern = &WEAPON_PATTERNS["AK-47"];
            currentWeapon = "AK-47";
            return;
        }

        currentPattern = &WEAPON_PATTERNS[weaponName];
        currentWeapon = weaponName;
        std::cout << "Weapon set to: " << weaponName << "\n";
    }

    void saveSettings(const std::string& filename) {
        std::ofstream file(filename);
        if (file.is_open()) {
            file << "dpi=" << dpi << "\n";
            file << "sens=" << sens << "\n";
            file << "scaleX=" << scaleX << "\n";
            file << "scaleY=" << scaleY << "\n";
            file << "weapon=" << currentWeapon << "\n";
            file.close();
            std::cout << "Settings saved to " << filename << "\n";
        }
        else {
            std::cerr << "Failed to save settings!\n";
        }
    }

    void loadSettings(const std::string& filename) {
        std::ifstream file(filename);
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                size_t pos = line.find('=');
                if (pos != std::string::npos) {
                    std::string key = line.substr(0, pos);
                    std::string value = line.substr(pos + 1);

                    if (key == "dpi") dpi = std::stof(value);
                    else if (key == "sens") sens = std::stof(value);
                    else if (key == "scaleX") scaleX = std::stof(value);
                    else if (key == "scaleY") scaleY = std::stof(value);
                    else if (key == "weapon") setWeapon(value);
                }
            }
            file.close();
            configure(dpi, sens, scaleX, scaleY);
            std::cout << "Settings loaded from " << filename << "\n";
        }
    }

    void printSettings() const {
        std::cout << "\nCurrent Configuration:\n";
        std::cout << "  DPI: " << dpi << "\n";
        std::cout << "  Sensitivity: " << sens << "\n";
        std::cout << "  X Scale: " << scaleX << "\n";
        std::cout << "  Y Scale: " << scaleY << "\n";
        std::cout << "  Weapon: " << currentWeapon << "\n";
    }

    void printAvailableWeapons() const {
        std::cout << "\nAvailable Weapons:\n";
        for (const auto& weapon : WEAPON_PATTERNS) {
            std::cout << "  " << weapon.first << "\n";
        }
    }

    void setScaleFactors(float x, float y) {
        scaleX = x;
        scaleY = y;
        saveSettings("recoil_settings.txt");
        std::cout << "X and Y scale factors are set to: X=" << x << " Y=" << y << "\n";
    }

    void setDPI(float newDPI) {
        dpi = newDPI;
        saveSettings("recoil_settings.txt");
        std::cout << "DPI set to: " << newDPI << "\n";
    }

    void setSensitivity(float newSens) {
        sens = newSens;
        configure(dpi, sens, scaleX, scaleY);
        saveSettings("recoil_settings.txt");
        std::cout << "Sensitivity set to: " << newSens << "\n";
    }

private:
    void controlLoop() {
        try {
            int delay = 99; // Default for AK-47
            if (currentWeapon == "M4A1-S") delay = 88;
            else if (currentWeapon == "M4A4") delay = 87;
            else if (currentWeapon == "FAMAS") delay = 88;
            else if (currentWeapon == "GALIL AR") delay = 90;
            else if (currentWeapon == "UMP-45") delay = 90;
            else if (currentWeapon == "AUG") delay = 88;
            else if (currentWeapon == "SG 553") delay = 89;

            while (isActive && currentBullet < currentPattern->size()) {
                applyRecoilCompensation();
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                currentBullet++;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error in control loop: " << e.what() << "\n";
        }
        isActive = false;
    }

    void applyRecoilCompensation() {
        const auto& pattern = (*currentPattern)[currentBullet];
        const int compX = static_cast<int>(pattern.first * conversionFactor * scaleX);
        const int compY = static_cast<int>(pattern.second * conversionFactor * scaleY);
        device.mouseMove(compX, compY);
    }

    makcu::Device& device;
    std::thread controlThread;
    std::atomic<bool> isActive;
    std::atomic<int> currentBullet;
    float conversionFactor = 1.0f;
    float dpi = 800.0f;
    float sens = 3.5f;
    float scaleX = 1.1f;
    float scaleY = 1.05f;
    const std::vector<std::pair<int, int>>* currentPattern = &WEAPON_PATTERNS["AK-47"];
    std::string currentWeapon = "AK-47";
};

bool fileExists(const std::string& filename) {
    std::ifstream file(filename);
    return file.good();
}

void createConfigFile(const std::string& filename) {
    std::cout << "No config file found. Let's create one.\n";

    float dpi, sens, scaleX, scaleY;
    std::string weapon;

    std::cout << "Enter your mouse DPI (Like 800): ";
    std::cin >> dpi;
    std::cout << "Enter your in-game sensitivity (Like 2.5): ";
    std::cin >> sens;
    std::cout << "Enter X scale factor (default is 1.0): ";
    std::cin >> scaleX;
    std::cout << "Enter Y scale factor (default is 1.0): ";
    std::cin >> scaleY;
    std::cout << "Enter default weapon (Like AK-47): ";
    std::cin.ignore(); // Clear newline from previous input
    std::getline(std::cin, weapon);

    std::ofstream file(filename);
    if (file.is_open()) {
        file << "dpi=" << dpi << "\n";
        file << "sens=" << sens << "\n";
        file << "scaleX=" << scaleX << "\n";
        file << "scaleY=" << scaleY << "\n";
        file << "weapon=" << weapon << "\n";
        file.close();
        std::cout << "Configuration saved to " << filename << "\n";
    }
    else {
        std::cerr << "Failed to create config file! Using defaults.\n";
    }
}

void consoleInput(RecoilControl& recoilControl) {
    std::string input;
    while (true) {
        std::cout << "\nEnter command: ";
        std::getline(std::cin, input);

        if (input.empty()) continue;

        std::string upperInput = input;
        std::transform(upperInput.begin(), upperInput.end(), upperInput.begin(), ::toupper);

        if (upperInput == "EXIT") {
            std::cout << "Exiting...\n";
            exit(0);
        }
        else if (upperInput == "SETTINGS") {
            recoilControl.printSettings();
        }
        else if (upperInput == "WEAPONS") {
            recoilControl.printAvailableWeapons();
        }
        else if (upperInput.find("SCALE") == 0) {
            try {
                size_t pos1 = input.find(' ');
                size_t pos2 = input.find(' ', pos1 + 1);
                if (pos1 != std::string::npos && pos2 != std::string::npos) {
                    float scaleX = std::stof(input.substr(pos1 + 1, pos2 - pos1 - 1));
                    float scaleY = std::stof(input.substr(pos2 + 1));
                    recoilControl.setScaleFactors(scaleX, scaleY);
                }
                else {
                    std::cout << "Usage: scale (X value) (Y value) (Like scale 1.2 1.3)\n";
                }
            }
            catch (...) {
                std::cout << "Invalid scale factors! Use numbers (Like scale 1.2 1.3)\n";
            }
        }
        else if (upperInput.find("DPI") == 0) {
            try {
                size_t pos = input.find(' ');
                if (pos != std::string::npos) {
                    float newDPI = std::stof(input.substr(pos + 1));
                    recoilControl.setDPI(newDPI);
                }
                else {
                    std::cout << "Usage: dpi 800 (or your DPI value)\n";
                }
            }
            catch (...) {
                std::cout << "Invalid DPI value! Use a number (Like dpi 800)\n";
            }
        }
        else if (upperInput.find("SENS") == 0) {
            try {
                size_t pos = input.find(' ');
                if (pos != std::string::npos) {
                    float newSens = std::stof(input.substr(pos + 1));
                    recoilControl.setSensitivity(newSens);
                }
                else {
                    std::cout << "Usage: sens 2.5 (or your sensitivity value)\n";
                }
            }
            catch (...) {
                std::cout << "Invalid sensitivity value! Use a number (Like sens 2.5)\n";
            }
        }
        else {
            recoilControl.setWeapon(input);
            recoilControl.saveSettings("recoil_settings.txt");
        }
    }
}
//Connect to makcu
int main() {
    makcu::Device device;

    std::cout << "Connecting to MAKCU device...\n";
    if (!device.connect()) {
        std::cerr << "Failed to connect to MAKCU device!\n";
        std::cerr << "Please ensure device is connected and drivers are installed.\n";
        return 1;
    }

    device.lockMouseX(false);
    device.lockMouseY(false);
    device.lockMouseLeft(false);
    device.lockMouseRight(false);
    device.lockMouseMiddle(false);

    std::cout << "Connected successfully!\n\n";

    RecoilControl recoilControl(device);
    const std::string configFile = "recoil_settings.txt";

    // Check for config file and create if needed
    if (!fileExists(configFile)) {
        createConfigFile(configFile);
    }

    // Load settings
    recoilControl.loadSettings(configFile);

    // Print current settings
    recoilControl.printSettings();
    recoilControl.printAvailableWeapons();

    // Start console input thread
    std::thread consoleThread(consoleInput, std::ref(recoilControl));
    consoleThread.detach();

    // Control setup
    device.setMouseButtonCallback([&](makcu::MouseButton button, bool pressed) {
        if (button == makcu::MouseButton::LEFT) {
            if (pressed) {
                recoilControl.start();
            }
            else {
                recoilControl.stop();
            }
        }
        });

    device.enableHighPerformanceMode(true);

    std::cout << "\nRecoil is ready :)\n";
    std::cout << "Hold left click to activate recoil compensation\n";
    std::cout << "Available commands:\n";
    std::cout << "  [weapon name] - Change weapon (Like AK-47)\n";
    std::cout << "  scale X Y - Set scale factors (Like scale 1.2 1.3)\n";
    std::cout << "  dpi [value] - Set DPI (Like. dpi 800)\n";
    std::cout << "  sens [value] - Set sensitivity (Like sens 2.5)\n";
    std::cout << "  settings - View current configuration\n";
    std::cout << "  weapons - List available weapons\n";
    std::cout << "  exit - Exit the program\n\n";

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
