#include <iostream>
#include <limits>
#include <string>
#include <regex>
#include "LostAndFoundSystem.h"
#include "User.h"
#include "Item.h"
#include "Claim.h"

// ============================================================
// main.cpp
// Menu-driven console interface. All input validation for the
// interactive session lives here; the system classes stay
// focused on data/logic (separation of concerns).
// ============================================================

// ---------- small input helpers ----------
static int readInt(const std::string& prompt) {
    int value;
    while (true) {
        std::cout << prompt;
        if (std::cin >> value) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return value;
        }
        if (std::cin.eof()) {
            std::cout << "\nNo more input received. Exiting.\n";
            std::exit(0);
        }
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "  Invalid number, please try again.\n";
    }
}

static std::string readLine(const std::string& prompt, bool allowEmpty = false) {
    std::string value;
    while (true) {
        std::cout << prompt;
        if (!std::getline(std::cin, value)) {
            std::cout << "\nNo more input received. Exiting.\n";
            std::exit(0);
        }
        // trim
        size_t start = value.find_first_not_of(" \t");
        size_t end = value.find_last_not_of(" \t");
        value = (start == std::string::npos) ? "" : value.substr(start, end - start + 1);
        if (!value.empty() || allowEmpty) return value;
        std::cout << "  This field cannot be empty.\n";
    }
}

static bool isValidDate(const std::string& d) {
    // expects YYYY-MM-DD
    static const std::regex pattern(R"(^\d{4}-\d{2}-\d{2}$)");
    return std::regex_match(d, pattern);
}

static std::string readDate(const std::string& prompt) {
    std::string d;
    while (true) {
        d = readLine(prompt);
        if (isValidDate(d)) return d;
        std::cout << "  Invalid date format. Please use YYYY-MM-DD (e.g. 2026-09-04).\n";
    }
}

static std::string chooseCategory() {
    const char* cats[] = {"Electronics", "Books", "Wallet", "ID/Card", "Clothing", "Keys", "Accessories", "Other"};
    std::cout << "Select category:\n";
    for (int i = 0; i < 8; i++) std::cout << "  " << (i + 1) << ". " << cats[i] << "\n";
    int c;
    while (true) {
        c = readInt("Choice: ");
        if (c >= 1 && c <= 8) return cats[c - 1];
        std::cout << "  Invalid choice, pick 1-8.\n";
    }
}

static void printItemList(const std::vector<Item*>& list) {
    if (list.empty()) {
        std::cout << "  (no items found)\n";
        return;
    }
    for (Item* it : list) it->display();
}

// ---------- flows ----------
static void doRegister(LostAndFoundSystem& sys) {
    std::cout << "\n--- Register New Student Account ---\n";
    std::string username;
    while (true) {
        username = readLine("Choose a username: ");
        if (sys.usernameExists(username)) {
            std::cout << "  That username is already taken. Try another.\n";
            continue;
        }
        break;
    }
    std::string password = readLine("Choose a password: ");
    std::string fullName = readLine("Full name: ");
    std::string contact = readLine("Contact info (phone/email): ");
    sys.registerStudent(username, password, fullName, contact);
    std::cout << "Registration successful! You can now log in.\n";
}

static void reportLostFlow(LostAndFoundSystem& sys, User* user) {
    std::cout << "\n--- Report Lost Item ---\n";
    std::string name = readLine("Item name: ");
    std::string category = chooseCategory();
    std::string desc = readLine("Description: ", true);
    std::string color = readLine("Color: ", true);
    std::string location = readLine("Location lost: ");
    std::string date = readDate("Date lost (YYYY-MM-DD): ");
    std::string time = readLine("Time (HH:MM, optional): ", true);
    std::string person = readLine("Your name: ");
    std::string contact = readLine("Contact info: ");
    Item* it = sys.reportLostItem(name, category, desc, color, location, date, time, person, contact, user->getID());
    std::cout << "Lost item reported with ID " << it->getID() << ".\n";

    auto matches = sys.matchLostItem(it->getID());
    if (!matches.empty()) {
        std::cout << "\nPossible matches found among reported found items:\n";
        int shown = 0;
        for (auto& m : matches) {
            m.item->display();
            std::cout << "   -> match score: " << m.score << "\n";
            if (++shown >= 5) break;
        }
    }
}

static void reportFoundFlow(LostAndFoundSystem& sys, User* user) {
    std::cout << "\n--- Report Found Item ---\n";
    std::string name = readLine("Item name: ");
    std::string category = chooseCategory();
    std::string desc = readLine("Description: ", true);
    std::string color = readLine("Color: ", true);
    std::string location = readLine("Location found: ");
    std::string date = readDate("Date found (YYYY-MM-DD): ");
    std::string time = readLine("Time (HH:MM, optional): ", true);
    std::string person = readLine("Your name: ");
    std::string contact = readLine("Contact info: ");
    Item* it = sys.reportFoundItem(name, category, desc, color, location, date, time, person, contact, user->getID());
    std::cout << "Found item reported with ID " << it->getID() << ".\n";
}

static void searchFlow(LostAndFoundSystem& sys) {
    std::cout << "\n--- Search Items ---\n"
              << "1. By name (partial)\n2. By category\n3. By location (partial)\n"
              << "4. By date\n5. By status\n6. By item ID\n";
    int choice = readInt("Choice: ");
    std::vector<Item*> results;
    switch (choice) {
        case 1: results = sys.searchByName(readLine("Keyword: ")); break;
        case 2: results = sys.searchByCategory(chooseCategory()); break;
        case 3: results = sys.searchByLocation(readLine("Location keyword: ")); break;
        case 4: results = sys.searchByDate(readDate("Date (YYYY-MM-DD): ")); break;
        case 5: results = sys.searchByStatus(readLine("Status: ")); break;
        case 6: {
            Item* it = sys.searchByID(readInt("Item ID: "));
            if (it) results.push_back(it);
            break;
        }
        default: std::cout << "  Invalid choice.\n"; return;
    }
    std::cout << "\nResults (" << results.size() << "):\n";
    printItemList(results);
}

static std::vector<Item*> maybeSort(std::vector<Item*> list) {
    std::cout << "Sort by? 1.Date 2.Name 3.Category 4.Location 5.Status 0.No sort\n";
    int c = readInt("Choice: ");
    const char* fields[] = {"", "date", "name", "category", "location", "status"};
    if (c >= 1 && c <= 5) LostAndFoundSystem::sortItems(list, fields[c]);
    return list;
}

static void viewAllFlow(LostAndFoundSystem& sys) {
    std::vector<Item*> list = sys.getAllItems();
    list = maybeSort(list);
    std::cout << "\nAll Items (" << list.size() << "):\n";
    printItemList(list);
}

static void submitClaimFlow(LostAndFoundSystem& sys, User* user) {
    std::cout << "\n--- Submit a Claim ---\n";
    std::vector<Item*> found = sys.searchByStatus("Found");
    std::cout << "Available found items:\n";
    printItemList(found);
    int itemID = readInt("\nEnter the Item ID you want to claim: ");
    Item* it = sys.findItemByID(itemID);
    if (!it || it->getType() != "FOUND") {
        std::cout << "  Invalid found-item ID.\n";
        return;
    }
    if (it->getStatus() != "Found") {
        std::cout << "  This item is not currently claimable (status: " << it->getStatus() << ").\n";
        return;
    }
    std::string desc = readLine("Describe why this item is yours: ");
    std::string date = readDate("Today's date (YYYY-MM-DD): ");
    Claim* c = sys.submitClaim(itemID, user->getID(), desc, date);
    if (c) std::cout << "Claim submitted with ID " << c->getClaimID() << ". Await admin review.\n";
    else std::cout << "  Could not submit claim.\n";
}

static void userMenu(LostAndFoundSystem& sys, User* user) {
    while (true) {
        std::cout << "\n========== USER MENU (" << user->getFullName() << ") ==========\n"
                  << "1. Report Lost Item\n2. Report Found Item\n3. Search Items\n"
                  << "4. View All Items\n5. View My Reports\n6. Submit Claim\n"
                  << "7. View My Claims\n8. Logout\n";
        int choice = readInt("Choice: ");
        switch (choice) {
            case 1: reportLostFlow(sys, user); break;
            case 2: reportFoundFlow(sys, user); break;
            case 3: searchFlow(sys); break;
            case 4: viewAllFlow(sys); break;
            case 5: {
                std::cout << "\nYour reports:\n";
                printItemList(sys.getItemsByUser(user->getID()));
                break;
            }
            case 6: submitClaimFlow(sys, user); break;
            case 7: {
                std::cout << "\nYour claims:\n";
                auto myClaims = sys.getClaimsByUser(user->getID());
                if (myClaims.empty()) std::cout << "  (none yet)\n";
                for (auto& c : myClaims) c.display();
                break;
            }
            case 8: std::cout << "Logging out...\n"; sys.saveAll(); return;
            default: std::cout << "  Invalid choice.\n";
        }
        sys.saveAll(); // persist after every action so nothing is lost
    }
}

static void adminMenu(LostAndFoundSystem& sys) {
    while (true) {
        std::cout << "\n========== ADMIN MENU ==========\n"
                  << "1. View All Items\n2. Search Items\n3. View Pending Reports\n"
                  << "4. Manage Claims\n5. Approve/Reject Reports\n6. Mark Item as Returned\n"
                  << "7. Delete Record\n8. System Statistics\n9. Logout\n";
        int choice = readInt("Choice: ");
        switch (choice) {
            case 1: viewAllFlow(sys); break;
            case 2: searchFlow(sys); break;
            case 3: {
                std::cout << "\nPending items (Lost / Found / Claim Pending):\n";
                auto lost = sys.searchByStatus("Lost");
                auto found = sys.searchByStatus("Found");
                auto pending = sys.searchByStatus("Claim Pending");
                printItemList(lost); printItemList(found); printItemList(pending);
                break;
            }
            case 4: {
                std::cout << "\n--- Manage Claims ---\n";
                for (const Claim& c : sys.getAllClaims()) c.display();
                int cid = readInt("\nEnter Claim ID to act on (0 to cancel): ");
                if (cid == 0) break;
                if (!sys.findClaimByID(cid)) { std::cout << "  Claim not found.\n"; break; }
                std::cout << "1. Approve 2. Reject 3. Request Verification\n";
                int act = readInt("Choice: ");
                if (act == 1) { sys.approveClaim(cid); std::cout << "Claim approved, item marked Returned.\n"; }
                else if (act == 2) { sys.rejectClaim(cid); std::cout << "Claim rejected.\n"; }
                else if (act == 3) { sys.requestVerification(cid); std::cout << "Verification requested.\n"; }
                else std::cout << "  Invalid choice.\n";
                break;
            }
            case 5: {
                int id = readInt("Enter Item ID to approve/reject: ");
                Item* it = sys.findItemByID(id);
                if (!it) { std::cout << "  Item not found.\n"; break; }
                it->display();
                std::cout << "1. Approve (keep as-is) 2. Reject (close record)\n";
                int act = readInt("Choice: ");
                if (act == 2) it->setStatus("Closed");
                std::cout << "Done.\n";
                break;
            }
            case 6: {
                int id = readInt("Enter Item ID to mark as Returned: ");
                if (sys.markReturned(id)) std::cout << "Item marked as Returned.\n";
                else std::cout << "  Item not found.\n";
                break;
            }
            case 7: {
                int id = readInt("Enter Item ID to delete: ");
                if (sys.deleteItem(id)) std::cout << "Item deleted.\n";
                else std::cout << "  Item not found.\n";
                break;
            }
            case 8: sys.printStatistics(); break;
            case 9: std::cout << "Logging out...\n"; sys.saveAll(); return;
            default: std::cout << "  Invalid choice.\n";
        }
        sys.saveAll();
    }
}

int main() {
    LostAndFoundSystem sys;
    sys.loadAll();

    std::cout << "=====================================\n"
              << "  MILITARY COLLEGE OF SIGNALS\n"
              << "     COLLEGE LOST & FOUND\n"
              << "=====================================\n";

    while (true) {
        std::cout << "\n1. Register\n2. Login\n3. Admin Login\n4. Exit\n";
        int choice = readInt("Choice: ");
        if (choice == 1) {
            doRegister(sys);
        } else if (choice == 2) {
            std::string u = readLine("Username: ");
            std::string p = readLine("Password: ");
            User* user = sys.login(u, p, "STUDENT");
            if (user) { std::cout << "Welcome, " << user->getFullName() << "!\n"; userMenu(sys, user); }
            else std::cout << "  Invalid username or password.\n";
        } else if (choice == 3) {
            std::string u = readLine("Admin username: ");
            std::string p = readLine("Admin password: ");
            User* admin = sys.login(u, p, "ADMIN");
            if (admin) { std::cout << "Welcome, Admin " << admin->getFullName() << "!\n"; adminMenu(sys); }
            else std::cout << "  Invalid admin credentials.\n";
        } else if (choice == 4) {
            sys.saveAll();
            std::cout << "Data saved. Goodbye!\n";
            break;
        } else {
            std::cout << "  Invalid menu choice.\n";
        }
    }
    return 0;
}
