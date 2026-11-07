#ifndef CASHIER_H
#define CASHIER_H

#include "DefaultUser.h"

class Cashier : public DefaultUser {
private:
    std::string staffId;

public:
    Cashier(std::string name, std::string username, std::string staffId)
        : DefaultUser(name, username),
          staffId(staffId) {}

    const std::string& getStaffId() const { return staffId; }
    bool canLogin() const override { return true; }
    std::string getRole() const override { return "cashier"; }
};

#endif
