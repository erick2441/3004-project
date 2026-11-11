#include "loginpage.h"
#include "ui_loginpage.h"
#include <QMessageBox>
#include <QLineEdit>

LoginPage::LoginPage(AuthService& authService, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LoginPage)
    , authService(authService)
{
    //initializes the login form and connects the username field's return key to identification
    ui->setupUi(this);

    connect(ui->usernameEdit, &QLineEdit::returnPressed, this, &LoginPage::on_loginButton_clicked);
}

LoginPage::~LoginPage()
{
    //releases the generated login form
    delete ui;
}

void LoginPage::clear()
{
    //clears the previous login entry and returns keyboard focus to the username field
    ui->usernameEdit->clear();
    ui->usernameEdit->setFocus();
}

void LoginPage::on_loginButton_clicked()
{
    //identifies the entered user and signals the appropriate page or displays a login message
    const std::string username =
        ui->usernameEdit->text().trimmed().toStdString();

    const std::string role = authService.login(username);

    if (role.empty()) {
        QMessageBox::warning(
            this, "Login failed", "Invalid name or username."
        );
        return;
    }

    if (role == "customer") {
        emit customerLoginSucceeded();
    }
    else if (role == "kitchen staff") {
        emit kitchenLoginSucceeded();
    }
    else {
            const auto user = authService.getCurrentUserInfo();
            QMessageBox::information(this, "User identified",
                QString("%1\n%2\nThis account's interface is not implemented yet.")
                    .arg(QString::fromStdString(user.role), QString::fromStdString(user.name)));
    }
}

