#ifndef ORDERQUEUEPAGE_H
#define ORDERQUEUEPAGE_H

#include <QWidget>
#include <vector>
#include "authservice.h"
#include "orderservice.h"

class QTimer;

namespace Ui {
class OrderQueuePage;
}

class OrderQueuePage : public QWidget
{
    Q_OBJECT

public:
    explicit OrderQueuePage(AuthService& authService, OrderService& orderService,
                            QWidget *parent = nullptr);
    ~OrderQueuePage();

    void refresh();
    void clear();

signals:
    void signOutRequested();
    void orderCompleted(int orderNumber);

private slots:
    void on_queueTable_itemSelectionChanged();
    void on_refreshButton_clicked();
    void on_detailsButton_clicked();
    void on_completeButton_clicked();
    void on_signOutButton_clicked();

private:
    Ui::OrderQueuePage *ui;
    AuthService& authService;
    OrderService& orderService;
    QTimer *refreshTimer;
    unsigned long long displayedSession = 0;
    bool completing = false;
    std::vector<OrderService::KitchenOrderInfo> displayedQueue;

    int selectedOrderNumber() const;
    bool hasKitchenSession() const;
    void clearDetails();
    void displaySelectedOrder();
};

#endif
