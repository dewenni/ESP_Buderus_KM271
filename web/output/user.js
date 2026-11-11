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

// --------------------------------------
// --------------------------------------
document.addEventListener("DOMContentLoaded", function () {
  // call user functions
  initPageHeader();
  initDashboardStatusIcons();
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

// --------------------------------------
// localization texts
// --------------------------------------
const user_translations = {
  ha_mqtt_hint: { de: "Benötigt eine aktivierte und konfigurierte MQTT-Verbindung.", en: "Requires an enabled and configured MQTT connection." },
  ha_mqtt_disabled: { de: "MQTT ist derzeit deaktiviert.", en: "MQTT is currently disabled." },
  ha_mqtt_open: { de: "MQTT-Einstellungen öffnen", en: "Open MQTT settings" },
  ui_connected: { de: "Verbunden", en: "Connected" },
  ui_disconnected: { de: "Keine Verbindung", en: "Disconnected" },
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
  status: {
    de: "Status",
    en: "Status",
  },
  op_values: {
    de: "Betriebswerte",
    en: "Operating-States",
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
  ww: {
    de: "WW",
    en: "WW",
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
  error_flags: {
    de: "Fehlermeldungen",
    en: "Error Flags",
  },
  burner: {
    de: "Brenner",
    en: "Burner",
  },
  pump: {
    de: "Umw\u00e4lzpumpe",
    en: "Flow Pump",
  },
  hw: {
    de: "WW",
    en: "HW",
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
  info_summer1: {
    de: "Umschalttemperatur zwischen Sommer / Winter",
    en: "Threshold to switch between Summer/Winter",
  },
  info_summer2: {
    de: "9:Sommer | 10..30:Schwelle (\u00b0C) | 31:Winter",
    en: "9:Summer | 10..30:Threshold (\u00b0C) | 31:Winter",
  },
  info_frost: {
    de: "Umschalttemperatur Frostschutz",
    en: "Threshold for Frostprotection",
  },
  info_designtemp: {
    de: "Auslegungstemperatur Heizkennlinie",
    en: "Design Temperature for heating curve",
  },
  info_switchoff: {
    de: "Umschaltschwelle f\u00fcr Absenkung Aussenhalt",
    en: "Threshold for reduction mode",
  },
  info_wwtemp: {
    de: "Solltemperatur f\u00fcr Warmwasser",
    en: "Setpoint for Hot Water",
  },
  info_unit_c: {
    de: "Einheit: \u00b0C",
    en: "Unit: \u00b0C",
  },
  info_ww_pump_circ1: {
    de: "Anzahl der Zyklen pro Stunde",
    en: "count of operation cycles per hour",
  },
  info_ww_pump_circ2: {
    de: "0:AUS | 1..6: Zyklen/Stunde | 7:EIN",
    en: "0:OFF | 1..6: cycles/hour | 7:ON",
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
  alarminfo: {
    de: "letzte Alarm Meldungen",
    en: "latest Alarm Messages",
  },
  operation: {
    de: "Betrieb",
    en: "Operation",
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
  frost_protection_threshold: {
    de: "Frost ab",
    en: "frost protection threshold",
  },
  summer_mode_threshold: {
    de: "Sommer ab",
    en: "summer mode threshold",
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
  interpretation: {
    de: "Auslegung",
    en: "interpretation",
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
    de: "Ferien_Tage",
    en: "holiday days",
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
    de: "BW1 Ausschaltoptimierung",
    en: "OV1 off time optimization",
  },
  ov1_on_time_optimization: {
    de: "BW1 Einschaltoptimierung",
    en: "OV1 on time optimization",
  },
  ov1_automatic: {
    de: "BW1 Automatik",
    en: "OV1 automatic",
  },
  ov1_ww_priority: {
    de: "BW1 Warmwasservorrang",
    en: "OV1 WW priority",
  },
  ov1_screed_drying: {
    de: "BW1 Estrichtrocknung",
    en: "OV1 screed drying",
  },
  ov1_holiday: {
    de: "BW1 Ferien",
    en: "OV1 holiday",
  },
  ov1_frost_protection: {
    de: "BW1 Frostschutz",
    en: "OV1 frost protection",
  },
  ov2_summer: {
    de: "BW2 Sommer",
    en: "OV2 summer",
  },
  ov2_day: {
    de: "BW2 Tag",
    en: "OV2 day",
  },
  ov2_no_conn_to_remotectrl: {
    de: "BW2 Keine Komm mit FB",
    en: "OV2 no conn to remotectrl",
  },
  ov2_remotectrl_error: {
    de: "BW2 FB fehlerhaft",
    en: "OV2 remotectrl error",
  },
  ov2_failure_flow_sensor: {
    de: "BW2 Fehler Vorlauffuehler",
    en: "OV2 failure flow sensor",
  },
  ov2_flow_at_maximum: {
    de: "BW2 Maximaler Vorlauf",
    en: "OV2 flow at maximum",
  },
  ov2_external_signal_input: {
    de: "BW2 Externer Stoereingang",
    en: "OV2 external signal input",
  },
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
    de: "BW1 Automatik",
    en: "OV1 auto",
  },
  ov1_disinfection: {
    de: "BW1 Desinfektion",
    en: "OV1 disinfection",
  },
  ov1_reload: {
    de: "BW1 Nachladung",
    en: "OV1 reload",
  },
  ov1_err_disinfection: {
    de: "BW1 Fehler Desinfektion",
    en: "OV1 err disinfection",
  },
  ov1_err_sensor: {
    de: "BW1 Fehler Fuehler",
    en: "OV1 err sensor",
  },
  ov1_stays_cold: {
    de: "BW1 Fehler bleibt kalt",
    en: "OV1 stays cold",
  },
  ov1_err_anode: {
    de: "BW1 Fehler Anode",
    en: "OV1 err anode",
  },
  ov2_load: {
    de: "BW2 Laden",
    en: "OV2 load",
  },
  ov2_manual: {
    de: "BW2 Manuell",
    en: "OV2 manual",
  },
  ov2_reload: {
    de: "BW2 Nachladen",
    en: "OV2 reload",
  },
  ov2_off_time_opt_duration: {
    de: "BW2 Ausschaltoptimierung",
    en: "OV2 off time opt duration",
  },
  ov2_on_time_opt_duration: {
    de: "BW2 Einschaltoptimierung",
    en: "OV2 on time opt duration",
  },
  ov2_hot: {
    de: "BW2 Warm",
    en: "OV2 hot",
  },
  ov2_priority: {
    de: "BW2 Vorrang",
    en: "OV2 priority",
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
  runtime_years: {
    de: "Laufzeit Jahre",
    en: "runtime years",
  },
  runtime_days: {
    de: "Laufzeit Tage",
    en: "runtime days",
  },
  runtime_hours: {
    de: "Laufzeit Stunden",
    en: "runtime hours",
  },
  runtime_minutes: {
    de: "Laufzeit Minuten",
    en: "runtime minutes",
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
    de: "aktives Programm",
    en: "active program",
  },
  man_prg_timer: {
    de: "Timer des manuellen Programms",
    en: "timer of manual program",
  },
};

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
  const indicator = document.getElementById("p00_web_connection");
  const label = document.getElementById("p00_web_connection_text");
  const update = () => {
    const connected = typeof ws !== "undefined" && ws && ws.readyState === WebSocket.OPEN;
    const language = document.getElementById("cfg_lang").value === "1" ? "en" : "de";
    indicator.classList.toggle("is-connected", !!connected);
    label.dataset.i18n = connected ? "ui_connected" : "ui_disconnected";
    label.textContent = (connected ? user_translations.ui_connected : user_translations.ui_disconnected)[language];
  };
  // The first socket may open while the existing disconnect bar is already hidden.
  if (typeof ws !== "undefined" && ws) ws.addEventListener("open", update);
  new MutationObserver(update).observe(document.getElementById("connectionLostBar"), {
    attributes: true, attributeFilter: ["style", "class"],
  });
  document.getElementById("cfg_lang").addEventListener("change", update);
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
  // get log type
  var selectedValue = selectElem.value;

  // get element-id of logger-level and logger-filter
  var levelSelect = document.getElementById("cfg_logger_level");
  var filterSelect = document.getElementById("cfg_logger_filter");

  if (selectedValue === "0") {
    levelSelect.style.display = "inline-block";
    filterSelect.style.display = "none";
  } else if (selectedValue === "1") {
    levelSelect.style.display = "none";
    filterSelect.style.display = "inline-block";
  }
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

