#ifndef INGREDIENT_REQUIREMENT_H
#define INGREDIENT_REQUIREMENT_H

#include <string>

struct IngredientRequirement {
    std::string name;
    int quantity;
    std::string unit;
};

#endif
