#ifndef ORDER_ITEM_H
#define ORDER_ITEM_H

#include <stdexcept>
#include <string>
#include <vector>
#include "../menu/IngredientRequirement.h"

/*
 *
 * "Each order record must include the order source (online or in-person),
 * items and quantities, tax, final total, loyalty points used or earned,
 * account credit used, payment status, and current order status."
 */
class OrderItem {
private:
    std::string name;
    int unitPriceCents;
    int quantity;
    std::vector<IngredientRequirement> ingredients;

public:
    OrderItem(std::string name, int unitPriceCents, int quantity = 1,
              const std::vector<IngredientRequirement>& ingredients = {})
        : name(name), unitPriceCents(unitPriceCents), quantity(quantity), ingredients(ingredients) {
        //This checks wether name is empty, has spaces in front,
        if (this->name.find_first_not_of(" \t\r\n") == std::string::npos) {
            throw std::invalid_argument("item name must not be blank.");
        }
        if (unitPriceCents < 0) {
            throw std::invalid_argument("unit price cannot be negative.");
        }
        if (quantity <= 0) {
            throw std::invalid_argument("quantity must be positive.");
        }
    }

    const std::string& getName() const { return name; }
    int getUnitPriceCents() const { return unitPriceCents; }
    int getQuantity() const { return quantity; }
    const std::vector<IngredientRequirement>& getIngredients() const { return ingredients; }

    
    int getLineTotalCents() const {
        return unitPriceCents * quantity;
    }
};

#endif
