#ifndef CUSTOMER_H
#define CUSTOMER_H

#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <memory>

#include "defaultuser.h"
#include "order.h"


class OrderService;
/*
 *
 * "Each must have a name, unique username, unique loyalty points number, email
 * address, home address, loyalty points balance, and account credit balance."
 */
class Customer : public DefaultUser {

friend class OrderService;

private:
    std::string loyaltyNumber;
    std::string email;
    std::string homeAddress;
    int loyaltyPoints;
    int accountCreditCents;
    std::vector<std::shared_ptr<Order>> orders;
    bool registeredWithOrderService = false;

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

    // AuthService validates both fields before calling this mutator.
        // Both string copies are prepared before entry, then committed by swapping.
    void updateContactInfo(std::string newEmail, std::string newHomeAddress) {
        email.swap(newEmail);
        homeAddress.swap(newHomeAddress);
    }


    int getLoyaltyPoints() const { return loyaltyPoints; }
    std::vector<Order> getOrders() const
    {
        std::vector<Order> result;
        result.reserve(orders.size());

        for (const auto& order : orders) {
            result.push_back(*order);
        }

        return result;
    }

    std::vector<Order> getOrderHistory() const
    {
        std::vector<Order> result;

        for (const auto& order : orders) {
            if (order->getStatus() == Order::Status::PickedUp ||
                order->getStatus() == Order::Status::Cancelled) {
                result.push_back(*order);
            }
        }

        return result;
    }
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

    void initializeOrderHistory(const std::vector<Order>& history) {
        if (registeredWithOrderService || !orders.empty() ||
            loyaltyPoints != 0 || accountCreditCents != 0) {
            throw std::invalid_argument(
                "default history requires a fresh customer."
            );
        }

        int points = 0;
        int creditCents = 0;
        std::time_t previousTime = 0;
        std::set<int> orderNumbers;

        for (const Order& order : history) {
            if (order.getCustomerUsername() != getUsername() ||
                order.getCustomerName() != getName()) {
                throw std::invalid_argument("historical order belongs to another customer.");
            }
            if (order.getStatus() != Order::Status::PickedUp &&
                order.getStatus() != Order::Status::Cancelled) {
                throw std::invalid_argument("history requires picked-up or cancelled orders.");
            }
            if (order.getSubmittedAt() < previousTime ||
                !orderNumbers.insert(order.getOrderNumber()).second) {
                throw std::invalid_argument("history must be chronological with unique numbers.");
            }
            previousTime = order.getSubmittedAt();
            if (order.getPointsRedeemed() > points ||
                order.getAccountCreditUsedCents() > creditCents) {
                throw std::invalid_argument("historical payment exceeds available balance.");
            }

            points -= order.getPointsRedeemed();
            creditCents -= order.getAccountCreditUsedCents();

            if (order.getStatus() == Order::Status::Cancelled) {
                points += order.getPointsReturnedOnCancellation();
                creditCents += order.getAccountCreditReceivedCents();
            } else {
                points += order.getPointsEarned();
            }
        }

        std::vector<std::shared_ptr<Order>> initializedOrders;
        initializedOrders.reserve(history.size());

        for (const Order& order : history) {
            initializedOrders.push_back(
                std::make_shared<Order>(order)
            );
        }

        orders.swap(initializedOrders);
        loyaltyPoints = points;
        accountCreditCents = creditCents;
    }
};

#endif
