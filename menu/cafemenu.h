#ifndef CAFE_MENU_H
#define CAFE_MENU_H

#include <vector>
#include "menuitem.h"

/*
 * Deliverable 1 - Default System Data
 *
 * "At least 10 menu items, including at least 3 sandwiches, 4 hot beverages,
 * and 3 cold beverages."
 */
class CafeMenu {
private:
    std::vector<MenuItem> items {
        {"turkey sandwich", "sandwiches", 800, {{"bread", 2, "slices"}, {"turkey", 100, "g"}}},
        {"veggie sandwich", "sandwiches", 700, {{"bread", 2, "slices"}, {"lettuce", 20, "g"},
                                             {"tomato", 40, "g"}, {"cucumber", 30, "g"}}},
        {"grilled cheese", "sandwiches", 600, {{"bread", 2, "slices"}, {"cheese", 50, "g"},
                                            {"butter", 10, "g"}}},
        {"coffee", "hot beverages", 300, {{"ground coffee", 15, "g"}, {"water", 250, "ml"}}},
        {"tea", "hot beverages", 250, {{"tea", 1, "bag"}, {"water", 250, "ml"}}},
        {"latte", "hot beverages", 450, {{"espresso", 30, "ml"}, {"milk", 200, "ml"}}},
        {"hot chocolate", "hot beverages", 350, {{"cocoa mix", 25, "g"}, {"milk", 250, "ml"}}},
        {"iced tea", "cold beverages", 250, {{"brewed tea", 200, "ml"}, {"ice", 80, "g"}}},
        {"iced coffee", "cold beverages", 350, {{"brewed coffee", 200, "ml"}, {"ice", 80, "g"}}},
        {"lemonade", "cold beverages", 300, {{"lemon juice", 50, "ml"}, {"water", 200, "ml"},
                                          {"sugar", 20, "g"}, {"ice", 80, "g"}}}
    };

public:
    const std::vector<MenuItem>& getItems() const {
        //returns a read-only reference to the cafe's default menu items and their recipes
        return items;
    }
};

#endif
