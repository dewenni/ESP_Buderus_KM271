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

    const { km271Alarms, km271Timers, ...simData } = await response.json();
    updateJSON(simData); // Aktualisiert die UI mit den Simulationsdaten
    if (km271Timers) window.dispatchEvent(new CustomEvent("webui:message", { detail: km271Timers }));
    if (km271Alarms) window.dispatchEvent(new CustomEvent("webui:message", { detail: km271Alarms }));
  } catch (error) {
    console.error("Fehler beim Laden von sim.json:", error);
  }
}

// Area switches and contextual help are local UI actions, never device commands.
function initEquipmentPages() {
  document.querySelectorAll('[data-equipment-tabs]').forEach(nav => {
    const tabs = [...nav.querySelectorAll('[role=tab]')];
    const select = tab => {
      tabs.forEach(other => {
        const active = other === tab;
        other.setAttribute('aria-selected', String(active));
        other.tabIndex = active ? 0 : -1;
        document.getElementById(other.getAttribute('aria-controls')).hidden = !active;
      });
    };
    tabs.forEach(tab => {
      tab.addEventListener('click', () => select(tab));
      tab.addEventListener('keydown', event => {
        const offset = event.key === 'ArrowRight' ? 1 : event.key === 'ArrowLeft' ? -1 : 0;
        if (!offset && event.key !== 'Home' && event.key !== 'End') return;
        event.preventDefault();
        const next = event.key === 'Home' ? tabs[0] : event.key === 'End' ? tabs[tabs.length - 1] : tabs[(tabs.indexOf(tab) + offset + tabs.length) % tabs.length];
        select(next); next.focus();
      });
    });
    select(tabs[0]);
  });
  const tip = document.createElement('div');
  tip.id = 'equipment_tooltip'; tip.className = 'equipment-tooltip';
  tip.setAttribute('role', 'tooltip'); tip.hidden = true;
  document.body.append(tip);
  let timer;
  const hide = () => {
    clearTimeout(timer); tip.hidden = true;

  };
  const show = button => {
    hide();
    tip.textContent = document.getElementById(button.id + '_text').textContent;
    tip.hidden = false;
    const rect = button.getBoundingClientRect(), box = tip.getBoundingClientRect();
    tip.style.left = `${Math.max(12, Math.min(rect.left, window.innerWidth - box.width - 12))}px`;
    tip.style.top = `${Math.max(12, Math.min(rect.top >= box.height + 20 ? rect.top - box.height - 8 : rect.bottom + 8, window.innerHeight - box.height - 12))}px`;
  };
  document.querySelectorAll('[data-setting-help]').forEach(button => {
    const card = button.closest('.control-card');
    card.addEventListener('pointerenter', event => {
      if (event.pointerType !== 'mouse') return;
      clearTimeout(timer); timer = setTimeout(() => show(button), 400);
    });
    card.addEventListener('pointerleave', () => { clearTimeout(timer); timer = setTimeout(hide, 150); });
    card.addEventListener('focusin', () => show(button));
    card.addEventListener('focusout', event => { if (!card.contains(event.relatedTarget)) hide(); });
    button.addEventListener('click', () => show(button));
  });
  tip.addEventListener('pointerenter', () => clearTimeout(timer));
  tip.addEventListener('pointerleave', hide);
  document.addEventListener('pointerdown', event => { if (!event.target.closest('[data-setting-help]') && !tip.contains(event.target)) hide(); });
  document.addEventListener('keydown', event => { if (event.key === 'Escape') hide(); });
  document.addEventListener('click', event => { if (event.target.closest('[data-tab], [data-equipment-tabs]')) hide(); });
  window.addEventListener('scroll', hide, true);
  window.addEventListener('resize', hide);
  document.getElementById('cfg_lang').addEventListener('change', hide);
}
