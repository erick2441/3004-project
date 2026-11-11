QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    dashboardpage.cpp \
    loginpage.cpp \
    main.cpp \
    mainwindow.cpp \
    menupage.cpp \
    orderdetailsdialog.cpp \
    orderqueuepage.cpp

HEADERS += \
    admin.h \
    authservice.h \
    cafemenu.h \
    cashier.h \
    customer.h \
    dashboardpage.h \
    defaultuser.h \
    defaultusers.h \
    ingredientrequirement.h \
    kitchenstaff.h \
    loginpage.h \
    mainwindow.h \
    manager.h \
    menuitem.h \
    menupage.h \
    order.h \
    orderdetailsdialog.h \
    orderitem.h \
    orderqueuepage.h \
    orderservice.h

FORMS += \
    dashboardpage.ui \
    loginpage.ui \
    mainwindow.ui \
    menupage.ui \
    orderdetailsdialog.ui \
    orderqueuepage.ui

RESOURCES += theme/theme.qrc

CONFIG += lrelease

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
