#include "User.h"
#include <iostream>
#include <sstream>

int User::nextUserID = 1;

User::User() : userID(0), username(""), password(""), fullName(""), contact("") {}

User::User(const std::string& username, const std::string& password,
           const std::string& fullName, const std::string& contact)
    : userID(nextUserID++), username(username), password(password), fullName(fullName), contact(contact) {}

User::User(int userID, const std::string& username, const std::string& password,
           const std::string& fullName, const std::string& contact)
    : userID(userID), username(username), password(password), fullName(fullName), contact(contact) {
    if (userID >= nextUserID) nextUserID = userID + 1;
}

void User::display() const {
    std::cout << "ID: " << userID << " | Username: " << username
              << " | Name: " << fullName << " | Role: " << getRole()
              << " | Contact: " << contact << "\n";
}

std::string User::serialize() const {
    std::ostringstream oss;
    oss << userID << "|" << username << "|" << password << "|" << fullName << "|" << contact << "|" << getRole();
    return oss.str();
}

Student::Student(const std::string& username, const std::string& password,
                  const std::string& fullName, const std::string& contact)
    : User(username, password, fullName, contact) {}

Student::Student(int userID, const std::string& username, const std::string& password,
                  const std::string& fullName, const std::string& contact)
    : User(userID, username, password, fullName, contact) {}

Admin::Admin(const std::string& username, const std::string& password,
             const std::string& fullName, const std::string& contact)
    : User(username, password, fullName, contact) {}

Admin::Admin(int userID, const std::string& username, const std::string& password,
             const std::string& fullName, const std::string& contact)
    : User(userID, username, password, fullName, contact) {}
