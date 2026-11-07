#ifndef MENU_ITEM_H
#define MENU_ITEM_H

#include <stdexcept>
#include <string>
#include <vector>
#include "IngredientRequirement.h"

/*
 * Deliverable 1 - Default System Data
 *
 * "Each menu item must have a name, category, and price."
 */
class MenuItem {
private:
    std::string name;
    std::string category;
    int priceCents;
    std::vector<IngredientRequirement> ingredients;

public:
    MenuItem(std::string itemName, std::string itemCategory, int itemPriceCents,
             const std::vector<IngredientRequirement>& recipe)
        : name(itemName), category(itemCategory), priceCents(itemPriceCents), ingredients(recipe) {
        bool nameIsBlank = name.find_first_not_of(" \t\r\n") == std::string::npos;
        bool categoryIsBlank = category.find_first_not_of(" \t\r\n") == std::string::npos;

        if (nameIsBlank) {
            throw std::invalid_argument("menu item name cannot be blank.");
        }

        if (categoryIsBlank) {
            throw std::invalid_argument("menu item category cannot be blank.");
        }

        if (priceCents < 0) {
            throw std::invalid_argument("menu item price cannot be negative.");
        }
    }

    const std::string& getName() const { return name; }
    const std::string& getCategory() const { return category; }
    int getPriceCents() const { return priceCents; }
    const std::vector<IngredientRequirement>& getIngredients() const { return ingredients; }
};

#endif
