//
// Created by Caisy Jeicam on 10.02.2023.
//

#include "Card.h"
#include <string>




Card::Card(Color color, Type type, int number = -1)
            : color(color)
            , type(type)
            , number(number)
    {}

std::string Card::toString() const {
    std::string result = "";
    switch (color) {
        case Color::RED:
            result += "Red ";
            break;
        case Color::YELLOW:
            result += "Yellow ";
            break;
        case Color::GREEN:
            result += "Green ";
            break;
        case Color::BLUE:
            result += "Blue ";
            break;
        case Color::WILD:
            result += "Wild ";
            break;
    }
    switch (type) {
        case Type::NUMBER:
            result += std::to_string(number);
            break;
        case Type::SKIP:
            result += "Skip";
            break;
        case Type::REVERSE:
            result += "Reverse";
            break;
        case Type::DRAW_TWO:
            result += "Draw Two";
            break;
        case Type::WILD:
            result += "";
            break;
        case Type::WILD_DRAW_FOUR:
            result += "Draw Four";
            break;
    }
    return result;
}

bool Card::canPlayOn(const Card& other) const {
    if (color == Color::WILD || other.color == Color::WILD) {
        return true;
    }
    return color == other.color || type == other.type;
}


