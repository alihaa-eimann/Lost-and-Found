#ifndef USER_H
#define USER_H

#include <string>

// ============================================================
// User.h
// ------------------------------------------------------------
// Base class for anyone who can log into the system.
// Student and Admin inherit from User (INHERITANCE).
// Data members are private/protected (ENCAPSULATION) and only
// reachable through getters or controlled setters.
// ============================================================

class User {
protected:
    int userID;
    std::string username;
    std::string password;   // stored as-is for a student project; not for real production use
    std::string fullName;
    std::string contact;

    static int nextUserID;  // STATIC MEMBER: shared id counter for every user in the system

public:
    User();
    User(const std::string& username, const std::string& password,
         const std::string& fullName, const std::string& contact);
    User(int userID, const std::string& username, const std::string& password,
         const std::string& fullName, const std::string& contact);
    virtual ~User() {}

    int getID() const { return userID; }
    std::string getUsername() const { return username; }
    std::string getFullName() const { return fullName; }
    std::string getContact() const { return contact; }

    bool checkPassword(const std::string& attempt) const { return attempt == password; }

    virtual std::string getRole() const = 0;   // "STUDENT" or "ADMIN" -> makes User abstract
    virtual void display() const;
    virtual std::string serialize() const;

    static void setNextID(int n) { nextUserID = n; }
};

class Student : public User {
public:
    Student(const std::string& username, const std::string& password,
            const std::string& fullName, const std::string& contact);
    Student(int userID, const std::string& username, const std::string& password,
            const std::string& fullName, const std::string& contact);

    std::string getRole() const override { return "STUDENT"; }
};

class Admin : public User {
public:
    Admin(const std::string& username, const std::string& password,
          const std::string& fullName, const std::string& contact);
    Admin(int userID, const std::string& username, const std::string& password,
          const std::string& fullName, const std::string& contact);

    std::string getRole() const override { return "ADMIN"; }
};

#endif // USER_H
