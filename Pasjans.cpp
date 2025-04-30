#include <iostream> 
#include <vector>
#include <stack>
#include <queue>
#include <algorithm>
#include <random>
#include <ctime>
#include <sstream>
#include <string>
#include <thread>

using namespace std;

class Suit {
public:
    static const int CLUBS = 0;
    static const int DIAMONDS = 1;
    static const int HEARTS = 2;
    static const int SPADES = 3;
};

class Card {
private:
    int rank;
    int suit;
    bool faceUp;
public:
    Card(int r, int s, bool faceUp = false) : rank(r), suit(s), faceUp(faceUp) {}
    int getRank() const { return rank; }
    int getSuit() const { return suit; }
    bool isFaceUp() const { return faceUp; }
    void flip() { faceUp = !faceUp; }
    void setFaceUp(bool value) { faceUp = value; }
    string toString() const {
        if (!faceUp) return "===";
        string rankStr;
        switch (rank) {
        case 1: rankStr = "A"; break;
        case 11: rankStr = "J"; break;
        case 12: rankStr = "Q"; break;
        case 13: rankStr = "K"; break;
        default: rankStr = to_string(rank);
        }
        char suitChar = "CDHS"[suit];
        return rankStr + suitChar;
    }
};

class Deck {
private:
    queue<Card> stock;
    stack<Card> waste;
public:
    Deck() {}
    void initialize();
    void drawCard();
    bool stockEmpty() const { return stock.empty(); }
    Card getWasteTop() const { return waste.top(); }
    void popWasteTop() { waste.pop(); }
};

void Deck::initialize() {
    vector<Card> allCards;
    for (int r = 1; r <= 13; ++r) {
        allCards.emplace_back(r, Suit::CLUBS);
        allCards.emplace_back(r, Suit::DIAMONDS);
        allCards.emplace_back(r, Suit::HEARTS);
        allCards.emplace_back(r, Suit::SPADES);
    }
    shuffle(allCards.begin(), allCards.end(), mt19937(static_cast<unsigned int>(time(0))));
    for (auto& card : allCards) {
        stock.push(card);
    }
}

void Deck::drawCard() {
    if (stock.empty()) return;
    Card card = stock.front();
    stock.pop();
    card.setFaceUp(true);
    waste.push(card);
}

class Game {
private:
    Deck deck;
    vector<vector<Card>> tableau;
    stack<Card> foundations[4];
    vector<Card> randomCards;

public:
    Game() : tableau(7) {}
    void initGame();
    void display() const;
    void generateRandomCards();
    void run();

    bool moveCardToFoundation(int fromColumn, const string& cardStr);
    bool moveTopWasteToColumn(int column);
    bool isValidMove(const Card& fromCard, const Card& toCard) const;
    bool moveWasteCardToFoundation(const string& cardStr, int foundationIndex);
};

void Game::initGame() {
    deck.initialize();
    for (int i = 0; i < 7; ++i) {
        for (int j = 0; j <= i; ++j) {
            deck.drawCard();
            Card card = deck.getWasteTop();
            deck.popWasteTop();

            card.setFaceUp(false);
            tableau[i].push_back(card);
        }
        if (!tableau[i].empty()) {
            tableau[i].back().setFaceUp(true);
        }
    }
    generateRandomCards();
}

string padCard(const string& card) {
    if (card.length() == 2) return "  " + card + "  ";
    if (card.length() == 3) return " " + card + "  ";
    return card;
}

void Game::generateRandomCards() {
    static vector<Card> savedCards;
    static size_t currentIndex = 0;

    if (deck.stockEmpty() && currentIndex >= savedCards.size()) {
        shuffle(savedCards.begin(), savedCards.end(), mt19937(static_cast<unsigned int>(time(0))));

        while (!savedCards.empty()) {
            Card card = savedCards.back();
            card.setFaceUp(false);
            deck.drawCard();
            deck.popWasteTop();
            deck.drawCard();
            deck.popWasteTop();

            savedCards.pop_back();
        }

        currentIndex = 0;
        savedCards.clear();
    }

    if (deck.stockEmpty()) {

        if (currentIndex < savedCards.size()) {
            randomCards.clear();
            for (int i = 0; i < 3 && currentIndex < savedCards.size(); ++i, ++currentIndex) {
                randomCards.push_back(savedCards[currentIndex]);
            }
        }
        return;
    }

    randomCards.clear();
    for (int i = 0; i < 3; ++i) {
        if (!deck.stockEmpty()) {
            deck.drawCard();
            Card card = deck.getWasteTop();
            deck.popWasteTop();

            card.setFaceUp(true);
            randomCards.push_back(card);
            savedCards.push_back(card);
        }
    }

    currentIndex = savedCards.size();
}


bool Game::isValidMove(const Card& fromCard, const Card& toCard) const {
    bool result = false;
    thread t([&]() {
        bool fromRed = (fromCard.getSuit() == Suit::DIAMONDS || fromCard.getSuit() == Suit::HEARTS);
        bool toRed = (toCard.getSuit() == Suit::DIAMONDS || toCard.getSuit() == Suit::HEARTS);
        result = (fromRed != toRed) && (fromCard.getRank() == toCard.getRank() - 1);
        });

    t.join();
    return result;
}

bool Game::moveCardToFoundation(int fromColumn, const string& cardStr) {
    if (fromColumn < 1 || fromColumn > 7) return false;
    fromColumn--;

    if (tableau[fromColumn].empty()) return false;

    Card& topCard = tableau[fromColumn].back();
    if (!topCard.isFaceUp()) return false;

    if (topCard.toString() != cardStr) return false;

    int suit = topCard.getSuit();
    int foundationIndex = suit;

    if (foundations[foundationIndex].empty()) {
        if (topCard.getRank() == 1) {
            foundations[foundationIndex].push(topCard);
            tableau[fromColumn].pop_back();
            if (!tableau[fromColumn].empty() && !tableau[fromColumn].back().isFaceUp())
                tableau[fromColumn].back().flip();
            return true;
        }
        return false;
    }

    if (topCard.getRank() == foundations[foundationIndex].top().getRank() + 1 &&
        topCard.getSuit() == foundations[foundationIndex].top().getSuit()) {
        foundations[foundationIndex].push(topCard);
        tableau[fromColumn].pop_back();
        if (!tableau[fromColumn].empty() && !tableau[fromColumn].back().isFaceUp())
            tableau[fromColumn].back().flip();
        return true;
    }

    return false;
}

bool Game::moveWasteCardToFoundation(const string& cardStr, int foundationIndex) {
    if (foundationIndex < 0 || foundationIndex > 3) return false;

    auto it = find_if(randomCards.begin(), randomCards.end(), [&](const Card& c) {
        return c.toString() == cardStr;
        });

    if (it == randomCards.end()) return false;

    Card card = *it;

    if (card.getSuit() != foundationIndex) return false;

    if (foundations[foundationIndex].empty()) {
        if (card.getRank() == 1) {
            foundations[foundationIndex].push(card);
            randomCards.erase(it);
            return true;
        }
        return false;
    }

    if (card.getRank() == foundations[foundationIndex].top().getRank() + 1) {
        foundations[foundationIndex].push(card);
        randomCards.erase(it);
        return true;
    }

    return false;
}

bool Game::moveTopWasteToColumn(int column) {
    if (randomCards.empty()) return false;
    if (column < 1 || column > 7) return false;
    column--;

    Card topWaste = randomCards.back();

    if (tableau[column].empty()) {
        if (topWaste.getRank() == 13) {
            tableau[column].push_back(topWaste);
            randomCards.pop_back();
            return true;
        }
        else {
            return false;
        }
    }

    Card& topTarget = tableau[column].back();
    if (topTarget.isFaceUp() && isValidMove(topWaste, topTarget)) {
        tableau[column].push_back(topWaste);
        randomCards.pop_back();
        return true;
    }

    return false;
}

void Game::display() const {
    cout << "        0                       8    9    10    11\n";
    cout << "--------------------------------------------------\n";
    for (const auto& card : randomCards) {
        cout << padCard(card.toString());
    }
    cout << "             ";
    for (int i = 0; i < 4; ++i) {
        if (foundations[i].empty()) {
            cout << "  0  ";
        }
        else {
            cout << padCard(foundations[i].top().toString());
        }
    }
    cout << "\n--------------------------------------------------\n";

    int maxRows = 0;
    for (const auto& pile : tableau) {
        if ((int)pile.size() > maxRows) maxRows = pile.size() + 4;
    }
    for (int row = 0; row < maxRows; ++row) {
        for (int col = 0; col < 7; ++col) {
            cout << "|";
            if (row < (int)tableau[col].size()) {
                cout << padCard(tableau[col][row].toString());
            }
            else {
                cout << "      ";
            }
        }
        cout << "|\n";
    }
    cout << "--------------------------------------------------\n";
    cout << "   1      2      3      4      5      6      7\n";
}

void Game::run() {
    initGame();
    display();
    string line;

    while (true) {
        cout << ">> ";
        getline(cin, line);
        if (line.empty()) continue;

        if (line == "q") break;

        if (line == "0") {
            if (!deck.stockEmpty()) {
                generateRandomCards();
            }
            display();
            continue;
        }

        istringstream iss(line);
        int from, to;
        string cardStr;
        if (iss >> from >> cardStr >> to) {
            if (from == 0) {
                if (to >= 9 && to <= 12) {
                    if (moveWasteCardToFoundation(cardStr, to - 8)) {
                        display();
                    }
                    else {
                        cout << "Nie można przenieść tej karty z talii pomocniczej do fundamentu.\n";
                    }
                    continue;
                }

                auto it = find_if(randomCards.begin(), randomCards.end(), [&](const Card& c) {
                    return c.toString() == cardStr;
                    });

                if (it == randomCards.end()) {
                    cout << "Nie znaleziono takiej karty w talii pomocniczej.\n";
                    continue;
                }

                if (to < 1 || to > 7) {
                    cout << "Nieprawidłowy numer kolumny docelowej.\n";
                    continue;
                }

                Card topWaste = *it;
                int targetColumn = to - 1;

                if (tableau[targetColumn].empty()) {
                    if (topWaste.getRank() == 13) {
                        tableau[targetColumn].push_back(topWaste);
                        randomCards.erase(it);
                        display();
                    }
                    else {
                        cout << "Można przenieść tylko króla na pustą kolumnę.\n";
                    }
                    continue;
                }

                Card& targetCard = tableau[targetColumn].back();
                if (targetCard.isFaceUp() && isValidMove(topWaste, targetCard)) {
                    tableau[targetColumn].push_back(topWaste);
                    randomCards.erase(it);
                    display();
                }
                else {
                    cout << "Nieprawidłowy ruch zgodnie z zasadami.\n";
                }
                continue;
            }

            if (to >= 9 && to <= 12) {
                if (moveCardToFoundation(from, cardStr)) {
                    display();
                }
                else {
                    cout << "Nie można przenieść tej karty do fundamentu.\n";
                }
                continue;
            }

            from--; to--;
            if (from < 0 || from >= 7 || to < 0 || to >= 7) {
                cout << "Nieprawidłowy numer kolumny.\n";
                continue;
            }

            auto& sourceCol = tableau[from];
            auto it = find_if(sourceCol.begin(), sourceCol.end(), [&](const Card& c) {
                return c.toString() == cardStr;
                });

            if (it == sourceCol.end() || !it->isFaceUp()) {
                cout << "Nie znaleziono odkrytej karty o nazwie " << cardStr << " w kolumnie " << from + 1 << ".\n";
                continue;
            }

            vector<Card> movingCards(it, sourceCol.end());

            if (tableau[to].empty()) {
                if (movingCards[0].getRank() == 13) {
                    tableau[to].insert(tableau[to].end(), movingCards.begin(), movingCards.end());
                    sourceCol.erase(it, sourceCol.end());
                }
                else {
                    cout << "Można przenieść tylko króla na pustą kolumnę.\n";
                    continue;
                }
            }
            else {
                Card& targetCard = tableau[to].back();
                if (!targetCard.isFaceUp()) {
                    cout << "Docelowa karta nie jest odkryta.\n";
                    continue;
                }
                if (!isValidMove(movingCards[0], targetCard)) {
                    cout << "Nieprawidłowy ruch zgodnie z zasadami.\n";
                    continue;
                }

                tableau[to].insert(tableau[to].end(), movingCards.begin(), movingCards.end());
                sourceCol.erase(it, sourceCol.end());
            }

            if (!sourceCol.empty() && !sourceCol.back().isFaceUp())
                sourceCol.back().flip();

            display();
        }
        else {
            cout << "Niepoprawny format. Użyj np.: 2 10h 4 lub 3 4h 10 (do fundamentu).\n";
        }
    }
}

int main() {
    Game game;
    game.run();
    return 0;
}