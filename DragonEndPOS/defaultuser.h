#ifndef DEFAULT_USER_H
#define DEFAULT_USER_H

#include <string>


class DefaultUser {
private:
    std::string name;
    std::string username;

public:
    explicit DefaultUser(std::string name = "guest", std::string username = "guest")
        : name(name), username(username) {
        //stores the account's name and username, using guest values when no arguments are supplied
    }

    virtual ~DefaultUser() = default;

    const std::string& getName() const {
        //returns the account's stored display name
        return name;
    }
    const std::string& getUsername() const {
        //returns the account's stored username
        return username;
    }
    virtual bool canLogin() const {
        //reports that the base guest account cannot use the login workflow
        return false;
    }
    virtual std::string getRole() const {
        //returns the guest role used by the base account type
        return "guest";
    }
};

#endif
