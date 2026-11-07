#ifndef CUSTOMER_H
#define CUSTOMER_H

#include "DefaultUser.h"
#include <stdexcept>

/*
 *
 * "Each must have a name, unique username, unique loyalty points number, email
 * address, home address, loyalty points balance, and account credit balance."
 */
class Customer : public DefaultUser {
private:
    std::string loyaltyNumber;
    std::string email;
    std::string homeAddress;
    int loyaltyPoints;
    int accountCreditCents;

public:
    Customer(std::string name, std::string username, std::string loyaltyNumber,
             std::string email, std::string homeAddress,
             int loyaltyPoints = 0, int accountCreditCents = 0)
        : DefaultUser(name, username),
          loyaltyNumber(loyaltyNumber), email(email),
          homeAddress(homeAddress), loyaltyPoints(loyaltyPoints),
          accountCreditCents(accountCreditCents) {
        if (loyaltyPoints < 0) throw std::invalid_argument("points cannot be negative.");
        if (accountCreditCents < 0) throw std::invalid_argument("credit cannot be negative.");
    }

    const std::string& getLoyaltyNumber() const { return loyaltyNumber; }
    const std::string& getEmail() const { return email; }
    const std::string& getHomeAddress() const { return homeAddress; }
    int getLoyaltyPoints() const { return loyaltyPoints; }
    /*
     *
     * "Loyalty points can be redeemed to cover up to 50% of the pre-tax cost
     * of an online or in-person order."
     *
     * "Loyalty points cannot be used to pay the tax on an order."
     *
     * "1 loyalty point is equivalent to $1 when paying an order."
     */
    void redeemLoyaltyPoints(int points) {
        if (points < 0 || points > loyaltyPoints) {
            throw std::invalid_argument("insufficient loyalty points.");
        }
        loyaltyPoints -= points;
    }

    /*
     *
     * "Loyalty points are earned at a rate of 1 point for every $3 paid, with
     * the number of points earned rounded down to the nearest whole point."
     *
     * "A customer does not earn loyalty points on an order if loyalty points
     * were redeemed toward that order."
     *
     * "Earned loyalty points are credited when the order reaches the Completed
     * state."
     */
    void earnLoyaltyPoints(int points) {
        if (points < 0) {
            throw std::invalid_argument("invalid loyalty points award.");
        }
        loyaltyPoints += points;
    }
    int getAccountCreditCents() const { return accountCreditCents; }
    /*
     * "Account credit can be applied toward future orders."
     */
    void useAccountCredit(int cents) {
        if (cents < 0 || cents > accountCreditCents) {
            throw std::invalid_argument("insufficient account credit.");
        }
        accountCreditCents -= cents;
    }
    bool canLogin() const override { return true; }
    std::string getRole() const override { return "customer"; }
};

#endif
