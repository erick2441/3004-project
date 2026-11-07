#ifndef ORDER_SERVICE_H
#define ORDER_SERVICE_H

#include "../orders/Order.h"
#include "../users/Customer.h"

/*
 *
 * "All data is held in memory only (no database in this deliverable). Store
 * the data objects in memory."
 */
class OrderService {
private:
    std::vector<Order> orders;
    std::vector<Customer*> customers;

    Customer* findCustomer(const std::string& username) const {
        for (Customer* customer : customers) {
            if (customer->getUsername() == username) return customer;
        }
        return nullptr;
    }

public:
    static constexpr int TAX_RATE_PERCENT = 15;

    const std::vector<Order>& getOrders() const { return orders; }

    void registerCustomer(Customer& customer) {
        if (findCustomer(customer.getUsername())) {
            throw std::invalid_argument("customer already registered.");
        }
        customers.push_back(&customer);
    }

    /*
     *
     * "Display the current order (In Progress or Completed)."
     *
     * "Orders placed in this prototype cannot reach Picked Up or Cancelled, so
     * they remain as a current order and do not update the order history."
     */
    const Order* getCurrentOrder(const std::string& username) const {
        if (username.empty()) return nullptr; // guests have no account.
        for (const Order& order : orders) {
            if (order.getCustomerUsername() == username) return &order;
        }
        return nullptr;
    }

    const Order* findOrder(int number) const {
        for (const Order& order : orders) {
            if (order.getOrderNumber() == number) return &order;
        }
        return nullptr;
    }

    /*
     * Deliverable 1 - Business Rules
     *
     * "A customer may have only one current order at a time."
     *
     * "Orders placed in this prototype are recorded as paid, with the amount
     * charged equal to the amount to be paid after loyalty points and account
     * credit are applied. No payment system is called."
     */
    bool submitOrder(Order order) {
        if (getCurrentOrder(order.getCustomerUsername())) return false;
        if (findOrder(order.getOrderNumber())) {
            throw std::invalid_argument("order number already exists.");
        }
        Customer* customer = findCustomer(order.getCustomerUsername());
        if (!order.getCustomerUsername().empty() && !customer) {
            throw std::invalid_argument("customer is not registered.");
        }
        if (customer && order.getPointsRedeemed() > customer->getLoyaltyPoints()) {
            throw std::invalid_argument("not enough loyalty points.");
        }
        if (customer && order.getAccountCreditUsedCents() > customer->getAccountCreditCents()) {
            throw std::invalid_argument("not enough account credit.");
        }
        order.submit(TAX_RATE_PERCENT);
        orders.push_back(order);
        if (customer) {
            customer->redeemLoyaltyPoints(orders.back().getPointsRedeemed());
            customer->useAccountCredit(orders.back().getAccountCreditUsedCents());
        }
        return true;
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
    bool completeOrder(int number) {
        for (Order& order : orders) {
            if (order.getOrderNumber() == number) {
                if (order.getStatus() != Order::Status::InProgress) return false;
                Customer* customer = findCustomer(order.getCustomerUsername());
                if (customer) customer->earnLoyaltyPoints(order.getPointsEarned());
                return order.complete();
            }
        }
        return false;
    }
};

#endif
