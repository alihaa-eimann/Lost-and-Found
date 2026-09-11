#include "Item.h"
#include <iostream>
#include <sstream>

int Item::nextItemID = 1; // static member definition, IDs start at 1

Item::Item()
    : itemID(0), itemName(""), category(""), description(""), color(""), location(""),
      date(""), time(""), personName(""), contactInfo(""), status(""), reportedByUserID(0) {}

Item::Item(const std::string& itemName, const std::string& category, const std::string& description,
           const std::string& color, const std::string& location, const std::string& date,
           const std::string& time, const std::string& personName, const std::string& contactInfo,
           const std::string& status, int reportedByUserID)
    : itemID(nextItemID++), itemName(itemName), category(category), description(description),
      color(color), location(location), date(date), time(time), personName(personName),
      contactInfo(contactInfo), status(status), reportedByUserID(reportedByUserID) {}

Item::Item(int itemID, const std::string& itemName, const std::string& category, const std::string& description,
           const std::string& color, const std::string& location, const std::string& date,
           const std::string& time, const std::string& personName, const std::string& contactInfo,
           const std::string& status, int reportedByUserID)
    : itemID(itemID), itemName(itemName), category(category), description(description),
      color(color), location(location), date(date), time(time), personName(personName),
      contactInfo(contactInfo), status(status), reportedByUserID(reportedByUserID) {
    if (itemID >= nextItemID) nextItemID = itemID + 1; // keep the counter ahead of loaded data
}

void Item::display() const {
    std::cout << "ID: " << itemID
              << " | Type: " << getType()
              << " | Name: " << itemName
              << " | Category: " << category
              << " | Color: " << color
              << " | Location: " << location
              << " | Date: " << date;
    if (!time.empty()) std::cout << " " << time;
    std::cout << " | Status: " << status
              << " | Contact: " << personName << " (" << contactInfo << ")"
              << " | Desc: " << description << "\n";
}

std::string Item::serialize() const {
    std::ostringstream oss;
    oss << itemID << "|" << getType() << "|" << itemName << "|" << category << "|" << description << "|"
        << color << "|" << location << "|" << date << "|" << time << "|" << personName << "|"
        << contactInfo << "|" << status << "|" << reportedByUserID;
    return oss.str();
}

// ---------------- LostItem ----------------
LostItem::LostItem(const std::string& itemName, const std::string& category, const std::string& description,
                     const std::string& color, const std::string& location, const std::string& date,
                     const std::string& time, const std::string& personName, const std::string& contactInfo,
                     int reportedByUserID)
    : Item(itemName, category, description, color, location, date, time, personName, contactInfo, "Lost", reportedByUserID) {}

LostItem::LostItem(int itemID, const std::string& itemName, const std::string& category, const std::string& description,
                     const std::string& color, const std::string& location, const std::string& date,
                     const std::string& time, const std::string& personName, const std::string& contactInfo,
                     const std::string& status, int reportedByUserID)
    : Item(itemID, itemName, category, description, color, location, date, time, personName, contactInfo, status, reportedByUserID) {}

void LostItem::display() const {
    std::cout << "[LOST]   ";
    Item::display();
}

// ---------------- FoundItem ----------------
FoundItem::FoundItem(const std::string& itemName, const std::string& category, const std::string& description,
                       const std::string& color, const std::string& location, const std::string& date,
                       const std::string& time, const std::string& personName, const std::string& contactInfo,
                       int reportedByUserID)
    : Item(itemName, category, description, color, location, date, time, personName, contactInfo, "Found", reportedByUserID) {}

FoundItem::FoundItem(int itemID, const std::string& itemName, const std::string& category, const std::string& description,
                       const std::string& color, const std::string& location, const std::string& date,
                       const std::string& time, const std::string& personName, const std::string& contactInfo,
                       const std::string& status, int reportedByUserID)
    : Item(itemID, itemName, category, description, color, location, date, time, personName, contactInfo, status, reportedByUserID) {}

void FoundItem::display() const {
    std::cout << "[FOUND]  ";
    Item::display();
}
