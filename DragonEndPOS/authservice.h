#ifndef AUTH_SERVICE_H
#define AUTH_SERVICE_H

#include <stdexcept>
#include <string>
#include <vector>

#include "defaultuser.h"
#include "customer.h"

class AuthService {
private:
    std::vector<DefaultUser*> users;
    std::string currentUsername;
    std::string currentRole;
    unsigned long long sessionId = 0;

    static std::string trimContactText(const std::string& text) {
        //removes leading and trailing whitespace from a contact-information field
        const std::string whitespace = " \t\r\n\f\v";
        const std::size_t first = text.find_first_not_of(whitespace);
        if (first == std::string::npos) return "";
        const std::size_t last = text.find_last_not_of(whitespace);
        return text.substr(first, last - first + 1);
    }

    Customer& currentCustomerForUpdate() {
        //finds the signed-in customer for editing and rejects missing or non-customer sessions
        if (currentUsername.empty() || currentRole != "customer") {
            throw std::logic_error(
                "Sign in as a customer to update contact information."
            );
        }
        for (DefaultUser* user : users) {
            if (user->getUsername() == currentUsername) {
                Customer* customer = dynamic_cast<Customer*>(user);
                if (customer) return *customer;
                break;
            }
        }
        throw std::logic_error("The signed-in customer could not be found.");
    }

public:
    void addUser(DefaultUser& user) {
        //registers an editable account after checking login eligibility, required identity, and uniqueness
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

    const DefaultUser* identifyUser(const std::string& nameOrUsername) const {
        //returns a unique name or username match, or null when the match is missing or ambiguous
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

    std::string identifyRole(const std::string& nameOrUsername) const {
        //returns the matching account's role or an empty string when identification fails
        const DefaultUser* match = identifyUser(nameOrUsername);
        return match ? match->getRole() : "";
    }

    struct UserInfo {
        std::string name;
        std::string username;
        std::string role;
    };

    UserInfo getCurrentUserInfo() const {
        //builds name, username, and role display data for the currently signed-in account
        if (currentUsername.empty()) {
            throw std::logic_error("No user is logged in.");
        }
        for (const DefaultUser* user : users) {
            if (user->getUsername() == currentUsername) {
                UserInfo info;
                info.name = user->getName();
                info.username = user->getUsername();
                info.role = user->getRole();
                return info;
            }
        }
        throw std::logic_error("The signed-in user could not be found.");
    }

    bool isKitchenStaff() const {
        //checks whether a signed-in account has the kitchen staff role
        return !currentUsername.empty() && currentRole == "kitchen staff";
    }

    struct CustomerInfo {
        std::string name;
        std::string email;
        std::string homeAddress;
        std::string loyaltyNumber;
        int loyaltyPoints = 0;
        int accountCreditCents = 0;
    };

    struct OrderHistoryInfo {
        int orderNumber = 0;
        std::string placedAt;
        std::string source;
        std::string status;
        int finalTotalCents = 0;
    };

    std::vector<OrderHistoryInfo> getCurrentCustomerOrderHistory() const {
        //builds display data for up to five recent picked-up or cancelled customer orders
        if (currentUsername.empty()) {
            throw std::logic_error("No user is logged in.");
        }
        const Customer* customer =
            dynamic_cast<const Customer*>(identifyUser(currentUsername));
        if (!customer) {
            throw std::logic_error("Logged-in user is not a customer.");
        }

        const auto history = customer->getOrderHistory();
        std::vector<OrderHistoryInfo> result;
        for (auto it = history.rbegin();
             it != history.rend() && result.size() < 5; ++it) {
            const Order& order = *it;
            if (order.getStatus() != Order::Status::PickedUp &&
                order.getStatus() != Order::Status::Cancelled) continue;
            OrderHistoryInfo info;
            info.orderNumber = order.getOrderNumber();
            info.placedAt = order.getSubmittedAtText();
            info.source = order.getPurchaseTypeName();
            info.status = order.getStatusName();
            info.finalTotalCents = order.getFinalPriceCents();
            result.push_back(info);
        }
        return result;
    }

    CustomerInfo getCustomerInfo(const std::string& nameOrUsername) const {
        //copies the matching customer's contact details and balances into display data
        const Customer* customer =
            dynamic_cast<const Customer*>(identifyUser(nameOrUsername));
        if (!customer) {
            throw std::invalid_argument("customer not found.");
        }
        CustomerInfo info;
        info.name = customer->getName();
        info.email = customer->getEmail();
        info.homeAddress = customer->getHomeAddress();
        info.loyaltyNumber = customer->getLoyaltyNumber();
        info.loyaltyPoints = customer->getLoyaltyPoints();
        info.accountCreditCents = customer->getAccountCreditCents();
        return info;
    }

    CustomerInfo getCurrentCustomerInfo() const {
        //returns display data for the signed-in customer and rejects a missing login
        if (currentUsername.empty()) {
            throw std::logic_error("No user is logged in.");
        }
        return getCustomerInfo(currentUsername);
    }

    void updateCurrentCustomerContact(
        const std::string& email,
        const std::string& homeAddress
    ) {
        //validates both contact fields and saves them to the original signed-in customer object
        Customer& customer = currentCustomerForUpdate();
        const std::string updatedEmail = trimContactText(email);
        const std::string updatedAddress = trimContactText(homeAddress);

        const std::size_t at = updatedEmail.find('@');
        if (updatedEmail.empty() ||
            at == std::string::npos || at == 0 || at + 1 == updatedEmail.size() ||
            updatedEmail.find('@', at + 1) != std::string::npos ||
            updatedEmail.find_first_of(" \t\r\n\f\v") != std::string::npos) {
            throw std::invalid_argument(
                "Enter an email address such as name@example.com."
            );
        }
        if (updatedAddress.empty()) {
            throw std::invalid_argument("Enter a home address.");
        }

        customer.updateContactInfo(updatedEmail, updatedAddress);
    }

    std::string login(const std::string& nameOrUsername) {
        //starts a fresh identification session and returns the matching user's role or an empty string
        logout();
        const DefaultUser* user = identifyUser(nameOrUsername);
        if (!user) return "";
        currentUsername = user->getUsername();
        currentRole = user->getRole();
        return currentRole;
    }

    const std::string& getCurrentUsername() const {
        //returns the username stored for the current login session
        return currentUsername;
    }
    const std::string& getCurrentRole() const {
        //returns the role stored for the current login session
        return currentRole;
    }
    unsigned long long getCurrentSessionId() const {
        //returns the identifier used to distinguish separate login sessions
        return sessionId;
    }

    void logout() {
        //clears the signed-in identity and advances the session identifier
        currentUsername.clear();
        currentRole.clear();
        ++sessionId;
    }
};

#endif
