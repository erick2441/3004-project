#include "menupage.h"
#include "ui_menupage.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStringList>
#include <QTableWidgetItem>
#include <QTreeWidgetItem>

#include <cmath>
#include <limits>
#include <map>

namespace {
QString money(int cents)
{
    //formats an integer amount in cents as dollars with two decimal places
    return QString("$%1").arg(cents / 100.0, 0, 'f', 2);
}
}

OrderPage::OrderPage(AuthService& authService, OrderService& orderService, QWidget *parent) :
    QWidget(parent), ui(new Ui::OrderPage),
    authService(authService), orderService(orderService)
{
    //sets up the menu, checkout controls, payment choices, and connections to page actions
    ui->setupUi(this);
    ui->treeWidget->setColumnCount(3);
    ui->treeWidget->setHeaderLabels(QStringList() << "Menu item" << "Price" << "Availability");
    ui->treeWidget->header()->setStretchLastSection(false);
    ui->treeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->treeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->treeWidget->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->treeWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

    ui->tableWidget_2->setColumnCount(4);
    ui->tableWidget_2->setHorizontalHeaderLabels(
        QStringList() << "Item" << "Unit price" << "Quantity" << "Line total");
    ui->tableWidget_2->horizontalHeader()->setStretchLastSection(false);
    ui->tableWidget_2->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int column = 1; column < 4; ++column) {
        ui->tableWidget_2->horizontalHeader()->setSectionResizeMode(
            column, QHeaderView::ResizeToContents);
    }
    ui->tableWidget_2->verticalHeader()->setVisible(false);
    ui->tableWidget_2->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget_2->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableWidget_2->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget_2->setSortingEnabled(false);
    ui->tableWidget_2->setAlternatingRowColors(true);

    ui->spinBox_3->setRange(1, std::numeric_limits<int>::max());
    ui->spinBox_3->setValue(1);
    ui->spinBox_3->setKeyboardTracking(false);
    ui->pointsSpin->setKeyboardTracking(false);
    ui->creditSpin->setDecimals(2);
    ui->creditSpin->setSingleStep(0.01);
    ui->creditSpin->setPrefix("$");
    ui->creditSpin->setKeyboardTracking(false);

    for (const std::string& method : orderService.getOnlinePaymentMethods()) {
        const QString text = QString::fromStdString(method);
        ui->paymentMethodCombo->addItem(text, text);
    }

    connect(ui->pushButton, &QPushButton::clicked, this, &OrderPage::backRequested);
    connect(ui->pushButton_4, &QPushButton::clicked, this, &OrderPage::addSelectedItem);
    connect(ui->pushButton_2, &QPushButton::clicked, this, &OrderPage::removeSelectedItem);
    connect(ui->pushButton_3, &QPushButton::clicked, this, &OrderPage::reviewAndPlaceOrder);
    connect(ui->pointsSpin, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &OrderPage::changePoints);
    connect(ui->creditSpin, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &OrderPage::changeCredit);
    connect(ui->paymentMethodCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &OrderPage::changePaymentMethod);
    connect(ui->treeWidget, &QTreeWidget::itemSelectionChanged, this, &OrderPage::updateActions);
    connect(ui->tableWidget_2, &QTableWidget::itemSelectionChanged, this, &OrderPage::updateActions);

    populateMenu();
    setUnavailable("Sign in as a customer to place an online order.");
}

OrderPage::~OrderPage()
{
    //releases the generated menu and checkout form
    delete ui;
}

void OrderPage::populateMenu()
{
    //groups the service's menu display records by category and fills the menu tree
    const QSignalBlocker blocker(ui->treeWidget);
    ui->treeWidget->clear();
    std::map<std::string, QTreeWidgetItem*> categories;
    for (const auto& info : orderService.getMenuInfo()) {
        QTreeWidgetItem*& category = categories[info.category];
        if (!category) {
            category = new QTreeWidgetItem(ui->treeWidget);
            category->setText(0, QString::fromStdString(info.category));
            category->setFirstColumnSpanned(true);
            category->setFlags(category->flags() & ~Qt::ItemIsSelectable);
        }
        auto* item = new QTreeWidgetItem(category);
        item->setText(0, QString::fromStdString(info.name));
        item->setText(1, money(info.unitPriceCents));
        item->setText(2, QString::fromStdString(info.availability));
        item->setData(0, Qt::UserRole, info.menuItemId);
        item->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
    }
    ui->treeWidget->expandAll();
}

void OrderPage::refresh()
{
    //reloads the menu and checkout display or shows why checkout is unavailable
    populateMenu();
    try {
        renderCheckout(orderService.getCheckoutInfo(authService));
    } catch (const std::exception& error) {
        setUnavailable(QString::fromUtf8(error.what()));
    }
}

void OrderPage::clear()
{
    //clears the draft and menu selection and resets checkout to its signed-out display
    orderService.clearCheckout();
    ui->treeWidget->clearSelection();
    ui->spinBox_3->setValue(1);
    setUnavailable("Sign in as a customer to place an online order.");
}

void OrderPage::setUnavailable(const QString& message)
{
    //resets checkout values, displays an availability message, and disables ordering controls
    OrderService::CheckoutInfo empty;
    renderCheckout(empty);
    ui->label_20->setText("Customer: —");
    ui->label_21->setText("Loyalty points: —");
    ui->label_22->setText("Account credit: —");
    ui->label_4->setText(message);
    ui->menuGroup->setEnabled(false);
    ui->orderCartGroup->setEnabled(false);
}

void OrderPage::renderCheckout(const OrderService::CheckoutInfo& info)
{
    //updates cart rows, balances, totals, payment choices, and action limits from service data
    rendering = true;
    displayed = info;
    ui->menuGroup->setEnabled(true);
    ui->orderCartGroup->setEnabled(true);
    ui->label_20->setText(QString("Customer: %1 (%2)")
        .arg(QString::fromStdString(info.customerName), QString::fromStdString(info.username)));
    ui->label_21->setText(QString("Loyalty number: %1 · Available points: %2")
        .arg(QString::fromStdString(info.loyaltyNumber)).arg(info.availablePoints));
    ui->label_22->setText(QString("Available account credit: %1").arg(money(info.availableCreditCents)));

    auto* table = ui->tableWidget_2;
    bool sameRows = table->rowCount() == static_cast<int>(info.items.size());
    for (int row = 0; sameRows && row < table->rowCount(); ++row) {
        sameRows = table->item(row, 0) &&
            table->item(row, 0)->data(Qt::UserRole).toInt() == info.items[row].menuItemId;
    }
    if (!sameRows) {
        table->clearContents();
        table->setRowCount(static_cast<int>(info.items.size()));
    }

    for (int row = 0; row < static_cast<int>(info.items.size()); ++row) {
        const auto& item = info.items[row];
        for (int column = 0; column < 4; ++column) {
            if (!table->item(row, column)) table->setItem(row, column, new QTableWidgetItem);
        }
        table->item(row, 0)->setText(QString::fromStdString(item.name));
        table->item(row, 0)->setToolTip(QString::fromStdString(item.name));
        table->item(row, 0)->setData(Qt::UserRole, item.menuItemId);
        table->item(row, 1)->setText(money(item.unitPriceCents));
        table->item(row, 2)->setText(QString::number(item.quantity));
        table->item(row, 3)->setText(money(item.lineTotalCents));
        table->item(row, 1)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        table->item(row, 3)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        auto* quantity = qobject_cast<QSpinBox*>(table->cellWidget(row, 2));
        if (!quantity) {
            quantity = new QSpinBox(table);
            quantity->setObjectName(QString("cartQuantity_%1").arg(item.menuItemId));
            quantity->setKeyboardTracking(false);
            table->setCellWidget(row, 2, quantity);
            const int id = item.menuItemId;
            connect(quantity, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                    this, [this, id](int count) {
                        //passes the edited cart quantity to the page's quantity-change handler
                        changeQuantity(id, count);
                    });
        }
        const QSignalBlocker blocker(quantity);
        quantity->setRange(1, item.maximumQuantity);
        quantity->setValue(item.quantity);
    }

    const QSignalBlocker pointsBlocker(ui->pointsSpin);
    const QSignalBlocker creditBlocker(ui->creditSpin);
    const QSignalBlocker methodBlocker(ui->paymentMethodCombo);
    ui->pointsSpin->setRange(0, info.maximumPoints);
    ui->pointsSpin->setValue(info.pointsRedeemed);
    ui->creditSpin->setRange(0, info.maximumCreditCents / 100.0);
    ui->creditSpin->setValue(info.accountCreditUsedCents / 100.0);
    const int methodIndex = ui->paymentMethodCombo->findData(QString::fromStdString(info.paymentMethod));
    ui->paymentMethodCombo->setCurrentIndex(methodIndex < 0 ? 0 : methodIndex);
    ui->subtotalValue->setText(money(info.subtotalCents));
    ui->taxValue->setText(money(info.taxCents));
    ui->totalValue->setText(money(info.finalTotalCents));
    ui->pointsUsedValue->setText(QString::number(info.pointsRedeemed));
    ui->creditUsedValue->setText(money(info.accountCreditUsedCents));
    ui->paymentValue->setText(money(info.amountDueCents));
    ui->pointsEarnedValue->setText(QString::number(info.pointsOnCompletion));
    ui->label_4->setText(QString::fromStdString(info.message));
    ui->tableWidget_2->setEnabled(!info.hasCurrentOrder);
    ui->pointsSpin->setEnabled(!info.hasCurrentOrder && !info.items.empty());
    ui->creditSpin->setEnabled(!info.hasCurrentOrder && !info.items.empty());
    ui->paymentMethodCombo->setEnabled(!info.hasCurrentOrder);
    rendering = false;
    updateActions();
}

void OrderPage::updateActions()
{
    //enables menu and checkout actions according to the session, selection, and draft state
    if (rendering) return;
    const bool customer = !displayed.username.empty();
    const bool editable = customer && !displayed.hasCurrentOrder;
    const auto* selected = ui->treeWidget->currentItem();
    ui->pushButton_4->setEnabled(editable && selected && selected->isSelected() &&
                                selected->data(0, Qt::UserRole).isValid());
    ui->spinBox_3->setEnabled(editable);
    ui->pushButton_2->setEnabled(editable &&
        !ui->tableWidget_2->selectionModel()->selectedRows().isEmpty());
    ui->pushButton_3->setEnabled(customer && displayed.canSubmit);
}

void OrderPage::reportError(const std::exception& error)
{
    //shows the checkout error and reloads checkout data or its unavailable state
    QMessageBox::warning(this, "Online order", QString::fromUtf8(error.what()));
    try {
        renderCheckout(orderService.getCheckoutInfo(authService));
    } catch (const std::exception& refreshError) {
        setUnavailable(QString::fromUtf8(refreshError.what()));
    }
}

void OrderPage::addSelectedItem()
{
    //adds the selected menu item and quantity through the order service and refreshes checkout
    const auto* selected = ui->treeWidget->currentItem();
    if (!selected || !selected->data(0, Qt::UserRole).isValid()) return;
    ui->spinBox_3->interpretText();
    try {
        renderCheckout(orderService.addMenuItem(authService,
            selected->data(0, Qt::UserRole).toInt(), ui->spinBox_3->value()));
    } catch (const std::exception& error) { reportError(error); }
}

void OrderPage::removeSelectedItem()
{
    //removes the selected cart item through the order service and refreshes checkout
    const auto rows = ui->tableWidget_2->selectionModel()->selectedRows();
    if (rows.isEmpty()) return;
    const auto* item = ui->tableWidget_2->item(rows.first().row(), 0);
    if (!item) return;
    try {
        renderCheckout(orderService.removeCartItem(authService, item->data(Qt::UserRole).toInt()));
    } catch (const std::exception& error) { reportError(error); }
}

void OrderPage::changeQuantity(int menuItemId, int quantity)
{
    //updates a cart item's quantity through the service when the page is not rendering
    if (rendering) return;
    try { renderCheckout(orderService.setCartQuantity(authService, menuItemId, quantity)); }
    catch (const std::exception& error) { reportError(error); }
}

void OrderPage::changePoints(int points)
{
    //updates draft loyalty-point redemption through the service and refreshes checkout
    if (rendering) return;
    try { renderCheckout(orderService.setRedeemedPoints(authService, points)); }
    catch (const std::exception& error) { reportError(error); }
}

void OrderPage::changeCredit(double dollars)
{
    //converts the entered credit to cents and updates the draft through the service
    if (rendering) return;
    const int cents = static_cast<int>(std::llround(dollars * 100.0));
    try { renderCheckout(orderService.setAccountCredit(authService, cents)); }
    catch (const std::exception& error) { reportError(error); }
}

void OrderPage::changePaymentMethod(int index)
{
    //stores the selected online payment method through the service and refreshes checkout
    if (rendering || index < 0) return;
    try {
        renderCheckout(orderService.setPaymentMethod(authService,
            ui->paymentMethodCombo->itemData(index).toString().toStdString()));
    } catch (const std::exception& error) { reportError(error); }
}

QString OrderPage::reviewText(const OrderService::CheckoutInfo& info) const
{
    //formats the checkout items, totals, benefits, payment method, and potential points for review
    QStringList lines;
    for (const auto& item : info.items) {
        lines << QString("%1 × %2 @ %3 = %4").arg(QString::fromStdString(item.name))
            .arg(item.quantity).arg(money(item.unitPriceCents), money(item.lineTotalCents));
    }
    lines << ""
          << QString("Subtotal: %1").arg(money(info.subtotalCents))
          << QString("Tax (15%): %1").arg(money(info.taxCents))
          << QString("Final price including tax: %1").arg(money(info.finalTotalCents))
          << QString("Loyalty points redeemed: %1").arg(info.pointsRedeemed)
          << QString("Account credit applied: %1").arg(money(info.accountCreditUsedCents))
          << QString("Amount to be paid: %1").arg(money(info.amountDueCents))
          << QString("Payment method: %1").arg(QString::fromStdString(info.paymentMethod))
          << QString("Points on completion: %1").arg(info.pointsOnCompletion);
    return lines.join("\n");
}

void OrderPage::reviewAndPlaceOrder()
{
    //commits active editors, displays the checkout review, and submits the confirmed paid order
    ui->pointsSpin->interpretText();
    ui->creditSpin->interpretText();
    for (int row = 0; row < ui->tableWidget_2->rowCount(); ++row) {
        auto* editor = qobject_cast<QSpinBox*>(ui->tableWidget_2->cellWidget(row, 2));
        if (editor) editor->interpretText();
    }
    try {
        const auto reviewed = orderService.getCheckoutInfo(authService);
        renderCheckout(reviewed);
        if (!reviewed.canSubmit) {
            QMessageBox::information(this, "Online order",
                reviewed.hasCurrentOrder ? "Only one order can be placed at a time"
                                         : "Add at least one item to your order.");
            return;
        }
        QMessageBox review(this);
        review.setObjectName("orderReviewDialog");
        review.setWindowTitle("Review online order");
        review.setTextFormat(Qt::PlainText);
        review.setText("Place this order?");
        review.setInformativeText(reviewText(reviewed));
        review.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
        review.setDefaultButton(QMessageBox::Cancel);
        if (review.exec() != QMessageBox::Yes) return;

        const int number = orderService.submitOnlineOrder(authService, reviewed.revision);
        renderCheckout(orderService.getCheckoutInfo(authService));
        QMessageBox::information(this, "Order placed",
            QString("Order #%1 has been placed.\nPayment status: paid.\nOrder status: in progress.")
                .arg(number));
        emit orderPlaced(number);
    } catch (const std::exception& error) { reportError(error); }
}
