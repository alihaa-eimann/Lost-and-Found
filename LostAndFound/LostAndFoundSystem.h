#ifndef LOSTANDFOUNDSYSTEM_H
#define LOSTANDFOUNDSYSTEM_H

#include <vector>
#include <string>
#include <unordered_map>
#include "Item.h"
#include "User.h"
#include "Claim.h"

// ============================================================
// LostAndFoundSystem.h
// ------------------------------------------------------------
// The "brain" of the application. Holds all data in memory
// using STL containers and knows how to load/save it to disk.
//
//   vector<Item*>        items   -> ordered list, easy to sort/search/iterate
//   vector<User*>        users   -> ordered list of all accounts
//   unordered_map<...>   usernameToIndex -> O(1) duplicate-username check & fast login lookup
//   vector<Claim>        claims  -> ordered list of claims (small objects, no need for pointers)
// ============================================================

struct MatchResult {
    Item* item;
    int score;
};

class LostAndFoundSystem {
private:
    std::vector<Item*> items;
    std::vector<User*> users;
    std::vector<Claim> claims;

    // username -> index in users vector, for O(1) duplicate checks and fast login
    std::unordered_map<std::string, int> usernameToIndex;

    std::string usersFile = "data/users.txt";
    std::string itemsFile = "data/items.txt";
    std::string claimsFile = "data/claims.txt";

    static std::string toLower(const std::string& s);
    static bool containsIgnoreCase(const std::string& haystack, const std::string& needle);
    static int dateSimilarityScore(const std::string& d1, const std::string& d2);

public:
    LostAndFoundSystem();
    ~LostAndFoundSystem();

    // ---------- persistence ----------
    void loadAll();
    void saveAll() const;

    // ---------- users / auth ----------
    bool usernameExists(const std::string& username) const;
    User* registerStudent(const std::string& username, const std::string& password,
                           const std::string& fullName, const std::string& contact);
    User* login(const std::string& username, const std::string& password, const std::string& requiredRole);
    User* findUserByID(int id) const;

    // ---------- items ----------
    Item* reportLostItem(const std::string& name, const std::string& category, const std::string& description,
                          const std::string& color, const std::string& location, const std::string& date,
                          const std::string& time, const std::string& personName, const std::string& contact,
                          int reportedByUserID);
    Item* reportFoundItem(const std::string& name, const std::string& category, const std::string& description,
                           const std::string& color, const std::string& location, const std::string& date,
                           const std::string& time, const std::string& personName, const std::string& contact,
                           int reportedByUserID);
    Item* findItemByID(int id) const;
    const std::vector<Item*>& getAllItems() const { return items; }
    std::vector<Item*> getItemsByUser(int userID) const;
    bool deleteItem(int itemID);

    // ---------- searching (overloaded by criteria) ----------
    std::vector<Item*> searchByName(const std::string& keyword) const;      // partial match
    std::vector<Item*> searchByCategory(const std::string& category) const;
    std::vector<Item*> searchByLocation(const std::string& location) const; // partial match
    std::vector<Item*> searchByDate(const std::string& date) const;
    std::vector<Item*> searchByStatus(const std::string& status) const;
    Item* searchByID(int id) const;

    // ---------- sorting ----------
    // field: "date" | "name" | "category" | "location" | "status"
    static void sortItems(std::vector<Item*>& list, const std::string& field);

    // ---------- matching ----------
    std::vector<MatchResult> matchLostItem(int lostItemID) const;

    // ---------- claims ----------
    Claim* submitClaim(int itemID, int userID, const std::string& description, const std::string& date);
    std::vector<Claim> getClaimsByUser(int userID) const;
    std::vector<Claim>& getAllClaimsMutable() { return claims; }
    const std::vector<Claim>& getAllClaims() const { return claims; }
    Claim* findClaimByID(int claimID);
    bool approveClaim(int claimID);
    bool rejectClaim(int claimID);
    bool requestVerification(int claimID);

    // ---------- admin utilities ----------
    bool markReturned(int itemID);
    void printStatistics() const;
};

#endif // LOSTANDFOUNDSYSTEM_H
