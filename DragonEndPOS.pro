QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += controls menu orders ui users

SOURCES += \
    main.cpp \
    ui/dashboardpage.cpp \
    ui/loginpage.cpp \
    ui/mainwindow.cpp \
    ui/menupage.cpp \
    ui/orderdetailsdialog.cpp \
    ui/orderqueuepage.cpp

HEADERS += \
    controls/authservice.h \
    controls/orderservice.h \
    menu/cafemenu.h \
    menu/ingredientrequirement.h \
    menu/menuitem.h \
    orders/order.h \
    orders/orderitem.h \
    ui/dashboardpage.h \
    ui/loginpage.h \
    ui/mainwindow.h \
    ui/menupage.h \
    ui/orderdetailsdialog.h \
    ui/orderqueuepage.h \
    users/admin.h \
    users/cashier.h \
    users/customer.h \
    users/defaultuser.h \
    users/defaultusers.h \
    users/kitchenstaff.h \
    users/manager.h

FORMS += \
    ui/dashboardpage.ui \
    ui/loginpage.ui \
    ui/mainwindow.ui \
    ui/menupage.ui \
    ui/orderdetailsdialog.ui \
    ui/orderqueuepage.ui

RESOURCES += theme/theme.qrc

CONFIG += lrelease

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
