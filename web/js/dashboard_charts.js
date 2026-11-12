// Temperatures and states are sampled by the ESP, once per minute.
// Browser timestamps are derived from ESP uptime, so NTP corrections on the ESP
// cannot move historical points. No sessionStorage or browser-side sampling.
const DASHBOARD_SAMPLE_MS = 60000;

class DashboardHistory {
  constructor() { this.rows = []; }
  prune(now) {
    const cutoff = now - 3600000;
    this.rows = this.rows.filter(row => row[0] >= cutoff && row[0] <= now);
  }
}

// Validate a page before touching the displayed history.
function dashboardHistoryRows(message, offset) {
  if (!Array.isArray(message.rows) || message.rows.length > 4 || !Number.isInteger(message.now)) return null;
  let previous = -Infinity;
  for (const row of message.rows) {
    if (!Array.isArray(row) || row.length !== 15 || !row.every(Number.isInteger) ||
        row[0] <= previous || row[0] > message.now || message.now - row[0] > 3600 ||
        row[1] < 0 || row[1] > 255 || row[2] < 0 || row[2] > 3 ||
        row.slice(3).some(v => v < -32768 || v > 32767)) return null;
    previous = row[0];
  }
  return message.rows.map(row => [offset + row[0] * 1000, ...row.slice(1)]);
}

// Bounded cubic interpolation: tangents flatten at extrema and cannot overshoot.
function dashboardSmoothPath(points) {
  if (!points.length) return "";
  const point = p => p.map(v => v.toFixed(2)).join(",");
  let path = `M${point(points[0])}`;
  if (points.length === 2) return path + `L${point(points[1])}`;
  const slopes = points.slice(1).map((p, i) => (p[1] - points[i][1]) / (p[0] - points[i][0]));
  const tangent = i => i === 0 ? slopes[0] : i === slopes.length ? slopes.at(-1) :
    slopes[i - 1] * slopes[i] > 0 ? Math.sign(slopes[i]) * Math.min(Math.abs(slopes[i - 1]), Math.abs(slopes[i])) : 0;
  for (let i = 1; i < points.length; ++i) {
    const a = points[i - 1], b = points[i], h = (b[0] - a[0]) / 3;
    path += `C${point([a[0] + h, a[1] + h * tangent(i - 1)])} ${point([b[0] - h, b[1] - h * tangent(i)])} ${point(b)}`;
  }
  return path;
}

// Separate measured paths from straight visual bridges; never add samples.
function dashboardChartPaths(rows, column, step, x, y) {
  let line = "", gaps = "", previous = null, missing = false, segment = [];
  const point = row => `${x(row[0]).toFixed(2)},${y(row[column]).toFixed(2)}`;
  const flush = () => { line += dashboardSmoothPath(segment); segment = []; };
  for (const row of rows) {
    if (row[column] === null) { missing = true; continue; }
    const gap = previous && (missing || row[0] - previous[0] > DASHBOARD_SAMPLE_MS * 1.5);
    if (gap) {
      gaps += `M${point(previous)}L${point(row)}`;
      if (!step) flush();
    }
    if (step) line += !previous || gap ? `M${point(row)}` : `H${x(row[0]).toFixed(2)}V${y(row[column]).toFixed(2)}`;
    else segment.push([x(row[0]), y(row[column])]);
    previous = row;
    missing = false;
  }
  if (!step) flush();
  return { line, gaps };
}

// Read real samples only. Every interval touching a gap remains unknown.
function dashboardChartSample(rows, time, sampleMs = DASHBOARD_SAMPLE_MS) {
  const before = rows.findLast(row => row[0] <= time);
  const after = rows.find(row => row[0] >= time);
  if (!before || before[1] === null) return null;
  if (!after) return time - before[0] <= sampleMs ? before : null;
  if (after[1] === null || after[0] - before[0] > sampleMs * 1.5) return null;
  return time - before[0] <= after[0] - time ? before : after;
}

function initDashboardCharts() {
  const get = id => document.getElementById(id);
  const ready = get("p01_chart_valid"), lost = get("connectionLostBar"), tab = get("tab1");
  const sampleMs = DASHBOARD_SAMPLE_MS, timeoutMs = 15000;
  const language = () => get("cfg_lang").value === "1" ? "en" : "de";
  const text = key => user_translations[key][language()];
  const number = element => element.textContent.trim() === "" ? NaN : Number(element.textContent.trim());
  const connected = () => typeof ws !== "undefined" && ws && ws.readyState === WebSocket.OPEN;
  let mask = 0, lastStatus = 0;
  const states = Object.fromEntries(["burner", "priority"].map((name, i) => [name, {
    label: () => text(i === 0 ? "chart_burner" : "chart_priority"), bit: 6 + i, valid: false, history: new DashboardHistory()
  }]));
  const charts = Array.from(document.querySelectorAll("[data-chart]"), card => {
    const state = states[card.dataset.chartState];
    let legend;
    if (state) {
      legend = document.createElement("div");
      legend.className = "dash-chart-legend";
      legend.innerHTML = `<span class="dash-chart-state-name"><i class="svg ${card.dataset.chartState === "burner" ? "i_flame" : "i_water"}" aria-hidden="true"></i><span></span></span><span class="dash-chart-key-on"></span><span class="dash-chart-key-off"></span>`;
      card.querySelector(".dash-history-plot").prepend(legend);
    }
    return { card, chart: card.querySelector(".dash-chart"), status: card.querySelector(".dash-chart-status"),
      fields: card.dataset.chart.split(",").map(get), bit: Number(card.dataset.chartBit),
      history: new DashboardHistory(), primary: Number(card.dataset.chartPrimary ?? 1), state, legend };
  });
  const ns = "http://www.w3.org/2000/svg";
  function node(tag, attrs, content) {
    const element = document.createElementNS(ns, tag);
    for (const [key, value] of Object.entries(attrs)) element.setAttribute(key, value);
    if (content !== undefined) element.textContent = content;
    return element;
  }
  function draw(c) {
    if (document.hidden || !tab.classList.contains("active") || !c.card.getClientRects().length) return;
    const now = Date.now();
    c.history.prune(now);
    const rows = c.history.rows, values = rows.flatMap(row => row.slice(1)).filter(Number.isFinite);
    const width = Math.floor(c.chart.clientWidth), height = Math.floor(c.chart.clientHeight);
    if (width < 90 || height < 60) return;
    const left = 29, right = width - 8, top = 9, bottom = height - 23;
    const min = values.length ? Math.floor((Math.min(...values) - 2) / 5) * 5 : 0;
    const max = values.length ? Math.max(min + 5, Math.ceil((Math.max(...values) + 2) / 5) * 5) : 100;
    const x = t => left + (t - (now - 3600000)) / 3600000 * (right - left);
    const y = v => bottom - (v - min) / (max - min) * (bottom - top);
    const time = t => new Date(t).toLocaleTimeString(language(), { hour: "2-digit", minute: "2-digit", hour12: false });
    const labels = c.fields.map(field => field.closest(".dash-reading").querySelector(".dash-reading-label").textContent.trim());
    const title = c.card.querySelector(".dash-header").textContent;
    const svg = node("svg", { viewBox: `0 0 ${width} ${height}`, role: "img", "aria-label": title });
    svg.append(node("title", {}, `${title}: ${labels.join(" / ")}`));
    const summary = () => {
      c.status.textContent = rows.some(row => row[1] !== null) ? "" : text("chart_wait");
    };
    summary();
    if (c.state) {
      c.legend.querySelector(".dash-chart-state-name span").textContent = c.state.label();
      c.legend.querySelector(".dash-chart-key-on").textContent = text("on");
      c.legend.querySelector(".dash-chart-key-off").textContent = text("chart_off");
      c.state.history.prune(now);
      const events = c.state.history.rows;
      for (let i = 0; i < events.length; ++i) {
        const [start, state] = events[i], end = events[i + 1]?.[0] ?? (c.state.valid ? Math.min(now, start + sampleMs) : start);
        if (state !== 1 || end <= start) continue;
        const rect = node("rect", { x: x(start), y: top, width: x(end) - x(start), height: bottom - top,
          class: "dash-chart-state" });
        rect.append(node("title", {}, `${c.state.label()}: ${text("on")} · ${time(start)}`));
        svg.append(rect);
      }
    }
    for (let i = 0; i <= 2; ++i) {
      const value = min + (max - min) * i / 2;
      svg.append(node("line", { x1: left, x2: right, y1: y(value), y2: y(value), class: "dash-chart-grid" }));
      svg.append(node("text", { x: left - 5, y: y(value) + 4, "text-anchor": "end" }, value));
      const t = now - 3600000 + 1800000 * i;
      svg.append(node("text", { x: x(t), y: height - 5, "text-anchor": i === 0 ? "start" : i === 2 ? "end" : "middle" }, time(t)));
    }
    for (const column of [1, 2]) {
      const step = column === 1 && c.fields[0].id.endsWith("_set");
      const paths = dashboardChartPaths(rows, column, step, x, y);
      const cls = column === c.primary + 1 ? "dash-chart-actual" : "dash-chart-target";
      svg.append(node("path", { d: paths.line, class: cls }));
      svg.append(node("path", { d: paths.gaps, class: `${cls} dash-chart-gap` }));
      const last = rows.at(-1);
      if (last && last[column] !== null) svg.append(node("circle", { cx: x(last[0]), cy: y(last[column]), r: 3, class: cls }));
    }
    const guide = node("line", { x1: 0, x2: 0, y1: top, y2: bottom, class: "dash-chart-guide", visibility: "hidden" });
    const overlay = node("rect", { x: left, y: top, width: right - left, height: bottom - top, fill: "transparent" });
    const inspect = event => {
      if (!rows.length) return;
      const bounds = svg.getBoundingClientRect();
      const t = now - 3600000 + ((event.clientX - bounds.left) / bounds.width * width - left) / (right - left) * 3600000;
      const row = dashboardChartSample(rows, t, sampleMs);
      const position = row ? x(row[0]) : Math.max(left, Math.min(right, x(t)));
      guide.setAttribute("x1", position); guide.setAttribute("x2", position); guide.setAttribute("visibility", "visible");
      c.status.textContent = !row ? `${time(t)} · ${text("chart_gap")}` :
        `${time(row[0])} · ${labels[0]} ${row[1].toLocaleString(language())} / ${labels[1]} ${row[2].toLocaleString(language())} °C`;
    };
    overlay.addEventListener("pointermove", inspect);
    overlay.addEventListener("pointerdown", inspect);
    overlay.addEventListener("pointerleave", () => { guide.setAttribute("visibility", "hidden"); summary(); });
    svg.append(guide, overlay); c.chart.replaceChildren(svg);
  }
  let remoteRows = [], sequence = 0, boot = null, clockOffset = 0;
  let requestId = 0, pending = null, timer, lastSync = 0;
  const schedule = (callback, delay) => { clearTimeout(timer); timer = setTimeout(callback, delay); };
  const retry = () => { pending = null; schedule(() => sync(true), 5000); };
  function requestPage() {
    if (!connected() || !pending) return;
    ws.send(JSON.stringify({ elementId: "dashboard_history", value: `${pending.id}:${pending.cursor}:${pending.end}` }));
    schedule(retry, 10000); // A full queue/lost response must not leave a half-loaded chart.
  }
  function sync(full = false) {
    if (!connected() || (pending && !full)) return;
    pending = { id: ++requestId, cursor: full ? 0 : sequence + 1, end: 0 };
    requestPage();
  }
  function applyHistory() {
    for (const c of charts) {
      c.history.rows = remoteRows.map(row => [row[0], ...(row[1] & (1 << c.bit) ?
        row.slice(3 + 2 * c.bit, 5 + 2 * c.bit).map(v => v / 10) : [null, null])]);
    }
    for (const [i, state] of Object.values(states).entries()) {
      state.history.rows = remoteRows.map(row => [row[0], row[1] & (1 << state.bit) ? (row[2] >> i) & 1 : null]);
      // Never extend an overlay across an interval the ESP did not sample.
      state.history.rows = state.history.rows.flatMap((row, i, rows) =>
        i && row[0] - rows[i - 1][0] > sampleMs * 1.5 ? [[rows[i - 1][0] + sampleMs, null], row] : [row]);
    }
    charts.forEach(draw);
  }
  window.addEventListener("webui:message", event => {
    const data = event.detail;
    if (data.type !== "dashboardHistory" || !pending || data.request !== pending.id) return;
    if (![data.boot, data.end, data.next, data.nextIn].every(v => Number.isInteger(v) && v >= 0) ||
        typeof data.reset !== "boolean" || data.nextIn > 60 || (data.next && data.next > data.end)) { retry(); return; }
    if (boot !== null && boot !== data.boot && !data.reset) { sync(true); return; }
    const offset = data.reset ? Date.now() - data.now * 1000 : clockOffset;
    const rows = dashboardHistoryRows(data, offset);
    if (!rows || (!data.reset && rows.length && remoteRows.length && rows[0][0] <= remoteRows.at(-1)[0])) { retry(); return; }
    clearTimeout(timer);
    if (data.reset) { remoteRows = []; clockOffset = offset; }
    boot = data.boot;
    remoteRows.push(...rows);
    remoteRows = remoteRows.filter(row => row[0] >= Date.now() - 3600000).slice(-61);
    applyHistory();
    if (data.next) {
      pending.cursor = data.next; pending.end = data.end; requestPage();
    } else {
      sequence = data.end; pending = null; lastSync = Date.now();
      schedule(() => sync(), Math.max(1000, data.nextIn * 1000 + 250));
    }
  });
  window.addEventListener("webui:connected", () => { mask = 0; sync(true); });
  function updateLive() {
    const online = connected() && Date.now() - lastStatus <= timeoutMs;
    let changed = false;
    for (const state of Object.values(states)) {
      const valid = online && !!(mask & (1 << state.bit));
      changed ||= state.valid !== valid;
      state.valid = valid;
    }
    if (changed) charts.forEach(draw);
  }
  const observer = new MutationObserver(mutations => {
    if (mutations.some(m => m.target === ready || ready.contains(m.target))) {
      mask = number(ready) || 0; lastStatus = Date.now();
    }
    if (!connected()) { mask = 0; pending = null; clearTimeout(timer); }
    updateLive();
  });
  observer.observe(ready, { childList: true, characterData: true, subtree: true });
  observer.observe(lost, { attributes: true, attributeFilter: ["style", "class"] });
  const resize = new ResizeObserver(() => charts.forEach(draw));
  charts.forEach(c => resize.observe(c.chart));
  new MutationObserver(() => charts.forEach(draw)).observe(tab, { attributes: true, attributeFilter: ["class"] });
  const localeObserver = new MutationObserver(() => charts.forEach(draw));
  charts.forEach(c => localeObserver.observe(c.card.querySelector(".dash-header"), { childList: true, characterData: true, subtree: true }));
  get("cfg_lang").addEventListener("change", () => charts.forEach(draw));
  document.addEventListener("visibilitychange", () => {
    if (!document.hidden) {
      if (Date.now() - lastSync > sampleMs * 1.5) sync(true);
      updateLive(); charts.forEach(draw);
    }
  });
  setInterval(() => { updateLive(); charts.forEach(draw); }, sampleMs);
  charts.forEach(draw); // Axes are visible immediately; the first page fills the history.
  sync(true); // The socket may already be open when the page initializes.
}
