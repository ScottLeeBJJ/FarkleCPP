//Farkle
//Scott Lee
//IT312
//Final Project
//2-22-25

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <algorithm>

// Function to display the rules from a text file
void displayRules() {
    std::ifstream rulesFile("rules.txt");
    if (rulesFile.is_open()) {
        std::string line;
        while (std::getline(rulesFile, line)) {
            std::cout << line << std::endl;
        }
        rulesFile.close();
    }
    else {
        std::cout << "Error: Could not open rules file!" << std::endl;
    }
}

// Function to get the number of players
int getNumberOfPlayers() {
    int players;
    do {
        std::cout << "Enter the number of players (2 or more): ";
        std::cin >> players;
    } while (players < 2);
    return players;
}

// Function to roll a given number of dice
void rollDice(std::vector<int>& dice) {
    for (size_t i = 0; i < dice.size(); i++) {
        dice[i] = rand() % 6 + 1; // Generates a random number between 1 and 6
    }
}

// Function to calculate points based on selected dice
int calculatePoints(const std::vector<int>& dice) {
    int points = 0;
    int counts[6] = { 0 };

    // Count the occurrences of each die face
    for (int die : dice) {
        counts[die - 1]++;
    }

    // Points for 1's and 5's
    points += counts[0] * 100;  // 1's are worth 100 points
    points += counts[4] * 50;   // 5's are worth 50 points

    // Points for three-of-a-kind
    for (int i = 0; i < 6; i++) {
        if (counts[i] >= 3) {
            if (i == 0) {
                points += 1000;  // Three 1's are worth 1000 points
            }
            else {
                points += (i + 1) * 100;  // Three of any other number is worth 100 times the value
            }
        }
    }

    return points;
}

// Function to let the player select dice to keep
std::vector<int> selectDiceToKeep(const std::vector<int>& rolledDice) {
    std::vector<int> keptDice;
    std::cout << "Select dice to keep (separate numbers with spaces, or type 0 to stop rolling): ";
    int die;
    while (std::cin >> die) {
        if (die == 0) break;  // Stop selection
        auto it = std::find(rolledDice.begin(), rolledDice.end(), die);
        if (it != rolledDice.end()) {
            keptDice.push_back(die);
        }
        else {
            std::cout << "Invalid selection. That die is not in your roll.\n";
        }
    }
    std::cin.clear();
    std::cin.ignore(1000, '\n');
    return keptDice;
}

// Function to handle a player's turn
void playTurn(int playerIndex, int& totalScore) {
    int turnPoints = 0;
    bool farkle = false;
    std::vector<int> dice(6, 0);

    while (true) {
        rollDice(dice);
        std::cout << "Player " << playerIndex << " rolled: ";
        for (int die : dice) {
            std::cout << die << " ";
        }
        std::cout << std::endl;

        int rollPoints = calculatePoints(dice);
        if (rollPoints == 0) {
            std::cout << "Farkle! You lost all points this turn.\n";
            farkle = true;
            break;
        }

        // Let player choose dice to keep
        std::vector<int> keptDice = selectDiceToKeep(dice);
        int keptPoints = calculatePoints(keptDice);

        if (keptPoints == 0) {
            std::cout << "Invalid selection. You must keep at least one scoring die.\n";
            continue;
        }

        turnPoints += keptPoints;
        std::cout << "Turn points: " << turnPoints << std::endl;

        if (keptDice.size() == 6) {
            std::cout << "Hot Dice! You must roll all six dice again.\n";
            continue;
        }

        // Ask if the player wants to roll again
        char choice;
        std::cout << "Roll again? (y/n): ";
        std::cin >> choice;
        if (choice != 'y' && choice != 'Y') {
            break;
        }

        // Reduce the number of dice for the next roll
        dice.resize(6 - keptDice.size());
    }

    if (!farkle) {
        totalScore += turnPoints;
        std::cout << "Player " << playerIndex << "'s total score: " << totalScore << std::endl;
    }
}

// Function to check if a player has won
void checkWinner(const std::vector<int>& playerScores) {
    for (size_t i = 0; i < playerScores.size(); i++) {
        if (playerScores[i] >= 10000) {
            std::cout << "Player " << i + 1 << " has won with " << playerScores[i] << " points!\n";
            exit(0);
        }
    }
}

// Main function
int main() {
    srand(static_cast<unsigned int>(time(0)));

    displayRules();

    int numPlayers = getNumberOfPlayers();
    std::vector<int> playerScores(numPlayers, 0);

    while (true) {
        for (int i = 0; i < numPlayers; i++) {
            std::cout << "\nPlayer " << i + 1 << "'s turn\n";
            playTurn(i + 1, playerScores[i]);
            checkWinner(playerScores);
        }
    }

    return 0;
}
