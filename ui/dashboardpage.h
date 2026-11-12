#ifndef CUSTOMERDASHBOARD_H
#define CUSTOMERDASHBOARD_H

#include <QWidget>
#include <QHeaderView>
#include <string>
#include "customer.h"
#include "authservice.h"
#include "orderservice.h"

namespace Ui {
class CustomerDashboard;
}

class CustomerDashboard : public QWidget
{
    Q_OBJECT

public:
    explicit CustomerDashboard(AuthService& authService, OrderService& orderService, QWidget *parent = nullptr);
    ~CustomerDashboard();

    void setCustomer();
    void populateOrderHistory();
    void populateCurrentOrder();
    void refresh();
    void clear();

signals:
    void orderPageRequested();
    void signOutRequested();

private slots:
    void on_orderDetailsButton_clicked();
    void on_menuButton_clicked();
    void on_updateInfoButton_clicked();
    void on_viewCurrentOrderButton_clicked();
    void on_logoutButton_clicked();

private:
    Ui::CustomerDashboard *ui;
    AuthService& authService;
    OrderService& orderService;

};

#endif // CUSTOMERDASHBOARD_H
