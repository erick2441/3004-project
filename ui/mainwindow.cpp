#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "loginpage.h"
#include "dashboardpage.h"
#include "menupage.h"
#include "orderqueuepage.h"

#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QScreen>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    //registers default data, creates the shared-service pages, and connects navigation and sign-out
    ui->setupUi(this);
    defaultUsers.registerWith(authService, orderService);

    auto *loginPage = new LoginPage(authService, this);
    auto *customerDashboard = new CustomerDashboard(authService, orderService, this);
    auto *orderPage = new OrderPage(authService, orderService, this);
    auto *orderQueuePage = new OrderQueuePage(authService, orderService, this);

    ui->pageStack->addWidget(loginPage);
    ui->pageStack->addWidget(customerDashboard);
    ui->pageStack->addWidget(orderPage);
    ui->pageStack->addWidget(orderQueuePage);
    ui->pageStack->setCurrentWidget(loginPage);

    connect(loginPage, &LoginPage::customerLoginSucceeded,
            this, [this, customerDashboard, orderQueuePage]() {
        //clears the kitchen view and opens the refreshed customer dashboard after customer login
        orderQueuePage->clear();
        customerDashboard->refresh();
        ui->pageStack->setCurrentWidget(customerDashboard);
    });

    connect(loginPage, &LoginPage::kitchenLoginSucceeded,
            this, [this, orderQueuePage]() {
        //refreshes and opens the kitchen queue after a successful kitchen login
        orderQueuePage->refresh();
        ui->pageStack->setCurrentWidget(orderQueuePage);
    });

    connect(customerDashboard, &CustomerDashboard::orderPageRequested,
            this, [this, orderPage]() {
        //refreshes and opens the menu page when the customer requests it
        orderPage->refresh();
        ui->pageStack->setCurrentWidget(orderPage);
    });

    connect(orderPage, &OrderPage::backRequested,
            this, [this, customerDashboard]() {
        //refreshes and opens the customer dashboard when the menu's back action is used
        customerDashboard->refresh();
        ui->pageStack->setCurrentWidget(customerDashboard);
    });

    connect(orderPage, &OrderPage::orderPlaced,
            this, [this, customerDashboard](int) {
        //refreshes and opens the customer dashboard after an order is placed
        customerDashboard->refresh();
        ui->pageStack->setCurrentWidget(customerDashboard);
    });



    const auto signOut = [this, loginPage, customerDashboard, orderPage, orderQueuePage]() {
        //logs out, clears drafts and page displays, and returns to login while retaining submitted orders
        authService.logout();
        orderPage->clear();
        customerDashboard->clear();
        orderQueuePage->clear();
        loginPage->clear();
        ui->pageStack->setCurrentWidget(loginPage);
    };
    connect(orderQueuePage, &OrderQueuePage::signOutRequested, this, signOut);

    connect(customerDashboard, &CustomerDashboard::signOutRequested, this, signOut
    );

    QMenu *sessionMenu = nullptr;
    for (QAction *action : menuBar()->actions()) {
        QMenu *menu = action->menu();
        if (menu && menu->title().remove('&').compare("Session", Qt::CaseInsensitive) == 0) {
            sessionMenu = menu;
            break;
        }
    }
    if (!sessionMenu) sessionMenu = menuBar()->addMenu(tr("Session"));
    QAction *signOutAction = sessionMenu->addAction(tr("Sign out"));
    signOutAction->setObjectName("returnToLoginAction");
    connect(signOutAction, &QAction::triggered, this, signOut);

    QTimer::singleShot(0, this, [this]() {
        //centers the main window on its screen unless it is maximized or fullscreen
        if (isMaximized() || isFullScreen()) return;
        QScreen *currentScreen = screen();
        if (!currentScreen) return;
        QRect windowFrame = frameGeometry();
        windowFrame.moveCenter(currentScreen->availableGeometry().center());
        move(windowFrame.topLeft());
    });
}

MainWindow::~MainWindow()
{
    //releases the generated main-window form
    delete ui;
}
