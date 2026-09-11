#include "Claim.h"
#include <iostream>
#include <sstream>

int Claim::nextClaimID = 1;

Claim::Claim() : claimID(0), itemID(0), userID(0), description(""), date(""), status("") {}

Claim::Claim(int itemID, int userID, const std::string& description, const std::string& date)
    : claimID(nextClaimID++), itemID(itemID), userID(userID), description(description),
      date(date), status("Pending") {}

Claim::Claim(int claimID, int itemID, int userID, const std::string& description,
             const std::string& date, const std::string& status)
    : claimID(claimID), itemID(itemID), userID(userID), description(description),
      date(date), status(status) {
    if (claimID >= nextClaimID) nextClaimID = claimID + 1;
}

void Claim::display() const {
    std::cout << "Claim ID: " << claimID << " | Item ID: " << itemID
              << " | User ID: " << userID << " | Status: " << status
              << " | Date: " << date << " | Note: " << description << "\n";
}

std::string Claim::serialize() const {
    std::ostringstream oss;
    oss << claimID << "|" << itemID << "|" << userID << "|" << description << "|" << date << "|" << status;
    return oss.str();
}
