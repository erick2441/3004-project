#ifndef AUTH_SERVICE_H
#define AUTH_SERVICE_H

#include <stdexcept>
#include <string>
#include <vector>
#include "../users/DefaultUser.h"

/*
 * "Users enter their name or username on the startup form; the system verifies
 * the name against the in-memory data structure and displays the appropriate
 * interface based on their user type (Customer, Kitchen Staff, Cashier,
 * Manager, or System Administrator)."
 */
class AuthService {
private:
    std::vector<const DefaultUser*> users;

public:

    void addUser(const DefaultUser& user) {
        if (!user.canLogin()) {
            throw std::invalid_argument("guests use guest checkout.");
        }
        if (user.getName().empty() || user.getUsername().empty()) {
            throw std::invalid_argument("name and username are required.");
        }
        for (const DefaultUser* existing : users) {
            if (existing->getUsername() == user.getUsername()) {
                throw std::invalid_argument("username already exists.");
            }
        }
        users.push_back(&user);
    }

    /*
     *
     * "Users enter their name or username on the startup form; the system
     * verifies the name against the in-memory data structure and displays the
     * appropriate interface based on their user type (Customer, Kitchen Staff,
     * Cashier, Manager, or System Administrator)."
     */
    const DefaultUser* identifyUser(const std::string& nameOrUsername) const {
        const DefaultUser* match = nullptr;
        for (const DefaultUser* user : users) {
            if (user->getUsername() == nameOrUsername ||
                user->getName() == nameOrUsername) {
                if (match) return nullptr;
                match = user;
            }
        }
        return match;
    }
};

#endif
