#ifndef CLAIM_H
#define CLAIM_H

#include <string>

// ============================================================
// Claim.h
// A claim links a Student (userID) to a FoundItem (itemID).
// ============================================================
class Claim {
private:
    int claimID;
    int itemID;
    int userID;
    std::string description;
    std::string date;
    std::string status; // "Pending", "Approved", "Rejected", "Verification Requested"

    static int nextClaimID;

public:
    Claim();
    Claim(int itemID, int userID, const std::string& description, const std::string& date);
    Claim(int claimID, int itemID, int userID, const std::string& description,
          const std::string& date, const std::string& status);

    int getClaimID() const { return claimID; }
    int getItemID() const { return itemID; }
    int getUserID() const { return userID; }
    std::string getDescription() const { return description; }
    std::string getDate() const { return date; }
    std::string getStatus() const { return status; }

    void setStatus(const std::string& s) { status = s; }

    void display() const;
    std::string serialize() const;

    static void setNextID(int n) { nextClaimID = n; }
};

#endif // CLAIM_H
