#ifndef ORDERDETAILSDIALOG_H
#define ORDERDETAILSDIALOG_H

#include <QDialog>
#include "orderservice.h"

class QStandardItemModel;

namespace Ui {
class OrderDetailsDialog;
}

class OrderDetailsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OrderDetailsDialog(QWidget *parent = nullptr);
    ~OrderDetailsDialog();

    void setOrderDetails(const OrderService::OrderDetailsInfo& details);

private:
    Ui::OrderDetailsDialog *ui;
    QStandardItemModel *itemsModel;
};

#endif
