#ifndef ADMIN_H
#define ADMIN_H

#include "defaultuser.h"

class Admin : public DefaultUser {
private:
    std::string staffId;

public:
    Admin(std::string name, std::string username, std::string staffId)
        : DefaultUser(name, username),
          staffId(staffId) {}

    const std::string& getStaffId() const { return staffId; }
    bool canLogin() const override { return true; }
    std::string getRole() const override { return "system administrator"; }
};

#endif
