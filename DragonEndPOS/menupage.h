#ifndef MENUPAGE_H
#define MENUPAGE_H

#include <QWidget>
#include "authservice.h"
#include "orderservice.h"

namespace Ui { class OrderPage; }

class OrderPage : public QWidget
{
    Q_OBJECT

public:
    explicit OrderPage(AuthService& authService, OrderService& orderService,
                       QWidget *parent = nullptr);
    ~OrderPage();

    void refresh();
    void clear();

signals:
    void backRequested();
    void orderPlaced(int orderNumber);

private:
    Ui::OrderPage *ui;
    AuthService& authService;
    OrderService& orderService;
    OrderService::CheckoutInfo displayed;
    bool rendering = false;

    void populateMenu();
    void renderCheckout(const OrderService::CheckoutInfo& info);
    void setUnavailable(const QString& message);
    void updateActions();
    void addSelectedItem();
    void removeSelectedItem();
    void changeQuantity(int menuItemId, int quantity);
    void changePoints(int points);
    void changeCredit(double dollars);
    void changePaymentMethod(int index);
    void reviewAndPlaceOrder();
    void reportError(const std::exception& error);
    QString reviewText(const OrderService::CheckoutInfo& info) const;
};

#endif
