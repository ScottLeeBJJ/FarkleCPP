// Farkle - Enhanced
// Scott Lee / Shiro
// IT312 -> Upgraded for GAM 495 Artifact
// Features: Menu, Balanced Verbose AI, Human manual selection, improved dice logic
// Compile with: g++ -std=c++17 -O2 -o farkle farkle.cpp

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <algorithm>
#include <map>
#include <numeric>
#include <iomanip>

using Dice = std::vector<int>;

// ---------- Rules display ----------
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
        std::cout << "1. Each player rolls 6 dice per turn.\n";
        std::cout << "2. Scoring:\n";
        std::cout << "   - 1s = 100 points\n";
        std::cout << "   - 5s = 50 points\n";
        std::cout << "   - Three-of-a-kind = face value * 100 (Three 1s = 1000 points)\n";
        std::cout << "3. Hot Dice: If all dice scored, roll all six again.\n";
        std::cout << "4. Farkle: No scoring dice loses all points for that turn.\n";
        std::cout << "5. Players may choose to keep scoring dice and re-roll remaining dice.\n";
        std::cout << "6. First player to reach 10,000 points wins.\n\n";
    }
}

// ---------- Basic utilities ----------
void rollDice(std::vector<int>& dice) {
    for (size_t i = 0; i < dice.size(); ++i) {
        dice[i] = rand() % 6 + 1;
    }
}

int calculatePoints(const Dice& dice) {
    if (dice.empty()) return 0;
    int points = 0;
    int counts[6] = { 0 };
    for (int d : dice) counts[d - 1]++;

    // Handle three-of-a-kind (and higher). For counts >= 3, count one triple worth tripleScore.
    for (int i = 0; i < 6; ++i) {
        if (counts[i] >= 3) {
            if (i == 0) points += 1000;               // three 1s
            else points += (i + 1) * 100;            // three of 2..6
            counts[i] -= 3;                          // remove the triple counted
        }
    }

    // Remaining 1s and 5s
    points += counts[0] * 100; // ones
    points += counts[4] * 50;  // fives

    return points;
}

// Helper to convert counts to explicit dice vector (used for making "kept dice" list)
Dice countsToDice(const int counts[6]) {
    Dice d;
    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < counts[i]; ++j) d.push_back(i + 1);
    }
    return d;
}

// ---------- Exact probability / expectation computations (cached) ----------
struct DiceStatsCache {
    std::map<int, double> farkleProb;
    std::map<int, double> expectedPoints;
};

DiceStatsCache statsCache;

// Recursively enumerate all outcomes for N dice and accumulate stats
void enumerateOutcomesRec(int diceLeft, std::vector<int>& current, long long& totalCombos,
    long long& zeroScoreCombos, long long& totalPoints) {
    if (diceLeft == 0) {
        ++totalCombos;
        int pts = calculatePoints(current);
        totalPoints += pts;
        if (pts == 0) ++zeroScoreCombos;
        return;
    }
    for (int face = 1; face <= 6; ++face) {
        current.push_back(face);
        enumerateOutcomesRec(diceLeft - 1, current, totalCombos, zeroScoreCombos, totalPoints);
        current.pop_back();
    }
}

void computeDiceStats(int diceCount) {
    if (statsCache.farkleProb.count(diceCount)) return;
    long long totalCombos = 0, zeroScoreCombos = 0, totalPoints = 0;
    std::vector<int> current;
    enumerateOutcomesRec(diceCount, current, totalCombos, zeroScoreCombos, totalPoints);
    double farkle = (double)zeroScoreCombos / (double)totalCombos;
    double expected = (double)totalPoints / (double)totalCombos;
    statsCache.farkleProb[diceCount] = farkle;
    statsCache.expectedPoints[diceCount] = expected;
}

// ---------- AI selection policy (B): keep 1s, 5s, triples ----------
Dice aiChooseDiceToKeep(const Dice& rolledDice) {
    int counts[6] = { 0 };
    for (int d : rolledDice) counts[d - 1]++;

    int keepCounts[6] = { 0 };

    // Keep triples (only one triple per face; higher counts produce triple + extras handled below)
    for (int i = 0; i < 6; ++i) {
        if (counts[i] >= 3) {
            keepCounts[i] += 3;
            counts[i] -= 3;
        }
    }
    // Keep remaining 1s and 5s
    keepCounts[0] += counts[0]; // ones
    keepCounts[4] += counts[4]; // fives

    return countsToDice(keepCounts);
}

// ---------- Human selection (unchanged behavior, select by face value) ----------
Dice selectDiceToKeepHuman(const Dice& rolledDice) {
    Dice keptDice;
    std::cout << "Select dice to keep by value (separate numbers with spaces), type 0 when done: ";
    int die;
    // show current available dice as values (not indices)
    std::cout << "\nRoll contains: ";
    for (int d : rolledDice) std::cout << d << ' ';
    std::cout << '\n';

    std::vector<int> available = rolledDice; // we'll remove used instances as selected
    while (std::cin >> die) {
        if (die == 0) break;
        auto it = std::find(available.begin(), available.end(), die);
        if (it != available.end()) {
            keptDice.push_back(die);
            available.erase(it);
        }
        else {
            std::cout << "Invalid selection. That die value is not available (or already used). Try again.\n";
        }
    }
    std::cin.clear();
    std::cin.ignore(1000, '\n');
    return keptDice;
}

// ---------- AI decision: balanced logic ----------
bool aiShouldRollAgain(int diceRemaining, int turnPoints, int keptPointsThisRoll) {
    // compute stats if not cached
    computeDiceStats(diceRemaining);
    double farkleChance = statsCache.farkleProb[diceRemaining];
    double expectedImmediate = statsCache.expectedPoints[diceRemaining];

    // Estimate expected gain from attempting another roll:
    double expectedGain = (1.0 - farkleChance) * expectedImmediate;

    // Decision thresholds (dynamic)
    if (turnPoints < 300) {
        return true; // aggressive when low turn score
    }

    if (turnPoints >= 1000) {
        return false; // bank large turn points
    }

    // thresholds scale with current turnPoints
    double threshold;
    if (turnPoints < 600) threshold = 150.0;
    else threshold = 100.0;

    // also if expectedGain is reasonably large relative to keptPointsThisRoll, prefer rolling
    double relative = expectedGain - keptPointsThisRoll * 0.25;

    // Balanced decision
    bool decision = (expectedGain > threshold) || (relative > 0 && expectedGain > 100.0);

    // small nudges: if diceRemaining == 6 and turnPoints == 0, allow rolling (first roll)
    if (diceRemaining == 6 && turnPoints == 0) decision = true;

    return decision;
}

// ---------- Play turn (handles human or AI) ----------
void playTurn(int playerIndex, int& totalScore, bool isAI) {
    int turnPoints = 0;
    bool farkleHappened = false;
    std::vector<int> dice(6, 0);

    while (true) {
        rollDice(dice);
        std::cout << "\nPlayer " << playerIndex << " rolled: ";
        for (int d : dice) std::cout << d << ' ';
        std::cout << '\n';

        int rollPoints = calculatePoints(dice);
        if (rollPoints == 0) {
            std::cout << "Farkle! No scoring dice. You lose all points for this turn.\n";
            farkleHappened = true;
            break;
        }

        Dice keptDice;
        if (!isAI) {
            // Human selection (unchanged)
            keptDice = selectDiceToKeepHuman(dice);
        }
        else {
            // AI selection policy B
            keptDice = aiChooseDiceToKeep(dice);
            int keptPts = calculatePoints(keptDice);
            std::cout << "AI keeps: ";
            if (keptDice.empty()) std::cout << "(none)";
            else {
                for (int d : keptDice) std::cout << d << ' ';
            }
            std::cout << " -> keptPoints this choice: " << keptPts << '\n';
        }

        int keptPoints = calculatePoints(keptDice);
        if (keptPoints == 0) {
            if (!isAI) {
                std::cout << "You must keep at least one scoring die. Try again.\n";
                continue;
            }
            else {
                // AI didn't find any scoring dice (shouldn't happen because we checked rollPoints), but handle gracefully
                std::cout << "AI couldn't identify scoring dice (unexpected). It will bank current points.\n";
                break;
            }
        }

        turnPoints += keptPoints;
        std::cout << "Turn points (so far): " << turnPoints << '\n';

        // Hot dice (all dice scoring)
        if ((int)keptDice.size() == 6) {
            std::cout << "Hot Dice! All dice scored. You will roll all six dice again automatically.\n";
            dice.assign(6, 0); // roll full set next loop
            continue;
        }

        // If human, ask choice to roll again
        if (!isAI) {
            char choice;
            std::cout << "Roll again? (y/n): ";
            std::cin >> choice;
            std::cin.ignore(1000, '\n');
            if (choice != 'y' && choice != 'Y') {
                break;
            }
            // reduce dice to roll for next iteration
            int newSize = 6 - (int)keptDice.size();
            if (newSize <= 0) newSize = 6;
            dice.resize(newSize);
            continue;
        }
        else {
            // AI decision, verbose output with probabilities
            int diceRemaining = 6 - (int)keptDice.size();
            if (diceRemaining <= 0) diceRemaining = 6;

            computeDiceStats(diceRemaining);
            double farkleChance = statsCache.farkleProb[diceRemaining];
            double expectedImmediate = statsCache.expectedPoints[diceRemaining];

            std::cout << std::fixed << std::setprecision(3);
            std::cout << "AI analysis: diceRemaining=" << diceRemaining
                << ", farkleChance=" << farkleChance
                << ", expectedImmediate=" << expectedImmediate << '\n';

            bool rollAgain = aiShouldRollAgain(diceRemaining, turnPoints, keptPoints);

            if (rollAgain) {
                std::cout << "AI decides to roll again.\n";
                // reduce dice for next roll
                int newSize = 6 - (int)keptDice.size();
                if (newSize <= 0) newSize = 6;
                dice.resize(newSize);
                continue;
            }
            else {
                std::cout << "AI decides to bank points this turn.\n";
                break;
            }
        }
    } // end while

    if (!farkleHappened) {
        totalScore += turnPoints;
        std::cout << "Player " << playerIndex << " banks " << turnPoints << " points. Total: " << totalScore << "\n";
    }
    else {
        std::cout << "Player " << playerIndex << "'s total remains: " << totalScore << "\n";
    }
}

// ---------- Winner check ----------
bool checkWinner(const std::vector<int>& playerScores) {
    for (size_t i = 0; i < playerScores.size(); ++i) {
        if (playerScores[i] >= 10000) {
            std::cout << "\n====== GAME OVER ======\n";
            std::cout << "Player " << i + 1 << " has won with " << playerScores[i] << " points!\n";
            return true;
        }
    }
    return false;
}

// ---------- Menu and setup ----------
int menu() {
    std::cout << "\n====== Farkle ======\n";
    std::cout << "1. Play Game\n";
    std::cout << "2. View Rules\n";
    std::cout << "3. Exit\n";
    std::cout << "Choose an option: ";
    int opt;
    std::cin >> opt;
    std::cin.ignore(1000, '\n');
    return opt;
}

void setupPlayers(int& humanCount, int& aiCount) {
    do {
        std::cout << "Enter number of human players (0 or more): ";
        std::cin >> humanCount;
    } while (humanCount < 0);

    do {
        std::cout << "Enter number of AI players (0 or more): ";
        std::cin >> aiCount;
    } while (aiCount < 0);

    if (humanCount + aiCount < 2) {
        std::cout << "Total players must be at least 2. Adding AI players to reach 2.\n";
        if (aiCount == 0) aiCount = 2 - humanCount;
    }
}

// ---------- Main ----------
int main() {
    srand(static_cast<unsigned int>(time(nullptr)));

    while (true) {
        int opt = menu();
        if (opt == 1) {
            displayRules();
            int humanCount = 0, aiCount = 0;
            setupPlayers(humanCount, aiCount);
            int totalPlayers = humanCount + aiCount;
            std::vector<int> playerScores(totalPlayers, 0);
            std::vector<bool> isAI(totalPlayers, false);
            for (int i = 0; i < totalPlayers; ++i) {
                if (i < humanCount) isAI[i] = false;
                else isAI[i] = true;
            }

            std::cout << "\nStarting game with " << humanCount << " human(s) and " << aiCount << " AI(s).\n";

            // Precompute dice stats cache for 1..6 dice
            for (int n = 1; n <= 6; ++n) computeDiceStats(n);

            bool gameOver = false;
            while (!gameOver) {
                for (int i = 0; i < totalPlayers; ++i) {
                    std::cout << "\n--- Player " << (i + 1) << (isAI[i] ? " (AI)" : " (Human)") << " turn ---\n";
                    playTurn(i + 1, playerScores[i], isAI[i]);
                    if (checkWinner(playerScores)) {
                        gameOver = true;
                        break;
                    }
                }
            }
        }
        else if (opt == 2) {
            displayRules();
        }
        else if (opt == 3) {
            std::cout << "Goodbye.\n";
            break;
        }
        else {
            std::cout << "Invalid option.\n";
        }
    }

    return 0;
}
