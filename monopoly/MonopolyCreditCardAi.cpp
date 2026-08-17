#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <limits>
#include <sstream>

// --- HELPER FUNCTIONS FOR C++98 COMPATIBILITY ---
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

// --- HARDWARE UI SIMULATOR ---
class HardwareMenuUI {
private:
    int cursorIndex; 
    int topIndex;    
    static const int MAX_VISIBLE = 9; 

public:
    HardwareMenuUI() {
        cursorIndex = 0;
        topIndex = 0;
    }

    void drawScreen(const std::vector<std::string>& menuItems) {
        int totalItems = menuItems.size();
        if (topIndex > 0) { std::cout << "    [ /\\ ] \n"; } else { std::cout << "\n"; }
        int limit = std::min(topIndex + MAX_VISIBLE, totalItems);
        for (int i = topIndex; i < limit; i++) {
            if (i == cursorIndex) { std::cout << " -> " << menuItems[i] << "\n"; } 
            else { std::cout << "    " << menuItems[i] << "\n"; }
        }
        if (topIndex + MAX_VISIBLE < totalItems) { std::cout << "    [ \\/ ] \n"; } else { std::cout << "\n"; }
    }

    void onButtonDown(int totalItems) {
        if (cursorIndex < totalItems - 1) {
            cursorIndex++;
            if (cursorIndex >= topIndex + MAX_VISIBLE) topIndex++;
        }
    }

    void onButtonUp() {
        if (cursorIndex > 0) {
            cursorIndex--;
            if (cursorIndex < topIndex) topIndex--;
        }
    }

    int onButtonEnter() { return cursorIndex; }
};

int runHardwareMenu(std::string title, const std::vector<std::string>& menuItems) {
    if (menuItems.empty()) {
        std::cout << "\n==============================\n" << "--- " << title << " ---\n  (No items available)\n\nPress [b] + Enter to go Back: ";
        std::string inputStr; std::cin >> inputStr;
        return -1;
    }

    HardwareMenuUI ui;
    std::string inputStr;
    char input;
    while(true) {
        std::cout << "\n==============================\n" << title << "\n";
        ui.drawScreen(menuItems);
        std::cout << "Controls: [w]=Up  [s]=Down  [e]=Enter  [b]=Back \nSelect: ";
        std::cin >> inputStr;
        input = inputStr[0];
        if (input == 'w') ui.onButtonUp();
        else if (input == 's') ui.onButtonDown(menuItems.size());
        else if (input == 'e') return ui.onButtonEnter();
        else if (input == 'b') return -1;
    }
}

int getValidInput(int min, int max) {
    int choice;
    while (true) {
        std::cin >> choice;
        if (std::cin.fail() || choice < min || choice > max) {
            std::cin.clear(); std::cin.ignore(256, '\n');
            std::cout << "Invalid. Enter a number between " << min << " and " << max << ":\n";
        } else return choice;
    }
}

// --- GAME CLASSES ---
class Property {
public:
    std::string name;
    int propertyIndex, owner, colourSet, houses, rent, houseCost, price;
    bool mortgaged;
    std::vector<int> houseRent;
    
    Property(int propertyIndex, std::string name, int price,  std::vector<int> houseRent) {
        this->propertyIndex = propertyIndex;
        this->name = name;
        this->price = price;
        this->houseRent = houseRent;
        rent = houseRent[0]; owner = 1000; houses = 0; mortgaged = false;
        if (propertyIndex >= 0 && propertyIndex < 2) { colourSet = 1; houseCost = 50; }
        else if (propertyIndex >= 2 && propertyIndex < 5) { colourSet = 2; houseCost = 50; }
        else if (propertyIndex >= 5 && propertyIndex < 8) { colourSet = 3; houseCost = 100; }
        else if (propertyIndex >= 8 && propertyIndex < 11) { colourSet = 4; houseCost = 100; }
        else if (propertyIndex >= 11 && propertyIndex < 14) { colourSet = 5; houseCost = 150; }
        else if (propertyIndex >= 14 && propertyIndex < 17) { colourSet = 6; houseCost = 150; }
        else if (propertyIndex >= 17 && propertyIndex < 20) { colourSet = 7; houseCost = 200; }
        else if (propertyIndex >= 20 && propertyIndex < 22) { colourSet = 8; houseCost = 200; }
        else if (propertyIndex >= 22 && propertyIndex < 26) { colourSet = 9; houseCost = 0; }
        else if (propertyIndex >= 26 && propertyIndex < 28) { colourSet = 10; houseCost = 0; }
    }
};

class Player {
public:
    int playerIndex, money;
    bool bankrupt;
    std::string name; // NEW

    Player(int x, int y, bool z, std::string n) : playerIndex(x), money(y), bankrupt(z), name(n) {}
};

// --- DATA FILTER FUNCTIONS ---
std::vector<int> getUnownedProperties(const std::vector<Property>& properties) {
    std::vector<int> p;
    for (size_t i = 0; i < properties.size(); i++) if (properties[i].owner == 1000) p.push_back(i);
    return p;
}

std::vector<int> getOwnedProperties(const std::vector<Property>& properties, int playerNum, bool includeMortgage) {
    std::vector<int> p;
    for (size_t i = 0; i < properties.size(); i++) {
        if (properties[i].owner == playerNum) {
            if (includeMortgage || !properties[i].mortgaged) p.push_back(i);
        }
    }
    return p;
}

std::vector<int> getBuildableProperties(const std::vector<Property>& properties, int playerNum) {
    std::vector<int> p;
    for (size_t i = 0; i < properties.size(); i++) {
        if (properties[i].owner == playerNum && !properties[i].mortgaged && 
            properties[i].colourSet != 9 && properties[i].colourSet != 10) p.push_back(i);
    }
    return p;
}

std::vector<int> getAllOwnedProperties(const std::vector<Property>& properties) {
    std::vector<int> p;
    for (size_t i = 0; i < properties.size(); i++) if (properties[i].owner != 1000) p.push_back(i);
    return p;
}

std::vector<int> getMortgagedProperties(const std::vector<Property>& properties, int playerNum) {
    std::vector<int> p;
    for (size_t i = 0; i < properties.size(); i++) if (properties[i].owner == playerNum && properties[i].mortgaged) p.push_back(i);
    return p;
}

// --- CORE GAME FUNCTIONS ---
std::vector<Property> initializeProperties();
void buyProperty (std::vector<Property> &properties, int propertyNum, std::vector<Player> &player, int playerNum);
void buyHouse (std::vector<Property> &properties, int propertyNum, int houses, std::vector<Player> &player, int playerNum);
void payRent (std::vector<Property> &properties,std::vector<Player> &players,int playerNum, int propertyNum);
void mortgage (Property &property, Player &player);
void unmortgage (Property &property, Player &player);
void trade (std::vector<Property> &properties,std::vector<Player> &players, int player1, int player2);
void railRoadRentChecker (std::vector<Property> &properties);
void utilitiesRentChecker (std::vector<Property> &properties);

int main() {
    std::vector<Property> properties = initializeProperties();
    std::vector<Player> players;
    
    std::vector<std::string> playerQuantOpts;
    playerQuantOpts.push_back("2 Players"); playerQuantOpts.push_back("3 Players");
    playerQuantOpts.push_back("4 Players"); playerQuantOpts.push_back("5 Players");
    playerQuantOpts.push_back("6 Players"); playerQuantOpts.push_back("7 Players");
    playerQuantOpts.push_back("8 Players");

    int playersQuant = runHardwareMenu("How many players?", playerQuantOpts);
    if(playersQuant == -1) return 0; 
    playersQuant += 2; 
    
    // Name Setup Loop
    for (int i = 0; i < playersQuant; i++) {
        std::string n;
        std::cout << "Enter name for Player " << (i + 1) << ": ";
        std::cin >> n;
        players.push_back(Player(i, 1500, false, n));
    }

    std::vector<std::string> pOpts;
    for(int i = 0; i < playersQuant; i++) pOpts.push_back(players[i].name);

    std::vector<std::string> mainOpts;
    mainOpts.push_back("Buy a property"); mainOpts.push_back("Buy a house/hotel");
    mainOpts.push_back("Pay rent"); mainOpts.push_back("Mortgage");
    mainOpts.push_back("Unmortgage property"); mainOpts.push_back("Complete a trade");
    mainOpts.push_back("Add money"); mainOpts.push_back("Subtract money");
    mainOpts.push_back("See assets");

    while (true) {
        int actioningPlayer = runHardwareMenu("Which player are you?", pOpts);
        if (actioningPlayer == -1) continue;
        
        int playerDecision = runHardwareMenu(players[actioningPlayer].name + "'s Action:", mainOpts);
        if (playerDecision == -1) continue;
        playerDecision++; 
        
        if (playerDecision == 1) { 
            std::vector<int> validIdxs = getUnownedProperties(properties);
            std::vector<std::string> uiOpts;
            for(size_t k = 0; k < validIdxs.size(); k++) {
                int idx = validIdxs[k];
                uiOpts.push_back(properties[idx].name + " ($" + intToStr(properties[idx].price) + ")");
            }
            int sel = runHardwareMenu("Which property to purchase?", uiOpts);
            if(sel != -1) buyProperty(properties, validIdxs[sel], players, actioningPlayer);
            
        } else if(playerDecision == 2) { 
            std::vector<int> validIdxs = getBuildableProperties(properties, actioningPlayer);
            std::vector<std::string> uiOpts;
            for(size_t k = 0; k < validIdxs.size(); k++) {
                int idx = validIdxs[k];
                uiOpts.push_back(properties[idx].name + " ($" + intToStr(properties[idx].houseCost) + ")");
            }
            int sel = runHardwareMenu("Which property to build on?", uiOpts);
            if(sel != -1) buyHouse(properties, validIdxs[sel], 1, players, actioningPlayer);
            
        } else if(playerDecision == 3) { 
            std::vector<int> validIdxs = getAllOwnedProperties(properties);
            std::vector<std::string> uiOpts;
            for(size_t k = 0; k < validIdxs.size(); k++) {
                int idx = validIdxs[k];
                uiOpts.push_back(properties[idx].name);
            }
            int sel = runHardwareMenu("Which property did you land on?", uiOpts);
            if(sel != -1) payRent(properties, players, actioningPlayer, validIdxs[sel]);
            
        } else if(playerDecision == 4) { 
            std::vector<int> validIdxs = getOwnedProperties(properties, actioningPlayer, false);
            std::vector<std::string> uiOpts;
            for(size_t k = 0; k < validIdxs.size(); k++) {
                int idx = validIdxs[k];
                uiOpts.push_back(properties[idx].name);
            }
            int sel = runHardwareMenu("Which property to mortgage?", uiOpts);
            if(sel != -1) { mortgage(properties[validIdxs[sel]], players[actioningPlayer]); railRoadRentChecker(properties); }
            
        } else if(playerDecision == 5) { 
            std::vector<int> validIdxs = getMortgagedProperties(properties, actioningPlayer);
            std::vector<std::string> uiOpts;
            for(size_t k = 0; k < validIdxs.size(); k++) {
                int idx = validIdxs[k];
                uiOpts.push_back(properties[idx].name);
            }
            int sel = runHardwareMenu("Which property to unmortgage?", uiOpts);
            if(sel != -1) { unmortgage(properties[validIdxs[sel]], players[actioningPlayer]); railRoadRentChecker(properties); }
            
        } else if(playerDecision == 6) { 
            std::vector<std::string> tOpts;
            std::vector<int> playerIds;
            for (int i = 0; i < (int)players.size(); i++){
                if (i != actioningPlayer) {
                    tOpts.push_back(players[i].name);
                    playerIds.push_back(i);
                }
            }
            int sel = runHardwareMenu("Trade with who?", tOpts);
            if(sel != -1) trade(properties, players, actioningPlayer, playerIds[sel]);
            
        } else if(playerDecision == 7) { 
            std::cout << "\nHow much money to add?\n> ";
            int moneyChange = getValidInput(1, 100000); 
            players[actioningPlayer].money += moneyChange;
            std::cout << "Balance: $" << players[actioningPlayer].money << "\nPress [Enter] to continue...";
            std::cin.ignore(); std::cin.get();
        } else if(playerDecision == 8) { 
            std::cout << "\nHow much money to subtract?\n> ";
            int moneyChange = getValidInput(1, 100000); 
            if (players[actioningPlayer].money >= moneyChange) players[actioningPlayer].money -= moneyChange;
            else std::cout << "Error: not enough money.\n";
            std::cout << "Balance: $" << players[actioningPlayer].money << "\nPress [Enter] to continue...";
            std::cin.ignore(); std::cin.get();
        } else if(playerDecision == 9) { 
            std::vector<std::string> assetsList;
            assetsList.push_back("CASH: $" + intToStr(players[actioningPlayer].money));
            for (size_t i = 0; i < properties.size(); i++) {
                if (properties[i].owner == actioningPlayer) {
                    std::string entry = properties[i].name;
                    if (properties[i].mortgaged) entry += " (MORTGAGED)";
                    else if(properties[i].houses != 0) entry += " (Houses: " + intToStr(properties[i].houses) + ")";
                    assetsList.push_back(entry);
                }  
            } 
            runHardwareMenu("Assets - " + players[actioningPlayer].name, assetsList);
        }
    }
    return 0;
}

// --- INITIALIZATION ---
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
    p.push_back(Property(22, "Reading Railroad",  200, makeRR(25, 50, 100, 200)));
    p.push_back(Property(23, "Pennsylvania Railroad", 200, makeRR(25, 50, 100, 200)));
    p.push_back(Property(24, "B. & O. Railroad", 200, makeRR(25, 50, 100, 200)));
    p.push_back(Property(25, "Short Line", 200, makeRR(25, 50, 100, 200)));
    p.push_back(Property(26, "Electric Company", 150, makeUT(4, 10)));
    p.push_back(Property(27, "Water Works", 150, makeUT(4, 10)));
    return p;
}

// --- LOGIC FUNCTIONS ---
void buyHouse (std::vector<Property> &properties, int propertyNum, int houses, std::vector<Player> &players, int playerNum){
    Property property = properties.at(propertyNum);
    Player player = players.at(playerNum);
    if (property.colourSet == 9 || property.colourSet == 10) { std::cout << "Error: No houses on RR/Utilities.\n"; return; }
    for (int i = 0; i < houses; i++) {
        if ((player.money >= property.houseCost && property.houses<=4)&& property.owner == playerNum) {
            player.money -= property.houseCost; property.houses++;
            property.rent = property.houseRent[property.houses];
            properties[propertyNum] = property; players[playerNum] = player;
            std::cout << "Purchase succesful.\nBalance: " << player.money << "\n";
        }else std::cout << "Purchasing Error\n";
    }
}

void buyProperty (std::vector<Property> &properties, int propertyNum, std::vector<Player> &players, int playerNum){
    Property property = properties.at(propertyNum);
    Player player = players.at(playerNum);
    if (player.money >= property.price && property.owner==1000) {
        player.money -= property.price; property.owner = playerNum;
        players[playerNum] = player; properties[propertyNum] = property;
        std::cout << "Purchase Succesful!\nBalance: " << player.money << "\n";
        railRoadRentChecker(properties);
    }else std::cout << "Purchasing Error\n";
}

void payRent (std::vector<Property> &properties,std::vector<Player> &players,int playerNum, int propertyNum){
    Property property = properties[propertyNum];
    Player player = players[playerNum];
    if (property.owner == playerNum || property.owner == 1000) {
        std::cout << "No rent due.\n"; std::cin.ignore(); std::cin.get(); return;
    }
    if (player.money >= property.rent) {
        if (propertyNum == 26 || propertyNum == 27) { 
            std::vector<std::string> diceOpts;
            for (int r = 2; r <= 12; r++) diceOpts.push_back(intToStr(r));
            int rollIndex = runHardwareMenu("Select your dice roll (2-12):", diceOpts);
            if (rollIndex == -1) return;
            int roll = rollIndex + 2;
            player.money -= (property.rent)*roll;
            players[playerNum] = player;
            players[property.owner].money += (property.rent)*roll;
            std::cout << "Paid Rent: $" << property.rent*roll << "\n";
        }else{
            player.money -= property.rent;
            players[playerNum] = player;
            players[property.owner].money += property.rent;
            std::cout << "Paid Rent: $" << property.rent << "\n";
        }
        std::cout << "Press [Enter] to continue..."; std::cin.ignore(); std::cin.get();
    }else std::cout<< "Transaction failed. Not enough money.\n";
}

void mortgage (Property &property, Player &player){
    if (property.houses != 0 && property.owner == player.playerIndex ) {
        property.houses--; player.money += (property.houseCost)/2;
        property.rent = property.houseRent[property.houses];
    }else if(property.mortgaged == false && property.owner == player.playerIndex){
        property.mortgaged = true; player.money += (property.price)/2;
        property.rent = 0;
    }
}

void unmortgage (Property &property, Player &player){
    if((property.mortgaged == true && property.owner == player.playerIndex)&& ((property.price)/2)*1.1 <= player.money){
        property.mortgaged = false; player.money -= (int)(((property.price)/2)*1.1);
        property.rent = property.houseRent[property.houses];
    }
}

void trade (std::vector<Property> &properties,std::vector<Player> &players, int player1, int player2){
    while (true){
        std::vector<int> validIdxs = getOwnedProperties(properties, player2, true);
        std::vector<std::string> opts; opts.push_back("DONE SELECTING");
        for(size_t k = 0; k < validIdxs.size(); k++) opts.push_back(properties[validIdxs[k]].name);
        int sel = runHardwareMenu(players[player1].name + " Receiving:", opts);
        if (sel <= 0) break; 
        properties[validIdxs[sel-1]].owner = 1001; 
    }
    while (true){
        std::vector<int> validIdxs = getOwnedProperties(properties, player1, true);
        std::vector<std::string> opts; opts.push_back("DONE SELECTING");
        for(size_t k = 0; k < validIdxs.size(); k++) opts.push_back(properties[validIdxs[k]].name);
        int sel = runHardwareMenu(players[player2].name + " Receiving:", opts);
        if (sel <= 0) break; 
        properties[validIdxs[sel-1]].owner = 1002; 
    }
    for (size_t i = 0; i < properties.size(); i++){
        if (properties[i].owner == 1001) properties[i].owner = player1;
        if (properties[i].owner == 1002) properties[i].owner = player2;
    }
    railRoadRentChecker(properties);
}

void railRoadRentChecker (std::vector<Property> &properties){
    int owners[] = {properties[22].owner, properties[23].owner, properties[24].owner, properties[25].owner};
    for(int i=0; i<4; i++){
        int count = 0;
        for(int j=0; j<4; j++) if(owners[i] == owners[j] && owners[i] != 1000) count++;
        properties[22+i].houses = count - 1;
        properties[22+i].rent = properties[22+i].houseRent[properties[22+i].houses];
    }
    utilitiesRentChecker(properties);
}

void utilitiesRentChecker (std::vector<Property> &properties){
    if (properties[26].owner == properties[27].owner && properties[26].owner != 1000) {
        properties[26].houses = 1; properties[27].houses = 1;
    }else{
        properties[26].houses = 0; properties[27].houses = 0;
    }
    properties[26].rent = properties[26].houseRent[properties[26].houses];
    properties[27].rent = properties[27].houseRent[properties[27].houses];
}