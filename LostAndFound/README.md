# College Lost & Found Management System
### C++ Console App + Web UI — Military College of Signals

A multi-file Lost & Found system built with standard C++17 and the STL, available in two
front ends that share the exact same backend code and data files:

- **Console app** (`lostfound`) — the original terminal UI, no dependencies at all.
- **Web app** (`lostfound_server`) — a browser UI (HTML/CSS/JS) served by a small C++ HTTP
  server, so you can run the whole thing from a browser instead of a terminal.

Both cover OOP, file handling, searching, sorting, and a simple item-matching algorithm; the
web server additionally covers basic REST/JSON API design and serving a browser front end.

---

## 1. Project Architecture

```
LostAndFound/
├── main.cpp                   Console UI + input validation (console app entry point)
├── User.h / User.cpp          User, Student, Admin
├── Item.h / Item.cpp          Item, LostItem, FoundItem
├── Claim.h / Claim.cpp        Claim
├── LostAndFoundSystem.h/.cpp  Core manager: storage, search, sort, matching, claims
├── server/
│   ├── server_main.cpp          Web server entry point: HTTP/JSON routes -> LostAndFoundSystem
│   └── third_party/
│       ├── httplib.h            Vendored single-header HTTP server (cpp-httplib, MIT)
│       └── json.hpp             Vendored single-header JSON library (nlohmann/json, MIT)
├── web/                        Static browser front end, served by lostfound_server
│   ├── index.html                Page structure (split auth screen + app shell)
│   ├── css/style.css             Visual design (navy/brass "registry" theme)
│   └── js/app.js                 All frontend logic (fetch calls, rendering, no framework)
├── Makefile                   Build script: `make` (console) / `make web` (server)
├── CMakeLists.txt             CLion/CMake build script (both targets)
├── README.md                  This file
└── data/
    ├── users.txt      (created at runtime, shared by both front ends)
    ├── items.txt      (created at runtime, shared by both front ends)
    └── claims.txt     (created at runtime, shared by both front ends)
```

**Layering:** `main.cpp` and `server/server_main.cpp` are the *only* two files that know about
console I/O or HTTP respectively. All business logic (registration rules, searching, sorting,
matching, claim approval) lives once, in `LostAndFoundSystem`, which in turn delegates data
behaviour to `User`/`Item`/`Claim`. Neither front end duplicates that logic — the console app
and the web app are two thin "views" over one shared "model", and because they read/write the
same `data/*.txt` files, an item reported through the browser shows up in the console app
(and vice versa) the next time either one loads.

## 2. Class Diagram (textual)

```
        User (abstract)                 Item (abstract)
     ────────────────────           ─────────────────────
     # userID, username,             # itemID, itemName, category,
       password, fullName,             description, color, location,
       contact                         date, time, personName,
     + getRole() = 0                   contactInfo, status,
     + display(), serialize()          reportedByUserID
            △                        + getType() = 0
      ┌─────┴─────┐                  + display(), serialize()
   Student       Admin                       △
                                      ┌───────┴────────┐
                                   LostItem         FoundItem

           Claim                        LostAndFoundSystem
     ──────────────────            ────────────────────────────
     - claimID, itemID,             - vector<Item*> items
       userID, description,         - vector<User*> users
       date, status                 - unordered_map<string,int> usernameToIndex
     + display(), serialize()       - vector<Claim> claims
                                     + register/login, report items,
                                       search*, sortItems, matchLostItem,
                                       submitClaim/approve/reject,
                                       loadAll/saveAll, printStatistics
```

## 3. Responsibilities of Each Class

| Class | Responsibility |
|---|---|
| `User` | Abstract base for any account: id, credentials, contact info, password check. |
| `Student` | Concrete user role; can report items, search, and submit claims. |
| `Admin` | Concrete user role; can review, approve/reject, and manage records. |
| `Item` | Abstract base for a reported item: shared fields + serialization. |
| `LostItem` | An item a student lost; status defaults to `Lost`. |
| `FoundItem` | An item a student found; status defaults to `Found`. |
| `Claim` | A student's request to own a found item, with its own status lifecycle. |
| `LostAndFoundSystem` | Owns all in-memory collections, file I/O, search/sort/matching, and claim workflow. |
| `server_main.cpp` *(web only)* | Translates HTTP/JSON requests into calls on `LostAndFoundSystem`; contains no business logic of its own — see §19. |

## 4. Data Structures Used (and why)

| Structure | Where | Why |
|---|---|---|
| `std::vector<Item*>` | all items | Preserves insertion/report order, supports `std::sort`, cheap iteration for search/listing. |
| `std::vector<User*>` | all accounts | Same reasons; small enough that O(n) admin listing is fine. |
| `std::unordered_map<std::string,int>` | `usernameToIndex` | O(1) average lookup for duplicate-username checks and login, instead of scanning every user. |
| `std::vector<Claim>` | all claims | Claims are small value objects (no polymorphism needed), so stored by value, not pointer. |
| `std::vector<MatchResult>` | matching results | Holds `(Item*, score)` pairs so they can be `std::sort`-ed by score descending. |

## 5. File Format Design

All files are plain text, one record per line, fields separated by `|` (pipe), chosen because
none of the data fields naturally contain a `|` character.

**users.txt**
```
userID|username|password|fullName|contact|role
1|admin|admin123|System Administrator|admin@mcs.edu.pk|ADMIN
```

**items.txt**
```
itemID|type|name|category|description|color|location|date|time|personName|contactInfo|status|reportedByUserID
1|LOST|Wallet|Wallet|Black leather wallet|Black|Library|2026-09-01|10:30|Alice Khan|0300-1111111|Lost|2
```

**claims.txt**
```
claimID|itemID|userID|description|date|status
1|2|2|It's mine, has my ID card inside|2026-09-02|Pending
```

A default admin account (`admin` / `admin123`) is auto-created on first run if no admin
exists yet in `users.txt`, so the system is usable immediately.

## 6. Program Flow

1. `main()` constructs a `LostAndFoundSystem` and calls `loadAll()`, which reads the three
   `data/*.txt` files (if present) and rebuilds every object in memory, restoring the
   `nextID` counters so new records don't collide with old ones.
2. The main menu offers Register / Login / Admin Login / Exit.
3. After login, the user/admin menu loop runs until logout; every action calls
   `saveAll()` immediately afterward so no data is lost even if the program is closed
   unexpectedly.
4. On Exit, `saveAll()` runs once more and the program terminates; the destructor frees
   all heap-allocated `Item*`/`User*` objects.

## 7. Where OOP Concepts Are Used

* **Encapsulation** – all class fields are `private`/`protected`; access is only through
  getters or controlled setters (e.g. `Item::setStatus`).
* **Inheritance** – `Student`/`Admin` extend `User`; `LostItem`/`FoundItem` extend `Item`.
* **Polymorphism** – `Item::getType()`/`display()` and `User::getRole()` are `virtual`;
  `LostAndFoundSystem` stores `vector<Item*>`/`vector<User*>` and calls these through base
  pointers, getting the correct derived behaviour automatically.
* **Abstract classes** – `Item` and `User` each declare a pure virtual function, so neither
  can be instantiated directly — every stored object is genuinely a `LostItem`, `FoundItem`,
  `Student`, or `Admin`.
* **Constructors / overloading** – `Item` and `User` both have a "new record" constructor
  and a "reload from file" constructor (constructor overloading); `LostAndFoundSystem`
  offers separate `reportLostItem`/`reportFoundItem` methods rather than one overloaded
  method, since their semantics differ enough to warrant distinct names for a beginner
  project, per the "don't force OOP" guidance.
* **Static members** – `Item::nextItemID`, `User::nextUserID`, `Claim::nextClaimID` are
  static counters shared across all instances of their class, guaranteeing unique IDs and
  staying correct even after reloading saved data.

## 8. Where Data Structures Are Used

See section 4 above — `vector` for ordered collections, `unordered_map` for O(1)
username lookups, and `std::sort` (STL algorithm) for all sorting operations.

## 9. Searching

Implemented as separate `LostAndFoundSystem` methods (a form of overloading-by-purpose):
`searchByName` (partial, case-insensitive), `searchByCategory`, `searchByLocation`
(partial), `searchByDate`, `searchByStatus`, `searchByID` (exact).

## 10. Sorting

`LostAndFoundSystem::sortItems(vector<Item*>&, field)` uses `std::sort` with a lambda
comparator per field (`date`, `name`, `category`, `location`, `status`). Both the "View All
Items" screen and search results can be sorted this way.

## 11. Matching Algorithm

For a given lost item, every *open* found item (not yet `Returned`/`Closed`) is scored:

| Match | Points |
|---|---|
| Same item name | +30 |
| Same category | +20 |
| Same color | +15 |
| Same location | +20 |
| Same date | +15 |
| Shared description keyword (≥4 letters) | +5 |

Results are sorted highest-score-first with `std::sort` and shown to the student right
after they report a lost item (top 5), so they can immediately see if their item may
already have been found.

## 12. Claim Workflow

1. Student picks a `Found` item and submits a claim → item status becomes `Claim Pending`.
2. Admin reviews the claim from *Manage Claims*:
   - **Approve** → claim `Approved`, item `Returned`.
   - **Reject** → claim `Rejected`, item reverts to `Found`.
   - **Request Verification** → claim `Verification Requested` (admin needs more proof).

## 13. Validation Implemented

* Empty required fields are rejected and re-prompted (`readLine`).
* Numeric menu input is validated (`readInt`); non-numeric input re-prompts instead of
  crashing.
* Dates must match `YYYY-MM-DD` (checked with `std::regex`).
* Duplicate usernames are rejected at registration.
* Login checks both username existence and password match, and (for the two entry points)
  the correct role (`STUDENT` vs `ADMIN`).
* Invalid item/claim IDs are checked with `findItemByID`/`findClaimByID` before use.
* Redirected/EOF input (e.g. during automated testing) causes a graceful exit instead of
  an infinite loop.

---

## 14. Compile & Run

Requires a C++17-capable compiler (e.g. g++ 9+).

### Console app

```bash
cd LostAndFound
make            # builds ./lostfound
./lostfound
```

Or compile manually:

```bash
g++ -std=c++17 -Wall -Wextra -O2 -o lostfound main.cpp Item.cpp User.cpp Claim.cpp LostAndFoundSystem.cpp
./lostfound
```

### Web app (browser UI)

```bash
cd LostAndFound
make web                 # builds ./lostfound_server
./lostfound_server       # listens on http://localhost:8080 by default
./lostfound_server 9000  # or pick a different port
```

Then open the printed address in a browser. The server must be started **from the project
root** (`LostAndFound/`), since it looks for `./web` (the frontend files) and `./data`
(the same on-disk records the console app uses) relative to the working directory.

Or compile manually (needs `-pthread` for the HTTP server's thread pool):

```bash
g++ -std=c++17 -Wall -Wextra -O2 -o lostfound_server server/server_main.cpp \
    User.cpp Item.cpp Claim.cpp LostAndFoundSystem.cpp -pthread
./lostfound_server
```

To clean build artifacts: `make clean`.

A default admin account is created automatically the first time either front end runs:
- **Username:** `admin`
- **Password:** `admin123`

### A note on "no external libraries"

The console app (`main.cpp` + the model classes) is still 100% standard C++ with no
dependencies, exactly as specified. The **web server only** vendors two small, widely-used
single-header libraries under `server/third_party/` so it can speak HTTP and JSON without
hand-rolling a socket/parsing layer, which would be a distraction from the actual project
(OOP, data structures, file handling):

- **cpp-httplib** (`httplib.h`) — turns a few method calls into a real HTTP server.
- **nlohmann/json** (`json.hpp`) — converts between C++ objects and JSON text.

Both are single files, MIT-licensed, and already sitting in `server/third_party/` — nothing
to download or install. If your assignment requires the *console* app to have zero
dependencies, that requirement is unaffected: `lostfound` never includes either header.

## 15. Sample Input / Output

```
=====================================
  MILITARY COLLEGE OF SIGNALS
     COLLEGE LOST & FOUND
=====================================

1. Register
2. Login
3. Admin Login
4. Exit
Choice: 1

--- Register New Student Account ---
Choose a username: alice
Choose a password: alice123
Full name: Alice Khan
Contact info (phone/email): 0300-1111111
Registration successful! You can now log in.

Choice: 2
Username: alice
Password: alice123
Welcome, Alice Khan!

========== USER MENU (Alice Khan) ==========
1. Report Lost Item
...
Choice: 1

--- Report Lost Item ---
Item name: Wallet
Select category:
  1. Electronics
  2. Books
  3. Wallet
  ...
Choice: 3
Description: Black leather wallet with student card
Color: Black
Location lost: Library
Date lost (YYYY-MM-DD): 2026-09-01
Time (HH:MM, optional): 10:30
Your name: Alice Khan
Contact info: 0300-1111111
Lost item reported with ID 1.
```

If a matching found item already exists, the console then prints something like:

```
Possible matches found among reported found items:
[FOUND]  ID: 2 | Type: FOUND | Name: Wallet | Category: Wallet | Color: Black | ...
   -> match score: 105
```

## 16. Test Cases

| # | Scenario | Steps | Expected Result |
|---|---|---|---|
| 1 | Register new student | Register with a fresh username | "Registration successful" message |
| 2 | Duplicate username | Register with an existing username | "already taken" message, re-prompted |
| 3 | Wrong login | Login with a bad password | "Invalid username or password" |
| 4 | Report lost item | Fill in a valid lost-item form | Item stored with new ID, status `Lost` |
| 5 | Report matching found item | Report a found item with the same name/category/color/location/date | Match score shown to be 100 (30+20+15+20+15) or more with a keyword bonus |
| 6 | Partial name search | Search by name `"wall"` | Returns items whose name contains "wall" (case-insensitive) |
| 7 | Sort by date | View all items, sort by date | Items appear oldest date first |
| 8 | Submit claim | Claim a `Found` item | Item status becomes `Claim Pending`, claim `Pending` |
| 9 | Admin approves claim | Admin approves the claim above | Claim becomes `Approved`, item becomes `Returned` |
| 10 | Admin rejects claim | Admin rejects a pending claim | Claim becomes `Rejected`, item reverts to `Found` |
| 11 | Invalid date format | Enter `01-09-2026` when a date is requested | Rejected, re-prompted until `YYYY-MM-DD` is given |
| 12 | Non-numeric menu choice | Type `abc` at a menu prompt | "Invalid number, please try again", no crash |
| 13 | Restart persistence | Exit, relaunch the program | Previously entered users/items/claims are still present |
| 14 | Delete record (admin) | Admin deletes an item by ID | Item removed from `items.txt` on next save |
| 15 | System statistics | Admin views statistics | Correct counts of items by status and claims by status |

## 17. Possible Viva Questions & Answers

**Q1: Why are `Item` and `User` abstract classes?**
A: Every real record in the system is specifically a lost/found item or a student/admin
account — there's no meaningful "generic Item" or "generic User". Making `getType()`/
`getRole()` pure virtual prevents instantiating the base class directly and forces every
subclass to define its identity, which is also what enables polymorphic storage in
`vector<Item*>`/`vector<User*>`.

**Q2: How does polymorphism show up in this project?**
A: `LostAndFoundSystem` stores base-class pointers (`Item*`, `User*`). When `display()` or
`getType()`/`getRole()` is called through those pointers, C++'s virtual dispatch runs the
correct derived-class version (`LostItem::display()` vs `FoundItem::display()`) without the
caller needing to know which subclass it actually is.

**Q3: Why use `unordered_map` for usernames instead of scanning the vector?**
A: `unordered_map` gives average O(1) lookup, so checking "does this username already
exist?" and logging in stay fast even as the number of users grows, instead of an O(n) scan
through every account.

**Q4: Why store `Claim` by value in a `vector<Claim>` but `Item`/`User` as pointers?**
A: Items and users need polymorphism (different subclasses behaving differently through a
common interface), which requires pointers/references. `Claim` has no subclasses and no
polymorphic behaviour, so storing it by value avoids unnecessary heap allocations and
manual memory management.

**Q5: How is data persistence implemented without a database?**
A: Plain `fstream` text files, one record per line, pipe-delimited. `saveAll()` serializes
every in-memory object with its own `serialize()` method; `loadAll()` parses each line back
into the right object type (deciding `LostItem` vs `FoundItem`, `Student` vs `Admin` from a
stored type/role field) and restores the static ID counters so new IDs never collide with
loaded ones.

**Q6: How does the matching algorithm work, and why isn't it AI/ML?**
A: It's a deterministic point-scoring rule: it compares specific attributes (name, category,
color, location, date, and a simple keyword check on the description) between a lost item
and every open found item, adds fixed weights for each match, and sorts by total score. This
is intentionally simple and explainable rather than a trained model.

**Q7: What happens to a found item's status through its lifecycle?**
A: `Found` → (a student claims it) → `Claim Pending` → (admin approves) → `Returned`, or
(admin rejects) → back to `Found`. An admin can also directly mark any item `Returned` or
`Closed`.

**Q8: How is invalid input handled so the program never crashes?**
A: `readInt`/`readLine` wrap `std::cin` in loops that detect failed extraction
(`cin.fail()`), clear the error state, discard the bad input, and re-prompt; dates are
additionally checked with a regex. EOF (no more input) triggers a graceful `exit(0)` instead
of looping forever.

**Q9: Why split the project into multiple files instead of one big .cpp?**
A: Separation of concerns — each class has its own header/implementation pair, which makes
the code easier to navigate, compile independently (faster incremental builds), and reuse.
`main.cpp` only contains the UI layer, so the underlying logic could be reused by a
different interface (e.g. a GUI) without modification.

**Q10: What STL algorithm powers sorting, and how are different fields supported?**
A: `std::sort` from `<algorithm>`, with a different lambda comparator selected based on the
requested field (`date`, `name`, `category`, `location`, `status`) — each lambda simply
compares that one attribute between two `Item*`.

**Q11: The web server adds routing code in `server_main.cpp`. Does that duplicate logic
already in `LostAndFoundSystem`?**
A: No — `server_main.cpp` has no business rules of its own. Every route just parses the
incoming JSON, calls the same public `LostAndFoundSystem` method the console app calls (e.g.
`reportLostItem`, `approveClaim`), and serializes the result back to JSON. This is the same
separation-of-concerns idea as `main.cpp`: the *view* (console text or HTTP/JSON) is thin, and
the *model* (`LostAndFoundSystem`) is where the real logic lives, so it only had to be written
once.

**Q12: How does the web server know who is making a request, since HTTP has no memory
between requests?**
A: `POST /api/login` returns the logged-in user's profile as JSON, and the browser keeps it
in `localStorage`, sending the user's `id` in the body of requests that need it (like
reporting an item). There's no session token and no server-side per-request auth check — a
deliberate simplification for a student project with no database. Conceptually it's the same
"logged-in user" idea `main.cpp` keeps in a local variable during a console session; a real
deployment would replace the plain `id` with a signed, expiring token the server verifies on
every request.

## 18. Possible Future Improvements

* Hash passwords instead of storing them in plain text.
* Replace the flat text files with SQLite for real concurrent multi-user access.
* Add email/SMS notifications when a high-scoring match is found.
* Add pagination for large item lists instead of printing everything at once.
* Add a "forgot password" / account recovery flow.
* Track full status history (audit log) per item, not just the current status.
* Allow image attachments for items (would require moving beyond plain text files).
* Fuzzy/Levenshtein-distance name matching instead of exact/substring matching, for typo
  tolerance in the matching algorithm.
* The web server now exists (see section 19) but still keeps everything in one process/one
  set of text files; a production version would add a real database and password hashing
  (both listed above) before letting more than one person use it over a network at once.

## 19. Web Frontend Architecture & API Reference

### How the pieces fit together

```
 Browser (web/index.html, css/style.css, js/app.js)
        │  fetch() calls, JSON over HTTP
        ▼
 server/server_main.cpp   (HTTP routes <-> JSON <-> C++ objects)
        │  calls the SAME public methods main.cpp calls
        ▼
 LostAndFoundSystem       (unchanged business logic)
        │
        ▼
 data/*.txt   (shared with the console app)
```

`server_main.cpp` adds nothing to the domain model — it is purely a translator between
HTTP/JSON and the existing `LostAndFoundSystem` API (the same class the console app's
`main.cpp` calls). Every route locks a single `std::mutex` around the shared
`LostAndFoundSystem` instance, so concurrent browser requests can't corrupt in-memory state,
and it saves to `data/*.txt` after every write so the browser and console app always agree.

**Authentication is intentionally simple**, matching the spirit of a student project with no
database: `POST /api/login` checks the username/password/role against `LostAndFoundSystem`
and returns the user's profile (id, username, full name, contact, role) as JSON. The browser
keeps that object in `localStorage` and includes the user's `id` in the body of requests that
need to know who's asking (e.g. reporting an item, submitting a claim). There are no session
tokens and no per-request server-side auth check — a real production deployment would add
signed/expiring tokens (e.g. JWT), HTTPS, and server-side authorization checks on every route;
here, the goal is demonstrating REST/JSON design end to end without that extra machinery.

### Endpoint reference

| Method & Path | Purpose |
|---|---|
| `GET /api/categories` | List of valid item categories |
| `POST /api/register` | Create a student account: `{username, password, fullName, contact}` |
| `POST /api/login` | Log in: `{username, password, role}` (`role` is `"STUDENT"`, `"ADMIN"`, or omitted for either) |
| `GET /api/items?sort=` | All items, optionally sorted (`date`\|`name`\|`category`\|`location`\|`status`) |
| `GET /api/items/:id` | A single item by ID |
| `GET /api/items/user/:userId` | Items reported by one user |
| `GET /api/items/:id/matches` | Match scores for a lost item against open found items |
| `POST /api/items/lost` | Report a lost item; response includes a `matches` array |
| `POST /api/items/found` | Report a found item |
| `POST /api/items/:id/return` | Mark an item Returned |
| `DELETE /api/items/:id` | Delete an item record |
| `GET /api/search?field=&q=&sort=` | Search by `name`\|`category`\|`location`\|`date`\|`status`\|`id` |
| `POST /api/claims` | Submit a claim: `{itemId, userId, description, date}` |
| `GET /api/claims` | All claims |
| `GET /api/claims/user/:userId` | One user's claims |
| `POST /api/claims/:id/approve` \| `/reject` \| `/verify` | Act on a claim |
| `GET /api/stats` | Same counts as the console "System Statistics" screen |

All request/response bodies are JSON. Errors come back as `{"error": "message"}` with an
appropriate HTTP status code (400 bad input, 401 login failure, 404 not found, 409 conflict).

### Frontend structure

`web/js/app.js` is a single, framework-free file: a small `api()` helper wraps `fetch`,
per-view `view*()` functions build HTML strings for each screen (Report Lost, Search, My
Claims, Manage Claims, Statistics, etc.), and item cards / claim rows attach their own click
handlers when rendered rather than relying on a router library. `web/css/style.css` defines
the visual language as CSS custom properties (`--ink-navy`, `--brass`, `--sage`, `--rust`, ...)
— a navy-and-brass "registry" theme (split login screen, hairline-bordered tag-style item
cards) chosen to fit a military college's administrative service rather than a generic SaaS
dashboard — built with plain CSS, no framework, to match the plain-C++ spirit of the backend.
