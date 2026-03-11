const $ = (id) => document.getElementById(id);

function setStatus(ok, text) {
  const el = $("status");
  el.textContent = text;
  el.classList.remove("ok", "bad");
  el.classList.add(ok ? "ok" : "bad");
}

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

function fmtLevels(levels) {
  if (!levels || levels.length === 0) return "(empty)\n";
  return levels
    .map((x) => `${x.qty.toString().padStart(6)} @ $${x.price.toFixed(2)}`)
    .join("\n");
}

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
    if (payload.type === "limit") payload.price = Number(price || 0);

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
}

async function init() {
  wireForms();
  try {
    await api("/api/health");
    setStatus(true, "Connected");
  } catch (e) {
    setStatus(false, `Backend unavailable: ${e.message}`);
  }
  await refreshTrades();

  let timer = null;
  const auto = $("autoTrades");
  function updateTimer() {
    if (timer) clearInterval(timer);
    timer = null;
    if (auto.checked) timer = setInterval(refreshTrades, 1500);
  }
  auto.addEventListener("change", updateTimer);
  updateTimer();
}

init();

