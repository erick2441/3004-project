#include "orderqueuepage.h"
#include "ui_orderqueuepage.h"
#include "orderdetailsdialog.h"

#include <QAbstractItemView>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QList>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QStringList>
#include <QTableWidgetItem>
#include <QTimer>
#include <QTreeWidgetItem>
#include <exception>

OrderQueuePage::OrderQueuePage(AuthService& authService, OrderService& orderService,
                               QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::OrderQueuePage)
    , authService(authService)
    , orderService(orderService)
    , refreshTimer(new QTimer(this))
{
    //sets up the kitchen queue, preparation views, service references, and automatic refresh timer
    ui->setupUi(this);
    ui->queueSplitter->setSizes(QList<int>() << 620 << 420);
    ui->queueSplitter->setStretchFactor(0, 3);
    ui->queueSplitter->setStretchFactor(1, 2);

    ui->queueTable->setSortingEnabled(false);
    ui->queueTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->queueTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->queueTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->queueTable->setAlternatingRowColors(true);
    ui->queueTable->verticalHeader()->setVisible(false);
    ui->queueTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->queueTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->queueTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->queueTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);

    ui->preparationTree->setSelectionMode(QAbstractItemView::NoSelection);
    ui->preparationTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->preparationTree->header()->setStretchLastSection(false);
    ui->preparationTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->preparationTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->preparationTree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->selectedOrderLabel->setTextFormat(Qt::PlainText);
    ui->staffLabel->setTextFormat(Qt::PlainText);
    ui->messageLabel->setTextFormat(Qt::PlainText);

    refreshTimer->setInterval(1500);
    connect(refreshTimer, &QTimer::timeout, this, [this]() {
        //refreshes the visible kitchen page when an order is not currently being completed
        if (isVisible() && !completing) refresh();
    });
    clear();
}

OrderQueuePage::~OrderQueuePage()
{
    //releases the generated kitchen page form
    delete ui;
}

bool OrderQueuePage::hasKitchenSession() const
{
    //checks that the displayed queue still belongs to the current kitchen login session
    return authService.isKitchenStaff() &&
           authService.getCurrentSessionId() == displayedSession;
}

int OrderQueuePage::selectedOrderNumber() const
{
    //returns the selected row's stored order number or zero when no order is selected
    const auto rows = ui->queueTable->selectionModel()->selectedRows();
    if (rows.isEmpty()) return 0;
    const auto* item = ui->queueTable->item(rows.first().row(), 0);
    return item ? item->data(Qt::UserRole).toInt() : 0;
}

void OrderQueuePage::clearDetails()
{
    //clears the preparation panel and disables actions that require a selected order
    ui->selectedOrderLabel->setText("Select an order to see its preparation details.");
    ui->preparationTree->clear();
    ui->detailsButton->setEnabled(false);
    ui->completeButton->setEnabled(false);
}

void OrderQueuePage::clear()
{
    //stops automatic refresh and clears the kitchen identity, queue, selection, and cached data
    refreshTimer->stop();
    displayedSession = 0;
    displayedQueue.clear();
    const QSignalBlocker blocker(ui->queueTable);
    ui->queueTable->clearContents();
    ui->queueTable->setRowCount(0);
    ui->queueTable->setEnabled(false);
    ui->refreshButton->setEnabled(false);
    ui->staffLabel->setText("Kitchen staff");
    ui->queueCountLabel->setText("0 orders waiting");
    ui->messageLabel->setText("Sign in as kitchen staff to view the order queue.");
    clearDetails();
}

void OrderQueuePage::refresh()
{
    //reloads changed queue data while preserving the selected order within the same kitchen session
    if (completing) return;
    if (!authService.isKitchenStaff()) {
        clear();
        return;
    }

    try {
        const int previousOrder = hasKitchenSession() ? selectedOrderNumber() : 0;
        const auto user = authService.getCurrentUserInfo();
        const auto queue = orderService.getKitchenQueue(authService);

        bool sameQueue = hasKitchenSession() && queue.size() == displayedQueue.size();
        for (std::size_t i = 0; sameQueue && i < queue.size(); ++i) {
            const auto& before = displayedQueue[i];
            const auto& after = queue[i];
            sameQueue = before.orderNumber == after.orderNumber &&
                        before.customerName == after.customerName &&
                        before.placedAt == after.placedAt && before.source == after.source;
        }
        if (sameQueue) return;
        displayedSession = authService.getCurrentSessionId();

        ui->staffLabel->setText(QString("Kitchen staff: %1 (%2)")
            .arg(QString::fromStdString(user.name), QString::fromStdString(user.username)));
        ui->queueCountLabel->setText(QString("%1 order(s) waiting")
            .arg(static_cast<int>(queue.size())));
        ui->queueTable->setEnabled(true);
        ui->refreshButton->setEnabled(true);

        int selectedRow = queue.empty() ? -1 : 0;
        {
            const QSignalBlocker blocker(ui->queueTable);
            ui->queueTable->clearContents();
            ui->queueTable->setRowCount(static_cast<int>(queue.size()));
            for (int row = 0; row < static_cast<int>(queue.size()); ++row) {
                const auto& order = queue[row];
                auto* number = new QTableWidgetItem(QString::number(order.orderNumber));
                number->setData(Qt::UserRole, order.orderNumber);
                ui->queueTable->setItem(row, 0, number);
                ui->queueTable->setItem(row, 1, new QTableWidgetItem(
                    QString::fromStdString(order.customerName)));
                ui->queueTable->setItem(row, 2, new QTableWidgetItem(
                    QString::fromStdString(order.placedAt)));
                ui->queueTable->setItem(row, 3, new QTableWidgetItem(
                    QString::fromStdString(order.source)));
                if (order.orderNumber == previousOrder) selectedRow = row;
            }
            if (selectedRow >= 0) {
                ui->queueTable->setCurrentCell(selectedRow, 0);
                ui->queueTable->selectRow(selectedRow);
            }
        }

        ui->messageLabel->setText(queue.empty()
            ? "No orders waiting for preparation."
            : "Oldest orders are listed first. Select an order to prepare it.");
        displayedQueue = queue;
        displaySelectedOrder();
        if (!refreshTimer->isActive()) refreshTimer->start();
    }
    catch (const std::exception& error) {
        clear();
        ui->messageLabel->setText(QString::fromUtf8(error.what()));
    }
}

void OrderQueuePage::displaySelectedOrder()
{
    //loads the selected order's preparation details and updates the available order actions
    clearDetails();
    if (!hasKitchenSession()) return;
    const int number = selectedOrderNumber();
    if (number <= 0) return;

    try {
        const auto details = orderService.getKitchenOrderDetails(authService, number);
        const auto& order = details.order;
        ui->selectedOrderLabel->setText(
            QString("Order #%1\nCustomer: %2\nPlaced: %3\nSource: %4 | Status: %5\nPayment: %6 | Total: $%7")
                .arg(order.orderNumber)
                .arg(QString::fromStdString(order.customerName))
                .arg(QString::fromStdString(order.placedAt))
                .arg(QString::fromStdString(order.source))
                .arg(QString::fromStdString(order.status))
                .arg(QString::fromStdString(order.paymentStatus))
                .arg(order.finalTotalCents / 100.0, 0, 'f', 2)
        );

        for (const auto& item : details.preparationItems) {
            auto* parentItem = new QTreeWidgetItem(ui->preparationTree);
            parentItem->setText(0, QString::fromStdString(item.name));
            parentItem->setToolTip(0, QString::fromStdString(item.name));
            parentItem->setText(1, QString::number(item.quantity));
            parentItem->setText(2, "item(s)");
            for (const auto& ingredient : item.ingredients) {
                auto* row = new QTreeWidgetItem(parentItem);
                row->setText(0, QString::fromStdString(ingredient.name));
                row->setToolTip(0, QString::fromStdString(ingredient.name));
                row->setText(1, QString::number(static_cast<qlonglong>(ingredient.quantity)));
                row->setText(2, QString::fromStdString(ingredient.unit));
            }
            if (item.ingredients.empty()) {
                auto* row = new QTreeWidgetItem(parentItem);
                row->setText(0, "No ingredient information recorded");
            }
        }
        ui->preparationTree->expandAll();
        ui->detailsButton->setEnabled(true);
        ui->completeButton->setEnabled(!completing);
    }
    catch (const std::exception& error) {
        ui->messageLabel->setText(QString::fromUtf8(error.what()));
    }
}

void OrderQueuePage::on_queueTable_itemSelectionChanged()
{
    //updates the preparation panel when the selected queue row changes
    displaySelectedOrder();
}

void OrderQueuePage::on_refreshButton_clicked()
{
    //requests an immediate refresh of the kitchen queue display
    refresh();
}

void OrderQueuePage::on_detailsButton_clicked()
{
    //opens the selected kitchen order in the existing details dialog and refreshes afterward
    if (!hasKitchenSession()) {
        refresh();
        return;
    }
    const int number = selectedOrderNumber();
    if (number <= 0) return;
    try {
        const auto details = orderService.getKitchenOrderDetails(authService, number);
        OrderDetailsDialog dialog(this);
        dialog.setOrderDetails(details.order);
        dialog.exec();
        refresh();
    }
    catch (const std::exception& error) {
        refresh();
        QMessageBox::warning(this, "Order details", QString::fromUtf8(error.what()));
    }
}

void OrderQueuePage::on_completeButton_clicked()
{
    //completes the selected order through the service and refreshes the queue or reports an error
    if (completing) return;
    if (!hasKitchenSession()) {
        refresh();
        return;
    }
    const int number = selectedOrderNumber();
    if (number <= 0) return;

    completing = true;
    ui->completeButton->setEnabled(false);
    try {
        orderService.completeKitchenOrder(authService, number);
        completing = false;
        refresh();
        ui->messageLabel->setText(QString("Order #%1 completed and removed from the preparation queue.")
            .arg(number));
        emit orderCompleted(number);
    }
    catch (const std::exception& error) {
        completing = false;
        refresh();
        QMessageBox::warning(this, "Complete order", QString::fromUtf8(error.what()));
    }
}

void OrderQueuePage::on_signOutButton_clicked()
{
    //requests that the main window sign out the current kitchen user
    emit signOutRequested();
}
