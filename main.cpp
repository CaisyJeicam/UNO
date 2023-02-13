#include <netdb.h>
#include <poll.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <unistd.h>
#include <netinet/in.h>
#include <iostream>
#include <cstring>
#include <random>
#include <thread>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <unistd.h>
#include "Card.h"

const int PORT = 1100;
const int BACKLOG = 10;
const int MAX_PLAYERS = 3;

std::vector<Card> deck;
std::vector<Card> discardPile;
std::vector<int> players;
std::unordered_map<int, std::vector<Card>> playerHands;
unsigned currentPlayer = 0;
bool gameOver = false;
int playOrder = 1;
bool activationFlag = false;

std::mutex mtx;
std::condition_variable cv;
std::queue<std::pair<int, std::string>> messages;

void initDeck() {
    deck.clear();
    for (int i = 0; i < 4; i++) {
        auto color = static_cast<Card::Color>(i);
        for (int j = 0; j <= 9; j++) {
            deck.emplace_back(Card(color, Card::Type::NUMBER, j));
            deck.emplace_back(Card(color, Card::Type::NUMBER, j));
        }
        deck.emplace_back(Card(color, Card::Type::SKIP, -1));
        deck.emplace_back(Card(color, Card::Type::SKIP, -1));
        deck.emplace_back(Card(color, Card::Type::REVERSE, -1));
        deck.emplace_back(Card(color, Card::Type::REVERSE, -1));
        deck.emplace_back(Card(color, Card::Type::DRAW_TWO, -1));
        deck.emplace_back(Card(color, Card::Type::DRAW_TWO, -1));
    }
    for (int i = 0; i < 4; i++) {
        deck.emplace_back(Card(Card::Color::WILD, Card::Type::WILD, -1));
        deck.emplace_back(Card(Card::Color::WILD, Card::Type::WILD_DRAW_FOUR, -1));
    }
}

void shuffleDeck() {
    std::shuffle(deck.begin(), deck.end(), std::mt19937(std::random_device()()));
    Card firstCard = deck.back();
    deck.pop_back();
    discardPile.push_back(firstCard);
}

void shuffleDiscardPile(){
    deck = discardPile;
    discardPile.clear();
    shuffleDeck();
}

Card drawCard() {
    if (deck.empty()) {
        shuffleDiscardPile();
    }
    Card drawnCard = deck.back();
    deck.pop_back();
    return drawnCard;
}

void dealStartingHand(unsigned maxPlayer){
    for (int i = 0; i < 7; i++)
    {
        for (int j = 0; j < maxPlayer; j++)
        {
            playerHands[j].push_back(drawCard());
            deck.pop_back();
        }
    }
}

void sendMessage(int player, const std::string& message) {
    int client = players[player];
    std::unique_lock<std::mutex> lock(mtx);
    messages.emplace(client, message);
    cv.notify_all();
}

void broadcastMessage(const std::string& message) {
    for (int player = 0; player < players.size(); player++) {
        sendMessage(player, message);
    }
}

void messageHandler(int player) {
    while (!gameOver) {
        std::unique_lock<std::mutex> lock(mtx);
        while (messages.empty()) {
            cv.wait(lock);
        }
        auto message = messages.front();
        if (message.first == player) {
            int n = send(player, message.second.c_str(), message.second.length(), 0);
            if (n < 0) {
                std::cerr << "Error sending message to player " << player << std::endl;
            }
            messages.pop();
        }
    }
}

void handleTurn(int player) {
    int client = players[player];
    std::string message;
    Card topCard = discardPile.back();
    if ((topCard.type == Card::Type::SKIP) && (activationFlag == 1)){
        sendMessage(player, "You skip your turn.");
        activationFlag = false;
        return;
    }
    else if ((topCard.type == Card::Type::WILD_DRAW_FOUR) && (activationFlag == 1)){
        sendMessage(player, "You draw four cards.");
        playerHands[player].push_back(drawCard());
        playerHands[player].push_back(drawCard());
        playerHands[player].push_back(drawCard());
        playerHands[player].push_back(drawCard());
        activationFlag = false;
        return;
    }
    else if ((topCard.type == Card::Type::DRAW_TWO) && (activationFlag == 1)){
        sendMessage(player, "You draw two cards.");
        playerHands[player].push_back(drawCard());
        playerHands[player].push_back(drawCard());
        activationFlag = false;
        return;
    }
    message = "Your turn. Your hand: ";
    for (Card card : playerHands[player]) {
        message += card.toString() + ", ";
    }
    message.pop_back();
    message.pop_back();
    sendMessage(player, message);
    sendMessage(player, "\nWhat would you like to play? (e.g. 'play 2'). Send 'play d' to draw a card");
    sendMessage(player, "Card on top of discard pile: " + topCard.toString());
    bool canPlay = false;
    while (!canPlay){
        char buffer[1024];
        int n = recv(client, buffer, 1024, 0);
        if (n < 0) {
            std::cerr << "Error receiving message from player " << player << std::endl;
            return;
        }
        buffer[n] = '\0';
        std::string input(buffer);
        if (input.substr(5,1) == "d"){
            playerHands[player].push_back(drawCard());
            sendMessage(player, "You draw card.");
            canPlay = true;
            continue;
        }
        int cardIndex = std::stoi(input.substr(5)) - 1;
        if ((cardIndex < 0) && (cardIndex >= playerHands[player].size())){
            continue;
        }
        else if (playerHands[player][cardIndex].canPlayOn(topCard)){
            Card currentCard = playerHands[player][cardIndex];
            playerHands[player].erase(playerHands[player].begin() + cardIndex);
            discardPile.push_back(currentCard);
            broadcastMessage("Player " + std::to_string(player) + " played " + currentCard.toString());
            unsigned handSize = playerHands[player].size();
            if (handSize > 1){
                broadcastMessage("Player " + std::to_string(player) + " has still " + std::to_string(handSize) + " cards");
            }
            else if (handSize == 1){
                broadcastMessage("Player has one card left. UNO!");
            }
            canPlay = true;
            activationFlag = true;
            continue;
        }
    }
    topCard = discardPile.back();
    if (topCard.type == Card::Type::REVERSE){
        broadcastMessage("Reverse flow.");
        playOrder *= -1;
    }

    if (playerHands[player].empty()) {
        gameOver = true;
        broadcastMessage("Player " + std::to_string(player) + " won the game!");
    }
}

int main(int argc, char** argv) {

    int server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return 1;
    }
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    if (bind(server, (sockaddr*) &address, sizeof(address)) < 0) {
        std::cerr << "Error binding socket" << std::endl;
        return 1;
    }
    if (listen(server, BACKLOG) < 0) {
        std::cerr << "Error listening on socket" << std::endl;
        return 1;
    }
    std::vector<std::thread> threads;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        sockaddr_in clientAddress;
        socklen_t clientAddressSize = sizeof(clientAddress);
        int client = accept(server, (sockaddr*) &clientAddress, &clientAddressSize);
        if (client < 0) {
            std::cerr << "Error accepting client" << std::endl;
            return 1;
        }
        players.push_back(client);
        threads.emplace_back(messageHandler, client);
    }
    initDeck();
    shuffleDeck();
    dealStartingHand(players.size());
    while (!gameOver) {
        handleTurn(currentPlayer);
        if ((playOrder == 1) && (currentPlayer == players.size() - 1)){
            currentPlayer = 0;
        }
        else if ((playOrder == -1) && (currentPlayer == 0)){
            currentPlayer = players.size() - 1;
        }
        else{
            currentPlayer += playOrder;
        }
    }
    for (auto& thread : threads) {
        thread.join();
    }
    return 0;
}