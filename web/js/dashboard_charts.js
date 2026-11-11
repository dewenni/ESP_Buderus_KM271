// Browser-only, bounded histories: [timestamp, first temperature, second temperature]
// or [timestamp, status]. null marks an unobserved interval.
class DashboardHistory {
  constructor(columns = 3, capacity = 120) {
    this.columns = columns;
    this.capacity = capacity;
    this.rows = [];
  }
  prune(now) {
    const cutoff = now - 3600000;
    const baseline = this.columns === 2 ? this.rows.findLast(row => row[0] < cutoff) : null;
    this.rows = this.rows.filter(row => row[0] >= cutoff && row[0] <= now);
    // Keep the known state at the left edge even if it has not changed for an hour.
    if (baseline) this.rows.unshift([cutoff, baseline[1]]);
    this.rows = this.rows.slice(-this.capacity);
  }
  push(now, ...values) {
    this.prune(now);
    this.rows.push([now, ...values]);
    if (this.rows.length > this.capacity) this.rows.shift();
  }
  gap(now) {
    if (this.rows.length && this.rows.at(-1)[1] !== null) this.push(now, ...Array(this.columns - 1).fill(null));
  }
  restore(rows, now) {
    if (!Array.isArray(rows)) return;
    let previous = -Infinity;
    this.rows = rows.slice(-this.capacity).filter(row => {
      if (!Array.isArray(row) || row.length !== this.columns || !Number.isFinite(row[0]) || row[0] <= previous ||
          !row.slice(1).every(value => value === null || Number.isFinite(value)) ||
          (this.columns === 2 && ![null, 0, 1].includes(row[1])) ||
          (this.columns === 3 && !(row[1] === null && row[2] === null) &&
           !row.slice(1).every(v => Number.isFinite(v) && v >= -100 && v <= 300))) return false;
      previous = row[0];
      return true;
    });
    this.rows = this.rows.filter(row => row[0] >= now - 3600000 && row[0] <= now);
    this.prune(now);
    // Preserve unknown intervals in storage and readouts, even when lines bridge them visually.
    if (this.rows.length) this.gap(this.rows.at(-1)[0] + 1);
  }
}

// Separate measured paths from visual bridges; never add interpolated samples.
function dashboardChartPaths(rows, column, step, x, y) {
  let line = "", gaps = "", previous = null, missing = false;
  const point = row => `${x(row[0]).toFixed(2)},${y(row[column]).toFixed(2)}`;
  for (const row of rows) {
    if (row[column] === null) { missing = true; continue; }
    const gap = previous && (missing || row[0] - previous[0] > 45000);
    if (gap) gaps += `M${point(previous)}L${point(row)}`;
    line += !previous || gap ? `M${point(row)}` : step ? `H${x(row[0]).toFixed(2)}V${y(row[column]).toFixed(2)}` : `L${point(row)}`;
    previous = row;
    missing = false;
  }
  return { line, gaps };
}

// Read real samples only. Every interval touching a gap remains unknown.
function dashboardChartSample(rows, time, sampleMs = 30000) {
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
  const sampleMs = 30000, timeoutMs = 15000, storageKey = "km271.dashboard.v1";
  const language = () => get("cfg_lang").value === "1" ? "en" : "de";
  const text = key => user_translations[key][language()];
  const number = element => element.textContent.trim() === "" ? NaN : Number(element.textContent.trim());
  const connected = () => getComputedStyle(lost).display === "none";
  const histories = {};
  let stored = {}, mask = 0, lastStatus = 0;
  try { stored = JSON.parse(sessionStorage.getItem(storageKey)) || {}; } catch (_) { /* Storage may be blocked. */ }
  function history(key, columns = 3, capacity = 120) {
    const series = new DashboardHistory(columns, capacity);
    series.restore(stored[key], Date.now());
    histories[key] = series;
    return series;
  }
  const states = Object.fromEntries(["burner", "priority"].map((name, i) => [name, {
    element: get(`p01_chart_${name}`), label: () => text(i === 0 ? "chart_burner" : "chart_priority"), bit: 6 + i, valid: false, observed: 0, history: history(name, 2, 256)
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
      history: history(card.dataset.chart), primary: Number(card.dataset.chartPrimary ?? 1), state, legend, valid: false };
  });
  function save() {
    try {
      sessionStorage.setItem(storageKey, JSON.stringify(Object.fromEntries(Object.entries(histories).map(([key, h]) => {
        h.prune(Date.now()); return [key, h.rows];
      }))));
    } catch (_) { /* Charts continue in RAM if storage is unavailable. */ }
  }
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
      c.status.textContent = c.valid ? "" : text("chart_wait");
    };
    summary();
    if (c.state) {
      c.legend.querySelector(".dash-chart-state-name span").textContent = c.state.label();
      c.legend.querySelector(".dash-chart-key-on").textContent = text("on");
      c.legend.querySelector(".dash-chart-key-off").textContent = text("chart_off");
      c.state.history.prune(now);
      const events = c.state.history.rows;
      for (let i = 0; i < events.length; ++i) {
        const [start, state] = events[i], end = events[i + 1]?.[0] ?? (c.state.valid ? now : start);
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
  function update(sample = false) {
    const now = Date.now(), online = connected() && now - lastStatus <= timeoutMs;
    let changed = false;
    for (const state of Object.values(states)) {
      const value = number(state.element), valid = online && !!(mask & (1 << state.bit)) && [0, 1].includes(value);
      if (state.valid && now - state.observed > sampleMs * 1.5) {
        state.history.gap(state.observed + 1); state.valid = false; changed = true;
      }
      if (valid && (!state.valid || value !== state.history.rows.at(-1)?.[1])) {
        state.history.push(now, value); changed = true;
      } else if (state.valid && !valid) { state.history.gap(Math.min(now, state.observed + timeoutMs)); changed = true; }
      state.valid = valid;
      if (valid) state.observed = now;
    }
    for (const c of charts) {
      const values = c.fields.map(number), valid = online && !!(mask & (1 << c.bit)) && values.every(Number.isFinite);
      if (valid && (sample || !c.valid)) {
        const last = c.history.rows.at(-1);
        if (last && now - last[0] > sampleMs * 1.5) c.history.gap(last[0] + 1);
        c.history.push(now, ...values); changed = true;
      } else if (c.valid && !valid) { c.history.gap(now); changed = true; }
      c.valid = valid;
    }
    if (changed) save();
    // Heartbeats only redraw on validity/status changes; sampling uses one shared timer.
    if (sample || changed) charts.forEach(draw);
  }
  const observer = new MutationObserver(mutations => {
    if (mutations.some(m => m.target === ready || ready.contains(m.target))) {
      mask = number(ready) || 0; lastStatus = Date.now();
    }
    // Once disconnected, a fresh readiness snapshot is required before rearming.
    if (!connected()) mask = 0;
    update();
  });
  for (const el of [ready, ...Object.values(states).map(s => s.element), ...charts.flatMap(c => c.fields)])
    observer.observe(el, { childList: true, characterData: true, subtree: true });
  observer.observe(lost, { attributes: true, attributeFilter: ["style", "class"] });
  const resize = new ResizeObserver(() => charts.forEach(draw));
  charts.forEach(c => resize.observe(c.chart));
  new MutationObserver(() => charts.forEach(draw)).observe(tab, { attributes: true, attributeFilter: ["class"] });
  const localeObserver = new MutationObserver(() => charts.forEach(draw));
  charts.forEach(c => localeObserver.observe(c.card.querySelector(".dash-header"), { childList: true, characterData: true, subtree: true }));
  get("cfg_lang").addEventListener("change", () => charts.forEach(draw));
  document.addEventListener("visibilitychange", () => { if (!document.hidden) update(true); });
  window.addEventListener("pagehide", () => { for (const h of Object.values(histories)) h.gap(Date.now()); save(); });
  setInterval(() => update(true), sampleMs);
  charts.forEach(draw); // Axes (and restored samples) are visible before the first live sample.
}
