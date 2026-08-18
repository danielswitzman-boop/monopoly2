#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>
#include <limits>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <cstdint>
#include <cerrno>
#include <cstring>

// ============================================================
// MONOPOLY CREDIT CARD MACHINE - RASPBERRY PI GPIO VERSION
// ============================================================
// Physical GPIO button mapping (BCM GPIO numbers):
//   Physical pin 11 = GPIO17 = UP
//   Physical pin 13 = GPIO27 = DOWN
//   Physical pin 15 = GPIO22 = SELECT
//   Physical pin 16 = GPIO23 = BACK
//   Physical pin 18 = GPIO24 = ON/OFF
//
// Every button should connect its GPIO pin to GND when pressed.
// The program uses internal pull-ups, so:
//   not pressed = HIGH
//   pressed     = LOW
//
// Build:
//   g++ -std=c++11 -Wall -Wextra -o MonopolyCreditCard_GPIO MonopolyCreditCard_GPIO.cpp -llgpio
//
// Run:
//   sudo ./MonopolyCreditCard_GPIO
// ============================================================

// ---------------- GPIO SETTINGS ----------------
const int PIN_UP     = 17; // physical 11
const int PIN_DOWN   = 27; // physical 13
const int PIN_SELECT = 22; // physical 15
const int PIN_BACK   = 23; // physical 16
const int PIN_POWER  = 24; // physical 18

const int DEBOUNCE_MS = 80;
const int POLL_MS = 10;

class GPIOButtons {
private:
    static const off_t GPIO_MAP_SIZE = 4096;
    static const off_t GPFSEL0   = 0x00;
    static const off_t GPLEV0    = 0x34;
    static const off_t GPPUD     = 0x94;
    static const off_t GPPUDCLK0 = 0x98;

    // /dev/gpiomem maps the GPIO peripheral directly, so this version
    // does not need lgpio, wiringPi, or another GPIO library.
    int memFd;
    volatile uint32_t* gpio;
    bool initialized;

    int pins[5];
    int lastState[5];

    void shortDelay() {
        // Raspberry Pi GPIO pull-up setup needs a short timing delay.
        usleep(200);
    }

    void writeReg(off_t offset, uint32_t value) {
        gpio[offset / sizeof(uint32_t)] = value;
        __sync_synchronize();
    }

    uint32_t readReg(off_t offset) {
        __sync_synchronize();
        return gpio[offset / sizeof(uint32_t)];
    }

    void setInput(int gpioNumber) {
        int functionRegister = gpioNumber / 10;
        int bit = (gpioNumber % 10) * 3;
        off_t offset = GPFSEL0 + (functionRegister * 4);

        uint32_t value = readReg(offset);
        value &= ~(7u << bit); // 000 = input
        writeReg(offset, value);
    }

    void setPullUp(int gpioNumber) {
        uint32_t bit = 1u << gpioNumber;

        // BCM2835 pull-up sequence.
        writeReg(GPPUD, 2); // 2 = pull-up
        shortDelay();
        writeReg(GPPUDCLK0, bit);
        shortDelay();
        writeReg(GPPUD, 0);
        writeReg(GPPUDCLK0, 0);
    }

    int readPin(int gpioNumber) {
        uint32_t value = readReg(GPLEV0);
        return (value & (1u << gpioNumber)) ? 1 : 0;
    }

    void waitForRelease(int gpioNumber) {
        while (readPin(gpioNumber) == 0) {
            usleep(POLL_MS * 1000);
        }
        usleep(DEBOUNCE_MS * 1000);
    }

public:
    GPIOButtons()
        : memFd(-1), gpio(nullptr), initialized(false) {
        pins[0] = PIN_UP;
        pins[1] = PIN_DOWN;
        pins[2] = PIN_SELECT;
        pins[3] = PIN_BACK;
        pins[4] = PIN_POWER;

        for (int i = 0; i < 5; ++i) {
            lastState[i] = 1;
        }
    }

    bool setup() {
        memFd = open("/dev/gpiomem", O_RDWR | O_SYNC);
        if (memFd < 0) {
            std::cerr << "ERROR: Could not open /dev/gpiomem.\n";
            std::cerr << "Try running the program with: sudo ./MonopolyCreditCard_GPIO\n";
            std::cerr << "System error: " << std::strerror(errno) << "\n";
            return false;
        }

        void* mapped = mmap(nullptr, GPIO_MAP_SIZE,
                            PROT_READ | PROT_WRITE, MAP_SHARED,
                            memFd, 0);

        if (mapped == MAP_FAILED) {
            std::cerr << "ERROR: Could not map GPIO memory.\n";
            std::cerr << "System error: " << std::strerror(errno) << "\n";
            close(memFd);
            memFd = -1;
            return false;
        }

        gpio = static_cast<volatile uint32_t*>(mapped);

        for (int i = 0; i < 5; ++i) {
            setInput(pins[i]);
            setPullUp(pins[i]);
            lastState[i] = readPin(pins[i]);
        }

        initialized = true;
        return true;
    }

    void cleanup() {
        if (gpio != nullptr) {
            // Leave all five GPIOs as inputs and turn the pulls off before exit.
            for (int i = 0; i < 5; ++i) {
                setInput(pins[i]);
            }

            munmap(const_cast<uint32_t*>(gpio), GPIO_MAP_SIZE);
            gpio = nullptr;
        }

        if (memFd >= 0) {
            close(memFd);
            memFd = -1;
        }

        initialized = false;
    }

    ~GPIOButtons() {
        cleanup();
    }

    // Returns:
    //   0 = none
    //   1 = up
    //   2 = down
    //   3 = select
    //   4 = back
    //   5 = power/off
    int waitForButton() {
        if (!initialized) return 5;

        while (true) {
            for (int i = 0; i < 5; ++i) {
                int state = readPin(pins[i]);

                // HIGH -> LOW means a new button press.
                if (lastState[i] == 1 && state == 0) {
                    usleep(DEBOUNCE_MS * 1000);
                    int confirmed = readPin(pins[i]);

                    if (confirmed == 0) {
                        waitForRelease(pins[i]);
                        lastState[i] = 1;
                        return i + 1;
                    }
                }

                lastState[i] = state;
            }

            usleep(POLL_MS * 1000);
        }
    }
};

// Global hardware controller so every menu/function can access the buttons.
GPIOButtons gpio;

// ---------------- HELPER FUNCTIONS ----------------
std::string intToStr(int num) {
    std::stringstream ss;
    ss << num;
    return ss.str();
}

std::vector<int> makeRent(int r1, int r2, int r3, int r4, int r5, int r6) {
    std::vector<int> v;
    v.push_back(r1); v.push_back(r2); v.push_back(r3);
    v.push_back(r4); v.push_back(r5); v.push_back(r6);
    return v;
}

std::vector<int> makeRR(int r1, int r2, int r3, int r4) {
    std::vector<int> v;
    v.push_back(r1); v.push_back(r2); v.push_back(r3); v.push_back(r4);
    return v;
}

std::vector<int> makeUT(int r1, int r2) {
    std::vector<int> v;
    v.push_back(r1); v.push_back(r2);
    return v;
}

// ---------------- HARDWARE UI ----------------
class HardwareMenuUI {
private:
    int cursorIndex;
    int topIndex;
    static const int MAX_VISIBLE = 8;

public:
    HardwareMenuUI() : cursorIndex(0), topIndex(0) {}

    void drawScreen(const std::vector<std::string>& menuItems) {
        int totalItems = static_cast<int>(menuItems.size());

        std::cout << "\n==============================\n";
        if (topIndex > 0) std::cout << "    ^ MORE ABOVE\n";
        else std::cout << "\n";

        int limit = std::min(topIndex + MAX_VISIBLE, totalItems);
        for (int i = topIndex; i < limit; ++i) {
            if (i == cursorIndex) std::cout << " -> " << menuItems[i] << "\n";
            else std::cout << "    " << menuItems[i] << "\n";
        }

        if (topIndex + MAX_VISIBLE < totalItems) std::cout << "    v MORE BELOW\n";
        else std::cout << "\n";
    }

    void onButtonDown(int totalItems) {
        if (cursorIndex < totalItems - 1) {
            ++cursorIndex;
            if (cursorIndex >= topIndex + MAX_VISIBLE) ++topIndex;
        }
    }

    void onButtonUp() {
        if (cursorIndex > 0) {
            --cursorIndex;
            if (cursorIndex < topIndex) --topIndex;
        }
    }

    int onButtonEnter() const { return cursorIndex; }
};

// Returns -1 for Back.
// Returns -2 for On/Off.
int runHardwareMenu(const std::string& title,
                    const std::vector<std::string>& menuItems) {
    if (menuItems.empty()) {
        std::cout << "\n==============================\n";
        std::cout << "--- " << title << " ---\n";
        std::cout << "No options available.\n";
        std::cout << "Press BACK to return.\n";

        while (true) {
            int button = gpio.waitForButton();
            if (button == 4) return -1;
            if (button == 5) return -2;
        }
    }

    HardwareMenuUI ui;

    while (true) {
        std::cout << "\n==============================\n";
        std::cout << title << "\n";
        ui.drawScreen(menuItems);
        std::cout << "UP=11  DOWN=13  SELECT=15  BACK=16  ON/OFF=18\n";

        int button = gpio.waitForButton();

        if (button == 1) {
            ui.onButtonUp();
        } else if (button == 2) {
            ui.onButtonDown(static_cast<int>(menuItems.size()));
        } else if (button == 3) {
            return ui.onButtonEnter();
        } else if (button == 4) {
            return -1;
        } else if (button == 5) {
            return -2;
        }
    }
}

// ---------------- GAME CLASSES ----------------
class Property {
public:
    std::string name;
    int propertyIndex;
    int owner;
    int colourSet;
    int houses;
    int rent;
    int houseCost;
    int price;
    bool mortgaged;
    std::vector<int> houseRent;

    Property(int propertyIndex, const std::string& name, int price,
             const std::vector<int>& houseRent)
        : name(name), propertyIndex(propertyIndex), owner(1000),
          colourSet(0), houses(0), rent(houseRent[0]), houseCost(0),
          price(price), mortgaged(false), houseRent(houseRent) {

        if (propertyIndex >= 0 && propertyIndex < 2) {
            colourSet = 1; houseCost = 50;
        } else if (propertyIndex >= 2 && propertyIndex < 5) {
            colourSet = 2; houseCost = 50;
        } else if (propertyIndex >= 5 && propertyIndex < 8) {
            colourSet = 3; houseCost = 100;
        } else if (propertyIndex >= 8 && propertyIndex < 11) {
            colourSet = 4; houseCost = 100;
        } else if (propertyIndex >= 11 && propertyIndex < 14) {
            colourSet = 5; houseCost = 150;
        } else if (propertyIndex >= 14 && propertyIndex < 17) {
            colourSet = 6; houseCost = 150;
        } else if (propertyIndex >= 17 && propertyIndex < 20) {
            colourSet = 7; houseCost = 200;
        } else if (propertyIndex >= 20 && propertyIndex < 22) {
            colourSet = 8; houseCost = 200;
        } else if (propertyIndex >= 22 && propertyIndex < 26) {
            colourSet = 9; houseCost = 0;
        } else if (propertyIndex >= 26 && propertyIndex < 28) {
            colourSet = 10; houseCost = 0;
        }
    }
};

class Player {
public:
    int playerIndex;
    int money;
    bool bankrupt;
    std::string name;

    Player(int x, int y, bool z, const std::string& n)
        : playerIndex(x), money(y), bankrupt(z), name(n) {}
};

// ---------------- DATA FILTER FUNCTIONS ----------------
std::vector<int> getUnownedProperties(const std::vector<Property>& properties) {
    std::vector<int> p;
    for (size_t i = 0; i < properties.size(); ++i) {
        if (properties[i].owner == 1000) p.push_back(static_cast<int>(i));
    }
    return p;
}

std::vector<int> getOwnedProperties(const std::vector<Property>& properties,
                                    int playerNum, bool includeMortgage) {
    std::vector<int> p;
    for (size_t i = 0; i < properties.size(); ++i) {
        if (properties[i].owner == playerNum) {
            if (includeMortgage || !properties[i].mortgaged) {
                p.push_back(static_cast<int>(i));
            }
        }
    }
    return p;
}

std::vector<int> getBuildableProperties(const std::vector<Property>& properties,
                                        int playerNum) {
    std::vector<int> p;
    for (size_t i = 0; i < properties.size(); ++i) {
        if (properties[i].owner == playerNum &&
            !properties[i].mortgaged &&
            properties[i].colourSet != 9 &&
            properties[i].colourSet != 10 &&
            properties[i].houses < 5) {
            p.push_back(static_cast<int>(i));
        }
    }
    return p;
}

std::vector<int> getAllOwnedProperties(const std::vector<Property>& properties) {
    std::vector<int> p;
    for (size_t i = 0; i < properties.size(); ++i) {
        if (properties[i].owner != 1000) p.push_back(static_cast<int>(i));
    }
    return p;
}

std::vector<int> getMortgagedProperties(const std::vector<Property>& properties,
                                        int playerNum) {
    std::vector<int> p;
    for (size_t i = 0; i < properties.size(); ++i) {
        if (properties[i].owner == playerNum && properties[i].mortgaged) {
            p.push_back(static_cast<int>(i));
        }
    }
    return p;
}

// ---------------- FORWARD DECLARATIONS ----------------
std::vector<Property> initializeProperties();
void buyProperty(std::vector<Property>& properties, int propertyNum,
                 std::vector<Player>& players, int playerNum);
void buyHouse(std::vector<Property>& properties, int propertyNum, int houses,
              std::vector<Player>& players, int playerNum);
void payRent(std::vector<Property>& properties, std::vector<Player>& players,
             int playerNum, int propertyNum);
void mortgage(Property& property, Player& player);
void unmortgage(Property& property, Player& player);
void trade(std::vector<Property>& properties, std::vector<Player>& players,
           int player1, int player2);
void railRoadRentChecker(std::vector<Property>& properties);
void utilitiesRentChecker(std::vector<Property>& properties);

// Returns true when the user presses ON/OFF anywhere in the program.
bool shouldExit(int result) {
    return result == -2;
}

// ---------------- MAIN ----------------
int main() {
    std::cout << "\nStarting Monopoly Credit Card Machine...\n";

    if (!gpio.setup()) {
        std::cerr << "\nGPIO setup failed. Program will now close.\n";
        return 1;
    }

    std::vector<Property> properties = initializeProperties();
    std::vector<Player> players;

    // The program uses a built-in name bank, so no keyboard is required.
    std::vector<std::string> nameBank;
    nameBank.push_back("Alex");
    nameBank.push_back("Ben");
    nameBank.push_back("Charlie");
    nameBank.push_back("Daniel");
    nameBank.push_back("Ethan");
    nameBank.push_back("Jack");
    nameBank.push_back("Liam");
    nameBank.push_back("Max");
    nameBank.push_back("Noah");
    nameBank.push_back("Owen");
    nameBank.push_back("Sam");
    nameBank.push_back("Will");

    std::vector<std::string> playerQuantOpts;
    playerQuantOpts.push_back("2 Players");
    playerQuantOpts.push_back("3 Players");
    playerQuantOpts.push_back("4 Players");
    playerQuantOpts.push_back("5 Players");
    playerQuantOpts.push_back("6 Players");
    playerQuantOpts.push_back("7 Players");
    playerQuantOpts.push_back("8 Players");

    int playersChoice = runHardwareMenu("How many players?", playerQuantOpts);
    if (playersChoice < 0) return 0;
    int playersQuant = playersChoice + 2;

    // Pick names using buttons.
    std::vector<bool> nameUsed(nameBank.size(), false);
    for (int i = 0; i < playersQuant; ++i) {
        std::vector<std::string> availableNames;
        std::vector<int> nameIds;

        for (size_t n = 0; n < nameBank.size(); ++n) {
            if (!nameUsed[n]) {
                availableNames.push_back(nameBank[n]);
                nameIds.push_back(static_cast<int>(n));
            }
        }

        int nameChoice = runHardwareMenu(
            "Choose name for Player " + intToStr(i + 1), availableNames);
        if (nameChoice < 0) return 0;

        int selectedName = nameIds[nameChoice];
        nameUsed[selectedName] = true;
        players.push_back(Player(i, 1500, false, nameBank[selectedName]));
    }

    std::vector<std::string> pOpts;
    for (int i = 0; i < playersQuant; ++i) {
        pOpts.push_back(players[i].name);
    }

    std::vector<std::string> mainOpts;
    mainOpts.push_back("Buy a property");
    mainOpts.push_back("Buy a house/hotel");
    mainOpts.push_back("Pay rent");
    mainOpts.push_back("Mortgage");
    mainOpts.push_back("Unmortgage property");
    mainOpts.push_back("Complete a trade");
    mainOpts.push_back("Add money");
    mainOpts.push_back("Subtract money");
    mainOpts.push_back("See assets");

    while (true) {
        int actioningPlayer = runHardwareMenu("Which player are you?", pOpts);
        if (shouldExit(actioningPlayer)) return 0;
        if (actioningPlayer < 0 || actioningPlayer >= playersQuant) continue;

        int playerDecision = runHardwareMenu(
            players[actioningPlayer].name + "'s Action:", mainOpts);
        if (shouldExit(playerDecision)) return 0;
        if (playerDecision < 0) continue;
        playerDecision++;

        if (playerDecision == 1) {
            std::vector<int> validIdxs = getUnownedProperties(properties);
            if (validIdxs.empty()) {
                std::vector<std::string> none;
                none.push_back("No unowned properties");
                int r = runHardwareMenu("Buy a property", none);
                if (shouldExit(r)) return 0;
                continue;
            }

            std::vector<std::string> uiOpts;
            for (size_t k = 0; k < validIdxs.size(); ++k) {
                int idx = validIdxs[k];
                uiOpts.push_back(properties[idx].name + " ($" +
                                 intToStr(properties[idx].price) + ")");
            }

            int sel = runHardwareMenu("Which property to purchase?", uiOpts);
            if (shouldExit(sel)) return 0;
            if (sel >= 0 && sel < static_cast<int>(validIdxs.size())) {
                buyProperty(properties, validIdxs[sel], players, actioningPlayer);
            }

        } else if (playerDecision == 2) {
            std::vector<int> validIdxs = getBuildableProperties(properties, actioningPlayer);
            if (validIdxs.empty()) {
                std::vector<std::string> none;
                none.push_back("No buildable properties");
                int r = runHardwareMenu("Build", none);
                if (shouldExit(r)) return 0;
                continue;
            }

            std::vector<std::string> uiOpts;
            for (size_t k = 0; k < validIdxs.size(); ++k) {
                int idx = validIdxs[k];
                uiOpts.push_back(properties[idx].name + " ($" +
                                 intToStr(properties[idx].houseCost) + ")");
            }

            int sel = runHardwareMenu("Which property to build on?", uiOpts);
            if (shouldExit(sel)) return 0;
            if (sel >= 0 && sel < static_cast<int>(validIdxs.size())) {
                buyHouse(properties, validIdxs[sel], 1, players, actioningPlayer);
            }

        } else if (playerDecision == 3) {
            // FIX: Never offer the current player's own properties as a rent target.
            std::vector<int> validIdxs;
            for (size_t i = 0; i < properties.size(); ++i) {
                if (properties[i].owner != 1000 &&
                    properties[i].owner != actioningPlayer) {
                    validIdxs.push_back(static_cast<int>(i));
                }
            }

            if (validIdxs.empty()) {
                std::vector<std::string> none;
                none.push_back("No rent properties available");
                int r = runHardwareMenu("Pay rent", none);
                if (shouldExit(r)) return 0;
                continue;
            }

            std::vector<std::string> uiOpts;
            for (size_t k = 0; k < validIdxs.size(); ++k) {
                int idx = validIdxs[k];
                uiOpts.push_back(properties[idx].name +
                                 " - Owner: " + players[properties[idx].owner].name);
            }

            int sel = runHardwareMenu("Which property did you land on?", uiOpts);
            if (shouldExit(sel)) return 0;
            if (sel >= 0 && sel < static_cast<int>(validIdxs.size())) {
                payRent(properties, players, actioningPlayer, validIdxs[sel]);
            }

        } else if (playerDecision == 4) {
            std::vector<int> validIdxs = getOwnedProperties(properties, actioningPlayer, false);
            if (validIdxs.empty()) {
                std::vector<std::string> none;
                none.push_back("No properties to mortgage");
                int r = runHardwareMenu("Mortgage", none);
                if (shouldExit(r)) return 0;
                continue;
            }

            std::vector<std::string> uiOpts;
            for (size_t k = 0; k < validIdxs.size(); ++k) {
                int idx = validIdxs[k];
                uiOpts.push_back(properties[idx].name);
            }

            int sel = runHardwareMenu("Which property to mortgage?", uiOpts);
            if (shouldExit(sel)) return 0;
            if (sel >= 0 && sel < static_cast<int>(validIdxs.size())) {
                mortgage(properties[validIdxs[sel]], players[actioningPlayer]);
                railRoadRentChecker(properties);
            }

        } else if (playerDecision == 5) {
            std::vector<int> validIdxs = getMortgagedProperties(properties, actioningPlayer);
            if (validIdxs.empty()) {
                std::vector<std::string> none;
                none.push_back("No mortgaged properties");
                int r = runHardwareMenu("Unmortgage", none);
                if (shouldExit(r)) return 0;
                continue;
            }

            std::vector<std::string> uiOpts;
            for (size_t k = 0; k < validIdxs.size(); ++k) {
                int idx = validIdxs[k];
                uiOpts.push_back(properties[idx].name);
            }

            int sel = runHardwareMenu("Which property to unmortgage?", uiOpts);
            if (shouldExit(sel)) return 0;
            if (sel >= 0 && sel < static_cast<int>(validIdxs.size())) {
                unmortgage(properties[validIdxs[sel]], players[actioningPlayer]);
                railRoadRentChecker(properties);
            }

        } else if (playerDecision == 6) {
            std::vector<std::string> tOpts;
            std::vector<int> playerIds;

            for (int i = 0; i < static_cast<int>(players.size()); ++i) {
                if (i != actioningPlayer) {
                    tOpts.push_back(players[i].name);
                    playerIds.push_back(i);
                }
            }

            int sel = runHardwareMenu("Trade with who?", tOpts);
            if (shouldExit(sel)) return 0;
            if (sel >= 0 && sel < static_cast<int>(playerIds.size())) {
                trade(properties, players, actioningPlayer, playerIds[sel]);
            }

        } else if (playerDecision == 7) {
            std::vector<std::string> moneyOpts;
            moneyOpts.push_back("$1");
            moneyOpts.push_back("$5");
            moneyOpts.push_back("$20");
            moneyOpts.push_back("$50");
            moneyOpts.push_back("$100");

            int sel = runHardwareMenu("Add how much money?", moneyOpts);
            if (shouldExit(sel)) return 0;
            if (sel >= 0 && sel < 5) {
                const int values[5] = {1, 5, 20, 50, 100};
                players[actioningPlayer].money += values[sel];
                std::cout << "Added $" << values[sel]
                          << " | Balance: $" << players[actioningPlayer].money << "\n";
            }

        } else if (playerDecision == 8) {
            std::vector<std::string> moneyOpts;
            moneyOpts.push_back("$1");
            moneyOpts.push_back("$5");
            moneyOpts.push_back("$20");
            moneyOpts.push_back("$50");
            moneyOpts.push_back("$100");

            int sel = runHardwareMenu("Subtract how much money?", moneyOpts);
            if (shouldExit(sel)) return 0;
            if (sel >= 0 && sel < 5) {
                const int values[5] = {1, 5, 20, 50, 100};
                int moneyChange = values[sel];

                if (players[actioningPlayer].money >= moneyChange) {
                    players[actioningPlayer].money -= moneyChange;
                    std::cout << "Subtracted $" << moneyChange
                              << " | Balance: $" << players[actioningPlayer].money << "\n";
                } else {
                    std::cout << "Error: not enough money.\n";
                }
            }

        } else if (playerDecision == 9) {
            std::vector<std::string> assetsList;
            assetsList.push_back("CASH: $" + intToStr(players[actioningPlayer].money));

            for (size_t i = 0; i < properties.size(); ++i) {
                if (properties[i].owner == actioningPlayer) {
                    std::string entry = properties[i].name;
                    if (properties[i].mortgaged) {
                        entry += " (MORTGAGED)";
                    } else if (properties[i].houses != 0) {
                        entry += " (Houses: " + intToStr(properties[i].houses) + ")";
                    }
                    assetsList.push_back(entry);
                }
            }

            int r = runHardwareMenu("Assets - " + players[actioningPlayer].name,
                                    assetsList);
            if (shouldExit(r)) return 0;
        }
    }

    return 0;
}

// ---------------- INITIALIZATION ----------------
std::vector<Property> initializeProperties() {
    std::vector<Property> p;
    p.push_back(Property(0, "Mediterranean Avenue", 60, makeRent(2, 10, 30, 90, 160, 250)));
    p.push_back(Property(1, "Baltic Avenue", 60, makeRent(4, 20, 60, 180, 320, 450)));
    p.push_back(Property(2, "Oriental Avenue", 100, makeRent(6, 30, 90, 270, 400, 550)));
    p.push_back(Property(3, "Vermont Avenue", 100, makeRent(6, 30, 90, 270, 400, 550)));
    p.push_back(Property(4, "Connecticut Avenue", 120, makeRent(8, 40, 100, 300, 450, 600)));
    p.push_back(Property(5, "St. Charles Place", 140, makeRent(10, 50, 150, 450, 625, 750)));
    p.push_back(Property(6, "States Avenue", 140, makeRent(10, 50, 150, 450, 625, 750)));
    p.push_back(Property(7, "Virginia Avenue", 160, makeRent(12, 60, 180, 500, 700, 900)));
    p.push_back(Property(8, "St. James Place", 180, makeRent(14, 70, 200, 550, 750, 950)));
    p.push_back(Property(9, "Tennessee Avenue", 180, makeRent(14, 70, 200, 550, 750, 950)));
    p.push_back(Property(10, "New York Avenue", 200, makeRent(16, 80, 220, 600, 800, 1000)));
    p.push_back(Property(11, "Kentucky Avenue", 220, makeRent(18, 90, 250, 700, 875, 1050)));
    p.push_back(Property(12, "Indiana Avenue", 220, makeRent(18, 90, 250, 700, 875, 1050)));
    p.push_back(Property(13, "Illinois Avenue", 240, makeRent(20, 100, 300, 750, 925, 1100)));
    p.push_back(Property(14, "Atlantic Avenue", 260, makeRent(22, 110, 330, 800, 975, 1150)));
    p.push_back(Property(15, "Ventnor Avenue", 260, makeRent(22, 110, 330, 800, 975, 1150)));
    p.push_back(Property(16, "Marvin Gardens", 280, makeRent(24, 120, 360, 850, 1025, 1200)));
    p.push_back(Property(17, "Pacific Avenue", 300, makeRent(26, 130, 390, 900, 1100, 1275)));
    p.push_back(Property(18, "North Carolina Avenue", 300, makeRent(26, 130, 390, 900, 1100, 1275)));
    p.push_back(Property(19, "Pennsylvania Avenue", 320, makeRent(28, 150, 450, 1000, 1200, 1400)));
    p.push_back(Property(20, "Park Place", 350, makeRent(35, 175, 500, 1100, 1300, 1500)));
    p.push_back(Property(21, "Boardwalk", 400, makeRent(50, 200, 600, 1400, 1700, 2000)));
    p.push_back(Property(22, "Reading Railroad", 200, makeRR(25, 50, 100, 200)));
    p.push_back(Property(23, "Pennsylvania Railroad", 200, makeRR(25, 50, 100, 200)));
    p.push_back(Property(24, "B. & O. Railroad", 200, makeRR(25, 50, 100, 200)));
    p.push_back(Property(25, "Short Line", 200, makeRR(25, 50, 100, 200)));
    p.push_back(Property(26, "Electric Company", 150, makeUT(4, 10)));
    p.push_back(Property(27, "Water Works", 150, makeUT(4, 10)));
    return p;
}

// ---------------- LOGIC FUNCTIONS ----------------
void buyHouse(std::vector<Property>& properties, int propertyNum, int houses,
              std::vector<Player>& players, int playerNum) {
    if (propertyNum < 0 || propertyNum >= static_cast<int>(properties.size())) return;
    if (playerNum < 0 || playerNum >= static_cast<int>(players.size())) return;
    if (houses <= 0) return;

    Property& property = properties[propertyNum];
    Player& player = players[playerNum];

    if (property.colourSet == 9 || property.colourSet == 10) {
        std::cout << "Error: No houses on railroads/utilities.\n";
        return;
    }

    if (property.owner != playerNum) {
        std::cout << "Error: You do not own this property.\n";
        return;
    }

    if (property.mortgaged) {
        std::cout << "Error: You cannot build on a mortgaged property.\n";
        return;
    }

    if (property.houses >= 5) {
        std::cout << "Error: This property already has a hotel.\n";
        return;
    }

    for (int i = 0; i < houses; ++i) {
        if (player.money < property.houseCost) {
            std::cout << "Error: Not enough money.\n";
            return;
        }

        player.money -= property.houseCost;
        ++property.houses;
        property.rent = property.houseRent[property.houses];
    }

    std::cout << "Purchase successful!\n";
    std::cout << "Balance: $" << player.money << "\n";
}

void buyProperty(std::vector<Property>& properties, int propertyNum,
                 std::vector<Player>& players, int playerNum) {
    if (propertyNum < 0 || propertyNum >= static_cast<int>(properties.size())) return;
    if (playerNum < 0 || playerNum >= static_cast<int>(players.size())) return;

    Property& property = properties[propertyNum];
    Player& player = players[playerNum];

    if (property.owner != 1000) {
        std::cout << "Error: Property is already owned.\n";
        return;
    }

    if (player.money < property.price) {
        std::cout << "Error: Not enough money.\n";
        return;
    }

    player.money -= property.price;
    property.owner = playerNum;

    std::cout << "Purchase successful!\n";
    std::cout << "Balance: $" << player.money << "\n";

    railRoadRentChecker(properties);
}

void payRent(std::vector<Property>& properties, std::vector<Player>& players,
             int playerNum, int propertyNum) {
    if (propertyNum < 0 || propertyNum >= static_cast<int>(properties.size())) return;
    if (playerNum < 0 || playerNum >= static_cast<int>(players.size())) return;

    Property& property = properties[propertyNum];
    Player& player = players[playerNum];

    // Never allow self-rent or rent to an unowned property.
    if (property.owner == playerNum || property.owner == 1000) {
        std::cout << "No rent due.\n";
        return;
    }

    if (property.owner < 0 || property.owner >= static_cast<int>(players.size())) {
        std::cout << "Error: Invalid property owner.\n";
        return;
    }

    int rentToPay = property.rent;

    if (propertyNum == 26 || propertyNum == 27) {
        std::vector<std::string> diceOpts;
        for (int r = 2; r <= 12; ++r) diceOpts.push_back(intToStr(r));

        int rollIndex = runHardwareMenu("Select your dice roll (2-12)", diceOpts);
        if (shouldExit(rollIndex)) return;
        if (rollIndex < 0 || rollIndex >= static_cast<int>(diceOpts.size())) return;

        int roll = rollIndex + 2;
        rentToPay *= roll;
    }

    if (player.money < rentToPay) {
        std::cout << "Transaction failed. Not enough money.\n";
        return;
    }

    player.money -= rentToPay;
    players[property.owner].money += rentToPay;

    std::cout << "Paid Rent: $" << rentToPay << "\n";
}

void mortgage(Property& property, Player& player) {
    if (property.owner != player.playerIndex) {
        std::cout << "Error: You do not own this property.\n";
        return;
    }

    if (property.mortgaged) {
        std::cout << "Error: Property is already mortgaged.\n";
        return;
    }

    // Existing houses must be sold one at a time before mortgaging.
    if (property.houses != 0) {
        --property.houses;
        player.money += property.houseCost / 2;
        property.rent = property.houseRent[property.houses];
        std::cout << "Sold one house for $" << property.houseCost / 2 << ".\n";
        return;
    }

    property.mortgaged = true;
    player.money += property.price / 2;
    property.rent = 0;
    std::cout << "Property mortgaged for $" << property.price / 2 << ".\n";
}

void unmortgage(Property& property, Player& player) {
    if (property.owner != player.playerIndex || !property.mortgaged) {
        std::cout << "Error: This property is not mortgaged to this player.\n";
        return;
    }

    int cost = static_cast<int>((property.price / 2) * 1.1);
    if (player.money < cost) {
        std::cout << "Error: Not enough money to unmortgage.\n";
        return;
    }

    player.money -= cost;
    property.mortgaged = false;
    property.rent = property.houseRent[property.houses];

    std::cout << "Property unmortgaged for $" << cost << ".\n";
}

void trade(std::vector<Property>& properties, std::vector<Player>& players,
           int player1, int player2) {
    if (player1 < 0 || player2 < 0 ||
        player1 >= static_cast<int>(players.size()) ||
        player2 >= static_cast<int>(players.size()) ||
        player1 == player2) {
        return;
    }

    while (true) {
        std::vector<int> validIdxs = getOwnedProperties(properties, player2, true);
        std::vector<std::string> opts;
        opts.push_back("DONE SELECTING");

        for (size_t k = 0; k < validIdxs.size(); ++k) {
            opts.push_back(properties[validIdxs[k]].name);
        }

        int sel = runHardwareMenu(players[player1].name + " Receiving:", opts);
        if (shouldExit(sel)) return;
        if (sel <= 0) break;

        if (sel - 1 < static_cast<int>(validIdxs.size())) {
            properties[validIdxs[sel - 1]].owner = 1001;
        }
    }

    while (true) {
        std::vector<int> validIdxs = getOwnedProperties(properties, player1, true);
        std::vector<std::string> opts;
        opts.push_back("DONE SELECTING");

        for (size_t k = 0; k < validIdxs.size(); ++k) {
            opts.push_back(properties[validIdxs[k]].name);
        }

        int sel = runHardwareMenu(players[player2].name + " Receiving:", opts);
        if (shouldExit(sel)) return;
        if (sel <= 0) break;

        if (sel - 1 < static_cast<int>(validIdxs.size())) {
            properties[validIdxs[sel - 1]].owner = 1002;
        }
    }

    for (size_t i = 0; i < properties.size(); ++i) {
        if (properties[i].owner == 1001) properties[i].owner = player1;
        if (properties[i].owner == 1002) properties[i].owner = player2;
    }

    railRoadRentChecker(properties);
}

void railRoadRentChecker(std::vector<Property>& properties) {
    if (properties.size() < 28) return;

    int owners[4] = {
        properties[22].owner,
        properties[23].owner,
        properties[24].owner,
        properties[25].owner
    };

    for (int i = 0; i < 4; ++i) {
        int count = 0;
        for (int j = 0; j < 4; ++j) {
            if (owners[i] == owners[j] && owners[i] != 1000) ++count;
        }

        properties[22 + i].houses = count - 1;
        if (properties[22 + i].houses < 0) properties[22 + i].houses = 0;
        properties[22 + i].rent =
            properties[22 + i].houseRent[properties[22 + i].houses];
    }

    utilitiesRentChecker(properties);
}

void utilitiesRentChecker(std::vector<Property>& properties) {
    if (properties.size() < 28) return;

    if (properties[26].owner == properties[27].owner &&
        properties[26].owner != 1000) {
        properties[26].houses = 1;
        properties[27].houses = 1;
    } else {
        properties[26].houses = 0;
        properties[27].houses = 0;
    }

    properties[26].rent = properties[26].houseRent[properties[26].houses];
    properties[27].rent = properties[27].houseRent[properties[27].houses];
}
