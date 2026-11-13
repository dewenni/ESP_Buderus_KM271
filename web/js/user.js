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
    label.textContent = user_translations[key][language];
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
