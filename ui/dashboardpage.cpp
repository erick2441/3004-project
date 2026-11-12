#include "dashboardpage.h"
#include "ui_dashboardpage.h"
#include "orderdetailsdialog.h"

#include <QMessageBox>
#include <QTableWidgetItem>
#include <QAbstractItemView>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <exception>

CustomerDashboard::CustomerDashboard(AuthService& authService,
                                     OrderService& orderService,
                                     QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CustomerDashboard),
    authService(authService),
    orderService(orderService)
{
    ui->setupUi(this);
    ui->historyTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Stretch
    );

    ui->currentOrderLabel->setText("No current order");
    ui->viewCurrentOrderButton->setEnabled(false);
}

CustomerDashboard::~CustomerDashboard()
{
    delete ui;
}

void CustomerDashboard::setCustomer()
{
    const AuthService::CustomerInfo customer =
        authService.getCurrentCustomerInfo();

    ui->nameValue->setText(QString::fromStdString(customer.name));
    ui->loyaltyNumberValue->setText(
        QString::fromStdString(customer.loyaltyNumber)
    );
    ui->emailValue->setText(QString::fromStdString(customer.email));
    ui->homeAddressValue->setText(
        QString::fromStdString(customer.homeAddress)
    );
    ui->loyaltyPointsValue->setText(
        QString::number(customer.loyaltyPoints)
    );
    ui->accountCreditValue->setText(
        QString("$%1").arg(customer.accountCreditCents / 100.0, 0, 'f', 2)
    );
}

void CustomerDashboard::populateOrderHistory()
{
    const auto history = authService.getCurrentCustomerOrderHistory();
    auto* table = ui->historyTable;

    const bool sortingEnabled = table->isSortingEnabled();
    table->setSortingEnabled(false);
    table->clearContents();
    table->setRowCount(static_cast<int>(history.size()));

    for (int row = 0; row < static_cast<int>(history.size()); ++row) {
        const auto& order = history[row];

        table->setItem(row, 0, new QTableWidgetItem(
            QString::number(order.orderNumber)
        ));
        table->setItem(row, 1, new QTableWidgetItem(
            QString::fromStdString(order.placedAt)
        ));
        table->setItem(row, 2, new QTableWidgetItem(
            QString::fromStdString(order.source)
        ));
        table->setItem(row, 3, new QTableWidgetItem(
            QString::fromStdString(order.status)
        ));
        table->setItem(row, 4, new QTableWidgetItem(
            QString("$%1").arg(order.finalTotalCents / 100.0, 0, 'f', 2)
        ));
    }

    table->setSortingEnabled(sortingEnabled);
}

void CustomerDashboard::populateCurrentOrder()
{
    // Clear the previous customer's display before looking up the current one.
    ui->currentOrderLabel->setText("No current order");
    ui->currentOrderLabel->setToolTip(QString());
    ui->viewCurrentOrderButton->setEnabled(false);

    try {
        OrderService::OrderDetailsInfo details;
        if (!orderService.getCurrentOrderDetails(authService, details)) {
            return;
        }

        ui->currentOrderLabel->setText(
            QString("Order #%1 | %2 | Total: $%3")
                .arg(details.orderNumber)
                .arg(QString::fromStdString(details.status))
                .arg(details.finalTotalCents / 100.0, 0, 'f', 2)
        );
        ui->viewCurrentOrderButton->setEnabled(true);
    }
    catch (const std::exception& error) {
        ui->currentOrderLabel->setText("Unable to load current order");
        ui->currentOrderLabel->setToolTip(QString::fromUtf8(error.what()));
    }
}

void CustomerDashboard::refresh()
{
    setCustomer();
    populateOrderHistory();
    populateCurrentOrder();
}

void CustomerDashboard::on_orderDetailsButton_clicked()
{
    const auto selectedRows =
        ui->historyTable->selectionModel()->selectedRows();

    if (selectedRows.isEmpty()) {
        QMessageBox::information(this, "Order details", "Select an order first.");
        return;
    }

    const int row = selectedRows.first().row();
    const auto *numberCell = ui->historyTable->item(row, 0);

    bool validNumber = false;
    const int orderNumber = numberCell
        ? numberCell->text().toInt(&validNumber)
        : 0;

    if (!validNumber) {
        QMessageBox::warning(this, "Order details",
                             "The selected order number is invalid.");
        return;
    }

    try {
        const auto details = orderService.getOrderDetails(orderNumber);
        OrderDetailsDialog dialog(this);
        dialog.setOrderDetails(details);
        dialog.exec();
    }
    catch (const std::exception& error) {
        QMessageBox::warning(this, "Order details",
                             QString::fromUtf8(error.what()));
    }
}

void CustomerDashboard::on_viewCurrentOrderButton_clicked()
{
    try {
        // Fetch fresh data for the signed-in customer, without a table selection.
        OrderService::OrderDetailsInfo details;
        if (!orderService.getCurrentOrderDetails(authService, details)) {
            populateCurrentOrder();
            QMessageBox::information(this, "Current order",
                                     "You do not have a current order.");
            return;
        }

        // Reuse the same dialog and display-data type as the history button.
        OrderDetailsDialog dialog(this);
        dialog.setOrderDetails(details);
        dialog.exec();
    }
    catch (const std::exception& error) {
        QMessageBox::warning(this, "Order details",
                             QString::fromUtf8(error.what()));
    }
}

void CustomerDashboard::on_menuButton_clicked()
{
    emit orderPageRequested();
}

void CustomerDashboard::on_logoutButton_clicked()
{
    emit signOutRequested();
}

void CustomerDashboard::on_updateInfoButton_clicked()
{
    try {
        const auto customer = authService.getCurrentCustomerInfo();
        const auto editingSession = authService.getCurrentSessionId();

        QDialog dialog(this);
        dialog.setWindowTitle("Update contact information");
        dialog.setMinimumWidth(480);

        auto* layout = new QFormLayout(&dialog);
        auto* emailEdit = new QLineEdit(
            QString::fromStdString(customer.email), &dialog
        );
        emailEdit->setObjectName("contactEmailEdit");
        auto* homeAddressEdit = new QLineEdit(
            QString::fromStdString(customer.homeAddress), &dialog
        );
        homeAddressEdit->setObjectName("contactHomeAddressEdit");

        auto* errorLabel = new QLabel(&dialog);
        errorLabel->setObjectName("contactErrorLabel");
        errorLabel->setTextFormat(Qt::PlainText);
        errorLabel->setWordWrap(true);
        errorLabel->hide();

        auto* buttons = new QDialogButtonBox(
            QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog
        );
        buttons->button(QDialogButtonBox::Save)->setDefault(true);
        layout->addRow("Email:", emailEdit);
        layout->addRow("Home address:", homeAddressEdit);
        layout->addRow(errorLabel);
        layout->addRow(buttons);

        connect(buttons, &QDialogButtonBox::rejected,
                &dialog, &QDialog::reject);

        connect(
            buttons, &QDialogButtonBox::accepted, &dialog,
            [this, &dialog, emailEdit, homeAddressEdit, errorLabel, editingSession]() {
                try {
                    // Keep the form tied to the session in which it was opened.
                    if (authService.getCurrentSessionId() != editingSession) {
                        throw std::logic_error(
                            "Your sign-in session changed. Close this window and try again."
                        );
                    }
                    authService.updateCurrentCustomerContact(
                        emailEdit->text().toStdString(),
                        homeAddressEdit->text().toStdString()
                    );
                    dialog.accept();
                }
                catch (const std::exception& error) {
                    errorLabel->setText(QString::fromUtf8(error.what()));
                    errorLabel->show();
                }
            }
        );

        emailEdit->setFocus();
        if (dialog.exec() == QDialog::Accepted) {
            refresh();
        }
    }
    catch (const std::exception& error) {
        QMessageBox::warning(this, "Update contact information",
                             QString::fromUtf8(error.what()));
    }
}

void CustomerDashboard::clear()
{
    ui->nameValue->clear();
    ui->loyaltyNumberValue->clear();
    ui->emailValue->clear();
    ui->homeAddressValue->clear();
    ui->loyaltyPointsValue->clear();
    ui->accountCreditValue->clear();
    ui->historyTable->clearContents();
    ui->historyTable->setRowCount(0);
    ui->currentOrderLabel->setText("No current order");
    ui->currentOrderLabel->setToolTip(QString());
    ui->viewCurrentOrderButton->setEnabled(false);
}
