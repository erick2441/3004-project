#ifndef DEFAULT_USER_H
#define DEFAULT_USER_H

#include <string>


class DefaultUser {
private:
    std::string name;
    std::string username;

public:
    explicit DefaultUser(std::string name = "guest", std::string username = "guest")
        : name(name), username(username) {}

    virtual ~DefaultUser() = default;

    const std::string& getName() const { return name; }
    const std::string& getUsername() const { return username; }
    virtual bool canLogin() const { return false; }
    virtual std::string getRole() const { return "guest"; }
};

#endif
