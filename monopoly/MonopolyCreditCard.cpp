#include <iostream>
#include <vector>
#include <string>
using namespace std;
class Property {
public:
    string name;
    int propertyIndex;
    int owner; // 1000 -> unowned
    int colourSet;
    int houses;
    int rent;
    bool mortgaged;
    int houseCost;
    int price;
     vector<int> houseRent;
    Property(int propertyIndex,  string name, int price,   vector<int> houseRent) {
        this->propertyIndex = propertyIndex;
        this->name = name;
        this->price = price;
        this->houseRent = houseRent;
        rent = houseRent[0];
        owner = 1000;
        houses = 0;
        mortgaged = false;

        if (propertyIndex >= 0 && propertyIndex < 2) { // brown
            colourSet = 1;
            houseCost = 50;
        }
        else if (propertyIndex >= 2 && propertyIndex < 5) { // light blue
            colourSet = 2;
            houseCost = 50;
        }
        else if (propertyIndex >= 5 && propertyIndex < 8) { // pink
            colourSet = 3;
            houseCost = 100;
        }
        else if (propertyIndex >= 8 && propertyIndex < 11) { // orange
            colourSet = 4;
            houseCost = 100;

        }
        else if (propertyIndex >= 11 && propertyIndex < 14) { // red
            colourSet = 5;
            houseCost = 150;
        }
        else if (propertyIndex >= 14 && propertyIndex < 17) { // yellow
            colourSet = 6;
            houseCost = 150;
        }
        else if (propertyIndex >= 17 && propertyIndex < 20) { // green
            colourSet = 7;
            houseCost = 200;
        }
        else if (propertyIndex >= 20 && propertyIndex < 22) { // dark blue
            colourSet = 8;
            houseCost = 200;
        }
        else if (propertyIndex >= 22 && propertyIndex < 26) { // railroads
            colourSet = 9;
            houseCost = 0;
        }
        else if (propertyIndex >= 26 && propertyIndex < 28) { // utilities
            colourSet = 10;
            houseCost = 0;
        }
    }
};


class Player {
public:
    int playerIndex;
    int money;
    bool bankrupt;

    Player(int x, int y, bool z) {
        playerIndex = x;
        money = y;
        bankrupt = z;
    }
};

void listProperties( vector<Property> &properties){

    for (int i = 0; i < properties.size(); i++)
    {
         cout << i+1 << ") " << properties[i].name << "\n";
    }
}
void listOwnedProperties( vector<Property> &properties, int playerNum, bool includeMortgage){
    for (int i = 0; i < properties.size(); i++)
    {
        if (includeMortgage)
        {
            if (playerNum == properties[i].owner)
            {
             cout << i+1 << ") " << properties[i].name << "\n";
            }
        }else{
            if (playerNum == properties[i].owner && properties[i].mortgaged == false)
            {
             cout << i+1 << ") " << properties[i].name << "\n";
            }
        }
        
        
    }
}
void listUnOwnedProperties( vector<Property> &properties){
    for (int i = 0; i < properties.size(); i++)
    {
        if (1000 == properties[i].owner){
             cout << i+1 << ") " << properties[i].name << "\n";
        }
    }
}
void listAllOwnedProperties( vector<Property> &properties){
    for (int i = 0; i < properties.size(); i++)
    {
        if (1000 != properties[i].owner)
        {
             cout << i+1 << ") " << properties[i].name << "\n";
        }
    }
}
void listMortgagedProperties( vector<Property> &properties, int playerNum){
    for (int i = 0; i < properties.size(); i++){
        if (playerNum == properties[i].owner && properties[i].mortgaged == true){
         cout << i+1 << ") " << properties[i].name << "\n";
        }   
    }
    
}

 vector<Property> initializeProperties();
void buyProperty ( vector<Property> &properties, int propertyNum,  vector<Player> &player, int playerNum);
void buyHouse ( vector<Property> &properties, int property, int houses,  vector<Player> &player, int playerNum);
void payRent ( vector<Property> &properties, vector<Player> &players,int playerNum, int propertyNum);
void mortgage (Property &property, Player &player);
void unmortgage (Property &property, Player &player);
void trade ( vector<Property> &properties, vector<Player> &players, int player1, int player2);
void railRoadRentChecker ( vector<Property> &properties);
void utilitiesRentChecker ( vector<Property> &properties);







int main() {
    int playersQuant;
    int actioningPlayer;
    int playerDecision;
    int propertyPurchase;
    int playerTrade;
    int moneyChange;
    int counter;
     vector<Property> properties = initializeProperties();
     vector<Player> players;

    
     cout << "How many players?\n";
     cin >> playersQuant;
    for (int i = 0; i < playersQuant; i++) {
        players.emplace_back(i, 1500, false);
    }

    while (true)
    {
         cout << "Which player are you? \n";
         cin >> actioningPlayer;
        actioningPlayer--;
        
         cout << "Player " << actioningPlayer+1 << " What would you like to do? \n1) Buy a property \n2) Buy a house/hotel \n3) Pay rent \n4) Mortgage\n5) Unmortgage property \n6) Complete a trade \n7) Add money \n8) Subtract money\n9) See assets\n";
         cin >> playerDecision;
        if (playerDecision == 1){ // 1) Buying properties
             cout << "Which property would you like to purchase? \n";  
            listUnOwnedProperties(properties);
              cin >> propertyPurchase;
            propertyPurchase--;
            buyProperty(properties, propertyPurchase, players, actioningPlayer);
        }else if(playerDecision == 2){//Buy a house/hotel
             cout << "Which property would you like to purchase a house on? \n";
            listOwnedProperties(properties, actioningPlayer, false);
             cin>> propertyPurchase;
            propertyPurchase--;
            buyHouse(properties, propertyPurchase, 1, players, actioningPlayer);
        }else if(playerDecision == 3){
             cout << "Which property did you land on? \n";
            listAllOwnedProperties(properties);
             cin >> propertyPurchase;
            propertyPurchase--;
            payRent(properties, players, actioningPlayer, propertyPurchase);
        }else if(playerDecision == 4){
             cout << "Which property would you like to mortage (if you choose one with a house it will mortgage a house)? \n";
            listOwnedProperties(properties, actioningPlayer, false);
             cin >> propertyPurchase;
            propertyPurchase--;
            mortgage(properties[propertyPurchase], players[actioningPlayer]);
            railRoadRentChecker(properties);
        }else if(playerDecision == 5){
             cout << "Which property would you like to unmortgage? \n";
            listMortgagedProperties(properties, actioningPlayer);
             cin >> propertyPurchase;
            propertyPurchase--;
            unmortgage(properties[propertyPurchase], players[actioningPlayer]);
            railRoadRentChecker(properties);
        }else if(playerDecision == 6){
             cout << "Which player would you like to make a with? \n";
            for (int i = 0; i < players.size(); i++){
                if (players[i].playerIndex != actioningPlayer)
                {
                     cout << "Player: " << i+1 << "\n";
                }
            }
             cin >> playerTrade;
            playerTrade--;
            trade(properties,players, actioningPlayer, playerTrade);
        }else if(playerDecision == 7){
             cout << "How much money would you like to add? \n";
             cin >> moneyChange;
            players[actioningPlayer].money += moneyChange;
             cout<< "Balance: $" << players[actioningPlayer].money << "\n";
        }else if(playerDecision == 8){
             cout << "How much money would you like to subtract? \n";
             cin >> moneyChange;
            if (players[actioningPlayer].money >= moneyChange)
            {
                players[actioningPlayer].money -= moneyChange;
            }else{
                 cout<< "Error: not enough moeny for transaction to go through. \n";
            }
             cout<< "Balance: 4" << players[actioningPlayer].money << "\n";
        }else if(playerDecision == 9){
             cout<< "Balance: $" << players[actioningPlayer].money << "\n";
            counter = 1;
            for (int i = 0; i < properties.size(); i++)
            {
                if (properties[i].owner == actioningPlayer)
                {
                     cout << counter << ") " << properties[i].name;
                    if (properties[i].mortgaged)
                    {
                         cout<< " - mortgaged";
                    }else if(properties[i].houses !=0){
                         cout<< " - houses: " << properties[i].houses;
                    }
                    
                    
                    counter++;
                     cout << "\n";
                }  
            } 
        }
        
    }


    return 0;
}


 vector<Property> initializeProperties() {
     vector<Property> properties;

     vector<int> mediterraneanRent;
    mediterraneanRent.push_back(2);
    mediterraneanRent.push_back(10);
    mediterraneanRent.push_back(30);
    mediterraneanRent.push_back(90);
    mediterraneanRent.push_back(160);
    mediterraneanRent.push_back(250);

     vector<int> balticRent;
    balticRent.push_back(4);
    balticRent.push_back(20);
    balticRent.push_back(60);
    balticRent.push_back(180);
    balticRent.push_back(320);
    balticRent.push_back(450);

     vector<int> orientalRent;
    orientalRent.push_back(6);
    orientalRent.push_back(30);
    orientalRent.push_back(90);
    orientalRent.push_back(270);
    orientalRent.push_back(400);
    orientalRent.push_back(550);

     vector<int> vermontRent;
    vermontRent.push_back(6);
    vermontRent.push_back(30);
    vermontRent.push_back(90);
    vermontRent.push_back(270);
    vermontRent.push_back(400);
    vermontRent.push_back(550);

     vector<int> connecticutRent;
    connecticutRent.push_back(8);
    connecticutRent.push_back(40);
    connecticutRent.push_back(100);
    connecticutRent.push_back(300);
    connecticutRent.push_back(450);
    connecticutRent.push_back(600);

     vector<int> stCharlesRent;
    stCharlesRent.push_back(10);
    stCharlesRent.push_back(50);
    stCharlesRent.push_back(150);
    stCharlesRent.push_back(450);
    stCharlesRent.push_back(625);
    stCharlesRent.push_back(750);

     vector<int> statesRent;
    statesRent.push_back(10);
    statesRent.push_back(50);
    statesRent.push_back(150);
    statesRent.push_back(450);
    statesRent.push_back(625);
    statesRent.push_back(750);

     vector<int> virginiaRent;
    virginiaRent.push_back(12);
    virginiaRent.push_back(60);
    virginiaRent.push_back(180);
    virginiaRent.push_back(500);
    virginiaRent.push_back(700);
    virginiaRent.push_back(900);

     vector<int> stJamesRent;
    stJamesRent.push_back(14);
    stJamesRent.push_back(70);
    stJamesRent.push_back(200);
    stJamesRent.push_back(550);
    stJamesRent.push_back(750);
    stJamesRent.push_back(950);

     vector<int> tennesseeRent;
    tennesseeRent.push_back(14);
    tennesseeRent.push_back(70);
    tennesseeRent.push_back(200);
    tennesseeRent.push_back(550);
    tennesseeRent.push_back(750);
    tennesseeRent.push_back(950);

     vector<int> newYorkRent;
    newYorkRent.push_back(16);
    newYorkRent.push_back(80);
    newYorkRent.push_back(220);
    newYorkRent.push_back(600);
    newYorkRent.push_back(800);
    newYorkRent.push_back(1000);

     vector<int> kentuckyRent;
    kentuckyRent.push_back(18);
    kentuckyRent.push_back(90);
    kentuckyRent.push_back(250);
    kentuckyRent.push_back(700);
    kentuckyRent.push_back(875);
    kentuckyRent.push_back(1050);

     vector<int> indianaRent;
    indianaRent.push_back(18);
    indianaRent.push_back(90);
    indianaRent.push_back(250);
    indianaRent.push_back(700);
    indianaRent.push_back(875);
    indianaRent.push_back(1050);

     vector<int> illinoisRent;
    illinoisRent.push_back(20);
    illinoisRent.push_back(100);
    illinoisRent.push_back(300);
    illinoisRent.push_back(750);
    illinoisRent.push_back(925);
    illinoisRent.push_back(1100);

     vector<int> atlanticRent;
    atlanticRent.push_back(22);
    atlanticRent.push_back(110);
    atlanticRent.push_back(330);
    atlanticRent.push_back(800);
    atlanticRent.push_back(975);
    atlanticRent.push_back(1150);

     vector<int> ventnorRent;
    ventnorRent.push_back(22);
    ventnorRent.push_back(110);
    ventnorRent.push_back(330);
    ventnorRent.push_back(800);
    ventnorRent.push_back(975);
    ventnorRent.push_back(1150);

     vector<int> marvinGardensRent;
    marvinGardensRent.push_back(24);
    marvinGardensRent.push_back(120);
    marvinGardensRent.push_back(360);
    marvinGardensRent.push_back(850);
    marvinGardensRent.push_back(1025);
    marvinGardensRent.push_back(1200);

     vector<int> pacificRent;
    pacificRent.push_back(26);
    pacificRent.push_back(130);
    pacificRent.push_back(390);
    pacificRent.push_back(900);
    pacificRent.push_back(1100);
    pacificRent.push_back(1275);

     vector<int> northCarolinaRent;
    northCarolinaRent.push_back(26);
    northCarolinaRent.push_back(130);
    northCarolinaRent.push_back(390);
    northCarolinaRent.push_back(900);
    northCarolinaRent.push_back(1100);
    northCarolinaRent.push_back(1275);

     vector<int> pennsylvaniaRent;
    pennsylvaniaRent.push_back(28);
    pennsylvaniaRent.push_back(150);
    pennsylvaniaRent.push_back(450);
    pennsylvaniaRent.push_back(1000);
    pennsylvaniaRent.push_back(1200);
    pennsylvaniaRent.push_back(1400);

     vector<int> parkPlaceRent;
    parkPlaceRent.push_back(35);
    parkPlaceRent.push_back(175);
    parkPlaceRent.push_back(500);
    parkPlaceRent.push_back(1100);
    parkPlaceRent.push_back(1300);
    parkPlaceRent.push_back(1500);

     vector<int> boardwalkRent;
    boardwalkRent.push_back(50);
    boardwalkRent.push_back(200);
    boardwalkRent.push_back(600);
    boardwalkRent.push_back(1400);
    boardwalkRent.push_back(1700);
    boardwalkRent.push_back(2000);

     vector<int> railroadRent;
    railroadRent.push_back(25);
    railroadRent.push_back(50);
    railroadRent.push_back(100);
    railroadRent.push_back(200);

     vector<int> utilityRent;
    utilityRent.push_back(4);
    utilityRent.push_back(10);
    
    // Brown Set ($60)
    properties.emplace_back(0, "Mediterranean Avenue", 60, mediterraneanRent);
    properties.emplace_back(1, "Baltic Avenue", 60, balticRent);

    // Light Blue Set ($100 - $120)
    properties.emplace_back(2, "Oriental Avenue", 100, orientalRent);
    properties.emplace_back(3, "Vermont Avenue", 100, vermontRent);
    properties.emplace_back(4, "Connecticut Avenue", 120, connecticutRent);

    // Pink Set ($140 - $160)
    properties.emplace_back(5, "St. Charles Place", 140, stCharlesRent);
    properties.emplace_back(6, "States Avenue", 140, statesRent);
    properties.emplace_back(7, "Virginia Avenue", 160, virginiaRent);

    // Orange Set ($180 - $200)
    properties.emplace_back(8, "St. James Place", 180, stJamesRent);
    properties.emplace_back(9, "Tennessee Avenue", 180, tennesseeRent);
    properties.emplace_back(10, "New York Avenue", 200, newYorkRent);

    // Red Set ($220 - $240)
    properties.emplace_back(11, "Kentucky Avenue", 220, kentuckyRent);
    properties.emplace_back(12, "Indiana Avenue", 220, indianaRent);
    properties.emplace_back(13, "Illinois Avenue", 240, illinoisRent);

    // Yellow Set ($260 - $280)
    properties.emplace_back(14, "Atlantic Avenue", 260, atlanticRent);
    properties.emplace_back(15, "Ventnor Avenue", 260, ventnorRent);
    properties.emplace_back(16, "Marvin Gardens", 280, marvinGardensRent);

    // Green Set ($300 - $320)
    properties.emplace_back(17, "Pacific Avenue", 300, pacificRent);
    properties.emplace_back(18, "North Carolina Avenue", 300, northCarolinaRent);
    properties.emplace_back(19, "Pennsylvania Avenue", 320, pennsylvaniaRent);

    // Dark Blue Set ($350 - $400)
    properties.emplace_back(20, "Park Place", 350, parkPlaceRent);
    properties.emplace_back(21, "Boardwalk", 400, boardwalkRent);

    // Railroads ($200)
    properties.emplace_back(22, "Reading Railroad",  200, railroadRent);
    properties.emplace_back(23, "Pennsylvania Railroad", 200, railroadRent);
    properties.emplace_back(24, "B. & O. Railroad", 200, railroadRent);
    properties.emplace_back(25, "Short Line", 200, railroadRent);

    // Utilities ($150)
    properties.emplace_back(26, "Electric Company", 150, utilityRent);
    properties.emplace_back(27, "Water Works", 150, utilityRent);

    return properties;
}

void buyHouse ( vector<Property> &properties, int propertyNum, int houses,  vector<Player> &players, int playerNum){
    Property property = properties.at(propertyNum);
    Player player = players.at(playerNum);
    for (int i = 0; i < houses; i++)
    {
        if ((player.money >= property.houseCost && property.houses<=4)&& property.owner == playerNum)
        {
            player.money = player.money - property.houseCost;
            property.houses++;
            property.rent = property.houseRent[property.houses];
            properties[propertyNum] = property;
            players[playerNum] = player;
             cout << "Purchase succesful. \nRemaining balance: " << player.money << "\n";
        }else{
             cout << "Purchasing Error \n";
        }
    }
    
    
    
}

void buyProperty ( vector<Property> &properties, int propertyNum,  vector<Player> &players, int playerNum){
    Property property = properties.at(propertyNum);
    Player player = players.at(playerNum);

    if (player.money >= property.price && property.owner==1000)
    {
        player.money = player.money - property.price;
        property.owner = playerNum;
        players[playerNum] = player;
        properties[propertyNum] = property;
         cout << "Purchase Succesful! \nRemaining balance: " << player.money << "\n";
        railRoadRentChecker(properties);
    }else{
         cout << "Purchasing Error \n";
    }
}

void payRent ( vector<Property> &properties, vector<Player> &players,int playerNum, int propertyNum){
    Property property = properties[propertyNum];
    Player player = players[playerNum];
    int roll;

    if (player.money>=property.rent && property.owner != 1000)
    {
        if (propertyNum == 26 || propertyNum ==27)//for utilities
        {
             cout << "What number did you roll? \n";
             cin >> roll;
            player.money = player.money-(property.rent)*roll;
            players[playerNum] = player;
        
            players[property.owner].money = players[property.owner].money + (property.rent)*roll;

             cout << "Rent $" << property.rent*roll << "\n";
             cout << "Player " << playerNum +1 << " Balance: $" << player.money << "\n";
             cout << "Player " << property.owner +1 << " Balance: $" << players[property.owner].money << "\n";
        }else{
            player.money = player.money-property.rent;
            players[playerNum] = player;
            players[property.owner].money = players[property.owner].money + property.rent;

             cout << "Rent $" << property.rent << "\n";
             cout << "Player " << playerNum +1 << " Balance: $" << player.money << "\n";
             cout << "Player " << property.owner +1 << " Balance: $" << players[property.owner].money << "\n";
        }
    }else{
         cout<< "Transaction failed. \n";
    }
    
}

void mortgage (Property &property, Player &player){
    if (property.houses != 0 && property.owner == player.playerIndex )
    {
        property.houses--;
        player.money += (property.houseCost)/2;
        property.rent = property.houseRent[property.houses];
         cout << "You have succesfully mortgaged a house for $" <<(property.houseCost)/2 << '\n' << "Remaining houses: " << property.houses << "\n" << "Balance: " << player.money << "\n";
    }else if(property.mortgaged == false && property.owner == player.playerIndex){
        property.mortgaged = true;
        player.money += (property.price)/2;
        property.rent = 0;
         cout << "You have succesfully mortgaged " << property.name << " for $" <<(property.price)/2 << '\n' << "Balance: " << player.money << "\n";
    }else{
         cout << "Transaction failed. \n";
    }
    
}

void unmortgage (Property &property, Player &player){
    if((property.mortgaged == true && property.owner == player.playerIndex)&& ((property.price)/2)*1.1 <= player.money){
        property.mortgaged = false;
        player.money -= ((property.price)/2)*1.1;
        property.rent = property.houseRent[property.houses];
         cout << "You have succesfully unmortgaged " << property.name << " for $" <<((property.price)/2)*1.1 << '\n' << "Balance: " << player.money << "\n";
    }else{
         cout << "Transaction failed. \n";
    }
    
}

void trade ( vector<Property> &properties, vector<Player> &players, int player1, int player2){
    int tradedProperties;
     cout << "Which properties will player " << player1 + 1 << " receive (enter 0 when done)? \n";
    while (true){
        listOwnedProperties(properties, player2, true);
         cin >> tradedProperties;
        if (tradedProperties == 0){
            break;
        }
        tradedProperties--;
        properties[tradedProperties].owner = 1001;
    }

     cout << "Which properties will player " << player2 + 1 << " recieve (enter 0 when done)? \n";
    while (true){
        listOwnedProperties(properties, player1, true);
         cin >> tradedProperties;
        if (tradedProperties == 0){
            break;
        }
        tradedProperties--;
        properties[tradedProperties].owner = 1002;
    }

     cout << "Player " << player1 +1 << " recieves: \n";
    listOwnedProperties(properties, 1001, true);
     cout << "\n";
     cout << "Player " << player2 +1 << " recieves: \n";
    listOwnedProperties(properties, 1002, true);
     cout << "\n";



    for (int i = 0; i < properties.size(); i++){
        if (properties[i].owner == 1001){
            properties[i].owner = player1;
        }

        if (properties[i].owner == 1002){
            properties[i].owner = player2;
        }
    }
    railRoadRentChecker(properties);
}

void railRoadRentChecker ( vector<Property> &properties){//22,23,24,25
    int ownerOne = properties[22].owner;
    int ownerTwo = properties[23].owner;
    int ownerThree = properties[24].owner;
    int ownerFour = properties[25].owner;
    int commonCounter;

    commonCounter = 0;
    if (ownerOne==ownerTwo && ownerOne !=1000){
        commonCounter ++;
    } if (ownerOne==ownerThree&& ownerOne !=1000){
        commonCounter ++;
    } if (ownerOne==ownerFour&& ownerOne !=1000){
        commonCounter ++;
    }
    properties[22].houses = commonCounter;
    
    commonCounter = 0;
    if (ownerTwo==ownerOne&& ownerTwo !=1000){//23
        commonCounter ++;
    }if (ownerTwo==ownerThree&& ownerTwo !=1000){
        commonCounter ++;
    }if (ownerTwo==ownerFour&& ownerTwo !=1000){
        commonCounter ++;
    }
    properties[23].houses = commonCounter;

    commonCounter = 0;
    if (ownerThree==ownerOne&& ownerThree !=1000){
        commonCounter ++;
    }if (ownerThree==ownerTwo&& ownerThree !=1000){
        commonCounter ++;
    }if (ownerThree==ownerFour&& ownerThree !=1000){
        commonCounter ++;
    }
    properties[24].houses = commonCounter;

    commonCounter = 0;
    if (ownerFour==ownerOne&& ownerFour !=1000){
        commonCounter ++;
    }if (ownerFour==ownerTwo&& ownerFour !=1000){
        commonCounter ++;
    }if (ownerFour==ownerThree&& ownerFour !=1000){
        commonCounter ++;
    }
    properties[25].houses = commonCounter;
    
    properties[22].rent = properties[22].houseRent[properties[22].houses];
    properties[23].rent = properties[23].houseRent[properties[23].houses];
    properties[24].rent = properties[24].houseRent[properties[24].houses];
    properties[25].rent = properties[25].houseRent[properties[25].houses];
    utilitiesRentChecker(properties);
}

void utilitiesRentChecker ( vector<Property> &properties){
    if (properties[26].owner == properties[27].owner && properties[26].owner != 1000)
    {
        properties[26].houses = 1;
        properties[27].houses = 1;
    }else{
        properties[26].houses = 0;
        properties[27].houses = 0;
    }
    properties[26].rent = properties[26].houseRent[properties[26].houses];
    properties[27].rent = properties[27].houseRent[properties[27].houses];
}


