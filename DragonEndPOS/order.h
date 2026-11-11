#ifndef ORDER_H
#define ORDER_H


#include <ctime>
#include <stdexcept>
#include <string>
#include <vector>


#include "orderitem.h"


class Order {
public:
    enum class Status { Draft, InProgress, Completed, PickedUp, Cancelled };
    enum class PurchaseType { Online, InPerson };


private:
    int orderNumber;
    std::vector<OrderItem> items;
    std::string customerUsername;
    std::string customerName;
    std::string paymentMethod;
    PurchaseType type;
    Status status = Status::Draft;
    std::time_t submittedAt = 0;
    int taxCents = 0;
    int amountChargedCents = 0;
    int pointsRedeemed = 0;
    int pointsEarned = 0;
    int accountCreditUsedCents = 0;


public:
    explicit Order(int orderNumber, std::string customerUsername = "",
                   std::string customerName = "guest",
                   const std::vector<OrderItem>& orderItems = {},
                   Status initialStatus = Status::Draft,
                   PurchaseType purchaseType = PurchaseType::Online,
                   std::time_t placedAt = 0,
                   int redeemedPoints = 0, int creditUsedCents = 0,
                   int taxRatePercent = 15,
                   std::string selectedPaymentMethod = "")
        : orderNumber(orderNumber), items(orderItems),
          customerUsername(customerUsername), customerName(customerName),
          paymentMethod(selectedPaymentMethod),
          type(purchaseType), status(initialStatus), submittedAt(placedAt),
          pointsRedeemed(redeemedPoints), accountCreditUsedCents(creditUsedCents) {
        //initializes and validates the order's identity, items, benefits, payment settings, and starting status
        if (orderNumber <= 0 || customerName.empty()) {
            throw std::invalid_argument("invalid order identity.");
        }
        if (type != PurchaseType::Online && type != PurchaseType::InPerson) {
            throw std::invalid_argument("invalid order purchase type.");
        }
        if (paymentMethod.empty()) {
            paymentMethod = type == PurchaseType::Online
                ? "dragon credit card" : "adventure credit card";
        }
        if ((type == PurchaseType::Online &&
             paymentMethod != "dragon credit card" &&
             paymentMethod != "sorcerer credit card") ||
            (type == PurchaseType::InPerson &&
             paymentMethod != "adventure credit card")) {
            throw std::invalid_argument("invalid payment method for this order source.");
        }
        if (status != Status::Draft && status != Status::InProgress &&
            status != Status::Completed && status != Status::PickedUp &&
            status != Status::Cancelled) {
            throw std::invalid_argument("invalid order status.");
        }
        if (placedAt < 0 || taxRatePercent < 0 || taxRatePercent > 100) {
            throw std::invalid_argument("invalid order date or tax rate.");
        }
        if (pointsRedeemed < 0 || pointsRedeemed > getMaxRedeemablePoints() ||
            (customerUsername.empty() && pointsRedeemed != 0)) {
            throw std::invalid_argument("invalid loyalty points redemption.");
        }
        if (accountCreditUsedCents < 0 ||
            (customerUsername.empty() && accountCreditUsedCents != 0)) {
            throw std::invalid_argument("invalid account credit amount.");
        }


        const int amountDue = getAmountDueCents(taxRatePercent);


        if (status == Status::Draft) {
            if (submittedAt != 0) {
                throw std::invalid_argument("a draft order has no submission date.");
            }
            return;
        }
        if (items.empty()) {
            throw std::invalid_argument("a submitted order must contain items.");
        }
        if (status == Status::Cancelled &&
            (type != PurchaseType::Online || customerUsername.empty())) {
            throw std::invalid_argument("only online customer orders can be cancelled.");
        }


        if (submittedAt == 0) submittedAt = std::time(nullptr);
        taxCents = calculateTaxCents(taxRatePercent);
        amountChargedCents = amountDue;
        if (pointsRedeemed == 0 && !customerUsername.empty() &&
            status != Status::Cancelled) {
            pointsEarned = getPointsOnCompletion(taxRatePercent);
        }
    }


    int getOrderNumber() const {
        //returns the order's identifying number
        return orderNumber;
    }
    const std::vector<OrderItem>& getItems() const {
        //returns a read-only reference to the items purchased in this order
        return items;
    }


    void addItem(const OrderItem& item) {
        //adds an item only while the order is still a draft
        if (status != Status::Draft) {
            throw std::invalid_argument("items can only be added to a draft order.");
        }
        items.push_back(item);
    }


    int getSubtotalCents() const {
        //sums the line totals of all order items to calculate the pre-tax subtotal in cents
        int subtotal = 0;
        for (const OrderItem& item : items) {
            subtotal += item.getLineTotalCents();
        }
        return subtotal;
    }


    const std::string& getCustomerUsername() const {
        //returns the username associated with the order
        return customerUsername;
    }
    const std::string& getCustomerName() const {
        //returns the customer name recorded on the order
        return customerName;
    }
    const std::string& getPaymentMethodName() const {
        //returns the card type selected for the order
        return paymentMethod;
    }
    Status getStatus() const {
        //returns the order's current workflow status
        return status;
    }
    PurchaseType getPurchaseType() const {
        //returns whether the order was placed online or in person
        return type;
    }
    std::time_t getSubmittedAt() const {
        //returns the stored submission timestamp
        return submittedAt;
    }
    int getTaxCents() const {
        //returns the tax amount recorded for the submitted order in cents
        return taxCents;
    }
    int getFinalPriceCents() const {
        //returns the full order price including the stored tax amount
        return getSubtotalCents() + taxCents;
    }
    int getAmountChargedCents() const {
        //returns the amount recorded as charged after points and account credit were applied
        return amountChargedCents;
    }
    int getAccountCreditUsedCents() const {
        //returns the account credit applied to this order in cents
        return accountCreditUsedCents;
    }
    int getPointsRedeemed() const {
        //returns the number of loyalty points applied toward this order
        return pointsRedeemed;
    }
    int getPointsEarned() const {
        //returns the order's stored loyalty-points earning amount
        return pointsEarned;
    }
    int getMaxRedeemablePoints() const {
        //calculates the maximum whole points that can cover half the pre-tax subtotal
        return getSubtotalCents() / 200;
    }

    int getPointsOnCompletion(int taxRatePercent) const {
        //calculates potential completion points when the customer has not redeemed loyalty points
        if (customerUsername.empty() || pointsRedeemed != 0) return 0;
        return getAmountDueCents(taxRatePercent) / 300;
    }


    std::string getSubmittedAtText() const {
        //formats the submission time as YYYY-MM-DD:HH:MM or returns an empty string when unavailable
        if (submittedAt == 0) return "";
        const std::tm* localTime = std::localtime(&submittedAt);
        if (!localTime) return "";
        char text[32];
        if (std::strftime(text, sizeof(text), "%Y-%m-%d:%H:%M", localTime) == 0) {
            return "";
        }
        return text;
    }


    void setAccountCreditUsed(int cents) {
        //validates and stores an account-credit amount on a draft order
        if (status != Status::Draft || cents < 0 ||
            (customerUsername.empty() && cents != 0)) {
            throw std::invalid_argument("invalid account credit amount.");
        }
        accountCreditUsedCents = cents;
    }


    int calculateTaxCents(int taxRatePercent) const {
        //validates the tax rate and calculates rounded tax on the full pre-tax subtotal
        if (taxRatePercent < 0 || taxRatePercent > 100) {
            throw std::invalid_argument("invalid tax rate.");
        }
        int subtotal = getSubtotalCents();
        return (subtotal / 100) * taxRatePercent +
               ((subtotal % 100) * taxRatePercent + 50) / 100;
    }


    int getAmountDueCents(int taxRatePercent) const {
        //calculates the amount to pay after tax, redeemed points, and account credit
        int remaining = getSubtotalCents() + calculateTaxCents(taxRatePercent)
                        - 100 * pointsRedeemed;
        if (accountCreditUsedCents > remaining) {
            throw std::invalid_argument("account credit exceeds the amount to pay.");
        }
        return remaining - accountCreditUsedCents;
    }


    void setPointsRedeemed(int points) {
        //validates and stores the number of loyalty points used on a draft order
        if (status != Status::Draft || points < 0 ||
            points > getMaxRedeemablePoints() ||
            (customerUsername.empty() && points != 0)) {
            throw std::invalid_argument("invalid loyalty points redemption.");
        }
        pointsRedeemed = points;
    }


    bool isPaid() const {
        //reports whether the order has been submitted, including orders that were later cancelled
        return status != Status::Draft;
    }


    std::string getPaymentStatusName() const {
        //returns display text describing an unpaid, paid, or refunded order
        if (status == Status::Draft) return "unpaid";
        if (status == Status::Cancelled) return "refunded as account credit";
        return "paid";
    }


    std::string getStatusName() const {
        //converts the current order status into its display text
        switch (status) {
        case Status::InProgress: return "in progress";
        case Status::Completed: return "completed";
        case Status::PickedUp: return "picked up";
        case Status::Cancelled: return "cancelled";
        default: return "draft";
        }
    }


    std::string getPurchaseTypeName() const {
        //returns display text for an online or in-person purchase
        return type == PurchaseType::Online ? "online" : "in person";
    }


    int getAccountCreditReceivedCents() const {
        //returns the account-credit refund for a cancelled order or zero otherwise
        return status == Status::Cancelled
            ? accountCreditUsedCents + amountChargedCents : 0;
    }


    int getPointsReturnedOnCancellation() const {
        //returns redeemed points for a cancelled order or zero otherwise
        return status == Status::Cancelled ? pointsRedeemed : 0;
    }


    void submit(int taxRatePercent) {
        //records payment totals and submission time and moves a valid draft order into progress
        if (status != Status::Draft || items.empty() || taxRatePercent < 0 ||
            taxRatePercent > 100) {
            throw std::invalid_argument("invalid order submission.");
        }
        int amountDue = getAmountDueCents(taxRatePercent);
        taxCents = calculateTaxCents(taxRatePercent);
        amountChargedCents = amountDue;
        pointsEarned = 0;
        if (pointsRedeemed == 0 && !customerUsername.empty()) {
            pointsEarned = getPointsOnCompletion(taxRatePercent);
        }
        submittedAt = std::time(nullptr);
        status = Status::InProgress;
    }


    bool complete() {
        //changes an in-progress order to completed and reports whether the transition succeeded
        if (status != Status::InProgress) return false;
        status = Status::Completed;
        return true;
    }


};


#endif


