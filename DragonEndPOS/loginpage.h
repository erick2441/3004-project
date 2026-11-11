#ifndef LOGINPAGE_H
#define LOGINPAGE_H

#include <QWidget>
#include "authservice.h"

namespace Ui {
class LoginPage;
}

class LoginPage : public QWidget
{
    Q_OBJECT

public:
    explicit LoginPage(AuthService& authService, QWidget *parent = nullptr);
    ~LoginPage();
    void clear();

signals:
    void customerLoginSucceeded();
    void kitchenLoginSucceeded();

private slots:
    void on_loginButton_clicked();

private:
    Ui::LoginPage *ui;
    AuthService& authService;
};

#endif
