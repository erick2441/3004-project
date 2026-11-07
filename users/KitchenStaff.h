#ifndef KITCHENSTAFF_H
#define KITCHENSTAFF_H

#include "DefaultUser.h"

/*
 * "2 unique kitchen staff, with name, unique username, and unique staff ID."
 */
class KitchenStaff : public DefaultUser {
private:
    std::string staffId;

public:
    KitchenStaff(std::string name, std::string username, std::string staffId)
        : DefaultUser(name, username),
          staffId(staffId) {}

    const std::string& getStaffId() const { return staffId; }
    bool canLogin() const override { return true; }
    std::string getRole() const override { return "kitchen staff"; }
};

#endif
