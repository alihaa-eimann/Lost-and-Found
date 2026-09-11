#ifndef ITEM_H
#define ITEM_H

#include <string>
#include <vector>

// ============================================================
// Item.h
// ------------------------------------------------------------
// Base class for anything reported to the Lost & Found system.
// LostItem and FoundItem inherit from Item (INHERITANCE).
// Virtual functions like getType()/display() allow the system
// to treat both kinds of items through a single Item* pointer
// and still get the correct behaviour (POLYMORPHISM).
// ============================================================

class Item {
protected:
    int itemID;
    std::string itemName;
    std::string category;      // Electronics, Books, Wallet, ID/Card, Clothing, Keys, Accessories, Other
    std::string description;
    std::string color;
    std::string location;
    std::string date;          // format: YYYY-MM-DD
    std::string time;          // optional, HH:MM or ""
    std::string personName;    // owner (lost) / finder (found)
    std::string contactInfo;
    std::string status;        // Lost, Found, Matched, Claim Pending, Returned, Closed
    int reportedByUserID;      // links item back to the user who reported it

    static int nextItemID;     // STATIC MEMBER: shared counter across all items

public:
    Item(); // default constructor
    Item(const std::string& itemName, const std::string& category, const std::string& description,
         const std::string& color, const std::string& location, const std::string& date,
         const std::string& time, const std::string& personName, const std::string& contactInfo,
         const std::string& status, int reportedByUserID); // parameterized constructor (overloading)
    // Constructor used only when re-loading a saved item from disk (keeps original ID)
    Item(int itemID, const std::string& itemName, const std::string& category, const std::string& description,
         const std::string& color, const std::string& location, const std::string& date,
         const std::string& time, const std::string& personName, const std::string& contactInfo,
         const std::string& status, int reportedByUserID);

    virtual ~Item() {}

    // ---- getters ----
    int getID() const { return itemID; }
    std::string getName() const { return itemName; }
    std::string getCategory() const { return category; }
    std::string getDescription() const { return description; }
    std::string getColor() const { return color; }
    std::string getLocation() const { return location; }
    std::string getDate() const { return date; }
    std::string getTime() const { return time; }
    std::string getPersonName() const { return personName; }
    std::string getContactInfo() const { return contactInfo; }
    std::string getStatus() const { return status; }
    int getReportedByUserID() const { return reportedByUserID; }

    // ---- setters ----
    void setStatus(const std::string& s) { status = s; }

    // ---- virtual (polymorphic) behaviour ----
    virtual std::string getType() const = 0;      // "LOST" or "FOUND"  -> pure virtual, makes Item abstract
    virtual void display() const;                  // can be overridden, but base version is reused
    virtual std::string serialize() const;          // turns the item into one line of text for the file

    static void setNextID(int n) { nextItemID = n; }
    static int  peekNextID() { return nextItemID; }
};

// ------------------------------------------------------------
// LostItem : reported by a student who lost something.
// ------------------------------------------------------------
class LostItem : public Item {
public:
    LostItem(const std::string& itemName, const std::string& category, const std::string& description,
              const std::string& color, const std::string& location, const std::string& date,
              const std::string& time, const std::string& personName, const std::string& contactInfo,
              int reportedByUserID);
    LostItem(int itemID, const std::string& itemName, const std::string& category, const std::string& description,
              const std::string& color, const std::string& location, const std::string& date,
              const std::string& time, const std::string& personName, const std::string& contactInfo,
              const std::string& status, int reportedByUserID);

    std::string getType() const override { return "LOST"; }
    void display() const override;
};

// ------------------------------------------------------------
// FoundItem : reported by a student who found something.
// ------------------------------------------------------------
class FoundItem : public Item {
public:
    FoundItem(const std::string& itemName, const std::string& category, const std::string& description,
                const std::string& color, const std::string& location, const std::string& date,
                const std::string& time, const std::string& personName, const std::string& contactInfo,
                int reportedByUserID);
    FoundItem(int itemID, const std::string& itemName, const std::string& category, const std::string& description,
                const std::string& color, const std::string& location, const std::string& date,
                const std::string& time, const std::string& personName, const std::string& contactInfo,
                const std::string& status, int reportedByUserID);

    std::string getType() const override { return "FOUND"; }
    void display() const override;
};

#endif // ITEM_H
