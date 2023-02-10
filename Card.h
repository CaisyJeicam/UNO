//
// Created by Caisy Jeicam on 10.02.2023.
//

#ifndef UNO_CARD_H
#define UNO_CARD_H


#include <string>

class Card {
public:
    enum class Color {
        RED,
        YELLOW,
        GREEN,
        BLUE,
        WILD,
    };

    enum class Type {
        NUMBER,
        SKIP,
        REVERSE,
        DRAW_TWO,
        WILD,
        WILD_DRAW_FOUR,
    };

    Card(Color color, Type type, int number);

    Color color;
    Type type;
    int number;

    std::string toString() const;
    bool canPlayOn(const Card& other) const;
};


#endif //UNO_CARD_H
