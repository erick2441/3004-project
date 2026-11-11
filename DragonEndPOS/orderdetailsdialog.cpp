#include "orderdetailsdialog.h"
#include "ui_orderdetailsdialog.h"

#include <QAbstractItemView>
#include <QHeaderView>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStringList>
#include <QTableView>

namespace {
QString money(int cents)
{
    //formats an integer amount in cents as dollars with two decimal places
    return QString("$%1").arg(cents / 100.0, 0, 'f', 2);
}
}

OrderDetailsDialog::OrderDetailsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OrderDetailsDialog),
    itemsModel(new QStandardItemModel(this))
{
    //initializes the order-details form and configures the read-only item table model
    ui->setupUi(this);

    itemsModel->setColumnCount(4);
    itemsModel->setHorizontalHeaderLabels(
        QStringList() << "Item" << "Unit price" << "Quantity" << "Line total"
    );

    auto* table = ui->orderDetailsTable;
    table->setModel(itemsModel);
    table->horizontalHeader()->setStretchLastSection(false);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setAlternatingRowColors(true);
    table->setSortingEnabled(false);
}

OrderDetailsDialog::~OrderDetailsDialog()
{
    //releases the generated order-details form
    delete ui;
}

void OrderDetailsDialog::setOrderDetails(const OrderService::OrderDetailsInfo& details)
{
    //fills the dialog's items, totals, benefits, identity, and status from service display data
    setWindowTitle(QString("Order #%1").arg(details.orderNumber));
    ui->orderNumberValue->setText(QString::number(details.orderNumber));
    ui->placedAtValue->setText(QString::fromStdString(details.placedAt));

    itemsModel->removeRows(0, itemsModel->rowCount());
    itemsModel->setRowCount(static_cast<int>(details.items.size()));

    for (int row = 0; row < static_cast<int>(details.items.size()); ++row) {
        const auto& item = details.items[row];
        auto* name = new QStandardItem(QString::fromStdString(item.name));
        auto* unitPrice = new QStandardItem(money(item.unitPriceCents));
        auto* quantity = new QStandardItem(QString::number(item.quantity));
        auto* lineTotal = new QStandardItem(money(item.lineTotalCents));

        unitPrice->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        quantity->setTextAlignment(Qt::AlignCenter);
        lineTotal->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        itemsModel->setItem(row, 0, name);
        itemsModel->setItem(row, 1, unitPrice);
        itemsModel->setItem(row, 2, quantity);
        itemsModel->setItem(row, 3, lineTotal);
    }

    ui->subtotalValue->setText(money(details.subtotalCents));
    ui->taxValue->setText(money(details.taxCents));
    ui->totalValue->setText(money(details.finalTotalCents));
    ui->loyaltyPointsUsedValue->setText(QString::number(details.pointsRedeemed));
    ui->accountCreditUsedValue->setText(money(details.accountCreditUsedCents));
    ui->amountChargedValue->setText(money(details.amountChargedCents));
    ui->loyaltyPointsReceivedValue->setText(QString::number(details.pointsEarned));
    ui->accountCreditReceivedValue->setText(money(details.accountCreditReceivedCents));
    ui->statusValue->setText(QString::fromStdString(details.status));
    ui->typeValue->setText(QString::fromStdString(details.source));
}
