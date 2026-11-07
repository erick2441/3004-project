#ifndef ORDER_H
#define ORDER_H

#include <vector>
#include <ctime>
#include "OrderItem.h"

class Order {
public:
    enum class Status { Draft, InProgress, Completed };

private:
    int orderNumber;
    std::vector<OrderItem> items;
    std::string customerUsername;
    std::string customerName;
    Status status = Status::Draft;
    std::time_t submittedAt = 0;
    int taxCents = 0;
    int amountChargedCents = 0;
    int pointsRedeemed = 0;
    int pointsEarned = 0;
    int accountCreditUsedCents = 0;

public:
    explicit Order(int orderNumber, std::string customerUsername = "",
                   std::string customerName = "guest")
        : orderNumber(orderNumber), customerUsername(customerUsername),
          customerName(customerName) {}

    int getOrderNumber() const { return orderNumber; }

    const std::vector<OrderItem>& getItems() const { return items; }

    void addItem(const OrderItem& item) {
        items.push_back(item);
    }

    int getSubtotalCents() const {
        int subtotal = 0;
        for (const OrderItem& item : items) {
            subtotal += item.getLineTotalCents();
        }
        return subtotal;
    }

    // Getters
    const std::string& getCustomerUsername() const { return customerUsername; }
    const std::string& getCustomerName() const { return customerName; }
    Status getStatus() const { return status; }
    std::time_t getSubmittedAt() const { return submittedAt; }
    int getTaxCents() const { return taxCents; }
    int getFinalPriceCents() const { return getSubtotalCents() + taxCents; }
    int getAmountChargedCents() const { return amountChargedCents; }
    int getAccountCreditUsedCents() const { return accountCreditUsedCents; }

    /*
     * "Account credit can be applied toward future orders."
     */
    void setAccountCreditUsed(int cents) {
        if (status != Status::Draft || cents < 0 ||
            (customerUsername.empty() && cents != 0)) {
            throw std::invalid_argument("invalid account credit amount.");
        }
        accountCreditUsedCents = cents;
    }

    int calculateTaxCents(int taxRatePercent) const {
        if (taxRatePercent < 0 || taxRatePercent > 100) {
            throw std::invalid_argument("invalid tax rate.");
        }
        int subtotal = getSubtotalCents();
        return (subtotal / 100) * taxRatePercent +
               ((subtotal % 100) * taxRatePercent + 50) / 100;
    }

    int getAmountDueCents(int taxRatePercent) const {
        int remaining = getSubtotalCents() + calculateTaxCents(taxRatePercent)
                        - 100 * pointsRedeemed;
        if (accountCreditUsedCents > remaining) {
            throw std::invalid_argument("account credit exceeds the amount to pay.");
        }
        return remaining - accountCreditUsedCents;
    }
    int getPointsRedeemed() const { return pointsRedeemed; }
    int getPointsEarned() const { return pointsEarned; }
    int getMaxRedeemablePoints() const {
        return getSubtotalCents() / 200;
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
    void setPointsRedeemed(int points) {
        if (status != Status::Draft || points < 0 ||
            points > getMaxRedeemablePoints() ||
            (customerUsername.empty() && points != 0)) {
            throw std::invalid_argument("invalid loyalty points redemption.");
        }
        pointsRedeemed = points;
    }
    bool isPaid() const { return status != Status::Draft; }

    std::string getStatusName() const {
        if (status == Status::InProgress) return "in progress";
        if (status == Status::Completed) return "completed";
        return "draft";
    }

    void submit(int taxRatePercent) {
        if (status != Status::Draft || items.empty() || taxRatePercent < 0 ||
            taxRatePercent > 100) {
            throw std::invalid_argument("invalid order submission.");
        }
        int amountDue = getAmountDueCents(taxRatePercent);
        taxCents = calculateTaxCents(taxRatePercent);
        amountChargedCents = amountDue;
        int earned = 0;
        if (pointsRedeemed == 0 && !customerUsername.empty()) {
            earned = amountChargedCents / 300;
        }
        pointsEarned = earned;
        submittedAt = std::time(nullptr);
        status = Status::InProgress;
    }

    bool complete() {
        if (status != Status::InProgress) return false;
        status = Status::Completed;
        return true;
    }
};

#endif
