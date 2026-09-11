// ================================================================
// server_main.cpp
// ----------------------------------------------------------------
// A small REST API server for the web front end.
//
// It does NOT duplicate any business logic: it simply sits on top
// of the exact same LostAndFoundSystem class used by the console
// app (main.cpp) and translates HTTP + JSON requests into calls on
// that object, then turns the results back into JSON. All the
// rules (matching score, claim workflow, search, sorting, file
// persistence) live in one place: LostAndFoundSystem.cpp.
//
// Libraries used (both are single-header, MIT licensed, and vendored
// under server/third_party/ so nothing needs to be installed):
//   - cpp-httplib (httplib.h)  -> minimal HTTP server
//   - nlohmann/json (json.hpp) -> JSON parsing/building
// ================================================================

#include "third_party/httplib.h"
#include "third_party/json.hpp"

#include "../User.h"
#include "../Item.h"
#include "../Claim.h"
#include "../LostAndFoundSystem.h"

#include <mutex>
#include <csignal>
#include <fstream>

using json = nlohmann::json;

// The whole system is single-instance, in-memory, and file-backed --
// exactly like the console app. A mutex keeps things safe if two
// browser requests arrive at the same moment (httplib serves requests
// on a small thread pool).
static LostAndFoundSystem sys;
static std::mutex sysMutex;

// ---------------------------------------------------------------
// JSON serialization helpers
// ---------------------------------------------------------------
static json userToJson(const User* u) {
    if (!u) return nullptr;
    return json{
        {"id", u->getID()},
        {"username", u->getUsername()},
        {"fullName", u->getFullName()},
        {"contact", u->getContact()},
        {"role", u->getRole()}
    };
}

static json itemToJson(const Item* it) {
    if (!it) return nullptr;
    return json{
        {"id", it->getID()},
        {"type", it->getType()},
        {"name", it->getName()},
        {"category", it->getCategory()},
        {"description", it->getDescription()},
        {"color", it->getColor()},
        {"location", it->getLocation()},
        {"date", it->getDate()},
        {"time", it->getTime()},
        {"personName", it->getPersonName()},
        {"contact", it->getContactInfo()},
        {"status", it->getStatus()},
        {"reportedByUserId", it->getReportedByUserID()}
    };
}

static json claimToJson(const Claim& c) {
    return json{
        {"id", c.getClaimID()},
        {"itemId", c.getItemID()},
        {"userId", c.getUserID()},
        {"description", c.getDescription()},
        {"date", c.getDate()},
        {"status", c.getStatus()}
    };
}

static json itemsToJson(const std::vector<Item*>& list) {
    json arr = json::array();
    for (Item* it : list) arr.push_back(itemToJson(it));
    return arr;
}

// Sends a JSON body with the right content type in one line.
static void sendJson(httplib::Response& res, const json& body, int status = 200) {
    res.status = status;
    res.set_content(body.dump(), "application/json");
}

static void sendError(httplib::Response& res, int status, const std::string& message) {
    sendJson(res, json{{"error", message}}, status);
}

// Safely reads a field out of a request body, throwing a friendly
// error (caught by the route wrapper below) if it's missing/blank.
static std::string requireField(const json& body, const std::string& key) {
    if (!body.contains(key) || !body[key].is_string() || body[key].get<std::string>().empty()) {
        throw std::runtime_error("Missing or empty field: " + key);
    }
    return body[key].get<std::string>();
}
static std::string optionalField(const json& body, const std::string& key, const std::string& fallback = "") {
    if (body.contains(key) && body[key].is_string()) return body[key].get<std::string>();
    return fallback;
}
static int requireIntField(const json& body, const std::string& key) {
    if (!body.contains(key) || !body[key].is_number_integer()) {
        throw std::runtime_error("Missing or invalid integer field: " + key);
    }
    return body[key].get<int>();
}

// Sorts an item list in place if the caller passed ?sort=field
static void applySortParam(const httplib::Request& req, std::vector<Item*>& list) {
    if (req.has_param("sort")) {
        LostAndFoundSystem::sortItems(list, req.get_param_value("sort"));
    }
}

// True if a regular file can be opened for reading at `path`.
static bool fileReadable(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    return f.good();
}

// Directory containing `path` (everything before the last slash/backslash),
// or "." if there isn't one. Plain string handling -- no <filesystem> --
// so this builds the same way on every compiler/toolchain.
static std::string dirOf(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos) return ".";
    return path.substr(0, pos);
}

// Finds the "web" folder that ships with the project regardless of the
// directory the server happens to be launched from (e.g. a CMake build
// folder, or double-clicking the binary from a file browser). Tries, in
// order: ./web relative to the current working directory, then ./web next
// to the executable itself, then ../web (in case the binary sits one level
// down, e.g. inside a build/ subfolder). Checks for web/index.html so it
// only picks a candidate that actually has the front end in it.
// `triedOut` is filled with every candidate path checked, purely so a
// helpful message can be shown in the browser if none of them pan out.
static std::string findWebRoot(const std::string& exePath, std::vector<std::string>& triedOut) {
    std::vector<std::string> candidates = { "web" };
    if (!exePath.empty()) {
        std::string exeDir = dirOf(exePath);
        candidates.push_back(exeDir + "/web");
        candidates.push_back(exeDir + "/../web");
    }
    for (const auto& c : candidates) {
        triedOut.push_back(c);
        if (fileReadable(c + "/index.html")) {
            return c;
        }
    }
    return ""; // none of the candidates panned out
}

int main(int argc, char** argv) {
    int port = 8080;
    if (argc > 1) {
        try { port = std::stoi(argv[1]); } catch (...) { /* keep default */ }
    }

    {
        std::lock_guard<std::mutex> lock(sysMutex);
        sys.loadAll();
    }

    httplib::Server server;

    // Allow the front end to be opened from a different origin during
    // development (e.g. a live-reload dev server). Harmless once the
    // server is also the one serving the static files below.
    server.set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type"}
    });
    server.Options(R"(.*)", [](const httplib::Request&, httplib::Response& res) { res.status = 204; });

    // Serve the front end (web/index.html, css, js) as static files.
    // findWebRoot() locates the web/ folder even if the server wasn't
    // launched from the project root (e.g. a CMake build directory).
    std::vector<std::string> triedPaths;
    std::string webRoot = findWebRoot(argc > 0 ? argv[0] : "", triedPaths);

    if (!webRoot.empty()) {
        server.set_mount_point("/", webRoot);
    } else {
        // Couldn't find web/index.html anywhere. Rather than a bare, unhelpful
        // 404, explain exactly what was tried directly in the browser -- this
        // is the only place the person will actually be looking.
        std::string triedList;
        for (const auto& t : triedPaths) {
            triedList += "<li><code>" + t + "/index.html</code></li>";
        }
        std::string diag =
            "<html><body style='font-family:sans-serif;max-width:640px;margin:40px auto;line-height:1.5'>"
            "<h2>Can't find the front end</h2>"
            "<p>The server is running, but it couldn't find <code>web/index.html</code>. "
            "It looked in:</p><ul>" + triedList + "</ul>"
            "<p>Make sure the <code>web</code> folder (with <code>index.html</code> inside it) "
            "sits right next to the server executable, then restart the server.</p>"
            "</body></html>";
        server.set_error_handler([diag](const httplib::Request&, httplib::Response& res) {
            if (res.status == 404) {
                res.set_content(diag, "text/html");
            }
        });
    }

    // ---------------- auth ----------------

    server.Post("/api/register", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(sysMutex);
        try {
            json body = json::parse(req.body);
            std::string username = requireField(body, "username");
            std::string password = requireField(body, "password");
            std::string fullName = requireField(body, "fullName");
            std::string contact  = requireField(body, "contact");

            if (sys.usernameExists(username)) {
                sendError(res, 409, "That username is already taken.");
                return;
            }
            User* u = sys.registerStudent(username, password, fullName, contact);
            sys.saveAll();
            sendJson(res, userToJson(u), 201);
        } catch (std::exception& e) {
            sendError(res, 400, e.what());
        }
    });

    server.Post("/api/login", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(sysMutex);
        try {
            json body = json::parse(req.body);
            std::string username = requireField(body, "username");
            std::string password = requireField(body, "password");
            std::string role = optionalField(body, "role", ""); // "STUDENT", "ADMIN", or "" for either

            User* u = sys.login(username, password, role);
            if (!u) {
                sendError(res, 401, "Invalid username, password, or role.");
                return;
            }
            sendJson(res, userToJson(u));
        } catch (std::exception& e) {
            sendError(res, 400, e.what());
        }
    });

    // ---------------- items ----------------

    server.Get("/api/items", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(sysMutex);
        std::vector<Item*> list = sys.getAllItems(); // copy of pointer vector, safe to sort locally
        applySortParam(req, list);
        sendJson(res, itemsToJson(list));
    });

    server.Get("/api/items/user/:userId", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(sysMutex);
        int userId = std::stoi(req.path_params.at("userId"));
        sendJson(res, itemsToJson(sys.getItemsByUser(userId)));
    });

    server.Get("/api/items/:id", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(sysMutex);
        int id = std::stoi(req.path_params.at("id"));
        Item* it = sys.findItemByID(id);
        if (!it) { sendError(res, 404, "Item not found."); return; }
        sendJson(res, itemToJson(it));
    });

    server.Get("/api/items/:id/matches", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(sysMutex);
        int id = std::stoi(req.path_params.at("id"));
        auto matches = sys.matchLostItem(id);
        json arr = json::array();
        for (auto& m : matches) {
            json entry = itemToJson(m.item);
            entry["score"] = m.score;
            arr.push_back(entry);
        }
        sendJson(res, arr);
    });

    server.Post("/api/items/lost", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(sysMutex);
        try {
            json body = json::parse(req.body);
            Item* it = sys.reportLostItem(
                requireField(body, "name"), requireField(body, "category"),
                optionalField(body, "description"), optionalField(body, "color"),
                requireField(body, "location"), requireField(body, "date"),
                optionalField(body, "time"), requireField(body, "personName"),
                requireField(body, "contact"), requireIntField(body, "userId"));
            sys.saveAll();

            json result = itemToJson(it);
            json matchArr = json::array();
            for (auto& m : sys.matchLostItem(it->getID())) {
                json entry = itemToJson(m.item);
                entry["score"] = m.score;
                matchArr.push_back(entry);
            }
            result["matches"] = matchArr;
            sendJson(res, result, 201);
        } catch (std::exception& e) {
            sendError(res, 400, e.what());
        }
    });

    server.Post("/api/items/found", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(sysMutex);
        try {
            json body = json::parse(req.body);
            Item* it = sys.reportFoundItem(
                requireField(body, "name"), requireField(body, "category"),
                optionalField(body, "description"), optionalField(body, "color"),
                requireField(body, "location"), requireField(body, "date"),
                optionalField(body, "time"), requireField(body, "personName"),
                requireField(body, "contact"), requireIntField(body, "userId"));
            sys.saveAll();
            sendJson(res, itemToJson(it), 201);
        } catch (std::exception& e) {
            sendError(res, 400, e.what());
        }
    });

    server.Post("/api/items/:id/return", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(sysMutex);
        int id = std::stoi(req.path_params.at("id"));
        if (!sys.markReturned(id)) { sendError(res, 404, "Item not found."); return; }
        sys.saveAll();
        sendJson(res, json{{"ok", true}});
    });

    server.Delete("/api/items/:id", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(sysMutex);
        int id = std::stoi(req.path_params.at("id"));
        if (!sys.deleteItem(id)) { sendError(res, 404, "Item not found."); return; }
        sys.saveAll();
        sendJson(res, json{{"ok", true}});
    });

    // ---------------- search ----------------
    // GET /api/search?field=name|category|location|date|status|id&q=...&sort=...
    server.Get("/api/search", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(sysMutex);
        std::string field = req.get_param_value("field");
        std::string q = req.get_param_value("q");
        std::vector<Item*> results;

        if (field == "name") results = sys.searchByName(q);
        else if (field == "category") results = sys.searchByCategory(q);
        else if (field == "location") results = sys.searchByLocation(q);
        else if (field == "date") results = sys.searchByDate(q);
        else if (field == "status") results = sys.searchByStatus(q);
        else if (field == "id") {
            try {
                Item* it = sys.searchByID(std::stoi(q));
                if (it) results.push_back(it);
            } catch (...) { /* non-numeric id -> no results */ }
        } else {
            sendError(res, 400, "field must be one of: name, category, location, date, status, id");
            return;
        }
        applySortParam(req, results);
        sendJson(res, itemsToJson(results));
    });

    // ---------------- claims ----------------

    server.Post("/api/claims", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(sysMutex);
        try {
            json body = json::parse(req.body);
            int itemId = requireIntField(body, "itemId");
            int userId = requireIntField(body, "userId");
            std::string description = requireField(body, "description");
            std::string date = requireField(body, "date");

            Claim* c = sys.submitClaim(itemId, userId, description, date);
            if (!c) { sendError(res, 400, "Item does not exist or is not a found item."); return; }
            sys.saveAll();
            sendJson(res, claimToJson(*c), 201);
        } catch (std::exception& e) {
            sendError(res, 400, e.what());
        }
    });

    server.Get("/api/claims", [](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(sysMutex);
        json arr = json::array();
        for (const Claim& c : sys.getAllClaims()) arr.push_back(claimToJson(c));
        sendJson(res, arr);
    });

    server.Get("/api/claims/user/:userId", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(sysMutex);
        int userId = std::stoi(req.path_params.at("userId"));
        json arr = json::array();
        for (const Claim& c : sys.getClaimsByUser(userId)) arr.push_back(claimToJson(c));
        sendJson(res, arr);
    });

    server.Post("/api/claims/:id/approve", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(sysMutex);
        int id = std::stoi(req.path_params.at("id"));
        if (!sys.approveClaim(id)) { sendError(res, 404, "Claim not found."); return; }
        sys.saveAll();
        sendJson(res, json{{"ok", true}});
    });

    server.Post("/api/claims/:id/reject", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(sysMutex);
        int id = std::stoi(req.path_params.at("id"));
        if (!sys.rejectClaim(id)) { sendError(res, 404, "Claim not found."); return; }
        sys.saveAll();
        sendJson(res, json{{"ok", true}});
    });

    server.Post("/api/claims/:id/verify", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(sysMutex);
        int id = std::stoi(req.path_params.at("id"));
        if (!sys.requestVerification(id)) { sendError(res, 404, "Claim not found."); return; }
        sys.saveAll();
        sendJson(res, json{{"ok", true}});
    });

    // ---------------- admin stats ----------------

    server.Get("/api/stats", [](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(sysMutex);
        int lost = 0, found = 0, matched = 0, pending = 0, returned = 0, closed = 0;
        for (Item* it : sys.getAllItems()) {
            if (it->getType() == "LOST") lost++; else found++;
            const std::string& st = it->getStatus();
            if (st == "Matched") matched++;
            else if (st == "Claim Pending") pending++;
            else if (st == "Returned") returned++;
            else if (st == "Closed") closed++;
        }
        int claimPending = 0, claimApproved = 0, claimRejected = 0;
        for (const Claim& c : sys.getAllClaims()) {
            if (c.getStatus() == "Approved") claimApproved++;
            else if (c.getStatus() == "Rejected") claimRejected++;
            else claimPending++;
        }
        sendJson(res, json{
            {"totalItems", (int)sys.getAllItems().size()},
            {"lostItems", lost}, {"foundItems", found},
            {"matchedItems", matched}, {"claimPendingItems", pending},
            {"returnedItems", returned}, {"closedItems", closed},
            {"totalClaims", (int)sys.getAllClaims().size()},
            {"claimsPending", claimPending}, {"claimsApproved", claimApproved},
            {"claimsRejected", claimRejected}
        });
    });

    server.Get("/api/categories", [](const httplib::Request&, httplib::Response& res) {
        sendJson(res, json::array({"Electronics", "Books", "Wallet", "ID/Card",
                                    "Clothing", "Keys", "Accessories", "Other"}));
    });

    server.listen("0.0.0.0", port);

    std::lock_guard<std::mutex> lock(sysMutex);
    sys.saveAll();
    return 0;
}
