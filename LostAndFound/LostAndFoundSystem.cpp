#include "LostAndFoundSystem.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <cmath>

LostAndFoundSystem::LostAndFoundSystem() {}

LostAndFoundSystem::~LostAndFoundSystem() {
    // we own the Item pointers (Users are also heap-allocated) -> clean them up
    for (Item* it : items) delete it;
    for (User* u : users) delete u;
}

// ---------------------------------------------------------------
// small string helpers used by search/matching
// ---------------------------------------------------------------
std::string LostAndFoundSystem::toLower(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(), ::tolower);
    return r;
}

bool LostAndFoundSystem::containsIgnoreCase(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) return true;
    return toLower(haystack).find(toLower(needle)) != std::string::npos;
}

// Very small heuristic: same date => 15, differs by a couple of days textually not attempted
// (kept simple on purpose, this is a beginner project) - exact match vs no match.
int LostAndFoundSystem::dateSimilarityScore(const std::string& d1, const std::string& d2) {
    if (d1.empty() || d2.empty()) return 0;
    if (d1 == d2) return 15;
    return 0;
}

// ---------------------------------------------------------------
// persistence
// ---------------------------------------------------------------
static std::vector<std::string> splitPipe(const std::string& line) {
    std::vector<std::string> parts;
    std::stringstream ss(line);
    std::string token;
    while (std::getline(ss, token, '|')) parts.push_back(token);
    return parts;
}

void LostAndFoundSystem::loadAll() {
    // ---- users ----
    std::ifstream uf(usersFile);
    if (uf.is_open()) {
        std::string line;
        while (std::getline(uf, line)) {
            if (line.empty()) continue;
            auto p = splitPipe(line);
            if (p.size() < 6) continue;
            int id = std::stoi(p[0]);
            std::string username = p[1], password = p[2], fullName = p[3], contact = p[4], role = p[5];
            User* u = nullptr;
            if (role == "ADMIN") u = new Admin(id, username, password, fullName, contact);
            else u = new Student(id, username, password, fullName, contact);
            users.push_back(u);
            usernameToIndex[username] = (int)users.size() - 1;
        }
        uf.close();
    }

    // ---- items ----
    std::ifstream itf(itemsFile);
    if (itf.is_open()) {
        std::string line;
        while (std::getline(itf, line)) {
            if (line.empty()) continue;
            auto p = splitPipe(line);
            if (p.size() < 13) continue;
            int id = std::stoi(p[0]);
            std::string type = p[1], name = p[2], category = p[3], desc = p[4], color = p[5],
                        location = p[6], date = p[7], time = p[8], person = p[9], contact = p[10],
                        status = p[11];
            int reporter = std::stoi(p[12]);
            Item* it = nullptr;
            if (type == "LOST")
                it = new LostItem(id, name, category, desc, color, location, date, time, person, contact, status, reporter);
            else
                it = new FoundItem(id, name, category, desc, color, location, date, time, person, contact, status, reporter);
            items.push_back(it);
        }
        itf.close();
    }

    // ---- claims ----
    std::ifstream cf(claimsFile);
    if (cf.is_open()) {
        std::string line;
        while (std::getline(cf, line)) {
            if (line.empty()) continue;
            auto p = splitPipe(line);
            if (p.size() < 6) continue;
            int cid = std::stoi(p[0]), itemID = std::stoi(p[1]), userID = std::stoi(p[2]);
            std::string desc = p[3], date = p[4], status = p[5];
            claims.push_back(Claim(cid, itemID, userID, desc, date, status));
        }
        cf.close();
    }

    // Ensure a default admin account always exists so the system is usable out of the box
    bool hasAdmin = false;
    for (User* u : users) if (u->getRole() == "ADMIN") { hasAdmin = true; break; }
    if (!hasAdmin) {
        User* a = new Admin("admin", "admin123", "System Administrator", "admin@mcs.edu.pk");
        users.push_back(a);
        usernameToIndex["admin"] = (int)users.size() - 1;
    }
}

void LostAndFoundSystem::saveAll() const {
    std::ofstream uf(usersFile);
    for (User* u : users) uf << u->serialize() << "\n";
    uf.close();

    std::ofstream itf(itemsFile);
    for (Item* it : items) itf << it->serialize() << "\n";
    itf.close();

    std::ofstream cf(claimsFile);
    for (const Claim& c : claims) cf << c.serialize() << "\n";
    cf.close();
}

// ---------------------------------------------------------------
// users / auth
// ---------------------------------------------------------------
bool LostAndFoundSystem::usernameExists(const std::string& username) const {
    return usernameToIndex.find(username) != usernameToIndex.end();
}

User* LostAndFoundSystem::registerStudent(const std::string& username, const std::string& password,
                                           const std::string& fullName, const std::string& contact) {
    if (usernameExists(username)) return nullptr; // caller should check first, this is a safety net
    User* s = new Student(username, password, fullName, contact);
    users.push_back(s);
    usernameToIndex[username] = (int)users.size() - 1;
    return s;
}

User* LostAndFoundSystem::login(const std::string& username, const std::string& password, const std::string& requiredRole) {
    auto it = usernameToIndex.find(username);
    if (it == usernameToIndex.end()) return nullptr;
    User* u = users[it->second];
    if (!u->checkPassword(password)) return nullptr;
    if (!requiredRole.empty() && u->getRole() != requiredRole) return nullptr;
    return u;
}

User* LostAndFoundSystem::findUserByID(int id) const {
    for (User* u : users) if (u->getID() == id) return u;
    return nullptr;
}

// ---------------------------------------------------------------
// items
// ---------------------------------------------------------------
Item* LostAndFoundSystem::reportLostItem(const std::string& name, const std::string& category, const std::string& description,
                                          const std::string& color, const std::string& location, const std::string& date,
                                          const std::string& time, const std::string& personName, const std::string& contact,
                                          int reportedByUserID) {
    Item* it = new LostItem(name, category, description, color, location, date, time, personName, contact, reportedByUserID);
    items.push_back(it);
    return it;
}

Item* LostAndFoundSystem::reportFoundItem(const std::string& name, const std::string& category, const std::string& description,
                                           const std::string& color, const std::string& location, const std::string& date,
                                           const std::string& time, const std::string& personName, const std::string& contact,
                                           int reportedByUserID) {
    Item* it = new FoundItem(name, category, description, color, location, date, time, personName, contact, reportedByUserID);
    items.push_back(it);
    return it;
}

Item* LostAndFoundSystem::findItemByID(int id) const {
    for (Item* it : items) if (it->getID() == id) return it;
    return nullptr;
}

std::vector<Item*> LostAndFoundSystem::getItemsByUser(int userID) const {
    std::vector<Item*> result;
    for (Item* it : items) if (it->getReportedByUserID() == userID) result.push_back(it);
    return result;
}

bool LostAndFoundSystem::deleteItem(int itemID) {
    for (size_t i = 0; i < items.size(); i++) {
        if (items[i]->getID() == itemID) {
            delete items[i];
            items.erase(items.begin() + i);
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------
// searching
// ---------------------------------------------------------------
std::vector<Item*> LostAndFoundSystem::searchByName(const std::string& keyword) const {
    std::vector<Item*> result;
    for (Item* it : items) if (containsIgnoreCase(it->getName(), keyword)) result.push_back(it);
    return result;
}

std::vector<Item*> LostAndFoundSystem::searchByCategory(const std::string& category) const {
    std::vector<Item*> result;
    for (Item* it : items) if (toLower(it->getCategory()) == toLower(category)) result.push_back(it);
    return result;
}

std::vector<Item*> LostAndFoundSystem::searchByLocation(const std::string& location) const {
    std::vector<Item*> result;
    for (Item* it : items) if (containsIgnoreCase(it->getLocation(), location)) result.push_back(it);
    return result;
}

std::vector<Item*> LostAndFoundSystem::searchByDate(const std::string& date) const {
    std::vector<Item*> result;
    for (Item* it : items) if (it->getDate() == date) result.push_back(it);
    return result;
}

std::vector<Item*> LostAndFoundSystem::searchByStatus(const std::string& status) const {
    std::vector<Item*> result;
    for (Item* it : items) if (toLower(it->getStatus()) == toLower(status)) result.push_back(it);
    return result;
}

Item* LostAndFoundSystem::searchByID(int id) const {
    return findItemByID(id);
}

// ---------------------------------------------------------------
// sorting - std::sort with lambdas per field
// ---------------------------------------------------------------
void LostAndFoundSystem::sortItems(std::vector<Item*>& list, const std::string& field) {
    if (field == "date") {
        std::sort(list.begin(), list.end(), [](Item* a, Item* b) { return a->getDate() < b->getDate(); });
    } else if (field == "name") {
        std::sort(list.begin(), list.end(), [](Item* a, Item* b) { return toLower(a->getName()) < toLower(b->getName()); });
    } else if (field == "category") {
        std::sort(list.begin(), list.end(), [](Item* a, Item* b) { return toLower(a->getCategory()) < toLower(b->getCategory()); });
    } else if (field == "location") {
        std::sort(list.begin(), list.end(), [](Item* a, Item* b) { return toLower(a->getLocation()) < toLower(b->getLocation()); });
    } else if (field == "status") {
        std::sort(list.begin(), list.end(), [](Item* a, Item* b) { return toLower(a->getStatus()) < toLower(b->getStatus()); });
    }
}

// ---------------------------------------------------------------
// matching: compare one lost item against every found item
// ---------------------------------------------------------------
std::vector<MatchResult> LostAndFoundSystem::matchLostItem(int lostItemID) const {
    std::vector<MatchResult> results;
    Item* lost = findItemByID(lostItemID);
    if (!lost || lost->getType() != "LOST") return results;

    for (Item* candidate : items) {
        if (candidate->getType() != "FOUND") continue;
        if (candidate->getStatus() == "Returned" || candidate->getStatus() == "Closed") continue;

        int score = 0;
        if (toLower(candidate->getName()) == toLower(lost->getName())) score += 30;
        if (toLower(candidate->getCategory()) == toLower(lost->getCategory())) score += 20;
        if (!lost->getColor().empty() && toLower(candidate->getColor()) == toLower(lost->getColor())) score += 15;
        if (toLower(candidate->getLocation()) == toLower(lost->getLocation())) score += 20;
        score += dateSimilarityScore(candidate->getDate(), lost->getDate());

        // small bonus for shared description keywords
        std::istringstream words(lost->getDescription());
        std::string w;
        while (words >> w) {
            if (w.size() >= 4 && containsIgnoreCase(candidate->getDescription(), w)) {
                score += 5;
                break; // only count the keyword bonus once, keeps scoring simple
            }
        }

        if (score > 0) results.push_back({candidate, score});
    }

    std::sort(results.begin(), results.end(), [](const MatchResult& a, const MatchResult& b) {
        return a.score > b.score;
    });
    return results;
}

// ---------------------------------------------------------------
// claims
// ---------------------------------------------------------------
Claim* LostAndFoundSystem::submitClaim(int itemID, int userID, const std::string& description, const std::string& date) {
    Item* it = findItemByID(itemID);
    if (!it || it->getType() != "FOUND") return nullptr;
    claims.push_back(Claim(itemID, userID, description, date));
    it->setStatus("Claim Pending");
    return &claims.back();
}

std::vector<Claim> LostAndFoundSystem::getClaimsByUser(int userID) const {
    std::vector<Claim> result;
    for (const Claim& c : claims) if (c.getUserID() == userID) result.push_back(c);
    return result;
}

Claim* LostAndFoundSystem::findClaimByID(int claimID) {
    for (Claim& c : claims) if (c.getClaimID() == claimID) return &c;
    return nullptr;
}

bool LostAndFoundSystem::approveClaim(int claimID) {
    Claim* c = findClaimByID(claimID);
    if (!c) return false;
    c->setStatus("Approved");
    Item* it = findItemByID(c->getItemID());
    if (it) it->setStatus("Returned");
    return true;
}

bool LostAndFoundSystem::rejectClaim(int claimID) {
    Claim* c = findClaimByID(claimID);
    if (!c) return false;
    c->setStatus("Rejected");
    Item* it = findItemByID(c->getItemID());
    if (it && it->getStatus() == "Claim Pending") it->setStatus("Found");
    return true;
}

bool LostAndFoundSystem::requestVerification(int claimID) {
    Claim* c = findClaimByID(claimID);
    if (!c) return false;
    c->setStatus("Verification Requested");
    return true;
}

// ---------------------------------------------------------------
// admin utilities
// ---------------------------------------------------------------
bool LostAndFoundSystem::markReturned(int itemID) {
    Item* it = findItemByID(itemID);
    if (!it) return false;
    it->setStatus("Returned");
    return true;
}

void LostAndFoundSystem::printStatistics() const {
    int lostCount = 0, foundCount = 0, returnedCount = 0, pendingCount = 0, matchedCount = 0, closedCount = 0;
    for (Item* it : items) {
        if (it->getType() == "LOST") lostCount++; else foundCount++;
        std::string st = it->getStatus();
        if (st == "Returned") returnedCount++;
        else if (st == "Claim Pending") pendingCount++;
        else if (st == "Matched") matchedCount++;
        else if (st == "Closed") closedCount++;
    }
    int pendingClaims = 0, approvedClaims = 0, rejectedClaims = 0;
    for (const Claim& c : claims) {
        if (c.getStatus() == "Pending" || c.getStatus() == "Verification Requested") pendingClaims++;
        else if (c.getStatus() == "Approved") approvedClaims++;
        else if (c.getStatus() == "Rejected") rejectedClaims++;
    }

    std::cout << "\n----- SYSTEM STATISTICS -----\n";
    std::cout << "Total Users        : " << users.size() << "\n";
    std::cout << "Total Items        : " << items.size() << "\n";
    std::cout << "  Lost Items       : " << lostCount << "\n";
    std::cout << "  Found Items      : " << foundCount << "\n";
    std::cout << "  Matched          : " << matchedCount << "\n";
    std::cout << "  Claim Pending    : " << pendingCount << "\n";
    std::cout << "  Returned         : " << returnedCount << "\n";
    std::cout << "  Closed           : " << closedCount << "\n";
    std::cout << "Total Claims       : " << claims.size() << "\n";
    std::cout << "  Pending/Review   : " << pendingClaims << "\n";
    std::cout << "  Approved         : " << approvedClaims << "\n";
    std::cout << "  Rejected         : " << rejectedClaims << "\n";
    std::cout << "------------------------------\n";
}
