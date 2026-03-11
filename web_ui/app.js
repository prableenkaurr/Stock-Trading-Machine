// File: app.js
// Author: Margarita Schemel
// This script powers the browser-based UI for the Stock Trading Machine.
// It calls the C++ HTTP API to place and cancel orders, fetch the current
// order book, show aggregate order counts per ticker, and stream trades.

const $ = (id) => document.getElementById(id);

// Updates the small status pill in the header to show whether the
// frontend is currently connected to the backend.
function setStatus(ok, text) {
  const el = $("status");
  el.textContent = text;
  el.classList.remove("ok", "bad");
  el.classList.add(ok ? "ok" : "bad");
}

// Thin wrapper around fetch() that always returns parsed JSON and throws
// on non-2xx responses with a friendly error message.
async function api(path, opts = {}) {
  const res = await fetch(path, {
    headers: { "Content-Type": "application/json" },
    ...opts,
  });
  const text = await res.text();
  let json;
  try {
    json = text ? JSON.parse(text) : {};
  } catch {
    json = { error: text || "Invalid response" };
  }
  if (!res.ok) {
    const msg = json?.error || `HTTP ${res.status}`;
    throw new Error(msg);
  }
  return json;
}

// Renders an array of {price, qty} levels as aligned text rows for the
// monospaced order book view.
function fmtLevels(levels) {
  if (!levels || levels.length === 0) return "(empty)\n";
  return levels
    .map((x) => `${x.qty.toString().padStart(6)} @ $${x.price.toFixed(2)}`)
    .join("\n");
}

// Queries the backend for all trades recorded so far and prints them in
// a compact, log-style format inside the Trades view.
async function refreshTrades() {
  const out = $("tradesView");
  try {
    const data = await api("/api/trades");
    const trades = data.trades || [];
    if (trades.length === 0) {
      out.textContent = "(no trades)\n";
      return;
    }
    out.textContent = trades
      .map(
        (t) =>
          `BUY#${t.buyOrderId} SELL#${t.sellOrderId} ${t.ticker} Qty:${t.quantity} @ $${t.price.toFixed(
            2
          )}`
      )
      .join("\n");
  } catch (e) {
    out.textContent = `Error: ${e.message}\n`;
  }
}

// Fetches aggregate bid/ask counts per ticker so the UI can show how
// many resting orders exist across all order books.
async function refreshStats() {
  const out = $("statsView");
  try {
    const data = await api("/api/stats");
    const tickers = data.tickers || [];
    if (tickers.length === 0) {
      out.textContent = "(no active order books)\n";
      return;
    }
    const rows = tickers
      .sort((a, b) => a.ticker.localeCompare(b.ticker))
      .map((t) => `${t.ticker.padEnd(8)} Bids: ${String(t.bids).padStart(5)}  Asks: ${String(t.asks).padStart(5)}`)
      .join("\n");
    out.textContent =
      rows +
      `\n${"─".repeat(32)}\n` +
      `${"TOTAL".padEnd(8)} Bids: ${String(data.totalBids).padStart(5)}  Asks: ${String(data.totalAsks).padStart(5)}\n`;
  } catch (e) {
    out.textContent = `Error: ${e.message}\n`;
  }
}

// Fetches a snapshot of the top N price levels for the requested ticker
// so we can render the current state of that order book.
async function refreshBook() {
  const out = $("bookView");
  const ticker = $("bookTicker").value.trim();
  const levels = parseInt($("bookLevels").value || "5", 10);
  if (!ticker) {
    out.textContent = "Enter a ticker.\n";
    return;
  }
  try {
    const data = await api(`/api/book?ticker=${encodeURIComponent(ticker)}&levels=${levels}`);
    const asks = data.asks || [];
    const bids = data.bids || [];
    const spread = data.spread === null ? "(n/a)" : `$${Number(data.spread).toFixed(2)}`;
    out.textContent =
      `Order Book: ${data.ticker}\n` +
      `\nASKS (best first)\n${fmtLevels(asks)}\n` +
      `\nSPREAD: ${spread}\n` +
      `\nBIDS (best first)\n${fmtLevels(bids)}\n`;
  } catch (e) {
    out.textContent = `Error: ${e.message}\n`;
  }
}

// Wires up form submissions and button clicks to the HTTP API. All DOM
// reads/writes live here so the rest of the code can stay logic-focused.
function wireForms() {
  const typeSel = $("orderType");
  const priceRow = $("priceRow");
  function updatePriceVisibility() {
    const isMarket = typeSel.value === "market";
    priceRow.style.display = isMarket ? "none" : "grid";
  }
  typeSel.addEventListener("change", updatePriceVisibility);
  updatePriceVisibility();

  $("placeForm").addEventListener("submit", async (ev) => {
    ev.preventDefault();
    const fd = new FormData(ev.target);
    const payload = {
      ticker: String(fd.get("ticker") || "").trim(),
      side: String(fd.get("side") || "").trim(),
      type: String(fd.get("type") || "").trim(),
      quantity: Number(fd.get("quantity") || 0),
    };
    const price = fd.get("price");
    if (payload.type === "limit") payload.price = parseInt(price || "0", 10);

    const out = $("placeResult");
    out.textContent = "Submitting…";
    try {
      const data = await api("/api/orders", { method: "POST", body: JSON.stringify(payload) });
      const trades = data.trades || [];
      out.textContent =
        `Order placed. ID: ${data.orderId}\n` +
        (trades.length ? `New trades: ${trades.length}\n` : "New trades: 0\n");
      await refreshTrades();
      // if user is viewing this ticker, refresh book too
      if ($("bookTicker").value.trim().toUpperCase() === payload.ticker.toUpperCase()) {
        await refreshBook();
      }
    } catch (e) {
      out.innerHTML = `<span class="err">Error: ${e.message}</span>`;
    }
  });

  $("cancelForm").addEventListener("submit", async (ev) => {
    ev.preventDefault();
    const fd = new FormData(ev.target);
    const payload = { orderId: Number(fd.get("orderId") || 0) };
    const out = $("cancelResult");
    out.textContent = "Submitting…";
    try {
      const data = await api("/api/cancel", { method: "POST", body: JSON.stringify(payload) });
      out.textContent = data.cancelled ? "Cancelled." : "Order not found / not cancellable.";
      await refreshBook();
    } catch (e) {
      out.innerHTML = `<span class="err">Error: ${e.message}</span>`;
    }
  });

  $("refreshBook").addEventListener("click", refreshBook);
  $("refreshTrades").addEventListener("click", refreshTrades);
  $("refreshStats").addEventListener("click", refreshStats);
}

// Entry point called once on page load. It wires the UI, checks the
// health endpoint, performs an initial data load and starts the timer
// that keeps trades and stats fresh.
async function init() {
  wireForms();
  try {
    await api("/api/health");
    setStatus(true, "Connected");
  } catch (e) {
    setStatus(false, `Backend unavailable: ${e.message}`);
  }
  await refreshTrades();
  await refreshStats();

  let timer = null;
  const auto = $("autoTrades");
  function updateTimer() {
    if (timer) clearInterval(timer);
    timer = null;
    if (auto.checked) timer = setInterval(() => { refreshTrades(); refreshStats(); }, 1500);
  }
  auto.addEventListener("change", updateTimer);
  updateTimer();
}

init();

