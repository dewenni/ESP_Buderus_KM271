// Interpret numeric fields, never localized MQTT strings. The four slots remain
// in controller order; this is a memory view, not a browser-side event log.
function decodeAlarmSnapshot(snapshot) {
  if (!snapshot || typeof snapshot.enabled !== "boolean" || typeof snapshot.connected !== "boolean" ||
      !Number.isInteger(snapshot.current) || snapshot.current < 0 || snapshot.current > 15 ||
      !Array.isArray(snapshot.slots) || snapshot.slots.length !== 4) return null;
  const entries = [];
  for (let index = 0; index < 4; ++index) {
    const slot = snapshot.slots[index];
    if (slot === null) {
      if (snapshot.current & (1 << index)) return null;
      continue;
    }
    if (typeof slot.title !== "string" || typeof slot.raw !== "string" ||
        !Array.isArray(slot.data) || slot.data.length !== 7 ||
        !slot.data.every(value => Number.isInteger(value) && value >= 0 && value <= 255)) return null;
    const [code, hour, minute, days, endHour, endMinute, endDays] = slot.data;
    if (!code) continue;
    const open = endHour === 255;
    entries.push({ index, title: slot.title, code, open,
      came: [hour, minute, open ? days : days + endDays],
      gone: open ? null : [endHour, endMinute, endDays] });
  }
  return entries;
}

function alarmView(snapshot, browserConnected) {
  const decoded = decodeAlarmSnapshot(snapshot);
  if (!decoded) return { state: "loading", entries: [], received: 0 };
  if (!snapshot.enabled) return { state: "disabled", entries: [], received: 0 };
  const live = browserConnected && snapshot.connected;
  const received = [0, 1, 2, 3].filter(index => snapshot.current & (1 << index)).length;
  const entries = live ? decoded.filter(entry => snapshot.current & (1 << entry.index)) : decoded;
  const count = entries.filter(entry => entry.open).length;
  const state = !live ? "offline" : count ? "active" : received === 4 ? "clear" : received ? "partial" : "loading";
  return { state, entries, received, count };
}

function initAlarms() {
  const content = document.getElementById("p08_alarm_content");
  const heading = document.querySelector('#tab8 [data-i18n="info_alarm_subtitle"]');
  const navigation = document.querySelectorAll('a[data-tab="tab8"]');
  let snapshot = null, fresh = false;
  const demo = isGitHubPages();
  const language = () => document.getElementById("cfg_lang").value === "1" ? "en" : "de";
  const text = key => user_translations[key][language()];
  const create = (tag, className, value) => {
    const element = document.createElement(tag);
    if (className) element.className = className;
    if (value !== undefined) element.textContent = value;
    return element;
  };
  const time = fields => {
    if (!fields || fields[0] > 23 || fields[1] > 59) return text("alarm_time_unknown");
    const [hour, minute, days] = fields;
    const day = days === 0 ? text("alarm_today") : days === 1 ? text("alarm_yesterday") : text("alarm_days_ago").replace("{n}", days);
    return `${day}, ${String(hour).padStart(2, "0")}:${String(minute).padStart(2, "0")}`;
  };
  const render = () => {
    const connected = demo || !!(fresh && typeof ws !== "undefined" && ws && ws.readyState === WebSocket.OPEN);
    const view = alarmView(snapshot, connected);
    const active = view.state === "active";
    navigation.forEach(link => {
      link.classList.toggle("has-active-alarm", active);
      link.setAttribute("aria-label", active ? `${text("alarm")}: ${text("alarm_active_hint")}` : text("alarm"));
    });
    const titles = {
      loading: text("alarm_loading"), disabled: text("alarm_disabled"),
      offline: text("alarm_unknown"), partial: text("alarm_partial"), clear: text("alarm_clear"),
      active: (view.received < 4 ? text("alarm_at_least") : view.count === 1 ? text("alarm_one_open") : text("alarm_many_open")).replace("{n}", view.count)
    };
    const hints = {
      loading: text("alarm_wait"), disabled: text("alarm_disabled_hint"),
      offline: connected ? text("alarm_serial_lost") : text("alarm_browser_lost"),
      partial: text("alarm_received").replace("{n}", view.received), clear: text("alarm_clear_hint"),
      active: view.received < 4 ? text("alarm_received").replace("{n}", view.received) : text("alarm_open_hint")
    };
    const summary = create("section", `alarm-summary${active ? " is-alarm" : ["clear"].includes(view.state) ? "" : " is-unknown"}`);
    summary.append(create("h2", "", titles[view.state]), create("p", "", hints[view.state]));
    content.replaceChildren(summary);
    const stale = view.state === "offline", uncertain = ["loading", "partial", "disabled"].includes(view.state) || (active && view.received < 4);
    for (const open of [true, false]) {
      const entries = view.entries.filter(entry => entry.open === open);
      const section = create("section", "alarm-section"), sectionHeading = create("div", "alarm-section-heading");
      const title = open ? (stale ? text("alarm_last_open") : text("alarm_open")) : (stale ? text("alarm_last_history") : text("alarm_history"));
      sectionHeading.append(create("h2", "", title), create("span", "alarm-count", uncertain ? "?" : String(entries.length)));
      section.append(sectionHeading);
      if (!entries.length) {
        const empty = view.state === "disabled" ? text("alarm_disabled_hint") : uncertain ? text("alarm_not_known") : stale ? text("alarm_no_cached") : open ? text("alarm_no_open") : text("alarm_no_history");
        section.append(create("p", "alarm-empty", empty));
      } else {
        const list = create("div", "alarm-list");
        entries.forEach(entry => {
          const row = create("article", `alarm-entry${open ? " is-open" : ""}`), identity = create("div", "alarm-identity");
          if (open) identity.append(create("span", "alarm-badge", stale ? text("alarm_last_active") : text("alarm_reported_active")));
          const title = entry.code >= 214 ? `${entry.title} · ${text("alarm_code")} ${entry.code}` : entry.title;
          identity.append(create("h3", "", title));
          row.append(identity);
          for (const came of [true, false]) {
            const date = create("div", "alarm-date");
            date.append(create("span", "alarm-date-label", came ? text("alarm_came") : text("alarm_gone")),
              create("span", "alarm-date-value", came ? time(entry.came) : open ? (stale ? text("alarm_last_active") : text("alarm_still_open")) : time(entry.gone)));
            row.append(date);
          }
          list.append(row);
        });
        section.append(list);
      }
      content.append(section);
    }
    for (let i = 0; i < 4; ++i) {
      const slot = snapshot?.slots[i];
      const visible = snapshot?.enabled && slot && (stale || (snapshot.current & (1 << i)));
      document.getElementById(`p08_err_msg${i + 1}`).textContent = visible ? slot.raw : text("alarm_not_received");
    }
  };
  window.addEventListener("webui:message", event => {
    if (event.detail.type !== "km271Alarms") return;
    if (decodeAlarmSnapshot(event.detail) === null) return;
    snapshot = event.detail;
    fresh = true;
    render();
  });
  ["webui:connected", "webui:disconnected"].forEach(event => window.addEventListener(event, () => { fresh = false; render(); }));
  // localizePage updates this heading, also when language arrives over WebSocket.
  new MutationObserver(render).observe(heading, { childList: true, characterData: true, subtree: true });
  render();
}
