#ifndef DEFAULT_USERS_H
#define DEFAULT_USERS_H

#include <ctime>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "admin.h"
#include "authservice.h"
#include "cafemenu.h"
#include "cashier.h"
#include "customer.h"
#include "defaultuser.h"
#include "kitchenstaff.h"
#include "manager.h"
#include "menuitem.h"
#include "order.h"
#include "orderitem.h"
#include "orderservice.h"

class DefaultUsers {
private:
    std::vector<std::unique_ptr<DefaultUser>> users;

    static std::time_t makeTimestamp(int day, int hour, int minute = 0) {
        //builds a local timestamp for the supplied October 2026 date and rejects invalid dates
        std::tm time = {};
        time.tm_year = 2026 - 1900;
        time.tm_mon = 10 - 1;
        time.tm_mday = day;
        time.tm_hour = hour;
        time.tm_min = minute;
        time.tm_isdst = -1;
        const std::time_t result = std::mktime(&time);
        if (result == static_cast<std::time_t>(-1)) {
            throw std::invalid_argument("invalid default order date.");
        }
        return result;
    }

    static OrderItem makeItem(const CafeMenu& menu, const std::string& name) {
        //finds a menu entry by name and creates a single-quantity historical order item from it
        for (const MenuItem& item : menu.getItems()) {
            if (item.getName() == name) {
                return OrderItem(item.getName(), item.getPriceCents(), 1,
                                 item.getIngredients());
            }
        }
        throw std::invalid_argument("default history menu item not found: " + name);
    }

    static std::vector<Order> makeHistory(
        const CafeMenu& menu, const Customer& customer,
        int firstOrderNumber, int variation) {
        //builds five chronological orders with payments and benefits tied to the supplied customer
        typedef Order::Status Status;
        typedef Order::PurchaseType PurchaseType;

        const std::string& username = customer.getUsername();
        const std::string& name = customer.getName();

        const char* sandwiches[] = {
            "turkey sandwich", "veggie sandwich", "grilled cheese"
        };
        const char* hotDrinks[] = {
            "coffee", "tea", "latte", "hot chocolate"
        };
        const char* coldDrinks[] = {
            "iced tea", "iced coffee", "lemonade"
        };

        return {
            Order(firstOrderNumber, username, name,
                {makeItem(menu, sandwiches[variation % 3]),
                 makeItem(menu, hotDrinks[variation % 4])},
                Status::PickedUp, PurchaseType::Online, makeTimestamp(1, 9),
                0, 0, OrderService::TAX_RATE_PERCENT),

            Order(firstOrderNumber + 1, username, name,
                {makeItem(menu, sandwiches[(variation + 1) % 3]),
                 makeItem(menu, coldDrinks[variation % 3])},
                Status::Cancelled, PurchaseType::Online, makeTimestamp(5, 10),
                0, 0, OrderService::TAX_RATE_PERCENT),

            Order(firstOrderNumber + 2, username, name,
                {makeItem(menu, sandwiches[(variation + 2) % 3]),
                 makeItem(menu, hotDrinks[(variation + 1) % 4])},
                Status::Cancelled, PurchaseType::Online, makeTimestamp(10, 11),
                1, 200, OrderService::TAX_RATE_PERCENT),

            Order(firstOrderNumber + 3, username, name,
                {makeItem(menu, sandwiches[variation % 3]),
                 makeItem(menu, coldDrinks[(variation + 1) % 3])},
                Status::PickedUp, PurchaseType::InPerson, makeTimestamp(15, 12),
                0, 500, OrderService::TAX_RATE_PERCENT),

            Order(firstOrderNumber + 4, username, name,
                {makeItem(menu, sandwiches[(variation + 1) % 3]),
                 makeItem(menu, hotDrinks[(variation + 2) % 4])},
                Status::PickedUp, PurchaseType::Online, makeTimestamp(20, 13),
                2, 0, OrderService::TAX_RATE_PERCENT)
        };
    }

    void addCustomer(const CafeMenu& menu, const std::string& name,
                     const std::string& username, const std::string& loyaltyNumber,
                     const std::string& email, const std::string& address,
                     int firstOrderNumber, int variation) {
        //creates a customer, initializes their order history and balances, and stores their ownership
        std::unique_ptr<Customer> customer(new Customer(
            name, username, loyaltyNumber, email, address));

        const std::vector<Order> history =
            makeHistory(menu, *customer, firstOrderNumber, variation);

        customer->initializeOrderHistory(history);
        users.push_back(std::move(customer));
    }

public:
    DefaultUsers() : DefaultUsers(CafeMenu()) {
        //loads the default accounts using a newly constructed cafe menu
    }

    explicit DefaultUsers(const CafeMenu& menu) {
        //creates the five customers with seeded histories and the six staff accounts
        users.reserve(11);

        addCustomer(menu, "Aria Stone", "aria", "LP1001",
                    "aria@example.com", "10 Example Lane, Hintonville",
                    1001, 0);
        addCustomer(menu, "Aaron Carter", "aaron", "LP1002",
                    "aaron@example.com", "20 Example Lane, Hintonville",
                    1006, 1);
        addCustomer(menu, "Astrid Martin", "astrid", "LP1003",
                    "astrid@example.com", "30 Example Lane, Hintonville",
                    1011, 2);
        addCustomer(menu, "Alex Rivera", "alex", "LP1004",
                    "alex@example.com", "40 Example Lane, Hintonville",
                    1016, 3);
        addCustomer(menu, "Allen Brooks", "allen", "LP1005",
                    "allen@example.com", "50 Example Lane, Hintonville",
                    1021, 4);

        users.emplace_back(new KitchenStaff("Timmy Tortoise", "timmy", "KS1001"));
        users.emplace_back(new KitchenStaff("Morgan Lee", "morgan", "KS1002"));

        users.emplace_back(new Cashier("Harold the Hare", "harold", "CS1001"));
        users.emplace_back(new Cashier("Jamie Chen", "jamie", "CS1002"));

        users.emplace_back(new Manager("Priya Shah", "priya", "MG1001"));
        users.emplace_back(new Admin("Avery Quinn", "avery", "AD1001"));
    }

    DefaultUsers(const DefaultUsers&) = delete;
    DefaultUsers& operator=(const DefaultUsers&) = delete;
    DefaultUsers(DefaultUsers&&) = delete;
    DefaultUsers& operator=(DefaultUsers&&) = delete;

    const std::vector<std::unique_ptr<DefaultUser>>& getUsers() const {
        //returns a read-only reference to the collection that owns the default user accounts
        return users;
    }

    std::vector<Order> getOrderHistory(
        const std::string& username
    ) const
    {
        //returns the matching customer's historical orders or an empty collection when unavailable
        for (const auto& user : users) {
            if (user->getUsername() == username) {
                const Customer* customer =
                    dynamic_cast<const Customer*>(user.get());

                if (customer) {
                    return customer->getOrderHistory();
                }

                break;
            }
        }

        return {};
    }

    void registerWith(AuthService& authService, OrderService& orderService) {
        //registers all owned accounts with authentication and their customer orders with the order service
        for (const auto& user : users) {
            authService.addUser(*user);

            Customer* customer = dynamic_cast<Customer*>(user.get());
            if (customer) {
                orderService.registerCustomer(*customer);
            }
        }
    }
};

#endif
