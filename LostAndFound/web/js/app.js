// ================================================================
// app.js — MCS Lost & Found web front end
// ----------------------------------------------------------------
// A small vanilla-JS single page app. No build step, no framework:
// everything talks to the C++ server over fetch() and renders plain
// HTML strings into #view-body. Kept deliberately simple so it's
// easy to read alongside the C++ backend for a university project.
// ================================================================

const API = ""; // same-origin: the C++ server serves this file too

// ---------------------------------------------------------------
// tiny API helper
// ---------------------------------------------------------------
async function api(method, path, body) {
  const opts = { method, headers: {} };
  if (body !== undefined) {
    opts.headers["Content-Type"] = "application/json";
    opts.body = JSON.stringify(body);
  }
  const res = await fetch(API + path, opts);
  let data = null;
  try { data = await res.json(); } catch (_) { /* empty body, fine */ }
  if (!res.ok) {
    const message = (data && data.error) ? data.error : `Request failed (${res.status})`;
    throw new Error(message);
  }
  return data;
}

// ---------------------------------------------------------------
// session (kept in localStorage so a page refresh doesn't log you out)
// ---------------------------------------------------------------
const Session = {
  get user() {
    try { return JSON.parse(localStorage.getItem("lf_user")); } catch (_) { return null; }
  },
  set user(u) { localStorage.setItem("lf_user", JSON.stringify(u)); },
  clear() { localStorage.removeItem("lf_user"); }
};

const CATEGORIES = ["Electronics", "Books", "Wallet", "ID/Card", "Clothing", "Keys", "Accessories", "Other"];

// ---------------------------------------------------------------
// small icon set (inline SVG, no external icon library needed)
// ---------------------------------------------------------------
const ICONS = {
  dashboard: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8"><rect x="3" y="3" width="8" height="8"/><rect x="13" y="3" width="8" height="5"/><rect x="13" y="12" width="8" height="9"/><rect x="3" y="13" width="8" height="8"/></svg>',
  lost: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8"><path d="M12 2 3 6v6c0 5 4 8.5 9 10 5-1.5 9-5 9-10V6l-9-4Z"/><path d="M12 8v5M12 16h.01"/></svg>',
  found: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8"><path d="m9 12 2 2 4-4"/><path d="M12 2 3 6v6c0 5 4 8.5 9 10 5-1.5 9-5 9-10V6l-9-4Z"/></svg>',
  search: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8"><circle cx="11" cy="11" r="7"/><path d="m21 21-4.3-4.3"/></svg>',
  list: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8"><path d="M8 6h13M8 12h13M8 18h13M3 6h.01M3 12h.01M3 18h.01"/></svg>',
  claim: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8"><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8Z"/><path d="M14 2v6h6M9 15l2 2 4-4"/></svg>',
  stats: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8"><path d="M3 3v18h18"/><rect x="7" y="12" width="3" height="6"/><rect x="12" y="8" width="3" height="10"/><rect x="17" y="5" width="3" height="13"/></svg>',
  pending: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8"><circle cx="12" cy="12" r="9"/><path d="M12 7v5l3 3"/></svg>',
};

// ---------------------------------------------------------------
// routing: one nav config per role
// ---------------------------------------------------------------
const STUDENT_NAV = [
  { id: "dashboard",   label: "Dashboard",        icon: "dashboard" },
  { id: "report-lost", label: "Report Lost Item", icon: "lost" },
  { id: "report-found",label: "Report Found Item",icon: "found" },
  { id: "browse",      label: "Search Items",     icon: "search" },
  { id: "my-reports",  label: "My Reports",       icon: "list" },
  { id: "my-claims",   label: "My Claims",        icon: "claim" },
];
const ADMIN_NAV = [
  { id: "admin-stats",   label: "Statistics",       icon: "stats" },
  { id: "browse",        label: "All Items",        icon: "list" },
  { id: "admin-pending",  label: "Pending Reports",  icon: "pending" },
  { id: "admin-claims",  label: "Manage Claims",    icon: "claim" },
];

let currentView = null;

function navFor(role) { return role === "ADMIN" ? ADMIN_NAV : STUDENT_NAV; }

function setView(viewId) {
  currentView = viewId;
  location.hash = "#/" + viewId;
  renderApp();
}

window.addEventListener("hashchange", () => {
  const id = (location.hash || "").replace(/^#\//, "");
  if (id) { currentView = id; renderApp(); }
});

// ---------------------------------------------------------------
// bootstrap
// ---------------------------------------------------------------
function boot() {
  wireAuthForms();
  const user = Session.user;
  if (user) {
    showAppShell(user);
  } else {
    showAuthScreen();
  }
}

function showAuthScreen() {
  document.getElementById("auth-screen").hidden = false;
  document.getElementById("app-shell").hidden = true;
}

function showAppShell(user) {
  document.getElementById("auth-screen").hidden = true;
  document.getElementById("app-shell").hidden = false;
  document.getElementById("current-user-name").textContent = user.fullName;
  document.getElementById("current-user-role").textContent = user.role === "ADMIN" ? "Front office" : "Student";
  renderSidebar(user);
  const initial = (location.hash || "").replace(/^#\//, "") || (user.role === "ADMIN" ? "admin-stats" : "dashboard");
  currentView = initial;
  renderApp();
}

function renderSidebar(user) {
  const nav = navFor(user.role);
  const html = nav.map(item => `
    <button class="nav-link" data-nav="${item.id}">${ICONS[item.icon]}<span>${item.label}</span></button>
  `).join("");
  document.getElementById("sidebar-nav").innerHTML = html;
  document.getElementById("sidebar-nav").querySelectorAll("[data-nav]").forEach(btn => {
    btn.addEventListener("click", () => setView(btn.dataset.nav));
  });
}

function markActiveNav() {
  document.querySelectorAll(".nav-link").forEach(el => {
    el.classList.toggle("is-active", el.dataset.nav === currentView);
  });
}

// ---------------------------------------------------------------
// auth forms
// ---------------------------------------------------------------
function wireAuthForms() {
  document.querySelectorAll(".auth-tab").forEach(tab => {
    tab.addEventListener("click", () => {
      document.querySelectorAll(".auth-tab").forEach(t => t.classList.remove("is-active"));
      tab.classList.add("is-active");
      document.getElementById("login-form").hidden = tab.dataset.tab !== "login";
      document.getElementById("register-form").hidden = tab.dataset.tab !== "register";
    });
  });

  document.getElementById("login-form").addEventListener("submit", async (e) => {
    e.preventDefault();
    const form = e.target;
    const errorEl = document.getElementById("login-error");
    errorEl.hidden = true;
    const asAdmin = form.asAdmin.checked;
    try {
      const user = await api("POST", "/api/login", {
        username: form.username.value.trim(),
        password: form.password.value,
        role: asAdmin ? "ADMIN" : "STUDENT"
      });
      Session.user = user;
      showAppShell(user);
    } catch (err) {
      errorEl.textContent = err.message;
      errorEl.hidden = false;
    }
  });

  document.getElementById("register-form").addEventListener("submit", async (e) => {
    e.preventDefault();
    const form = e.target;
    const errorEl = document.getElementById("register-error");
    errorEl.hidden = true;
    try {
      const user = await api("POST", "/api/register", {
        fullName: form.fullName.value.trim(),
        username: form.username.value.trim(),
        password: form.password.value,
        contact: form.contact.value.trim()
      });
      Session.user = user;
      showAppShell(user);
    } catch (err) {
      errorEl.textContent = err.message;
      errorEl.hidden = false;
    }
  });

  document.getElementById("logout-btn").addEventListener("click", () => {
    Session.clear();
    showAuthScreen();
  });
}

// ---------------------------------------------------------------
// toast
// ---------------------------------------------------------------
let toastTimer = null;
function toast(message, isError) {
  const el = document.getElementById("toast");
  el.textContent = message;
  el.classList.toggle("is-error", !!isError);
  el.hidden = false;
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => { el.hidden = true; }, 3200);
}

// ---------------------------------------------------------------
// modal
// ---------------------------------------------------------------
function openModal(html) {
  document.getElementById("modal").innerHTML = html;
  document.getElementById("modal-backdrop").hidden = false;
}
function closeModal() {
  document.getElementById("modal-backdrop").hidden = true;
  document.getElementById("modal").innerHTML = "";
}
document.getElementById("modal-backdrop").addEventListener("click", (e) => {
  if (e.target.id === "modal-backdrop") closeModal();
});

// ---------------------------------------------------------------
// formatting helpers
// ---------------------------------------------------------------
function esc(s) {
  return String(s ?? "").replace(/[&<>"']/g, c => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" }[c]));
}
function statusClass(status) {
  const s = status.toLowerCase().replace(/\s+/g, "-");
  return "status-" + s;
}
function itemCard(item) {
  return `
    <div class="item-card type-${item.type.toLowerCase()}" data-item-id="${item.id}">
      <div class="item-card-top">
        <span class="tag tag-${item.type.toLowerCase()}">${item.type}</span>
        <span class="item-card-id">#${String(item.id).padStart(4, "0")}</span>
      </div>
      <p class="item-card-name">${esc(item.name)}</p>
      <p class="item-card-meta">${esc(item.category)} &middot; ${esc(item.location)} &middot; ${esc(item.date)}</p>
      <p class="item-card-desc">${esc(item.description || "No description provided.")}</p>
      <div style="margin-top:10px;"><span class="tag tag-status ${statusClass(item.status)}">${esc(item.status)}</span></div>
    </div>`;
}
function categoryOptions(selected) {
  return CATEGORIES.map(c => `<option value="${c}" ${c === selected ? "selected" : ""}>${c}</option>`).join("");
}
function todayISO() { return new Date().toISOString().slice(0, 10); }

// ---------------------------------------------------------------
// main render dispatcher
// ---------------------------------------------------------------
async function renderApp() {
  markActiveNav();
  const user = Session.user;
  const body = document.getElementById("view-body");
  const eyebrow = document.getElementById("view-eyebrow");
  const title = document.getElementById("view-title");
  const actions = document.getElementById("view-actions");
  actions.innerHTML = "";
  body.innerHTML = `<div class="empty-state">Loading&hellip;</div>`;

  const views = {
    "dashboard":      () => viewStudentDashboard(user),
    "report-lost":    () => viewReportForm(user, "LOST"),
    "report-found":   () => viewReportForm(user, "FOUND"),
    "browse":         () => viewBrowse(user),
    "my-reports":     () => viewMyReports(user),
    "my-claims":      () => viewMyClaims(user),
    "admin-stats":    () => viewAdminStats(user),
    "admin-pending":  () => viewAdminPending(user),
    "admin-claims":   () => viewAdminClaims(user),
  };
  const meta = {
    "dashboard":      ["Overview", "Dashboard"],
    "report-lost":    ["New report", "Report a Lost Item"],
    "report-found":   ["New report", "Report a Found Item"],
    "browse":         ["Registry", user.role === "ADMIN" ? "All Items" : "Search Items"],
    "my-reports":     ["Registry", "My Reports"],
    "my-claims":      ["Claims", "My Claims"],
    "admin-stats":    ["Front office", "System Statistics"],
    "admin-pending":  ["Front office", "Pending Reports"],
    "admin-claims":   ["Front office", "Manage Claims"],
  };
  const m = meta[currentView] || ["Registry", "Dashboard"];
  eyebrow.textContent = m[0];
  title.textContent = m[1];

  const renderFn = views[currentView] || views["dashboard"];
  try {
    await renderFn();
  } catch (err) {
    body.innerHTML = `<div class="empty-state">Couldn't load this view: ${esc(err.message)}</div>`;
  }
}

// ================================================================
// VIEW: student dashboard
// ================================================================
async function viewStudentDashboard(user) {
  const [items, claims] = await Promise.all([
    api("GET", `/api/items/user/${user.id}`),
    api("GET", `/api/claims/user/${user.id}`)
  ]);
  const open = items.filter(i => !["Returned", "Closed"].includes(i.status));
  const pendingClaims = claims.filter(c => c.status === "Pending" || c.status === "Verification Requested");

  document.getElementById("view-body").innerHTML = `
    <div class="stat-grid">
      <div class="stat-card"><span class="num">${items.length}</span><span class="label">Total reports filed</span></div>
      <div class="stat-card"><span class="num">${open.length}</span><span class="label">Still open</span></div>
      <div class="stat-card"><span class="num">${claims.length}</span><span class="label">Claims submitted</span></div>
      <div class="stat-card"><span class="num">${pendingClaims.length}</span><span class="label">Awaiting review</span></div>
    </div>

    <div class="panel">
      <h2>Your most recent reports</h2>
      <p class="panel-sub">The last items you reported lost or found.</p>
      ${items.length ? `<div class="item-grid">${items.slice(0, 6).map(itemCard).join("")}</div>`
                     : `<div class="empty-state">You haven't reported anything yet. Use "Report Lost Item" or "Report Found Item" in the sidebar.</div>`}
    </div>
  `;
  wireItemCardClicks(user);
}

// ================================================================
// VIEW: report lost / found form
// ================================================================
function viewReportForm(user, type) {
  const isLost = type === "LOST";
  document.getElementById("view-body").innerHTML = `
    <div class="panel" style="max-width:640px;">
      <h2>${isLost ? "Report a lost item" : "Report a found item"}</h2>
      <p class="panel-sub">${isLost
        ? "Give as much detail as you can — the registry automatically compares this against found reports and shows possible matches."
        : "Thank you for handing this in. The registry will hold it until an owner comes forward and claims it."}</p>
      <form id="report-form">
        <div class="field-row">
          <label class="field"><span>Item name</span><input name="name" required placeholder="e.g. Black Ridge wallet"></label>
          <label class="field"><span>Category</span><select name="category">${categoryOptions()}</select></label>
        </div>
        <label class="field"><span>Description</span><textarea name="description" placeholder="Distinguishing details, contents, brand, etc."></textarea></label>
        <div class="field-row">
          <label class="field"><span>Color</span><input name="color" placeholder="e.g. Brown"></label>
          <label class="field"><span>Location ${isLost ? "lost" : "found"}</span><input name="location" required placeholder="e.g. Main Cafeteria"></label>
        </div>
        <div class="field-row">
          <label class="field"><span>Date ${isLost ? "lost" : "found"}</span><input name="date" type="date" required value="${todayISO()}"></label>
          <label class="field"><span>Time (optional)</span><input name="time" type="time"></label>
        </div>
        <div class="field-row">
          <label class="field"><span>Your name</span><input name="personName" required value="${esc(user.fullName)}"></label>
          <label class="field"><span>Contact info</span><input name="contact" required value="${esc(user.contact)}"></label>
        </div>
        <p class="form-error" id="report-error" hidden></p>
        <button type="submit" class="btn btn-primary">Submit report</button>
      </form>
    </div>
  `;

  document.getElementById("report-form").addEventListener("submit", async (e) => {
    e.preventDefault();
    const f = e.target;
    const errorEl = document.getElementById("report-error");
    errorEl.hidden = true;
    const payload = {
      name: f.name.value.trim(), category: f.category.value,
      description: f.description.value.trim(), color: f.color.value.trim(),
      location: f.location.value.trim(), date: f.date.value,
      time: f.time.value, personName: f.personName.value.trim(),
      contact: f.contact.value.trim(), userId: user.id
    };
    try {
      const result = await api("POST", `/api/items/${isLost ? "lost" : "found"}`, payload);
      if (isLost && result.matches && result.matches.length) {
        showMatchesModal(result, result.matches);
      } else {
        toast(`${isLost ? "Lost" : "Found"} item reported with ID #${String(result.id).padStart(4, "0")}.`);
        setView(isLost ? "my-reports" : "my-reports");
      }
    } catch (err) {
      errorEl.textContent = err.message;
      errorEl.hidden = false;
    }
  });
}

function showMatchesModal(reportedItem, matches) {
  openModal(`
    <h2>Report filed &mdash; possible matches found</h2>
    <p class="panel-sub">Your lost item <strong>${esc(reportedItem.name)}</strong> (#${String(reportedItem.id).padStart(4, "0")})
      was compared against open found reports. Highest-scoring matches are below.</p>
    ${matches.slice(0, 5).map(m => `
      <div class="match-row">
        <div>
          <strong>${esc(m.name)}</strong> &middot; ${esc(m.category)} &middot; ${esc(m.location)}
          <div class="item-card-meta">#${String(m.id).padStart(4, "0")} &middot; found ${esc(m.date)}</div>
        </div>
        <span class="match-score">${m.score} pts</span>
      </div>
    `).join("")}
    <div class="modal-actions">
      <button class="btn btn-primary" id="modal-ok">Got it</button>
    </div>
  `);
  document.getElementById("modal-ok").addEventListener("click", () => { closeModal(); setView("my-reports"); });
}

// ================================================================
// VIEW: browse / search (shared by student + admin)
// ================================================================
async function viewBrowse(user) {
  const body = document.getElementById("view-body");
  body.innerHTML = `
    <div class="toolbar">
      <select id="search-field">
        <option value="">All items</option>
        <option value="name">Name contains&hellip;</option>
        <option value="category">Category is&hellip;</option>
        <option value="location">Location contains&hellip;</option>
        <option value="date">Date is&hellip;</option>
        <option value="status">Status is&hellip;</option>
        <option value="id">Item ID is&hellip;</option>
      </select>
      <input id="search-query" placeholder="Search term&hellip;" hidden>
      <select id="search-category" hidden>${categoryOptions()}</select>
      <select id="search-status" hidden>
        ${["Lost","Found","Matched","Claim Pending","Returned","Closed"].map(s => `<option value="${s}">${s}</option>`).join("")}
      </select>
      <button class="btn btn-ghost btn-small" id="search-go">Search</button>
      <span style="flex:1"></span>
      <select id="sort-field">
        <option value="">Sort by&hellip;</option>
        <option value="date">Date</option>
        <option value="name">Name</option>
        <option value="category">Category</option>
        <option value="location">Location</option>
        <option value="status">Status</option>
      </select>
    </div>
    <div id="browse-results"><div class="empty-state">Loading&hellip;</div></div>
  `;

  const fieldSel = document.getElementById("search-field");
  const queryInput = document.getElementById("search-query");
  const categorySel = document.getElementById("search-category");
  const statusSel = document.getElementById("search-status");

  function syncFieldInputs() {
    const f = fieldSel.value;
    queryInput.hidden = !(f && f !== "category" && f !== "status");
    categorySel.hidden = f !== "category";
    statusSel.hidden = f !== "status";
  }
  fieldSel.addEventListener("change", syncFieldInputs);
  syncFieldInputs();

  async function runSearch() {
    const field = fieldSel.value;
    const sort = document.getElementById("sort-field").value;
    let items;
    if (!field) {
      items = await api("GET", "/api/items" + (sort ? `?sort=${sort}` : ""));
    } else {
      const q = field === "category" ? categorySel.value : field === "status" ? statusSel.value : queryInput.value.trim();
      const params = new URLSearchParams({ field, q });
      if (sort) params.set("sort", sort);
      items = await api("GET", "/api/search?" + params.toString());
    }
    const results = document.getElementById("browse-results");
    results.innerHTML = items.length
      ? `<div class="item-grid">${items.map(itemCard).join("")}</div>`
      : `<div class="empty-state">No items match that search.</div>`;
    wireItemCardClicks(user);
  }

  document.getElementById("search-go").addEventListener("click", runSearch);
  document.getElementById("sort-field").addEventListener("change", runSearch);
  queryInput.addEventListener("keydown", (e) => { if (e.key === "Enter") runSearch(); });
  runSearch();
}

// ================================================================
// VIEW: my reports (student)
// ================================================================
async function viewMyReports(user) {
  const items = await api("GET", `/api/items/user/${user.id}`);
  document.getElementById("view-body").innerHTML = items.length
    ? `<div class="item-grid">${items.map(itemCard).join("")}</div>`
    : `<div class="empty-state">You haven't filed any reports yet.</div>`;
  wireItemCardClicks(user);
}

// ================================================================
// VIEW: my claims (student)
// ================================================================
async function viewMyClaims(user) {
  const claims = await api("GET", `/api/claims/user/${user.id}`);
  if (!claims.length) {
    document.getElementById("view-body").innerHTML = `<div class="empty-state">You haven't submitted any claims. Find an item under "Search Items" and open it to submit a claim.</div>`;
    return;
  }
  const items = await Promise.all(claims.map(c => api("GET", `/api/items/${c.itemId}`).catch(() => null)));
  const rows = claims.map((c, i) => {
    const it = items[i];
    return `<tr>
      <td class="item-card-id">#${String(c.id).padStart(4, "0")}</td>
      <td>${it ? esc(it.name) : "(item removed)"}</td>
      <td>${esc(c.description)}</td>
      <td>${esc(c.date)}</td>
      <td><span class="tag tag-status ${statusClass(c.status)}">${esc(c.status)}</span></td>
    </tr>`;
  }).join("");
  document.getElementById("view-body").innerHTML = `
    <table class="data-table">
      <thead><tr><th>Claim</th><th>Item</th><th>Note</th><th>Date</th><th>Status</th></tr></thead>
      <tbody>${rows}</tbody>
    </table>`;
}

// ================================================================
// VIEW: admin statistics
// ================================================================
async function viewAdminStats() {
  const s = await api("GET", "/api/stats");
  document.getElementById("view-body").innerHTML = `
    <div class="stat-grid">
      <div class="stat-card"><span class="num">${s.totalItems}</span><span class="label">Total items on record</span></div>
      <div class="stat-card"><span class="num">${s.lostItems}</span><span class="label">Lost reports</span></div>
      <div class="stat-card"><span class="num">${s.foundItems}</span><span class="label">Found reports</span></div>
      <div class="stat-card"><span class="num">${s.claimPendingItems}</span><span class="label">Awaiting claim review</span></div>
      <div class="stat-card"><span class="num">${s.returnedItems}</span><span class="label">Returned to owner</span></div>
      <div class="stat-card"><span class="num">${s.closedItems}</span><span class="label">Closed records</span></div>
      <div class="stat-card"><span class="num">${s.totalClaims}</span><span class="label">Claims submitted</span></div>
      <div class="stat-card"><span class="num">${s.claimsPending}</span><span class="label">Claims pending</span></div>
      <div class="stat-card"><span class="num">${s.claimsApproved}</span><span class="label">Claims approved</span></div>
      <div class="stat-card"><span class="num">${s.claimsRejected}</span><span class="label">Claims rejected</span></div>
    </div>`;
}

// ================================================================
// VIEW: admin pending reports (anything still open)
// ================================================================
async function viewAdminPending(user) {
  const items = await api("GET", "/api/items");
  const open = items.filter(i => !["Returned", "Closed"].includes(i.status));
  document.getElementById("view-body").innerHTML = open.length
    ? `<div class="item-grid">${open.map(itemCard).join("")}</div>`
    : `<div class="empty-state">Nothing open &mdash; every record has been resolved.</div>`;
  wireItemCardClicks(user);
}

// ================================================================
// VIEW: admin manage claims
// ================================================================
async function viewAdminClaims() {
  const claims = await api("GET", "/api/claims");
  if (!claims.length) {
    document.getElementById("view-body").innerHTML = `<div class="empty-state">No claims have been submitted yet.</div>`;
    return;
  }
  const items = await Promise.all(claims.map(c => api("GET", `/api/items/${c.itemId}`).catch(() => null)));
  const rows = claims.map((c, i) => {
    const it = items[i];
    const isFinal = c.status === "Approved" || c.status === "Rejected";
    return `<tr data-claim-row="${c.id}">
      <td class="item-card-id">#${String(c.id).padStart(4, "0")}</td>
      <td>${it ? esc(it.name) : "(item removed)"}</td>
      <td>User #${c.userId}</td>
      <td>${esc(c.description)}</td>
      <td><span class="tag tag-status ${statusClass(c.status)}">${esc(c.status)}</span></td>
      <td class="actions">
        ${isFinal ? "&mdash;" : `
          <button class="btn btn-secondary btn-small" data-claim-action="approve" data-claim-id="${c.id}">Approve</button>
          <button class="btn btn-danger btn-small" data-claim-action="reject" data-claim-id="${c.id}">Reject</button>
          ${c.status !== "Verification Requested" ? `<button class="btn btn-ghost btn-small" data-claim-action="verify" data-claim-id="${c.id}">Request verification</button>` : ""}
        `}
      </td>
    </tr>`;
  }).join("");
  document.getElementById("view-body").innerHTML = `
    <table class="data-table">
      <thead><tr><th>Claim</th><th>Item</th><th>Filed by</th><th>Note</th><th>Status</th><th>Actions</th></tr></thead>
      <tbody>${rows}</tbody>
    </table>`;

  document.querySelectorAll("[data-claim-action]").forEach(btn => {
    btn.addEventListener("click", async () => {
      const id = btn.dataset.claimId;
      const action = btn.dataset.claimAction;
      try {
        await api("POST", `/api/claims/${id}/${action}`);
        toast(`Claim #${String(id).padStart(4, "0")} ${action === "verify" ? "sent for verification" : action + "d"}.`);
        viewAdminClaims();
      } catch (err) {
        toast(err.message, true);
      }
    });
  });
}

// ================================================================
// item detail modal + claim submission (shared)
// ================================================================
function wireItemCardClicks(user) {
  document.querySelectorAll("[data-item-id]").forEach(card => {
    card.addEventListener("click", () => openItemDetail(parseInt(card.dataset.itemId, 10), user));
  });
}

async function openItemDetail(itemId, user) {
  const item = await api("GET", `/api/items/${itemId}`);
  const isOwner = item.reportedByUserId === user.id;
  const isAdmin = user.role === "ADMIN";
  const canClaim = user.role === "STUDENT" && item.type === "FOUND" && item.status === "Found" && !isOwner;

  let matchesHtml = "";
  if (item.type === "LOST" && (isOwner || isAdmin)) {
    const matches = await api("GET", `/api/items/${item.id}/matches`);
    if (matches.length) {
      matchesHtml = `<h3 style="font-size:14px;margin:18px 0 8px;">Possible matches</h3>` +
        matches.slice(0, 5).map(m => `
          <div class="match-row">
            <div><strong>${esc(m.name)}</strong> &middot; ${esc(m.location)}
              <div class="item-card-meta">#${String(m.id).padStart(4, "0")}</div></div>
            <span class="match-score">${m.score} pts</span>
          </div>`).join("");
    }
  }

  openModal(`
    <span class="tag tag-${item.type.toLowerCase()}">${item.type}</span>
    <h2 style="margin-top:8px;">${esc(item.name)}</h2>
    <p class="item-card-id">#${String(item.id).padStart(4, "0")}</p>
    <dl class="detail-grid">
      <dt>Category</dt><dd>${esc(item.category)}</dd>
      <dt>Color</dt><dd>${esc(item.color || "&mdash;")}</dd>
      <dt>Location</dt><dd>${esc(item.location)}</dd>
      <dt>Date</dt><dd>${esc(item.date)} ${esc(item.time || "")}</dd>
      <dt>Reported by</dt><dd>${esc(item.personName)} &middot; ${esc(item.contact)}</dd>
      <dt>Status</dt><dd><span class="tag tag-status ${statusClass(item.status)}">${esc(item.status)}</span></dd>
      <dt>Description</dt><dd>${esc(item.description || "&mdash;")}</dd>
    </dl>
    ${matchesHtml}
    <div class="modal-actions">
      ${canClaim ? `<button class="btn btn-primary" id="claim-btn">Submit a claim</button>` : ""}
      ${isAdmin && item.status !== "Returned" ? `<button class="btn btn-secondary" id="return-btn">Mark returned</button>` : ""}
      ${isAdmin ? `<button class="btn btn-danger" id="delete-btn">Delete record</button>` : ""}
      <button class="btn btn-ghost" id="close-btn">Close</button>
    </div>
  `);

  document.getElementById("close-btn").addEventListener("click", closeModal);

  const claimBtn = document.getElementById("claim-btn");
  if (claimBtn) claimBtn.addEventListener("click", () => openClaimForm(item, user));

  const returnBtn = document.getElementById("return-btn");
  if (returnBtn) returnBtn.addEventListener("click", async () => {
    try {
      await api("POST", `/api/items/${item.id}/return`);
      toast("Item marked as returned.");
      closeModal();
      renderApp();
    } catch (err) { toast(err.message, true); }
  });

  const deleteBtn = document.getElementById("delete-btn");
  if (deleteBtn) deleteBtn.addEventListener("click", async () => {
    if (!confirm("Delete this record permanently?")) return;
    try {
      await api("DELETE", `/api/items/${item.id}`);
      toast("Record deleted.");
      closeModal();
      renderApp();
    } catch (err) { toast(err.message, true); }
  });
}

function openClaimForm(item, user) {
  openModal(`
    <h2>Submit a claim</h2>
    <p class="panel-sub">Explain how you can identify <strong>${esc(item.name)}</strong> (#${String(item.id).padStart(4, "0")}).
      An admin will review this before it's marked returned.</p>
    <form id="claim-form">
      <label class="field"><span>Proof / description</span>
        <textarea name="description" required placeholder="e.g. Serial number, contents, a distinguishing mark&hellip;"></textarea></label>
      <p class="form-error" id="claim-error" hidden></p>
      <div class="modal-actions">
        <button type="submit" class="btn btn-primary">Submit claim</button>
        <button type="button" class="btn btn-ghost" id="claim-cancel">Cancel</button>
      </div>
    </form>
  `);
  document.getElementById("claim-cancel").addEventListener("click", closeModal);
  document.getElementById("claim-form").addEventListener("submit", async (e) => {
    e.preventDefault();
    const errorEl = document.getElementById("claim-error");
    errorEl.hidden = true;
    try {
      await api("POST", "/api/claims", {
        itemId: item.id, userId: user.id,
        description: e.target.description.value.trim(),
        date: todayISO()
      });
      toast("Claim submitted for review.");
      closeModal();
      setView("my-claims");
    } catch (err) {
      errorEl.textContent = err.message;
      errorEl.hidden = false;
    }
  });
}

boot();
