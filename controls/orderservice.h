#ifndef ORDER_SERVICE_H
#define ORDER_SERVICE_H

#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>
#include <memory>
#include <limits>

#include "customer.h"
#include "order.h"
#include "authservice.h"
#include "cafemenu.h"

/*
 *
 * "All data is held in memory only (no database in this deliverable). Store
 * the data objects in memory."
 */
class OrderService {
private:
    static void requireKitchenStaff(const AuthService& auth) {
        //rejects queue operations unless the signed-in user is kitchen staff
        if (!auth.isKitchenStaff()) {
            throw std::logic_error("Sign in as kitchen staff to access the preparation queue.");
        }
    }

    static int maximumSubtotal()
    {
        //calculates the subtotal limit used to keep taxed order totals within the integer range
        return static_cast<int>(
            static_cast<long long>(std::numeric_limits<int>::max()) * 100 /
            (100 + TAX_RATE_PERCENT)
        );
    }

    std::vector<std::shared_ptr<Order>> orders;
    std::vector<Customer*> customers;

    Customer* findCustomer(const std::string& username) const {
        //finds a registered customer by username or returns null when no match exists
        for (Customer* customer : customers) {
            if (customer->getUsername() == username) return customer;
        }
        return nullptr;
    }

public:
    OrderService() = default;

    OrderService(const OrderService&) = delete;
    OrderService& operator=(const OrderService&) = delete;

    OrderService(OrderService&&) = delete;
    OrderService& operator=(OrderService&&) = delete;

    static constexpr int TAX_RATE_PERCENT = 15;

    struct OrderItemInfo {
        std::string name;
        int unitPriceCents = 0;
        int quantity = 0;
        int lineTotalCents = 0;
    };

    struct OrderDetailsInfo {
        int orderNumber = 0;
        std::string customerUsername;
        std::string customerName;
        std::string placedAt;
        std::string source;
        std::string status;
        std::string paymentStatus;
        std::string paymentMethod;
        std::vector<OrderItemInfo> items;
        int subtotalCents = 0;
        int taxCents = 0;
        int finalTotalCents = 0;
        int pointsRedeemed = 0;
        int accountCreditUsedCents = 0;
        int amountChargedCents = 0;
        int pointsEarned = 0;
        int accountCreditReceivedCents = 0;
        int pointsReturned = 0;
    };

    std::vector<Order> getOrders() const
    {
        //returns copies of all stored orders without exposing the shared order objects
        std::vector<Order> result;
        result.reserve(orders.size());

        for (const auto& order : orders) {
            result.push_back(*order);
        }

        return result;
    }

    std::vector<Order> getCurrentOrders() const {
        //returns copies of orders that are in progress or completed
        std::vector<Order> result;
        for (const auto& order : orders) {
            if (order->getStatus() == Order::Status::InProgress ||
                order->getStatus() == Order::Status::Completed) {
                result.push_back(*order);
            }
        }
        return result;
    }

    void registerCustomer(Customer& customer)
    {
        //registers a customer and shares their existing orders after checking for duplicate identities
        if (findCustomer(customer.getUsername()) ||
            customer.registeredWithOrderService) {
            throw std::invalid_argument(
                "customer already registered."
            );
        }

        for (const auto& order : customer.orders) {
            if (findOrder(order->getOrderNumber())) {
                throw std::invalid_argument(
                    "order number already exists."
                );
            }
        }

        orders.reserve(orders.size() + customer.orders.size());
        customers.reserve(customers.size() + 1);

        for (const auto& order : customer.orders) {
            orders.push_back(order);
        }

        customers.push_back(&customer);
        customer.registeredWithOrderService = true;
    }

    /*
     *
     * "Display the current order (In Progress or Completed)."
     *
     * "Orders placed in this prototype cannot reach Picked Up or Cancelled, so
     * they remain as a current order and do not update the order history."
     */
    const Order* getCurrentOrder(const std::string& username) const
    {
        //finds the customer's in-progress or completed order, or returns null when none exists
        if (username.empty()) {
            return nullptr;
        }

        for (const auto& order : orders) {
            if (order->getCustomerUsername() == username &&
                (order->getStatus() == Order::Status::InProgress ||
                 order->getStatus() == Order::Status::Completed)) {
                return order.get();
            }
        }

        return nullptr;
    }

    const Order* findOrder(int number) const
    {
        //finds a stored order by number or returns null when it cannot be found
        for (const auto& order : orders) {
            if (order->getOrderNumber() == number) {
                return order.get();
            }
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
        //validates and submits an order, shares it with its customer, and deducts redeemed benefits
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

        const auto storedOrder = std::make_shared<Order>(order);

        orders.reserve(orders.size() + 1);

        if (customer) {
            customer->orders.reserve(customer->orders.size() + 1);
        }

        orders.push_back(storedOrder);

        if (customer) {
            customer->orders.push_back(storedOrder);

            customer->redeemLoyaltyPoints(
                storedOrder->getPointsRedeemed()
            );

            customer->useAccountCredit(
                storedOrder->getAccountCreditUsedCents()
            );
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
        //completes an in-progress order and credits eligible loyalty points to its customer once
        for (const auto& order : orders) {
            if (order->getOrderNumber() == number) {
                if (order->getStatus() != Order::Status::InProgress) {
                    return false;
                }

                Customer* customer =
                    findCustomer(order->getCustomerUsername());

                if (customer) {
                    customer->earnLoyaltyPoints(
                        order->getPointsEarned()
                    );
                }

                return order->complete();
            }
        }

        return false;
    }

    OrderDetailsInfo getOrderDetails(int orderNumber) const {
        //copies an order's receipt, payment, benefit, status, and item information into display data
        const Order* order = findOrder(orderNumber);
        if (!order) {
            throw std::invalid_argument("order not found.");
        }

        OrderDetailsInfo info;
        info.orderNumber = order->getOrderNumber();
        info.customerUsername = order->getCustomerUsername();
        info.customerName = order->getCustomerName();
        info.placedAt = order->getSubmittedAtText();
        info.source = order->getPurchaseTypeName();
        info.status = order->getStatusName();
        info.paymentStatus = order->getPaymentStatusName();
        info.paymentMethod = order->getPaymentMethodName();
        info.subtotalCents = order->getSubtotalCents();
        info.taxCents = order->getTaxCents();
        info.finalTotalCents = order->getFinalPriceCents();
        info.pointsRedeemed = order->getPointsRedeemed();
        info.accountCreditUsedCents = order->getAccountCreditUsedCents();
        info.amountChargedCents = order->getAmountChargedCents();

        if (order->getStatus() == Order::Status::Completed ||
            order->getStatus() == Order::Status::PickedUp) {
            info.pointsEarned = order->getPointsEarned();
        }
        info.accountCreditReceivedCents = order->getAccountCreditReceivedCents();
        info.pointsReturned = order->getPointsReturnedOnCancellation();

        info.items.reserve(order->getItems().size());
        for (const OrderItem& item : order->getItems()) {
            OrderItemInfo itemInfo;
            itemInfo.name = item.getName();
            itemInfo.unitPriceCents = item.getUnitPriceCents();
            itemInfo.quantity = item.getQuantity();
            itemInfo.lineTotalCents = item.getLineTotalCents();
            info.items.push_back(itemInfo);
        }
        return info;
    }

    struct KitchenOrderInfo {
        int orderNumber = 0;
        std::string customerName;
        std::string placedAt;
        std::string source;
    };

    struct KitchenIngredientInfo {
        std::string name;
        long long quantity = 0;
        std::string unit;
    };

    struct KitchenItemInfo {
        std::string name;
        int quantity = 0;
        std::vector<KitchenIngredientInfo> ingredients;
    };

    struct KitchenOrderDetailsInfo {
        OrderDetailsInfo order;
        std::vector<KitchenItemInfo> preparationItems;
    };

    std::vector<KitchenOrderInfo> getKitchenQueue(const AuthService& auth) const {
        //builds display records for paid in-progress orders while preserving their enqueue order
        requireKitchenStaff(auth);
        std::vector<KitchenOrderInfo> result;
        for (const auto& order : orders) {
            if (order->getStatus() != Order::Status::InProgress || !order->isPaid()) {
                continue;
            }
            KitchenOrderInfo info;
            info.orderNumber = order->getOrderNumber();
            info.customerName = order->getCustomerName();
            info.placedAt = order->getSubmittedAtText();
            info.source = order->getPurchaseTypeName();
            result.push_back(info);
        }
        return result;
    }

    KitchenOrderDetailsInfo getKitchenOrderDetails(
        const AuthService& auth, int orderNumber
    ) const {
        //checks kitchen access and builds order details with ingredient quantities for each item
        requireKitchenStaff(auth);
        const Order* order = findOrder(orderNumber);
        if (!order || order->getStatus() != Order::Status::InProgress || !order->isPaid()) {
            throw std::logic_error("This order is no longer in the preparation queue.");
        }

        KitchenOrderDetailsInfo details;
        details.order = getOrderDetails(orderNumber);
        for (const auto& item : order->getItems()) {
            KitchenItemInfo itemInfo;
            itemInfo.name = item.getName();
            itemInfo.quantity = item.getQuantity();
            for (const auto& ingredient : item.getIngredients()) {
                KitchenIngredientInfo ingredientInfo;
                ingredientInfo.name = ingredient.name;
                ingredientInfo.quantity = static_cast<long long>(ingredient.quantity)
                                        * item.getQuantity();
                ingredientInfo.unit = ingredient.unit;
                itemInfo.ingredients.push_back(ingredientInfo);
            }
            details.preparationItems.push_back(itemInfo);
        }
        return details;
    }

    void completeKitchenOrder(const AuthService& auth, int orderNumber) {
        //checks kitchen access and completes an order that is still in the preparation queue
        requireKitchenStaff(auth);
        const Order* order = findOrder(orderNumber);
        if (!order || order->getStatus() != Order::Status::InProgress || !order->isPaid()) {
            throw std::logic_error("This order is no longer in the preparation queue.");
        }
        if (!completeOrder(orderNumber)) {
            throw std::logic_error("The order could not be completed. Refresh the queue.");
        }
    }

    bool getCurrentOrderDetails(
        const AuthService& auth,
        OrderDetailsInfo& details
    ) const
    {
        //returns current-order display data for the signed-in customer or clears it when absent
        details = OrderDetailsInfo();
        if (auth.getCurrentRole() != "customer" ||
            auth.getCurrentUsername().empty()) {
            return false;
        }

        const Order* currentOrder = getCurrentOrder(auth.getCurrentUsername());
        if (!currentOrder) {
            return false;
        }

        details = getOrderDetails(currentOrder->getOrderNumber());
        return true;
    }

    std::vector<const Order*> getLastOrders(
        const std::string& username
    ) const
    {
        //finds up to five picked-up or cancelled customer orders and sorts them newest first
        std::vector<const Order*> result;

        if (username.empty()) {
            return result;
        }

        for (const auto& order : orders) {
            if (order->getCustomerUsername() == username &&
                (order->getStatus() == Order::Status::PickedUp ||
                 order->getStatus() == Order::Status::Cancelled)) {
                result.push_back(order.get());
            }
        }

        std::sort(
            result.begin(),
            result.end(),
            [](const Order* left, const Order* right) {
                //compares submission times and order numbers to place newer historical orders first
                if (left->getSubmittedAt() != right->getSubmittedAt()) {
                    return left->getSubmittedAt() >
                           right->getSubmittedAt();
                }

                return left->getOrderNumber() >
                       right->getOrderNumber();
            }
        );

        if (result.size() > 5) {
            result.resize(5);
        }

        return result;
    }
    struct MenuItemInfo {
        int menuItemId = 0;
        std::string name;
        std::string category;
        int unitPriceCents = 0;
        std::string availability;
    };

    struct CartItemInfo {
        int menuItemId = 0;
        std::string name;
        int unitPriceCents = 0;
        int quantity = 0;
        int lineTotalCents = 0;
        int maximumQuantity = 1;
    };

    struct CheckoutInfo {
        std::string customerName;
        std::string username;
        std::string loyaltyNumber;
        std::vector<CartItemInfo> items;
        int availablePoints = 0;
        int availableCreditCents = 0;
        int subtotalCents = 0;
        int taxCents = 0;
        int finalTotalCents = 0;
        int pointsRedeemed = 0;
        int accountCreditUsedCents = 0;
        int amountDueCents = 0;
        int pointsOnCompletion = 0;
        int maximumPoints = 0;
        int maximumCreditCents = 0;
        bool hasCurrentOrder = false;
        int currentOrderNumber = 0;
        bool canSubmit = false;
        std::string paymentMethod;
        std::string message;
        unsigned long long revision = 0;
    };

    std::vector<MenuItemInfo> getMenuInfo() const
    {
        //builds menu display records containing item IDs, names, categories, prices, and availability
        std::vector<MenuItemInfo> result;
        int id = 1;
        for (const MenuItem& item : menu.getItems()) {
            MenuItemInfo info;
            info.menuItemId = id++;
            info.name = item.getName();
            info.category = item.getCategory();
            info.unitPriceCents = item.getPriceCents();
            info.availability = "Available";
            result.push_back(info);
        }
        return result;
    }
    std::vector<std::string> getOnlinePaymentMethods() const
    {
        //returns the two card types accepted for online orders
        return {"dragon credit card", "sorcerer credit card"};
    }
    CheckoutInfo getCheckoutInfo(const AuthService& auth)
    {
        //validates the customer session and builds the draft's totals, benefits, items, and action limits
        Customer& customer = checkoutCustomer(auth);
        normalizeBenefits(customer);
        const Order draft = checkoutDraft(customer, 1);

        CheckoutInfo info;
        info.customerName = customer.getName();
        info.username = customer.getUsername();
        info.loyaltyNumber = customer.getLoyaltyNumber();
        info.availablePoints = customer.getLoyaltyPoints();
        info.availableCreditCents = customer.getAccountCreditCents();
        info.subtotalCents = draft.getSubtotalCents();
        info.taxCents = draft.calculateTaxCents(TAX_RATE_PERCENT);
        info.finalTotalCents = info.subtotalCents + info.taxCents;
        info.pointsRedeemed = checkout.points;
        info.accountCreditUsedCents = checkout.creditCents;
        info.amountDueCents = draft.getAmountDueCents(TAX_RATE_PERCENT);
        info.pointsOnCompletion = draft.getPointsOnCompletion(TAX_RATE_PERCENT);
        info.maximumPoints = std::min(info.availablePoints, draft.getMaxRedeemablePoints());
        info.maximumCreditCents = std::min(info.availableCreditCents,
                                         info.finalTotalCents - checkout.points * 100);
        info.paymentMethod = checkout.paymentMethod;
        info.message = checkout.notice;
        info.revision = checkout.revision;

        for (const CheckoutLine& line : checkout.lines) {
            const MenuItem& item = menuItem(line.menuItemId);
            CartItemInfo row;
            row.menuItemId = line.menuItemId;
            row.name = item.getName();
            row.unitPriceCents = item.getPriceCents();
            row.quantity = line.quantity;
            row.lineTotalCents = item.getPriceCents() * line.quantity;
            row.maximumQuantity = item.getPriceCents() == 0
                ? std::numeric_limits<int>::max()
                : (maximumSubtotal() - (info.subtotalCents - row.lineTotalCents)) /
                  item.getPriceCents();
            info.items.push_back(row);
        }

        const Order* current = getCurrentOrder(customer.getUsername());
        info.hasCurrentOrder = current != nullptr;
        if (current) {
            info.currentOrderNumber = current->getOrderNumber();
            info.message = "Only one order can be placed at a time";
        } else if (info.message.empty()) {
            info.message = info.items.empty()
                ? "Choose items from the menu to build your order."
                : "Review your items and payment amounts before placing your order.";
        }
        info.canSubmit = !info.hasCurrentOrder && !info.items.empty();
        return info;
    }
    CheckoutInfo addMenuItem(
        const AuthService& auth, int menuItemId, int quantity)
    {
        //adds an item to the draft, merges matching quantities, and validates the updated total
        Customer& customer = checkoutCustomer(auth);
        requireEditable(customer);
        menuItem(menuItemId);
        if (quantity <= 0) throw std::invalid_argument("Quantity must be at least one.");

        std::vector<CheckoutLine> updated = checkout.lines;
        auto found = std::find_if(updated.begin(), updated.end(),
            [menuItemId](const CheckoutLine& line) {
                //checks whether a draft line represents the menu item being added
                return line.menuItemId == menuItemId;
            });
        if (found == updated.end()) {
            updated.push_back(CheckoutLine(menuItemId, quantity));
        } else {
            if (quantity > std::numeric_limits<int>::max() - found->quantity) {
                throw std::invalid_argument("The item quantity is too large.");
            }
            found->quantity += quantity;
        }
        checkedSubtotal(updated);
        checkout.lines.swap(updated);
        checkout.notice.clear();
        touchCheckout();
        return getCheckoutInfo(auth);
    }
    CheckoutInfo setCartQuantity(
        const AuthService& auth, int menuItemId, int quantity)
    {
        //changes an existing draft item's quantity after validating the new quantity and total
        Customer& customer = checkoutCustomer(auth);
        requireEditable(customer);
        if (quantity <= 0) throw std::invalid_argument("Quantity must be at least one.");
        std::vector<CheckoutLine> updated = checkout.lines;
        auto found = std::find_if(updated.begin(), updated.end(),
            [menuItemId](const CheckoutLine& line) {
                //checks whether a draft line represents the menu item whose quantity is being changed
                return line.menuItemId == menuItemId;
            });
        if (found == updated.end()) throw std::invalid_argument("The item is not in your order.");
        found->quantity = quantity;
        checkedSubtotal(updated);
        checkout.lines.swap(updated);
        checkout.notice.clear();
        touchCheckout();
        return getCheckoutInfo(auth);
    }
    CheckoutInfo removeCartItem(const AuthService& auth, int menuItemId)
    {
        //removes the selected item from the draft and returns the updated checkout information
        Customer& customer = checkoutCustomer(auth);
        requireEditable(customer);
        auto found = std::find_if(checkout.lines.begin(), checkout.lines.end(),
            [menuItemId](const CheckoutLine& line) {
                //checks whether a draft line represents the menu item selected for removal
                return line.menuItemId == menuItemId;
            });
        if (found == checkout.lines.end()) throw std::invalid_argument("Select an item in your order.");
        checkout.lines.erase(found);
        checkout.notice.clear();
        touchCheckout();
        return getCheckoutInfo(auth);
    }
    CheckoutInfo setRedeemedPoints(const AuthService& auth, int points)
    {
        //validates and saves the draft's redeemed points and returns updated checkout information
        Customer& customer = checkoutCustomer(auth);
        requireEditable(customer);
        const CheckoutInfo before = getCheckoutInfo(auth);
        if (points < 0 || points > before.maximumPoints) {
            throw std::invalid_argument("Points exceed your balance or half the pre-tax order cost.");
        }
        checkout.points = points;
        checkout.notice.clear();
        touchCheckout();
        return getCheckoutInfo(auth);
    }
    CheckoutInfo setAccountCredit(const AuthService& auth, int cents)
    {
        //validates and saves the draft's account credit and returns updated checkout information
        Customer& customer = checkoutCustomer(auth);
        requireEditable(customer);
        const CheckoutInfo before = getCheckoutInfo(auth);
        if (cents < 0 || cents > before.maximumCreditCents) {
            throw std::invalid_argument("Account credit exceeds your balance or the remaining amount.");
        }
        checkout.creditCents = cents;
        checkout.notice.clear();
        touchCheckout();
        return getCheckoutInfo(auth);
    }
    CheckoutInfo setPaymentMethod(
        const AuthService& auth, const std::string& method)
    {
        //validates the selected online card type and saves it to the current draft
        Customer& customer = checkoutCustomer(auth);
        requireEditable(customer);
        const auto methods = getOnlinePaymentMethods();
        if (std::find(methods.begin(), methods.end(), method) == methods.end()) {
            throw std::invalid_argument("Online orders require a dragon or sorcerer credit card.");
        }
        checkout.paymentMethod = method;
        checkout.notice.clear();
        touchCheckout();
        return getCheckoutInfo(auth);
    }
    int submitOnlineOrder(const AuthService& auth, unsigned long long reviewedRevision)
    {
        //rechecks the reviewed draft, submits it as a paid order, and clears its checkout contents
        const CheckoutInfo reviewed = getCheckoutInfo(auth);
        if (reviewed.hasCurrentOrder) throw std::logic_error("Only one order can be placed at a time");
        if (reviewed.revision != reviewedRevision) {
            throw std::logic_error("Your order changed. Review the updated totals before placing it.");
        }
        if (!reviewed.canSubmit) throw std::invalid_argument("Add at least one item to your order.");

        Customer& customer = checkoutCustomer(auth);
        const int number = nextOrderNumber();
        Order order = checkoutDraft(customer, number);
        if (!submitOrder(order)) throw std::logic_error("Only one order can be placed at a time");

        checkout.lines.clear();
        checkout.points = 0;
        checkout.creditCents = 0;
        checkout.notice.clear();
        touchCheckout();
        return number;
    }
    void clearCheckout()
    {
        //resets the checkout cart and assigns a new revision to invalidate earlier reviews
        checkout = CheckoutCart();
        touchCheckout();
    }

private:
    struct CheckoutLine {
        int menuItemId;
        int quantity;
        CheckoutLine(int id, int count) : menuItemId(id), quantity(count) {
            //stores the menu item ID and quantity for one draft cart line
        }
    };

    struct CheckoutCart {
        std::string username;
        unsigned long long sessionId = 0;
        std::vector<CheckoutLine> lines;
        int points = 0;
        int creditCents = 0;
        std::string paymentMethod = "dragon credit card";
        std::string notice;
        unsigned long long revision = 0;
    };

    CafeMenu menu;
    CheckoutCart checkout;
    unsigned long long nextCheckoutRevision = 1;

    Customer& checkoutCustomer(const AuthService& auth)
    {
        //validates the customer session and resets the draft when its owner or session changes
        if (auth.getCurrentUsername().empty() || auth.getCurrentRole() != "customer") {
            clearCheckout();
            throw std::logic_error("Sign in as a customer to place an online order.");
        }
        Customer* customer = findCustomer(auth.getCurrentUsername());
        if (!customer) {
            clearCheckout();
            throw std::logic_error("The customer is not registered with the order service.");
        }
        if (checkout.username != customer->getUsername() ||
            checkout.sessionId != auth.getCurrentSessionId()) {
            clearCheckout();
            checkout.username = customer->getUsername();
            checkout.sessionId = auth.getCurrentSessionId();
        }
        return *customer;
    }
    void requireEditable(const Customer& customer) const
    {
        //rejects draft changes when the customer already has a current order
        if (getCurrentOrder(customer.getUsername())) {
            throw std::logic_error("Only one order can be placed at a time");
        }
    }
    const MenuItem& menuItem(int menuItemId) const
    {
        //validates a menu item ID and returns its corresponding menu entry
        if (menuItemId <= 0 ||
            static_cast<std::size_t>(menuItemId) > menu.getItems().size()) {
            throw std::invalid_argument("Select a menu item.");
        }
        return menu.getItems()[static_cast<std::size_t>(menuItemId - 1)];
    }
    int checkedSubtotal(const std::vector<CheckoutLine>& lines) const
    {
        //validates draft quantities and sums their prices while checking the allowed subtotal limit
        long long subtotal = 0;
        for (const CheckoutLine& line : lines) {
            if (line.quantity <= 0) {
                throw std::invalid_argument("Quantity must be at least one.");
            }
            const MenuItem& item = menuItem(line.menuItemId);
            const long long total = static_cast<long long>(item.getPriceCents()) * line.quantity;
            if (total > maximumSubtotal() - subtotal) {
                throw std::invalid_argument("This quantity makes the order total too large.");
            }
            subtotal += total;
        }
        return static_cast<int>(subtotal);
    }
    void normalizeBenefits(const Customer& customer)
    {
        //reduces draft points or credit when the available balances or order limits no longer allow them
        const int subtotal = checkedSubtotal(checkout.lines);
        const int maxPoints = std::min(customer.getLoyaltyPoints(), subtotal / 200);
        const int tax = (subtotal / 100) * TAX_RATE_PERCENT +
                        ((subtotal % 100) * TAX_RATE_PERCENT + 50) / 100;
        const int adjustedPoints = std::min(checkout.points, maxPoints);
        const int maxCredit = std::min(customer.getAccountCreditCents(),
                                      subtotal + tax - adjustedPoints * 100);
        const int adjustedCredit = std::min(checkout.creditCents, maxCredit);
        if (adjustedPoints != checkout.points || adjustedCredit != checkout.creditCents) {
            checkout.points = adjustedPoints;
            checkout.creditCents = adjustedCredit;
            checkout.notice = "Points or account credit were adjusted to match your order.";
            touchCheckout();
        }
    }
    Order checkoutDraft(const Customer& customer, int orderNumber) const
    {
        //constructs a draft order from the cart items, customer identity, benefits, and payment method
        checkedSubtotal(checkout.lines);
        std::vector<OrderItem> items;
        for (const CheckoutLine& line : checkout.lines) {
            const MenuItem& item = menuItem(line.menuItemId);
            items.push_back(OrderItem(item.getName(), item.getPriceCents(),
                                      line.quantity, item.getIngredients()));
        }
        return Order(orderNumber, customer.getUsername(), customer.getName(), items,
                     Order::Status::Draft, Order::PurchaseType::Online, 0,
                     checkout.points, checkout.creditCents, TAX_RATE_PERCENT,
                     checkout.paymentMethod);
    }
    int nextOrderNumber() const
    {
        //returns the next unused order number after checking for integer overflow
        int largest = 0;
        for (const auto& order : orders) largest = std::max(largest, order->getOrderNumber());
        if (largest == std::numeric_limits<int>::max()) {
            throw std::overflow_error("No order numbers are available.");
        }
        return largest + 1;
    }
    void touchCheckout()
    {
        //assigns a new revision number so draft changes invalidate an earlier checkout review
        checkout.revision = nextCheckoutRevision++;
    }
};

#endif

