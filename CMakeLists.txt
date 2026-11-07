cmake_minimum_required(VERSION 3.22)
project(notes_example LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(notes_example
    main.cpp
    users/DefaultUser.h
    users/Customer.h
    users/Manager.h
    users/Admin.h
    users/KitchenStaff.h
    users/Cashier.h
    controls/AuthService.h
    orders/OrderItem.h
    orders/Order.h
    controls/OrderService.h
    menu/MenuItem.h
    menu/IngredientRequirement.h
    menu/CafeMenu.h
)

