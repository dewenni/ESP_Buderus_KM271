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

// --------------------------------------
// --------------------------------------
document.addEventListener("DOMContentLoaded", function () {
  // call user functions
  initPageHeader();
  initMobileNavigation();
  initDashboardStatusIcons();
  initControlSliders();
  initControlHelp();
  initInformationPages();
  initLogType();
  initDashboardCharts();
  initWeeklyPrograms();
  initMqttConnectionStatus();
  initHomeAssistantSettings();
  initWiFiApDialog();
  initWiFiSsidDialog();
  loadSimulatedData();

  // ETH: open dialog to show w5500 configuration
  document
    .getElementById("p12_eth_open_dialog_btn")
    .addEventListener("click", function () {
      document.getElementById("p12_eth_dialog").showModal();
    });

  // ETH: close dialog to show w5500 configuration
  document
    .getElementById("p12_eth_close_dialog_btn")
    .addEventListener("click", function () {
      document.getElementById("p12_eth_dialog").close();
    });

  // GPIO: open dialog to show GPIO configuration
  document
    .getElementById("p12_gpio_open_dialog_btn")
    .addEventListener("click", function () {
      document.getElementById("p12_gpio_dialog").showModal();
    });

  // GPIO: close dialog to show GPIO configuration
  document
    .getElementById("p12_gpio_close_dialog_btn")
    .addEventListener("click", function () {
      document.getElementById("p12_gpio_dialog").close();
    });

  // Event-Listener for pushover test-message
  document
    .getElementById("p12_pushover_test_btn")
    .addEventListener("click", function () {
      var text = document.getElementById("p12_pushover_test_msg").value;
      sendData("p12_pushover_test_msg_cmd", text);
    });

  // Event-Listener for oilcounter set value
  document
    .getElementById("p02_oilmeter_btn")
    .addEventListener("click", function () {
      var text = document.getElementById("p02_oilmeter_set").value;
      sendData("p02_oilmeter_set_cmd", text);
    });

  // refresh log when tab10 is clicked
  document.querySelectorAll('a[data-tab="tab10"]').forEach(function (el) {
    el.addEventListener("click", function (e) {
      e.preventDefault();
      console.log("refresh log");
      sendData("refresh_log", "");
    });
  });
});

// GitHub OTA reuses the existing commands and WebSocket targets.
function compareReleaseVersions(release, installed) {
  const parse = value => /^v?(\d+)\.(\d+)\.(\d+)$/.exec(value.trim());
  const a = parse(release), b = parse(installed);
  if (!a || !b) return null;
  for (let i = 1; i <= 3; i++) {
    if (Number(a[i]) !== Number(b[i])) return Number(a[i]) > Number(b[i]) ? 1 : -1;
  }
  return 0;
}
document.addEventListener('DOMContentLoaded', () => {
  const state = document.getElementById('p11_git_state');
  const progress = document.getElementById('p11_git_progress');
  const release = document.getElementById('p00_dialog_git_version');
  const installed = document.getElementById('p00_dialog_version');
  const check = document.getElementById('p11_check_git_btn');
  const install = document.getElementById('p00_update_btn');
  const action = document.getElementById('p00_update_btn_hide');
  const bar = document.getElementById('github_progress_bar');
  const feedback = document.getElementById('p11_git_feedback');
  const t = key => user_translations[key][document.getElementById('cfg_lang').value === '1' ? 'en' : 'de'];
  const messages = {
    idle: user_translations.info_git_idle,
    checking: user_translations.info_git_checking,
    ready: user_translations.info_git_ready,
    current: user_translations.info_git_current,
    error: user_translations.info_git_error,
    noasset: user_translations.info_git_noasset,
    updating: user_translations.info_git_updating,
    done: user_translations.info_git_done,
    failed: user_translations.info_git_failed,
    offline: user_translations.info_git_offline,
  };
  const render = () => {
    const mode = state.textContent.trim();
    const order = compareReleaseVersions(release.textContent, installed.textContent);
    const busy = mode === 'checking' || mode === 'updating';
    check.disabled = busy || mode === 'done';
    check.setAttribute('aria-busy', mode === 'checking' ? 'true' : 'false');
    action.style.display = mode === 'ready' ? '' : 'none';
    install.disabled = mode !== 'ready';
    install.textContent = `${t(order === -1 ? 'info_git_downgrade' : 'info_git_install')} ${release.textContent}`;
    let message = messages[mode] || messages.idle;
    if (mode === 'ready') message = order === -1 ? user_translations.info_git_older : order === 1 ? user_translations.info_git_newer : messages.ready;
    feedback.textContent = message[document.getElementById('cfg_lang').value === '1' ? 'en' : 'de'] + (mode === 'updating' ? `: ${progress.textContent}%` : '');
    bar.hidden = mode !== 'updating' && mode !== 'done';
    bar.value = Number(progress.textContent) || 0;
  };
  const observer = new MutationObserver(render);
  [state, progress, release, installed].forEach(element => observer.observe(element, { childList: true, characterData: true, subtree: true }));
  check.addEventListener('click', () => {
    state.textContent = 'checking';
    release.textContent = '—';
    release.removeAttribute('href');
    progress.textContent = '0';
    render();
    sendData('check_git_version', '');
  });
  install.addEventListener('click', () => {
    if (state.textContent.trim() !== 'ready') return;
    const older = compareReleaseVersions(release.textContent, installed.textContent) === -1;
    if (!window.confirm(`${release.textContent}\n\n${t(older ? 'info_git_confirm_older' : 'info_git_confirm')}`)) return;
    state.textContent = 'updating';
    progress.textContent = '0';
    render();
    sendData('p00_update_btn', true);
  });
  document.getElementById('cfg_lang').addEventListener('change', render);
  window.addEventListener('webui:connected', () => { state.textContent = 'idle'; release.textContent = '—'; release.removeAttribute('href'); render(); });
  window.addEventListener('webui:disconnected', () => { state.textContent = 'offline'; render(); });
  document.getElementById('p00_version').addEventListener('click', () => {
    document.querySelector('.nav-list a[data-tab="tab11"]').click();
    document.getElementById('github-update-info').scrollIntoView({ block: 'center', behavior: 'smooth' });
    check.focus({ preventScroll: true });
  });
  render();
});

// Presentation only: reuse the existing received values; no polling or commands.
function initInformationPages() {
  const updates = new Map();
  const observer = new MutationObserver(mutations => {
    const changed = new Set();
    mutations.forEach(mutation => {
      let target = mutation.target.nodeType === 3 ? mutation.target.parentElement : mutation.target;
      while (target && !updates.has(target)) target = target.parentElement;
      if (target) changed.add(updates.get(target));
    });
    changed.forEach(update => update());
  });
  const watch = (source, update) => {
    const previous = updates.get(source);
    updates.set(source, previous ? () => { previous(); update(); } : update);
    observer.observe(source, { childList: true, characterData: true, subtree: true });
    update();
  };
  document.querySelectorAll('[data-info-state]').forEach(cell => {
    const enabled = cell.dataset.infoEnabled && document.getElementById(cell.dataset.infoEnabled);
    const disabledSource = cell.dataset.infoDisabledSource && document.getElementById(cell.dataset.infoDisabledSource);
    const update = () => {
      const value = cell.textContent.trim().toLowerCase();
      const type = cell.dataset.infoState;
      const connected = /^(verbunden|connected)$/.test(value);
      const disabled = (enabled && !enabled.checked) || (disabledSource && /^(inaktiv|inactive)$/.test(disabledSource.textContent.trim().toLowerCase()));
      cell.classList.toggle('is-active', type === 'number' ? value !== '' && Number(value) > 0 : type === 'on' && /^(ein|on)$/.test(value));
      cell.classList.toggle('is-error', (type === 'error' && /^(fehler|error)$/.test(value)) || (type === 'alarm' && value.includes('>>') && !value.includes('<<')));
      cell.classList.toggle('is-connected', type === 'connection' && connected);
      cell.classList.toggle('is-disconnected', type === 'connection' && !disabled && /^(nicht verbunden|not connected)$/.test(value));
    };
    watch(cell, update);
    if (enabled) enabled.addEventListener('change', update);
    if (disabledSource) watch(disabledSource, update);
  });
  document.querySelectorAll('[data-info-meter]').forEach(meter => {
    const source = document.getElementById(meter.dataset.infoMeter);
    watch(source, () => {
      const raw = source.textContent.trim();
      const value = Number(raw.replace(',', '.'));
      meter.hidden = raw === '' || !Number.isFinite(value) || value < 0 || value > 100;
      if (!meter.hidden) meter.value = value;
    });
  });
  document.querySelectorAll('[data-selected-file]').forEach(label => {
    const input = document.getElementById(label.dataset.selectedFile);
    input.addEventListener('change', () => {
      label.hidden = !input.files.length;
      if (input.files.length) {
        label.removeAttribute('data-i18n');
        label.textContent = input.files[0].name;
      } else {
        label.dataset.i18n = 'info_no_file';
        const lang = document.getElementById('cfg_lang').value === '1' ? 'en' : 'de';
        label.textContent = user_translations.info_no_file[lang];
      }
      // The existing file-selection handlers enable submission; keep empty selections disabled.
      document.getElementById(input.id === 'ota_file_input' ? 'ota_update_btn' : 'file_upload_btn').disabled = !input.files.length;
      if (!input.files.length) {
        document.getElementById(input.id === 'ota_file_input' ? 'ota_status_txt' : 'upload_status_txt').style.display = 'none';
        if (input.id === 'ota_file_input') document.getElementById('ota_progress_bar').style.display = 'none';
      }
    });
  });
}

// --------------------------------------
// localization texts
// --------------------------------------
const user_translations = {
  info_git_installed: {"de": "Installiert:", "en": "Installed:"},
  info_git_release: {"de": "Letztes Release:", "en": "Latest release:"},
  info_git_idle: {"de": "Prüfe, welche Version auf GitHub verfügbar ist.", "en": "Check which version is available on GitHub."},
  info_git_checking: {"de": "Version wird geprüft …", "en": "Checking version …"},
  info_git_ready: {"de": "Eine andere Version ist verfügbar.", "en": "A different version is available."},
  info_git_newer: {"de": "Eine neuere Version ist verfügbar.", "en": "A newer version is available."},
  info_git_older: {"de": "Das Release ist älter als die installierte Version.", "en": "This release is older than the installed version."},
  info_git_current: {"de": "Bereits aktuell.", "en": "Already up to date."},
  info_git_error: {"de": "Prüfung fehlgeschlagen. Bitte erneut versuchen.", "en": "Check failed. Please try again."},
  info_git_noasset: {"de": "Keine passende Firmware für dieses Gerät im Release gefunden.", "en": "No matching firmware for this device was found in the release."},
  info_git_install: {"de": "Version installieren", "en": "Install version"},
  info_git_downgrade: {"de": "Ältere Version installieren", "en": "Install older version"},
  info_git_confirm: {"de": "Diese Version installieren? Der ESP wird anschließend neu gestartet.", "en": "Install this version? The ESP will restart afterwards."},
  info_git_confirm_older: {"de": "Diese Version ist älter als die installierte Firmware. Trotzdem installieren?", "en": "This version is older than the installed firmware. Install anyway?"},
  info_git_updating: {"de": "Installation läuft", "en": "Installing"},
  info_git_done: {"de": "Installation erfolgreich. Neustart bestätigen.", "en": "Installation successful. Confirm restart."},
  info_git_failed: {"de": "Installation fehlgeschlagen. Bitte vor einem weiteren Versuch erneut prüfen.", "en": "Installation failed. Check again before retrying."},
  info_git_offline: {"de": "Verbindung unterbrochen. Bitte nach dem Wiederverbinden erneut prüfen.", "en": "Connection lost. Check again after reconnecting."},

  info_update_local: { de: "Lokale Datei", en: "Local file" },
  info_update_local_hint: { de: "Eine Firmware-Datei von deinem Computer auf den ESP übertragen.", en: "Upload a firmware file from your computer to the ESP." },
  info_update_github: { de: "Von GitHub", en: "From GitHub" },
  info_update_github_hint: { de: "Eine veröffentlichte Version direkt aus dem Projekt herunterladen.", en: "Download a published version directly from the project." },
  info_install_file: { de: "Datei installieren", en: "Install file" },
  ui_reload: { de: "Seite neu laden", en: "Reload page" },
  info_firmware_used: { de: "Firmware-Speicher belegt", en: "Firmware storage used" },
  info_ram_free: { de: "RAM frei", en: "Free RAM" },
  info_largest_free_block: { de: "Größter freier Speicherblock", en: "Largest free memory block" },
  info_minimum_free_ram: { de: "Niedrigster freier RAM seit Neustart", en: "Lowest free RAM since restart" },
  info_memory_used: { de: "belegt", en: "used" },

  info_log_recording: { de: "Aufzeichnung", en: "Recording" },
  info_firmware_update: { de: "Firmware-Update", en: "Firmware update" },
  info_configuration: { de: "Konfiguration", en: "Configuration" },
  info_check_github: { de: "GitHub-Version prüfen", en: "Check GitHub version" },
  info_import: { de: "Importieren", en: "Import" },
  info_export: { de: "Exportieren", en: "Export" },
  info_show: { de: "Anzeigen", en: "Show" },

  info_ethernet: { de: "Ethernet", en: "Ethernet" },
  info_current_values: { de: "Aktuelle Werte", en: "Current values" },
  info_operating_states: { de: "Betriebszustände", en: "Operating states" },
  info_monitoring: { de: "Überwachung", en: "Monitoring" },
  info_memory: { de: "Speicher", en: "Memory" },
  info_circuit_subtitle: { de: "Konfiguration und Betriebszustände", en: "Configuration and operating states" },
  info_ww_subtitle: { de: "Speicher und Warmwasserbereitung", en: "Cylinder and hot water preparation" },
  info_burner_subtitle: { de: "Kessel, Betrieb und Überwachung", en: "Boiler, operation and monitoring" },
  info_solar_subtitle: { de: "Temperaturen und Anlagenzustand", en: "Temperatures and system status" },
  info_general_subtitle: { de: "Übergreifende Einstellungen der Logamatic", en: "General Logamatic settings" },
  info_alarm_subtitle: { de: "Meldungen der Logamatic", en: "Logamatic messages" },
  info_tools_subtitle: { de: "Firmware und Konfigurationsverwaltung", en: "Firmware and configuration management" },
  info_log_subtitle: { de: "Ereignisse und Fehlersuche", en: "Events and troubleshooting" },
  info_alarm_memory: { de: "Meldespeicher", en: "Message memory" },
  info_alarm_hint: { de: "Die Meldungen können auch bereits quittierte Störungen enthalten. Ein Eintrag bedeutet nicht automatisch, dass die Störung aktuell anliegt.", en: "Messages may include previously acknowledged faults. An entry does not necessarily mean the fault is currently active." },
  info_mqtt_error_hint: { de: "Der letzte Fehler ist gespeichert und beschreibt nicht zwingend den aktuellen Verbindungszustand.", en: "The last error is stored and does not necessarily describe the current connection state." },
  info_years: { de: "Jahre", en: "years" },
  info_days: { de: "Tage", en: "days" },
  info_hours: { de: "Std.", en: "hrs" },
  info_minutes: { de: "Min.", en: "min" },
  info_firmware_file: { de: "Firmware-Datei", en: "Firmware file" },
  info_config_file: { de: "Konfigurationsdatei", en: "Configuration file" },
  info_no_file: { de: "Keine Datei ausgewählt", en: "No file selected" },
  info_log_source: { de: "Logbuch", en: "Log" },
  info_log_level: { de: "Level", en: "Level" },
  info_log_order: { de: "Reihenfolge", en: "Order" },
  info_solar_status: { de: "Status 9147", en: "Status 9147" },

  control_help: { de: "Hilfe", en: "Help" },
  control_summer_threshold: { de: "Sommergrenze", en: "Summer threshold" },
  control_frost_threshold: { de: "Frostschutz", en: "Frost protection" },
  control_design_temp: { de: "Auslegungstemperatur", en: "Design temperature" },
  control_help_hc_mode: { de: "Nacht und Tag wählen dauerhaft den jeweiligen Betrieb. Auto wechselt anhand des an der Logamatic gewählten Heizprogramms zwischen Tag- und Nachtbetrieb.", en: "Night and Day select the respective operating mode continuously. Auto switches between day and night operation according to the heating program selected on the Logamatic." },
  control_help_program: { de: "Wählt das Zeitprogramm der Logamatic für den Automatikbetrieb. „Eigen“ verwendet das individuelle Programm der Regelung; die übrigen Einträge sind deren vordefinierte Programme. Die Schaltzeiten werden hier nicht bearbeitet.", en: "Selects the Logamatic schedule for automatic operation. Custom uses the controller’s individual program; the other entries are its predefined programs. Switching times are not edited here." },
  control_help_holiday_days: { de: "Dauer des Urlaubsbetriebs: 0–99 Tage. Die Logamatic verringert den Wert täglich um einen Tag. 0 bedeutet keinen laufenden Urlaubszeitraum. Die zugehörige Temperatur wird separat eingestellt.", en: "Holiday operation duration: 0–99 days. The Logamatic decreases the value by one each day. 0 means no active holiday period. Its temperature is set separately." },
  control_help_reduction: { de: "Bestimmt die Betriebsweise während der Absenkzeiten: Abschalt, Reduziert, Raumhalt oder Aussenhalt. Raumhalt berücksichtigt die Raumtemperatur, Aussenhalt die Außentemperatur. Die tatsächliche Wirkung hängt von der Ausstattung und Konfiguration der Logamatic ab.", en: "Selects operation during setback periods: Off, Reduced, Room hold or Outdoor hold. Room hold uses room temperature; Outdoor hold uses outdoor temperature. Actual behavior depends on the Logamatic equipment and configuration." },
  control_help_day: { de: "Raum-Solltemperatur für den Tagbetrieb: 10–30 °C in Schritten von 0,5 °C. Es handelt sich nicht um die Vorlauftemperatur. Der Wert beeinflusst die Heizungsregelung auch über die Heizkennlinie.", en: "Room temperature setpoint for day operation: 10–30 °C in 0.5 °C steps. This is not the flow temperature. The value also affects heating control through the heating curve." },
  control_help_night: { de: "Raum-Solltemperatur für den Nacht- bzw. reduzierten Betrieb: 10–30 °C in Schritten von 0,5 °C. Ob und wie geheizt wird, hängt zusätzlich von der Absenkungsart ab.", en: "Room temperature setpoint for night or reduced operation: 10–30 °C in 0.5 °C steps. Heating behavior also depends on the selected reduction mode." },
  control_help_holiday_temp: { de: "Raum-Solltemperatur während des eingestellten Urlaubszeitraums: 10–30 °C in Schritten von 0,5 °C. Die Dauer wird über Urlaubstage festgelegt.", en: "Room temperature setpoint during the selected holiday period: 10–30 °C in 0.5 °C steps. Set its duration using Holiday days." },
  control_help_summer: { de: "Außentemperaturgrenze für die Sommer-/Winterumschaltung. 10–30 °C: temperaturabhängige Umschaltung. Die Randstellungen sind Sonderwerte: 9 = dauerhaft Sommer, 31 = dauerhaft Winter.", en: "Outdoor temperature threshold for summer/winter switching. 10–30 °C: temperature-dependent switching. The end positions are special values: 9 = permanent summer, 31 = permanent winter." },
  control_help_frost: { de: "Temperaturschwelle für den Frostschutz des Heizkreises: −20 bis +10 °C in Schritten von 1 °C.", en: "Temperature threshold for heating-circuit frost protection: −20 to +10 °C in 1 °C steps." },
  control_help_design: { de: "Auslegungstemperatur der Heizkennlinie: 30–90 °C in Schritten von 1 °C. Sie beschreibt die Auslegung des Vorlaufs und ist keine Raum-Solltemperatur.", en: "Heating curve design temperature: 30–90 °C in 1 °C steps. It describes the flow temperature design point, not a room temperature setpoint." },
  control_help_switch_on: { de: "Aufschalttemperatur des Heizkreises: 0–10 °C in Schritten von 1 °C. Dieser Parameter wird direkt an die Logamatic übertragen; seine genaue Wirkung richtet sich nach der verwendeten Regelung und deren Konfiguration.", en: "Heating-circuit switch-on temperature: 0–10 °C in 1 °C steps. This parameter is sent directly to the Logamatic; its exact effect depends on the controller and its configuration." },
  control_help_switch_off: { de: "Außentemperaturschwelle für die Absenkungsart Aussenhalt: −20 bis +10 °C in Schritten von 1 °C. Der Wert gehört zur außentemperaturabhängigen Absenkung.", en: "Outdoor temperature threshold for Outdoor hold reduction: −20 to +10 °C in 1 °C steps. This value belongs to outdoor-temperature-dependent setback operation." },
  control_help_ww_mode: { de: "Wählt Nacht-, Tag- oder Automatikbetrieb der Warmwasserbereitung. Auto nutzt die zeitabhängige Freigabe der Logamatic. Die genaue Freigabe ist von deren Konfiguration abhängig.", en: "Selects night, day or automatic operation for hot water. Auto uses the Logamatic’s timed enable. The exact enable behavior depends on its configuration." },
  control_help_ww_temp: { de: "Solltemperatur für das Warmwasser: 30–60 °C in Schritten von 1 °C. Die aktuelle Speichertemperatur wird separat im Dashboard angezeigt.", en: "Hot-water setpoint: 30–60 °C in 1 °C steps. The current cylinder temperature is displayed separately on the dashboard." },
  control_help_circulation: { de: "Ansteuerung der Warmwasser-Zirkulationspumpe. 0 = AUS; 1–6 = Anzahl der Zyklen pro Stunde; 7 = EIN. Die Zahlen 1–6 sind keine Temperaturwerte.", en: "Hot-water circulation pump control. 0 = OFF; 1–6 = cycles per hour; 7 = ON. Values 1–6 are not temperatures." },
  control_help_oil_feedback: { de: "„Setzen“ überträgt den eingegebenen Zählerstand. Der Istwert daneben zeigt den vom ESP bestätigten Stand in Litern; er wird erst mit dessen Rückmeldung aktualisiert.", en: "Set sends the entered meter reading. The actual value beside it shows the reading confirmed by the ESP in liters and updates only after its response." },

  control_subtitle: { de: "Betriebsarten und Temperaturen", en: "Operating modes and temperatures" },
  control_oilmeter_set: { de: "Zählerstand setzen", en: "Set meter reading" },
  summer: { de: "Sommer", en: "Summer" },
  winter: { de: "Winter", en: "Winter" },
  ha_mqtt_hint: { de: "Benötigt eine aktivierte und konfigurierte MQTT-Verbindung.", en: "Requires an enabled and configured MQTT connection." },
  ha_mqtt_disabled: { de: "MQTT ist derzeit deaktiviert.", en: "MQTT is currently disabled." },
  ha_mqtt_open: { de: "MQTT-Einstellungen öffnen", en: "Open MQTT settings" },
  ui_connected: { de: "Verbunden", en: "Connected" },
  ui_disconnected: { de: "Verbindung unterbrochen – verbinde erneut …", en: "Disconnected – reconnecting …" },
  dashboard_subtitle: { de: "Heizung im Überblick", en: "Heating at a glance" },
  system_subtitle: { de: "Verbindungen und Gerät", en: "Connections and device" },
  settings_subtitle: { de: "Konfiguration", en: "Configuration" },
  chart_target: { de: "Soll", en: "Target" },
  chart_actual: { de: "Ist", en: "Actual" },
  chart_current: { de: "Aktuell", en: "Current" },
  chart_damped: { de: "Gedämpft", en: "Damped" },
  chart_wait: { de: "Warte auf gültige Daten …", en: "Waiting for valid data …" },
  chart_off: { de: "AUS", en: "OFF" },
  chart_gap: { de: "Keine Messdaten in diesem Zeitraum", en: "No measurements in this interval" },
  chart_burner: { de: "Brenner", en: "Burner" },
  chart_priority: { de: "Warmwasservorrang", en: "Hot water priority" },
  weekly_program_hint: {
    de: "Wochenplan des manuellen Programms. Das aktive Programm wird oben angezeigt.",
    en: "Weekly schedule of the manual program. The active program is shown above.",
  },
  weekly_program_loading: { de: "Schaltzeiten werden geladen …", en: "Loading switch points …" },
  weekly_program_empty: { de: "Keine Schaltzeiten für das manuelle Programm hinterlegt.", en: "No switch points stored for the manual program." },
  weekly_program_invalid: { de: "Schaltzeiten nicht eindeutig darstellbar. Bitte die Schaltpunkte prüfen.", en: "The schedule cannot be displayed unambiguously. Please check the switch points." },
  weekly_program_details: { de: "Schaltpunkte anzeigen", en: "Show switch points" },
  weekly_program_on: { de: "An", en: "On" },
  weekly_program_off: { de: "Aus", en: "Off" },
  service_access: { de: "Service-Zugänge", en: "Service access" },
  service_password: { de: "Service-Passwort", en: "Service password" },
  service_telnet: { de: "Telnet aktivieren", en: "Enable Telnet" },
  service_hint: {
    de: "Zentrales Passwort für Service Zugänge wie z.B. Telnet und Remote-OTA (ArduinoOTA). Ohne Passwort bleiben beide gesperrt. Im Setup-Modus gilt admin / km271-2026!.",
    en: "Central password for service access such as Telnet and Remote-OTA (ArduinoOTA). Without a password, both remain locked. In setup mode, admin / km271-2026! applies.",
  },
  wifi_ssid_search: { de: "Suche WLAN", en: "Find Wi-Fi" },
  wifi_ssid_title: { de: "WLAN auswählen", en: "Select Wi-Fi network" },
  wifi_ssid_select: {
    de: "Verfügbare WLAN-Netze",
    en: "Available Wi-Fi networks",
  },
  wifi_ssid_hint: {
    de: "Versteckte WLAN-Namen können manuell eingegeben werden. Bei einem anderen WLAN-Namen wird die feste BSSID zurückgesetzt.",
    en: "Hidden Wi-Fi names can be entered manually. Selecting a different Wi-Fi name clears the fixed BSSID.",
  },
  wifi_ssid_choose: {
    de: "Bitte WLAN auswählen",
    en: "Select a Wi-Fi network",
  },
  wifi_ssid_found: {
    de: "WLAN auswählen und mit Übernehmen bestätigen.",
    en: "Select a network and confirm with Apply.",
  },
  wifi_ssid_empty: {
    de: "Keine WLAN-Netze gefunden.",
    en: "No Wi-Fi networks found.",
  },
  wifi_ap_bssid: {
    de: "Access Point (BSSID, optional)",
    en: "Access point (BSSID, optional)",
  },
  wifi_ap_search: { de: "Suche AP", en: "Find AP" },
  wifi_ap_hint: {
    de: "Leer = automatische Auswahl. Mit fester BSSID erfolgt kein Wechsel zu anderen Access Points. Gilt ab der nächsten WLAN-Verbindung oder nach einem Neustart.",
    en: "Empty = automatic selection. A fixed BSSID prevents switching to other access points. Applies on the next Wi-Fi connection or after a restart.",
  },
  wifi_ap_title: { de: "Access Point auswählen", en: "Select access point" },
  wifi_ap_select: {
    de: "Verfügbare Access Points",
    en: "Available access points",
  },
  wifi_ap_retry: { de: "Erneut suchen", en: "Scan again" },
  wifi_ap_apply: { de: "Übernehmen", en: "Apply" },
  wifi_ap_auto: { de: "Automatisch auswählen", en: "Select automatically" },
  wifi_ap_current: { de: "aktuell verbunden", en: "currently connected" },
  wifi_ap_channel: { de: "Kanal", en: "Channel" },
  wifi_ap_not_found: { de: "nicht gefunden", en: "not found" },
  wifi_ap_scanning: { de: "Suche läuft…", en: "Scanning…" },
  wifi_ap_found: {
    de: "Access Point auswählen und mit Übernehmen bestätigen.",
    en: "Select an access point and confirm with Apply.",
  },
  wifi_ap_empty: {
    de: "Keine Access Points für diesen WLAN-Namen gefunden.",
    en: "No access points found for this Wi-Fi name.",
  },
  wifi_ap_invalid_ssid: {
    de: "Bitte zuerst einen WLAN-Namen mit 1 bis 32 Byte eingeben.",
    en: "Enter a Wi-Fi name of 1 to 32 bytes first.",
  },
  wifi_ap_failed: {
    de: "Die Suche ist fehlgeschlagen. Bitte erneut versuchen.",
    en: "The scan failed. Please try again.",
  },
  wifi_ap_busy: {
    de: "Das Gerät führt bereits eine Suche oder ein Update aus. Bitte später erneut versuchen.",
    en: "The device is already scanning or updating. Please try again later.",
  },
  wifi_ap_ssid_changed: {
    de: "Der WLAN-Name wurde geändert. Bitte den Dialog schließen und erneut suchen.",
    en: "The Wi-Fi name has changed. Close this dialog and scan again.",
  },
  control: {
    de: "Bedienung",
    en: "Control",
  },
  dashboard: {
    de: "Dashboard",
    en: "Dashboard",
  },
  auto: {
    de: "Auto",
    en: "Auto",
  },
  programs: {
    de: "Programme",
    en: "Programs",
  },
  config: {
    de: "Konfiguration",
    en: "Config",
  },
  temperatures: {
    de: "Temperaturen",
    en: "Temperatures",
  },
  day: {
    de: "Tag",
    en: "Day",
  },
  night: {
    de: "Nacht",
    en: "Night",
  },
  day_night: {
    de: "Tag/Nacht",
    en: "Day/Night",
  },
  summer_winter: {
    de: "Sommer/Winter",
    en: "Summer/Winter",
  },
  hc1: {
    de: "HK1",
    en: "HC1",
  },
  hc2: {
    de: "HK2",
    en: "HC2",
  },
  act_value: {
    de: "Istwert",
    en: "Actual Value",
  },
  set_temp: {
    de: "Solltemperatur",
    en: "Set Temperature",
  },
  act_temp: {
    de: "Isttemperatur",
    en: "Actual Temperature",
  },
  opmode: {
    de: "Betriebsart",
    en: "Operation Mode",
  },
  burner: {
    de: "Brenner",
    en: "Burner",
  },
  pump: {
    de: "Umw\u00e4lzpumpe",
    en: "Flow Pump",
  },
  hot_water: {
    de: "Warmwasser",
    en: "Hot Water",
  },
  burner_temp: {
    de: "Kessel-Vorlauf",
    en: "Boiler Temperature",
  },
  flow: {
    de: "Vorlauf",
    en: "Flow Temperature",
  },
  heating_circuit_1: {
    de: "Heizkreis 1",
    en: "Heating Circuit 1",
  },
  heating_circuit_2: {
    de: "Heizkreis 2",
    en: "Heating Circuit 2",
  },
  general: {
    de: "Allgemeine Werte",
    en: "General Values",
  },
  logamatic_version: {
    de: "Logamatic-Version",
    en: "Logamatic-Version",
  },
  logamatic_modul: {
    de: "Logamatic-Modul",
    en: "Logamatic-Module",
  },
  prg: {
    de: "Programm",
    en: "Program",
  },
  hc_prg_custom: {
    de: "Eigen",
    en: "custom",
  },
  hc_prg_family: {
    de: "Familie",
    en: "family",
  },
  hc_prg_early: {
    de: "Frueh",
    en: "early",
  },
  hc_prg_late: {
    de: "Spaet",
    en: "late",
  },
  hc_prg_am: {
    de: "Vormittag",
    en: "AM",
  },
  hc_prg_pm: {
    de: "Nachmittag",
    en: "PM",
  },
  hc_prg_noon: {
    de: "Mittag",
    en: "noon",
  },
  hc_prg_single: {
    de: "Single",
    en: "single",
  },
  hc_prg_senior: {
    de: "Senior",
    en: "senior",
  },
  info_unit_c: {
    de: "\u00b0C",
    en: "\u00b0C",
  },
  oilmeter: {
    de: "\u00d6lz\u00e4hler",
    en: "Oil-Meter",
  },
  set: {
    de: "setzen",
    en: "set",
  },
  button_ntp: {
    de: "schreibe NTP-Datum/Zeit auf Logamatic",
    en: "write NTP-date/time to Logamatic",
  },
  button_dti: {
    de: "schreibe Datum/Zeit auf Logamatic",
    en: "write date/time to Logamatic",
  },
  alarm: {
    de: "Alarme",
    en: "Alarms",
  },
  alarm_info: {
    de: "Alarme + Info",
    en: "Alarms + Info",
  },
  lifetimes: {
    de: "Laufzeiten",
    en: "Runtimes",
  },
  limits: {
    de: "Grenzwerte",
    en: "Limits",
  },
  datetime: {
    de: "Datum und Uhrzeit",
    en: "Date and Time",
  },
  logamatic: {
    de: "Logamatic",
    en: "Logamatic",
  },
  act_date: {
    de: "aktuelles Datum (auf ESP)",
    en: "actual date (on ESP)",
  },
  act_time: {
    de: "aktuelle Uhrzeit (auf ESP)",
    en: "actual time (on ESP)",
  },
  ntp_auto_sync: {
    de: "schreibe ntp Zeit nach jedem PowerOn",
    en: "write ntp time on PowerOn",
  },
  set_date: {
    de: "Datum setzen",
    en: "set date",
  },
  set_time: {
    de: "Uhrzeit setzen",
    en: "set time",
  },
  gpio: {
    de: "GPIO-Zuweisung",
    en: "GPIO-Settings",
  },
  led_wifi: {
    de: "LED-WiFi",
    en: "LED-WiFi",
  },
  led_heartbeat: {
    de: "LED-Heartbeat",
    en: "LED-Heartbeat",
  },
  led_logmode: {
    de: "LED-Logmode",
    en: "LED-Logmode",
  },
  led_oilcounter: {
    de: "LED-\u00d6lz\u00e4hler",
    en: "LED-Oilcounter",
  },
  trig_oilcounter: {
    de: "Impuls-\u00d6lz\u00e4hler",
    en: "Trigger-Oilcounter",
  },
  km271_tx: {
    de: "KM271-TX",
    en: "KM271-TX",
  },
  km271_rx: {
    de: "KM271-RX",
    en: "KM271-RX",
  },
  oil_hardware: {
    de: "\u00d6lz\u00e4hler Hardware",
    en: "Oil Meter Hardware",
  },
  oil_virtual: {
    de: "\u00d6lz\u00e4hler virtuell",
    en: "Oil Meter virtual",
  },
  oil_par1_kg_h: {
    de: "Verbrauch Kg/h",
    en: "consumption Kg/h",
  },
  oil_par2_kg_l: {
    de: "\u00d6l-Dichte Kg/L",
    en: "oil density Kg/L",
  },
  pulse_per_liter: {
    de: "Impule pro Liter",
    en: "pulses per liter",
  },
  debounce_time: {
    de: "Entprellzeit (ms)",
    en: "debounce time (ms)",
  },
  virt_calc_offset: {
    de: "Berechnungskorrekturwert für virtuellen Verbrauch",
    en: "Calculation Correction Value for Virtual Consumption",
  },
  oil_unit_info_set: {
    de: "Die Eingabe erfolgt als Ganzzahl in 100stel Liter\n(12345 = 123,45 Liter)",
    en: "The input is a whole number in 100th of a liter\n(12345 = 123.45 liters)",
  },
  temp_out: {
    de: "Au\u00dfentemperatur",
    en: "Temperature outdoor",
  },
  mqtt_cfg_ret: {
    de: "Config Nachrichten als retain",
    en: "config messages as retain",
  },
  mqtt_connection_test: {
    de: "Verbindung testen",
    en: "Test connection",
  },
  optional_sensor: {
    de: "optionale Sensoren",
    en: "optional sensors",
  },
  sens1: {
    de: "Sensor 1",
    en: "Sensor 1",
  },
  sens2: {
    de: "Sensor 2",
    en: "Sensor 2",
  },
  name: {
    de: "Name",
    en: "Name",
  },
  filter: {
    de: "Filter",
    en: "Filter",
  },
  pushover: {
    de: "Pushover",
    en: "Pushover",
  },
  api_token: {
    de: "API-Token",
    en: "API-Token",
  },
  user_key: {
    de: "User-Key",
    en: "User-Key",
  },
  test_msg: {
    de: "Testnachricht",
    en: "test message",
  },
  reduct_mode_off: {
    de: "Abschalt",
    en: "off",
  },
  reduct_mode_fixed: {
    de: "Reduziert",
    en: "fixed",
  },
  reduct_mode_room: {
    de: "Raumhalt",
    en: "room",
  },
  reduct_mode_outdoors: {
    de: "Aussenhalt",
    en: "outdoors",
  },
  night_temp: {
    de: "Nachttemperatur",
    en: "night temp",
  },
  day_temp: {
    de: "Tagtemperatur",
    en: "day temp",
  },
  operation_mode: {
    de: "Betriebsart",
    en: "operation mode",
  },
  holiday_temp: {
    de: "Urlaubtemperatur",
    en: "holiday temp",
  },
  max_temp: {
    de: "Max Temperatur",
    en: "max temp",
  },
  switch_on_temperature: {
    de: "Aufschalttemperatur",
    en: "switch on temperature",
  },
  switch_off_threshold: {
    de: "Aussenhalt ab",
    en: "switch off threshold",
  },
  reduction_mode: {
    de: "Absenkungsart",
    en: "reduction mode",
  },
  heating_system: {
    de: "Heizsystem",
    en: "heating system",
  },
  temp_offset: {
    de: "Temperatur_Offset",
    en: "temp offset",
  },
  remotecontrol: {
    de: "Fernbedienung",
    en: "remote control",
  },
  holiday_days: {
    de: "Urlaubstage",
    en: "Holiday days",
  },
  time_offset: {
    de: "Uhrzeit_Offset",
    en: "time offset",
  },
  priority: {
    de: "Vorrang",
    en: "priority",
  },
  temp: {
    de: "Temperatur",
    en: "temp",
  },
  processing: {
    de: "Aufbereitung",
    en: "processing",
  },
  circulation: {
    de: "Zirkulation",
    en: "circulation",
  },
  building_type: {
    de: "Gebaeudeart",
    en: "building type",
  },
  display: {
    de: "Anzeige",
    en: "display",
  },
  burner_type: {
    de: "Brennerart",
    en: "burner type",
  },
  max_boiler_temperature: {
    de: "Max_Kesseltemperatur",
    en: "max boiler temperature",
  },
  pump_logic_temp: {
    de: "Pumplogik",
    en: "pump logic",
  },
  exhaust_gas_temperature_threshold: {
    de: "Abgastemperaturschwelle",
    en: "exhaust gas temperature threshold",
  },
  burner_min_modulation: {
    de: "Brenner_Min_Modulation",
    en: "burner min modulation",
  },
  burner_modulation_runtime: {
    de: "Brenner_Mod_Laufzeit",
    en: "burner modulation runtime",
  },
  ov1_off_time_optimization: {
    de: "Ausschaltoptimierung",
    en: "off time optimization",
  },
  ov1_on_time_optimization: {
    de: "Einschaltoptimierung",
    en: "on time optimization",
  },
  ov1_automatic: {
    de: "Automatik",
    en: "automatic",
  },
  ov1_ww_priority: {
    de: "Warmwasservorrang",
    en: "WW priority",
  },
  ov1_screed_drying: {
    de: "Estrichtrocknung",
    en: "screed drying",
  },
  ov1_holiday: {
    de: "Ferien",
    en: "holiday",
  },
  ov1_frost_protection: {
    de: "Frostschutz",
    en: "frost protection",
  },
  ov2_summer: {
    de: "Sommer",
    en: "summer",
  },
  ov2_day: {
    de: "Tag",
    en: "day",
  },
  ov2_no_conn_to_remotectrl: { de: "Keine Verbindung zur Fernbedienung", en: "No connection to remote control" },
  ov2_remotectrl_error: { de: "Fernbedienung fehlerhaft", en: "Remote control fault" },
  ov2_failure_flow_sensor: { de: "Vorlauffühler fehlerhaft", en: "Flow sensor fault" },
  ov2_flow_at_maximum: {
    de: "Maximaler Vorlauf",
    en: "flow at maximum",
  },
  ov2_external_signal_input: { de: "Externer Störeingang", en: "External fault input" },
  flow_setpoint: {
    de: "Vorlaufsolltemperatur",
    en: "Flow setpoint",
  },
  flow_temp: {
    de: "Vorlaufisttemperatur",
    en: "Flow temp",
  },
  room_setpoint: {
    de: "Raumsolltemperatur",
    en: "Room setpoint",
  },
  room_temp: {
    de: "Raumisttemperatur",
    en: "Room temp",
  },
  off_time_opt_duration: {
    de: "Ausschaltoptimierung",
    en: "Off time opt duration",
  },
  mixer: {
    de: "Mischerstellung",
    en: "Mixer",
  },
  heat_curve_10C: {
    de: "Heizkennlinie 10 Grad",
    en: "Heat curve 10C",
  },
  heat_curve_0C: {
    de: "Heizkennlinie 0 Grad",
    en: "Heat curve 0C",
  },
  "heat_curve_-10C": {
    de: "Heizkennlinie -10 Grad",
    en: "Heat curve -10C",
  },
  ov1_auto: {
    de: "Automatik",
    en: "auto",
  },
  ov1_disinfection: {
    de: "Desinfektion",
    en: "disinfection",
  },
  ov1_reload: {
    de: "Nachladung",
    en: "reload",
  },
  ov1_err_disinfection: { de: "Desinfektion fehlerhaft", en: "Disinfection fault" },
  ov1_err_sensor: { de: "Temperaturfühler fehlerhaft", en: "Temperature sensor fault" },
  ov1_stays_cold: { de: "Speicher bleibt kalt", en: "Cylinder stays cold" },
  ov1_err_anode: { de: "Anodenfehler", en: "Anode fault" },
  ov2_load: {
    de: "Laden",
    en: "load",
  },
  ov2_manual: {
    de: "Manuell",
    en: "manual",
  },
  ov2_reload: {
    de: "Nachladen",
    en: "reload",
  },
  ov2_off_time_opt_duration: {
    de: "Ausschaltoptimierung",
    en: "off time opt duration",
  },
  ov2_on_time_opt_duration: {
    de: "Einschaltoptimierung",
    en: "on time opt duration",
  },
  ov2_hot: {
    de: "Warm",
    en: "hot",
  },
  ov2_priority: {
    de: "Vorrang",
    en: "priority",
  },
  on_time_opt_duration: {
    de: "Einschaltoptimierung",
    en: "on time opt duration",
  },
  pump_type_charge: {
    de: "Pumpentyp Ladepumpe",
    en: "pump type charge",
  },
  pump_type_circulation: {
    de: "Pumpentyp Zirkulationspumpe",
    en: "pump type circulation",
  },
  pump_type_groundwater_solar: {
    de: "Pumpentyp Absenkung Solar",
    en: "pump type groundwater solar",
  },
  boiler_setpoint: {
    de: "Kessel Vorlaufsolltemperatur",
    en: "Boiler setpoint",
  },
  boiler_temp: {
    de: "Kessel Vorlaufisttemperatur",
    en: "Boiler temp",
  },
  boiler_switch_on_temp: {
    de: "Brenner Einschalttemperatur",
    en: "Boiler switch on temp",
  },
  boiler_switch_off_temp: {
    de: "Brenner Ausschalttemperatur",
    en: "Boiler switch off temp",
  },
  boiler_failure_burner: {
    de: "Kessel Fehler Brennerstörung",
    en: "Boiler failure burner",
  },
  boiler_failure_boiler_sensor: {
    de: "Kessel Fehler Kesselfühler",
    en: "Boiler failure boiler sensor",
  },
  boiler_failure_aux_sensor: {
    de: "Kessel Fehler Zusatzfühler",
    en: "Boiler failure aux sensor",
  },
  boiler_failure_boiler_stays_cold: {
    de: "Kessel Fehler Kessel bleibt kalt",
    en: "Boiler failure boiler stays cold",
  },
  boiler_failure_exhaust_gas_sensor: {
    de: "Kessel Fehler Abgasfühler",
    en: "Boiler failure exhaust gas sensor",
  },
  boiler_failure_exhaust_gas_over_limit: {
    de: "Kessel Fehler Abgas max Grenzwert",
    en: "Boiler failure exhaust gas over limit",
  },
  boiler_failure_safety_chain: {
    de: "Kessel Fehler Sicherungskette",
    en: "Boiler failure safety chain",
  },
  boiler_failure_external: {
    de: "Kessel Fehler Externe Störung",
    en: "Boiler failure external",
  },
  boiler_state_exhaust_gas_test: {
    de: "Kessel Betrieb Abgastest",
    en: "Boiler state exhaust gas test",
  },
  boiler_state_stage1: {
    de: "Kessel Betrieb Betrieb Stufe1",
    en: "Boiler state stage1",
  },
  boiler_state_boiler_protection: {
    de: "Kessel Betrieb Kesselschutz",
    en: "Boiler state boiler protection",
  },
  boiler_state_active: {
    de: "Kessel Betrieb Unter Betrieb",
    en: "Boiler state active",
  },
  boiler_state_performance_free: {
    de: "Kessel Betrieb Leistung frei",
    en: "Boiler state performance free",
  },
  boiler_state_performance_high: {
    de: "Kessel Betrieb Leistung hoch",
    en: "Boiler state performance high",
  },
  boiler_state_stage2: {
    de: "Kessel Betrieb BetriebStufe2",
    en: "Boiler state stage2",
  },
  burner_control: {
    de: "Brenner Ansteuerung",
    en: "Burner control",
  },
  exhaust_gas_temp: {
    de: "Abgastemperatur",
    en: "Exhaust gas temp",
  },
  outside_temp: {
    de: "Außentemperatur",
    en: "Outside temp",
  },
  outside_temp_damped: {
    de: "Außentemperatur gedämpft",
    en: "Outside temp damped",
  },
  send: {
    de: "senden",
    en: "send",
  },
  log_mode_1: {
    de: "Modus: Alarm",
    en: "Mode: Alarm",
  },
  log_mode_2: {
    de: "Modus: Alarm + Info",
    en: "Modus: Alarm + Info",
  },
  log_mode_3: {
    de: "Modus: Logamatic Werte",
    en: "Mode: Logamatic values",
  },
  log_mode_4: {
    de: "Modus: unbekannte Datagramme",
    en: "Mode: unknown datagramms",
  },
  log_mode_5: {
    de: "Modus: debug Datagramme",
    en: "Mode: debug datagramms",
  },
  log_type_1: {
    de: "Logbuch: SYSTEM",
    en: "Logbook: SYSTEM",
  },
  log_type_2: {
    de: "Logbuch: KM271",
    en: "Logbook: KM271",
  },
  description: {
    de: "Beschreibung",
    en: "description",
  },
  simulation: {
    de: "Simulation",
    en: "Simulation",
  },
  generate_simdata: {
    de: "generiere Simulationsdaten",
    en: "generate simulation data",
  },
  select_template: {
    de: "wähle Vorlage...",
    en: "select template...",
  },
  km271_refresh: {
    de: "empfange Daten...",
    en: "receiving data...",
  },
  timer: {
    de: "Timer",
    en: "timer",
  },
  message: {
    de: "Meldung",
    en: "Message",
  },
  cyclic_send: {
    de: "zyklisches Senden [min]",
    en: "cyclic send [min]",
  },
  solar: {
    de: "Solar",
    en: "Solar",
  },
  on: {
    de: "EIN",
    en: "ON",
  },
  solar_max: {
    de: "Max Solar",
    en: "min Solar",
  },
  solar_min: {
    de: "Min Solar",
    en: "min Solar",
  },
  load: {
    de: "Ladung",
    en: "load",
  },
  collector: {
    de: "Kollektor",
    en: "Collector",
  },
  km271_info: {
    de: "KM271 Information",
    en: "KM271 Information",
  },
  sent: {
    de: "gesendet",
    en: "sent",
  },
  received: {
    de: "empfangen",
    en: "received",
  },
  act_prg: {
    de: "Aktives Programm",
    en: "Active program",
  },
};

// Reuse received network classes and RSSI; no additional requests or timers.
document.addEventListener('DOMContentLoaded', () => {
  const wifi = document.getElementById('p00_wifi_icon');
  const eth = document.getElementById('p00_eth_icon');
  const rssi = document.getElementById('p09_wifi_rssi');
  const language = document.getElementById('cfg_lang');
  const version = document.getElementById('p00_version');
  const logout = document.querySelector('.sidebar-logout');
  const update = () => {
    const en = language?.value === '1';
    const connected = en ? 'connected' : 'verbunden';
    const disconnected = en ? 'not connected' : 'nicht verbunden';
    const wifiConnected = /\bi_wifi_[1-4]\b/.test(wifi.className);
    const signal = rssi?.textContent.match(/-?\d+/)?.[0];
    const wifiText = `WLAN ${wifiConnected ? connected : disconnected}${wifiConnected && signal ? ` · ${signal} dBm` : ''}`;
    wifi.title = wifiText;
    wifi.setAttribute('aria-label', wifiText);
    const ethText = `Ethernet ${eth.classList.contains('i_eth_ok') ? connected : disconnected}`;
    eth.title = ethText;
    eth.setAttribute('aria-label', ethText);
    version.title = en ? 'Version and GitHub updates' : 'Version und GitHub-Updates';
    logout.title = en ? 'Log out' : 'Abmelden';
    logout.setAttribute('aria-label', logout.title);
  };
  const observer = new MutationObserver(update);
  [wifi, eth].forEach(icon => observer.observe(icon, { attributes: true, attributeFilter: ['class'] }));
  if (rssi) observer.observe(rssi, { childList: true, characterData: true, subtree: true });
  if (language) {
    language.addEventListener('change', update);
    observer.observe(language, { attributes: true, attributeFilter: ['value'] });
  }
  update();
});

function initControlSliders() {
  const sources = new Map();
  const update = slider => {
    const min = Number(slider.min), max = Number(slider.max);
    const progress = max > min ? (Number(slider.value) - min) / (max - min) * 100 : 0;
    slider.style.setProperty("--control-progress", `${Math.max(0, Math.min(100, progress))}%`);
  };
  // Existing input and WebSocket handlers both update the linked value text.
  // One shared observer keeps the fill in sync without extra commands or timers.
  const observer = new MutationObserver(mutations => {
    const changed = new Set(mutations.map(m => sources.get(m.target.nodeType === 3 ? m.target.parentElement : m.target)));
    changed.forEach(slider => { if (slider) update(slider); });
  });
  document.querySelectorAll(".control-page input[type=range]").forEach(slider => {
    const source = document.getElementById(slider.dataset.valueId);
    sources.set(source, slider);
    observer.observe(source, { childList: true, characterData: true, subtree: true });
    update(slider);
  });
}

function initMobileNavigation() {
  const menu = document.getElementById("topMenuDropdown").parentElement;
  // Close explicitly without Pico's full-screen summary pseudo-element.
  document.addEventListener("click", event => {
    if (!menu.contains(event.target) || event.target.closest("a[data-tab]")) menu.open = false;
  });
  menu.addEventListener("keydown", event => {
    if (event.key === "Escape") {
      menu.open = false;
      menu.querySelector("summary").focus();
    }
  });
}

function initHomeAssistantSettings() {
  const mqtt = document.getElementById("cfg_mqtt_enable");
  const section = document.getElementById("p12_ha_settings");
  const update = () => {
    document.getElementById("p12_ha_mqtt_warning").hidden = mqtt.checked;
  };
  mqtt.addEventListener("change", update);
  section.addEventListener("toggle", update);
  document.getElementById("p12_ha_mqtt_link").addEventListener("click", event => {
    event.preventDefault();
    const target = document.getElementById("p12_mqtt_settings");
    target.open = true;
    target.scrollIntoView({ block: "nearest" });
    mqtt.focus({ preventScroll: true });
  });
  update();
}

function initPageHeader() {
  const status = document.querySelector(".app-connection-actions");
  const indicator = document.getElementById("p00_web_connection");
  const label = document.getElementById("p00_web_connection_text");
  const reload = document.getElementById("p99_reloadButton");
  const desktop = document.querySelector(".app-header");
  const mobile = document.querySelector(".topMenu");
  const content = document.querySelector(".home-section");
  const narrow = window.matchMedia("(max-width: 768px)");
  const controls = document.querySelectorAll("[data-connection-required]");
  const layout = () => {
    const host = narrow.matches ? mobile : desktop;
    if (status.parentElement !== host) host.append(status);
    if (narrow.matches) content.style.top = (mobile.getBoundingClientRect().bottom + 20) + "px";
    else content.style.removeProperty("top");
  };
  const update = () => {
    const connected = !!(typeof ws !== "undefined" && ws && ws.readyState === WebSocket.OPEN);
    const language = document.getElementById("cfg_lang").value === "1" ? "en" : "de";
    const key = connected ? "ui_connected" : "ui_disconnected";
    indicator.classList.toggle("is-connected", connected);
    label.dataset.i18n = key;
    label.textContent = (connected ? user_translations.ui_connected : user_translations.ui_disconnected)[language];
    reload.hidden = connected || isGitHubPages();
    reload.title = reload.ariaLabel = user_translations.ui_reload[language];
    // Inert preserves individual disabled states set by the ESP on reconnect.
    controls.forEach(control => { control.inert = !connected && !isGitHubPages(); });
    layout();
  };
  window.addEventListener("webui:connected", update);
  // Retain the existing library's reconnect signal without displaying a footer bar.
  new MutationObserver(update).observe(document.getElementById("connectionLostBar"), {
    attributes: true, attributeFilter: ["style", "class"],
  });
  document.getElementById("cfg_lang").addEventListener("change", update);
  narrow.addEventListener("change", layout);
  new ResizeObserver(layout).observe(mobile);
  update();
}

function initDashboardStatusIcons() {
  document.querySelectorAll(".dashboard-status [data-active-value]").forEach(card => {
    const source = document.getElementById(card.dataset.activeValue);
    const update = () => {
      const raw = source.textContent.trim();
      const value = Number(raw);
      card.classList.toggle("is-active", raw !== "" && Number.isFinite(value) && value > 0);
    };
    new MutationObserver(update).observe(source, { childList: true, characterData: true, subtree: true });
    update();
  });
}

function initMqttConnectionStatus() {
  const input = document.getElementById("p12_mqtt_connection");
  const state = document.getElementById("p12_mqtt_connection_state");
  const update = () => {
    if (state.textContent === "connected") {
      input.setAttribute("aria-invalid", "false");
    } else if (state.textContent === "disconnected") {
      input.setAttribute("aria-invalid", "true");
    } else {
      input.removeAttribute("aria-invalid");
    }
  };
  new MutationObserver(update).observe(state, {
    childList: true,
    characterData: true,
    subtree: true,
  });
  update();
}

function validateHost(input) {
  input.value = input.value.trim();
  if (/^[\d.]+$/.test(input.value)) {
    validateIP(input);
    return;
  }
  const hostPattern =
    /^[a-zA-Z0-9]([a-zA-Z0-9\-]{0,61}[a-zA-Z0-9])?(\.[a-zA-Z0-9]([a-zA-Z0-9\-]{0,61}[a-zA-Z0-9])?)*$/;
  if (!hostPattern.test(input.value)) {
    input.setAttribute("aria-invalid", "true");
  } else {
    input.setAttribute("aria-invalid", "false");
    sendData(input.id, input.value);
  }
}

function initLogType() {
  var typeSelect = document.getElementById("p10_logger_type");
  changeLogType(typeSelect);
}

function changeLogType(selectElem) {
  const systemLog = selectElem.value === "0";
  document.getElementById("log_level_field").hidden = !systemLog;
  document.getElementById("log_filter_field").hidden = systemLog;
}

async function loadSimulatedData() {
  if (!isGitHubPages()) {
    return; // Kein Simulationsmodus, wenn nicht auf GitHub Pages
  }

  console.log("GitHub Pages erkannt – Simulationsdaten werden geladen.");

  try {
    const response = await fetch("sim.json");
    if (!response.ok)
      throw new Error("Fehler beim Abrufen der Simulationsdaten");

    const simData = await response.json();
    updateJSON(simData); // Aktualisiert die UI mit den Simulationsdaten
  } catch (error) {
    console.error("Fehler beim Laden von sim.json:", error);
  }
}

// Time-Table parsing and rendering for weekly program
function parseWeeklyProgram(rows) {
  if (rows.length !== 14 || rows.some((row) => !row.trim()))
    return { status: "loading" };
  const weekdays = {
    mo: 0,
    mon: 0,
    di: 1,
    tue: 1,
    mi: 2,
    wed: 2,
    do: 3,
    thu: 3,
    fr: 4,
    fri: 4,
    sa: 5,
    sat: 5,
    so: 6,
    sun: 6,
  };
  const points = [];
  for (const row of rows) {
    const slots = row.split("|");
    if (slots.length !== 3) return { status: "invalid" };
    for (const slot of slots) {
      const value = slot.replace(/^\s*SP\d+:\s*/, "").trim();
      if (value === "--") continue;
      const match = /^(\w+)\s+(\d{2}):(\d{2})\s*\((An|Aus|On|Off)\)$/i.exec(
        value,
      );
      if (!match) return { status: "invalid" };
      const day = weekdays[match[1].toLowerCase()];
      const hour = Number(match[2]),
        minute = Number(match[3]);
      if (day === undefined || hour > 23 || minute > 59)
        return { status: "invalid" };
      points.push({
        time: day * 1440 + hour * 60 + minute,
        on: /^(An|On)$/i.test(match[4]),
      });
    }
  }
  if (!points.length) return { status: "empty" };
  points.sort((a, b) => a.time - b.time);
  for (let i = 1; i < points.length; ++i) {
    if (
      points[i].time === points[i - 1].time &&
      points[i].on !== points[i - 1].on
    )
      return { status: "invalid" };
  }

  const days = Array.from({ length: 7 }, () => []);
  // The last switch point in the previous week determines Monday at 00:00.
  let on = points[points.length - 1].on;
  let start = 0;
  const append = (end) => {
    while (start < end) {
      const day = Math.floor(start / 1440);
      const limit = Math.min(end, (day + 1) * 1440);
      if (on) {
        const intervals = days[day];
        const from = start - day * 1440,
          to = limit - day * 1440;
        const previous = intervals[intervals.length - 1];
        if (previous && previous.end === from) previous.end = to;
        else intervals.push({ start: from, end: to });
      }
      start = limit;
    }
  };
  for (const point of points) {
    append(point.time);
    on = point.on;
  }
  append(7 * 1440);
  return { status: "ready", days };
}

function weeklyProgramTime(minutes) {
  return `${String(Math.floor(minutes / 60)).padStart(2, "0")}:${String(minutes % 60).padStart(2, "0")}`;
}

// Initialize weekly program charts
function initWeeklyPrograms() {
  document.querySelectorAll(".weekly-program").forEach((program) => {
    const prefix = program.dataset.timerPrefix;
    const rows = Array.from({ length: 14 }, (_, i) =>
      document.getElementById(`${prefix}_t${String(i + 1).padStart(2, "0")}`),
    );
    const source = program.querySelector("table");
    const hint = program.querySelector("[data-i18n=weekly_program_hint]");
    const status = program.querySelector(".weekly-program-status");
    const scroll = program.querySelector(".weekly-program-scroll");
    const legend = program.querySelector(".weekly-program-legend");
    const chart = program.querySelector(".weekly-program-chart");
    const render = () => {
      const language =
        document.getElementById("cfg_lang").value === "1" ? "en" : "de";
      const text = (key) => user_translations[key][language];
      const schedule = parseWeeklyProgram(rows.map((row) => row.textContent));
      const ready = schedule.status === "ready";
      status.hidden = ready;
      scroll.hidden = legend.hidden = !ready;
      chart.replaceChildren();
      if (!ready) {
        const messages = {
          loading: text("weekly_program_loading"),
          empty: text("weekly_program_empty"),
          invalid: text("weekly_program_invalid"),
        };
        status.textContent = messages[schedule.status];
        return;
      }
      const create = (tag, className, content) => {
        const element = document.createElement(tag);
        element.className = className;
        if (content !== undefined) element.textContent = content;
        return element;
      };
      const header = create("div", "weekly-program-header");
      header.append(create("span", "", ""));
      const body = create("div", "weekly-program-body");
      const axis = create("div", "weekly-program-axis");
      axis.setAttribute("aria-hidden", "true");
      for (let hour = 0; hour <= 24; hour += 3) {
        const tick = create("span", "", weeklyProgramTime(hour * 60));
        tick.style.top = `${(hour / 24) * 100}%`;
        axis.append(tick);
      }
      body.append(axis);
      for (let day = 0; day < 7; ++day) {
        const date = new Date(Date.UTC(2024, 0, 1 + day)); // Monday to Sunday, independent of browser timezone.
        const dayName = new Intl.DateTimeFormat(language, {
          weekday: "long",
          timeZone: "UTC",
        }).format(date);
        const shortName = new Intl.DateTimeFormat(language, {
          weekday: "short",
          timeZone: "UTC",
        }).format(date);
        header.append(create("span", "", shortName));
        const column = create("div", "weekly-program-day");
        column.setAttribute("role", "img");
        const ranges = schedule.days[day].map(
          (interval) =>
            `${weeklyProgramTime(interval.start)}–${weeklyProgramTime(interval.end)}`,
        );
        column.setAttribute(
          "aria-label",
          `${dayName}: ${ranges.length ? `${text("weekly_program_on")} ${ranges.join(", ")}` : text("weekly_program_off")}`,
        );
        for (const interval of schedule.days[day]) {
          const from = weeklyProgramTime(interval.start),
            to = weeklyProgramTime(interval.end);
          const block = create("div", "weekly-program-block");
          block.style.top = `${(interval.start / 1440) * 100}%`;
          block.style.height = `${((interval.end - interval.start) / 1440) * 100}%`;
          block.title = `${dayName}: ${from}–${to} (${text("weekly_program_on")})`;
          if (interval.end - interval.start >= 90) {
            block.append(create("span", "", from), create("span", "", to));
          }
          column.append(block);
        }
        body.append(column);
      }
      chart.append(header, body);
    };
    const observer = new MutationObserver(render);
    observer.observe(source, {
      childList: true,
      characterData: true,
      subtree: true,
    });
    // localizePage updates this text when the WebUI language changes.
    observer.observe(hint, {
      childList: true,
      characterData: true,
      subtree: true,
    });
    render();
  });
}

// Native dialog shared by both heating circuits, hot water and the oil meter.
function initControlHelp() {
  const dialog = document.getElementById("control_help_dialog");
  document.querySelectorAll("[data-control-help]").forEach(button => {
    button.addEventListener("click", () => {
      dialog.querySelector("h2").textContent = button.closest(".control-section-heading").querySelector("h2").textContent;
      dialog.querySelectorAll("[data-control-help-content]").forEach(content => {
        content.hidden = content.dataset.controlHelpContent !== button.dataset.controlHelp;
      });
      button.focus();
      dialog.showModal();
    });
  });
  dialog.querySelector("[data-control-help-close]").addEventListener("click", () => dialog.close());
  dialog.addEventListener("click", event => { if (event.target === dialog) dialog.close(); });
}

function validateWiFiBssid(input) {
  input.value = input.value.trim().toUpperCase();
  if (!input.value) {
    input.removeAttribute("aria-invalid");
    return true;
  }
  const valid = /^(?:[0-9A-F]{2}:){5}[0-9A-F]{2}$/.test(input.value);
  input.setAttribute("aria-invalid", valid ? "false" : "true");
  return valid;
}

function wifiApText(key) {
  const lang = document.getElementById("cfg_lang").value === "1" ? "en" : "de";
  return user_translations[key][lang];
}

function initWiFiApDialog() {
  const input = document.getElementById("cfg_wifi_bssid");
  const dialog = document.getElementById("p12_wifi_ap_dialog");
  const list = document.getElementById("p12_wifi_ap_list");
  const status = document.getElementById("p12_wifi_ap_status");
  const payload = document.getElementById("p12_wifi_ap_results");
  const search = document.getElementById("p12_wifi_ap_scan_btn");
  const retry = document.getElementById("p12_wifi_ap_retry_btn");
  const apply = document.getElementById("p12_wifi_ap_apply_btn");
  let ssid = "";
  let selected = "";
  let timer;

  const setBusy = (busy) => {
    search.disabled = retry.disabled = apply.disabled = list.disabled = busy;
    status.setAttribute("aria-busy", busy ? "true" : "false");
  };
  const render = (accessPoints) => {
    list.replaceChildren();
    const addOption = (value, text) => {
      const option = document.createElement("option");
      option.value = value;
      option.textContent = text;
      list.appendChild(option);
    };
    addOption("", wifiApText("wifi_ap_auto"));
    const seen = new Set();
    accessPoints.slice().sort((a, b) => b.rssi - a.rssi).forEach((ap) => {
      const bssid = String(ap.bssid).toUpperCase();
      if (!/^(?:[0-9A-F]{2}:){5}[0-9A-F]{2}$/.test(bssid) || seen.has(bssid)) return;
      seen.add(bssid);
      const current = ap.current ? ` — ${wifiApText("wifi_ap_current")}` : "";
      addOption(bssid, `${bssid} — ${ap.rssi} dBm — ${wifiApText("wifi_ap_channel")} ${ap.channel}${current}`);
    });
    if (selected && !seen.has(selected)) {
      addOption(selected, `${selected} — ${wifiApText("wifi_ap_not_found")}`);
    }
    list.value = selected;
  };
  const fail = (key) => {
    clearTimeout(timer);
    setBusy(false);
    status.textContent = wifiApText(key);
  };
  const scan = () => {
    clearTimeout(timer);
    if (!ssid || new TextEncoder().encode(ssid).length > 32) {
      fail("wifi_ap_invalid_ssid");
      return;
    }
    setBusy(true);
    status.textContent = wifiApText("wifi_ap_scanning");
    // Pass the field value explicitly so a preceding edit need not be saved yet.
    sendData("p12_wifi_ap_scan_cmd", ssid);
    timer = setTimeout(() => fail("wifi_ap_failed"), 25000);
  };

  search.addEventListener("click", () => {
    ssid = document.getElementById("cfg_wifi_ssid").value;
    selected = validateWiFiBssid(input) ? input.value : "";
    document.getElementById("p12_wifi_ap_ssid").textContent = ssid;
    render([]);
    if (!dialog.open) dialog.showModal();
    scan();
  });
  retry.addEventListener("click", scan);
  list.addEventListener("change", () => { selected = list.value; });
  document.getElementById("p12_wifi_ap_close_btn").addEventListener("click", () => dialog.close());
  apply.addEventListener("click", () => {
    if (document.getElementById("cfg_wifi_ssid").value !== ssid) {
      fail("wifi_ap_ssid_changed");
      return;
    }
    input.value = list.value;
    if (validateWiFiBssid(input)) {
      sendData(input.id, input.value);
      dialog.close();
    }
  });
  new MutationObserver(() => {
    let result;
    try { result = JSON.parse(payload.textContent); } catch { return; }
    if (result.scan === "ssid" || result.ssid !== ssid) return;
    if (result.status === "scanning") {
      setBusy(true);
      status.textContent = wifiApText("wifi_ap_scanning");
      return;
    }
    clearTimeout(timer);
    setBusy(false);
    if (result.status === "done") {
      render(Array.isArray(result.aps) ? result.aps : []);
      status.textContent = wifiApText(result.aps && result.aps.length ? "wifi_ap_found" : "wifi_ap_empty");
    } else {
      fail(result.status === "busy" ? "wifi_ap_busy" : result.status === "invalid_ssid" ? "wifi_ap_invalid_ssid" : "wifi_ap_failed");
    }
  }).observe(payload, { childList: true, characterData: true, subtree: true });
}

function initWiFiSsidDialog() {
  const input = document.getElementById("cfg_wifi_ssid");
  const dialog = document.getElementById("p12_wifi_ssid_dialog");
  const list = document.getElementById("p12_wifi_ssid_list");
  const status = document.getElementById("p12_wifi_ssid_status");
  const search = document.getElementById("p12_wifi_ssid_scan_btn");
  const retry = document.getElementById("p12_wifi_ssid_retry_btn");
  const apply = document.getElementById("p12_wifi_ssid_apply_btn");
  let selected = "";
  let busy = false;
  let timer;

  const setBusy = (value) => {
    busy = value;
    search.disabled = retry.disabled = list.disabled = busy;
    apply.disabled = busy || !list.value;
    status.setAttribute("aria-busy", busy ? "true" : "false");
  };
  const render = (accessPoints) => {
    const networks = new Map();
    accessPoints.forEach((ap) => {
      const ssid = String(ap.ssid ?? "");
      if (!ssid || new TextEncoder().encode(ssid).length > 32) return;
      const previous = networks.get(ssid);
      const current = Boolean(ap.current || (previous && previous.current));
      networks.set(ssid, { ...(previous && previous.rssi > ap.rssi ? previous : ap), ssid, current });
    });
    list.replaceChildren();
    const addOption = (value, text) => {
      const option = document.createElement("option");
      option.value = value;
      option.textContent = text;
      list.appendChild(option);
      return option;
    };
    addOption("", wifiApText("wifi_ssid_choose")).disabled = true;
    [...networks.values()].sort((a, b) => b.rssi - a.rssi).forEach((network) => {
      const current = network.current ? ` — ${wifiApText("wifi_ap_current")}` : "";
      addOption(network.ssid, `${network.ssid} — ${network.rssi} dBm — ${wifiApText("wifi_ap_channel")} ${network.channel}${current}`);
    });
    if (selected && !networks.has(selected)) {
      addOption(selected, `${selected} — ${wifiApText("wifi_ap_not_found")}`);
    }
    list.value = selected;
    setBusy(busy);
    return networks.size;
  };
  const fail = (key) => {
    clearTimeout(timer);
    setBusy(false);
    status.textContent = wifiApText(key);
  };
  const scan = () => {
    clearTimeout(timer);
    setBusy(true);
    status.textContent = wifiApText("wifi_ap_scanning");
    sendData("p12_wifi_ssid_scan_cmd", "");
    timer = setTimeout(() => fail("wifi_ap_failed"), 25000);
  };
  search.addEventListener("click", () => {
    selected = input.value;
    render([]);
    if (!dialog.open) dialog.showModal();
    scan();
  });
  retry.addEventListener("click", scan);
  list.addEventListener("change", () => {
    selected = list.value;
    setBusy(busy);
  });
  document.getElementById("p12_wifi_ssid_close_btn").addEventListener("click", () => dialog.close());
  apply.addEventListener("click", () => {
    if (busy || !list.value) return;
    if (input.value !== list.value) {
      const bssid = document.getElementById("cfg_wifi_bssid");
      bssid.value = "";
      validateWiFiBssid(bssid);
      sendData(bssid.id, "");
    }
    input.value = list.value;
    sendData(input.id, input.value);
    dialog.close();
  });
  const payload = document.getElementById("p12_wifi_ap_results");
  new MutationObserver(() => {
    let result;
    try { result = JSON.parse(payload.textContent); } catch { return; }
    if (result.scan !== "ssid") return;
    if (result.status === "scanning") {
      setBusy(true);
      status.textContent = wifiApText("wifi_ap_scanning");
      return;
    }
    clearTimeout(timer);
    setBusy(false);
    if (result.status === "done") {
      const count = render(Array.isArray(result.aps) ? result.aps : []);
      status.textContent = wifiApText(count ? "wifi_ssid_found" : "wifi_ssid_empty");
    } else {
      fail(result.status === "busy" ? "wifi_ap_busy" : "wifi_ap_failed");
    }
  }).observe(payload, { childList: true, characterData: true, subtree: true });
}

