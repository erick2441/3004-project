#ifndef MANAGER_H
#define MANAGER_H

#include "DefaultUser.h"
class Manager : public DefaultUser {
private:
    std::string staffId;

public:
    Manager(std::string name, std::string username, std::string staffId)
        : DefaultUser(name, username),
          staffId(staffId) {}

    const std::string& getStaffId() const { return staffId; }
    bool canLogin() const override { return true; }
    std::string getRole() const override { return "manager"; }
};

#endif
