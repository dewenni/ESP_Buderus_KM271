// Raw positions are stable; chronological pairing is only a view of these bytes.
function decodeTimerProgram(raw) {
  if (!Array.isArray(raw) || raw.length !== 84 || raw.some(v => !Number.isInteger(v) || v < 0 || v > 255)) return { error: "timer_invalid" };
  const points = [];
  for (let i = 0; i < 42; ++i) {
    const a = raw[i * 2], b = raw[i * 2 + 1];
    if (a === 0xc2 && b === 0x90) continue;
    const day = a >> 5, state = a & 31;
    if (day > 6 || state > 1 || b > 144 || (state && b === 144)) return { error: "timer_invalid" };
    points.push({ index: i, time: (day * 1440 + b * 10) % 10080, on: state === 1 });
  }
  points.sort((a, b) => a.time - b.time);
  const periods = [];
  if (points.length % 2) return { error: "timer_invalid" };
  for (let i = 0; i < points.length; ++i) {
    const p = points[i], next = points[(i + 1) % points.length];
    if (p.on === next.on || p.time === next.time) return { error: "timer_invalid" };
    if (p.on) periods.push({ on: p.index, off: next.index, start: p.time, end: next.time + (next.time <= p.time ? 10080 : 0) });
  }
  return { periods, used: points.length };
}

function timerSegments(period, day) {
  return [-10080, 0, 10080].flatMap(shift => {
    const from = Math.max(day * 1440, period.start + shift), to = Math.min((day + 1) * 1440, period.end + shift);
    return to > from ? [{ from: from - day * 1440, to: to - day * 1440 }] : [];
  });
}

// Same bounds as the firmware. A local output is useful for validation only;
// the displayed device cache changes exclusively on the ESP's response.
function applyTimerChange(raw, change) {
  const schedule = decodeTimerProgram(raw);
  if (schedule.error) return schedule;
  const add = change.op === "add", remove = change.op === "remove";
  if (!["add", "edit", "remove"].includes(change.op)) return { error: "timer_invalid" };
  if (!add && !schedule.periods.some(p => p.on === change.on && p.off === change.off)) return { error: "timer_stale" };
  const days = change.days;
  if (!remove && (!Number.isInteger(days) || days < 1 || days > 127 || (!add && (days & (days - 1))) ||
      !Number.isInteger(change.from) || change.from < 0 || change.from >= 1440 || change.from % 10 ||
      !Number.isInteger(change.to) || change.to < 0 || change.to > 1440 || change.to % 10 || typeof change.next !== "boolean" ||
      change.to + (change.next ? 1440 : 0) <= change.from)) return { error: "timer_order" };
  const output = raw.slice();
  const clear = index => { output[index * 2] = 0xc2; output[index * 2 + 1] = 0x90; };
  if (!add) { clear(change.on); clear(change.off); }
  const set = (index, value, on) => {
    value %= 10080;
    output[index * 2] = !on && value % 1440 === 0 ? ((value / 1440 + 6) % 7) << 5 : ((Math.floor(value / 1440) << 5) | Number(on));
    output[index * 2 + 1] = !on && value % 1440 === 0 ? 144 : value % 1440 / 10;
  };
  if (!remove) for (let day = 0; day < 7; ++day) {
    if (!(days & (1 << day))) continue;
    const start = day * 1440 + change.from, end = day * 1440 + change.to + (change.next ? 1440 : 0);
    if (schedule.periods.some(p => (add || p.on !== change.on) && [-10080, 0, 10080].some(shift => start <= p.end + shift && end >= p.start + shift))) return { error: "timer_overlap" };
    const free = Array.from({ length: 42 }, (_, i) => i).filter(i => output[i * 2] === 0xc2 && output[i * 2 + 1] === 0x90);
    if (free.length < 2) return { error: "timer_capacity" };
    const on = add ? free[0] : change.on, off = add ? free[1] : change.off;
    set(on, start, true); set(off, end, false);
  }
  if (decodeTimerProgram(output).error) return { error: "timer_overlap" };
  return { raw: output };
}

function timerTime(minutes) {
  return `${String(Math.floor(minutes / 60)).padStart(2, "0")}:${String(minutes % 60).padStart(2, "0")}`;
}

function initWeeklyPrograms() {
  const roots = [...document.querySelectorAll(".weekly-program")], dialog = document.getElementById("timer_dialog");
  let snapshot = null, fresh = false, draft = null, pending = null, message = "";
  let pendingTimer;
  let opener;
  let sequence = Math.floor(Math.random() * 0x7fffffff);
  const connected = () => !!(typeof ws !== "undefined" && ws && ws.readyState === WebSocket.OPEN);
  // Explicit references keep dynamically returned firmware error keys checked.
  const errors = {
    timer_busy: user_translations.timer_busy,
    timer_capacity: user_translations.timer_capacity,
    timer_demo: user_translations.timer_demo,
    timer_disabled: user_translations.timer_disabled,
    timer_invalid: user_translations.timer_invalid,
    timer_loading: user_translations.timer_loading,
    timer_offline: user_translations.timer_offline,
    timer_order: user_translations.timer_order,
    timer_overlap: user_translations.timer_overlap,
    timer_partial: user_translations.timer_partial,
    timer_saving: user_translations.timer_saving,
    timer_stale: user_translations.timer_stale,
    timer_unknown: user_translations.timer_unknown,
  };
  const text = key => (errors[key] || user_translations[key])?.[document.getElementById("cfg_lang").value === "1" ? "en" : "de"] || key;
  const dayNames = short => Array.from({ length: 7 }, (_, day) => new Intl.DateTimeFormat(document.getElementById("cfg_lang").value === "1" ? "en" : "de", { weekday: short ? "short" : "long", timeZone: "UTC" }).format(new Date(Date.UTC(2024, 0, 1 + day))));
  const circuit = hc => snapshot?.circuits?.find(c => c.hc === hc);
  const usable = hc => fresh && connected() && snapshot.connected && !snapshot.demo && !isGitHubPages() && circuit(hc)?.enabled && circuit(hc)?.valid === 16383;
  const blocked = hc => !usable(hc) || !!snapshot?.busy || !!pending;
  const periodLabel = p => {
    const first = Math.floor(p.start / 1440), last = Math.floor((p.end - 1) / 1440) % 7;
    return `${dayNames(false)[first]} ${timerTime(p.start % 1440)}–${first !== Math.floor((p.end - 1) / 1440) ? dayNames(false)[last] + " " : ""}${timerTime(p.end % 1440 || 1440)}`;
  };
  const create = (tag, cls, content) => {
    const el = document.createElement(tag); el.className = cls;
    if (content !== undefined) el.textContent = content;
    return el;
  };
  function render() {
    roots.forEach(root => {
      const hc = Number(root.dataset.circuit), state = circuit(hc), schedule = decodeTimerProgram(state?.raw);
      const chart = root.querySelector(".timer-chart"); chart.replaceChildren();
      const status = root.querySelector(".timer-status");
      const key = !snapshot ? "timer_loading" : snapshot.demo || isGitHubPages() ? "timer_demo" : !fresh || !connected() || !snapshot.connected ? "timer_offline" : !state?.enabled ? "timer_disabled" : state.valid !== 16383 ? "timer_loading" : snapshot.busy ? "timer_saving" : schedule.error;
      status.textContent = key ? text(key) : ""; status.hidden = !key;
      const axis = create("div", "timer-axis"), ticks = create("div", "timer-ticks");
      for (let i = 0; i <= 8; ++i) { const tick = create("span", i % 2 ? "timer-minor-tick" : "", String(i * 3).padStart(2, "0")); tick.style.left = `${i * 12.5}%`; ticks.append(tick); }
      axis.append(create("span", ""), ticks, create("span", "")); chart.append(axis);
      for (let day = 0; day < 7; ++day) {
        const row = create("div", "timer-day-row"), name = create("span", "timer-day-name");
        name.append(create("span", "timer-long-day", dayNames(false)[day]), create("span", "timer-short-day", dayNames(true)[day]));
        const track = create("div", "timer-track");
        const periods = schedule.periods || [];
        let count = 0;
        periods.forEach(period => timerSegments(period, day).forEach(segment => {
          ++count;
          const bar = create("button", "timer-bar"); bar.type = "button";
          bar.style.left = `${segment.from / 14.4}%`; bar.style.width = `${(segment.to - segment.from) / 14.4}%`;
          bar.dataset.timerEdit = String(period.on); bar.dataset.hc = String(hc);
          const label = periodLabel(period); bar.setAttribute("aria-label", `${text("timer_edit")}: ${label}`);
          bar.dataset.tooltip = label;
          bar.disabled = blocked(hc) || !!schedule.error;
          track.append(bar);
        }));
        track.append(create("span", "timer-raster"));
        if (!count && !schedule.error) track.append(create("span", "timer-no-period", text("timer_empty")));
        const add = create("button", "timer-action timer-add", "+"); add.type = "button";
        add.dataset.timerAdd = String(day); add.dataset.hc = String(hc); add.setAttribute("aria-label", `${dayNames(false)[day]}: ${text("timer_add")}`);
        add.disabled = blocked(hc) || !!schedule.error || schedule.used > 40;
        row.append(name, track, add); chart.append(row);
      }
      root.querySelector(".timer-capacity").textContent = schedule.used === undefined ? "" : `${schedule.used} / 42 ${text("timer_points")}`;
    });
  }
  function request() {
    return { op: draft.remove ? "remove" : draft.on === undefined ? "add" : "edit", on: draft.on, off: draft.off,
      days: draft.days, from: draft.from, to: draft.to, next: draft.next };
  }
  function stale() {
    const state = circuit(draft.hc);
    return !usable(draft.hc) || snapshot.session !== draft.session || state.revision !== draft.revision;
  }
  function dialogError() {
    if (message) return message;
    if (pending) return "timer_saving";
    if (snapshot?.busy) return "timer_busy";
    if (stale()) return "timer_stale";
    return applyTimerChange(draft.raw, request()).error || "";
  }
  function updateDialogState() {
    if (!draft) return;
    const error = dialogError(), waiting = !!pending;
    const headingDays = dialog.querySelector("header p");
    if (headingDays) headingDays.textContent = dayNames(false).filter((_, day) => draft.days & (1 << day)).join(", ");
    const feedback = dialog.querySelector(".timer-dialog-feedback"); feedback.textContent = error ? text(error) : "";
    feedback.setAttribute("role", waiting ? "status" : "alert");
    dialog.querySelectorAll("input, select, [data-timer-remove], [data-timer-confirm]").forEach(el => { el.disabled = waiting; });
    dialog.querySelector("[data-timer-save]").disabled = waiting || !!error;
    dialog.querySelector("[data-timer-cancel]").textContent = text(waiting ? "timer_close" : "timer_cancel");
  }
  const timeOptions = (value, end) => Array.from({ length: end ? 145 : 144 }, (_, i) => i * 10).map(m => `<option value="${m}" ${value === m ? "selected" : ""}>${timerTime(m)}</option>`).join("");
  function renderDialog() {
    const add = draft.on === undefined;
    dialog.querySelector("article").innerHTML = `<header><h2 id="timer_dialog_title">${text(draft.remove ? "timer_confirm" : add ? "timer_add" : "timer_edit")}</h2><p>${dayNames(false).filter((_, day) => draft.days & (1 << day)).join(", ")}</p></header>
      ${draft.remove ? `<p>${text("timer_confirm_note")}</p><p class="timer-preview">${timerTime(draft.from)}–${timerTime(draft.to)}${draft.next ? " · " + text("timer_next_day") : ""}</p>` : `${add ? `<fieldset><legend>${text("timer_days")}</legend><div class="timer-days">${dayNames(true).map((name, day) => `<label><input type="checkbox" data-timer-day="${day}" ${draft.days & (1 << day) ? "checked" : ""}>${name}</label>`).join("")}</div></fieldset>` : `<label>${text("timer_days")}<select data-timer-field="day">${dayNames(false).map((name, day) => `<option value="${day}" ${draft.days === 1 << day ? "selected" : ""}>${name}</option>`).join("")}</select></label>`}
      <div class="timer-time-fields"><label>${text("timer_from")}<select data-timer-field="from">${timeOptions(draft.from, false)}</select></label><label>${text("timer_to")}<select data-timer-field="to">${timeOptions(draft.to, true)}</select></label></div>
      <label class="timer-next"><input type="checkbox" data-timer-field="next" ${draft.next ? "checked" : ""}>${text("timer_next")}</label><p class="timer-preview">${text("timer_raster")}</p>`}
      <p class="timer-dialog-feedback" aria-live="polite"></p><div class="timer-buttons"><button type="button" class="timer-action" data-timer-cancel>${text("timer_cancel")}</button><button type="button" class="timer-action timer-primary" data-timer-save>${text(draft.remove ? "timer_delete" : "timer_save")}</button></div>
      ${!add && !draft.remove ? `<button type="button" class="timer-remove" data-timer-remove>${text("timer_remove")}</button>` : ""}`;
    updateDialogState();
  }
  function open(hc, day, on) {
    if (blocked(hc)) return;
    const state = circuit(hc), schedule = decodeTimerProgram(state.raw);
    if (schedule.error) return;
    const p = schedule.periods.find(p => p.on === on);
    if (p && p.end > (Math.floor(p.start / 1440) + 2) * 1440) { roots.find(root => Number(root.dataset.circuit) === hc).querySelector(".timer-status").textContent = text("timer_long_period"); roots.find(root => Number(root.dataset.circuit) === hc).querySelector(".timer-status").hidden = false; return; }
    opener = { hc, day: p ? Math.floor(p.start / 1440) : day, on };
    draft = { hc, session: snapshot.session, revision: state.revision, raw: state.raw.slice(), days: 1 << day,
      from: 720, to: 780, next: false, remove: false };
    if (p) {
      const startDay = Math.floor(p.start / 1440);
      Object.assign(draft, { on: p.on, off: p.off, days: 1 << startDay, from: p.start % 1440, to: p.end % 1440 || 1440, next: p.end > (startDay + 1) * 1440 });
    } else {
      for (const minute of [720, 540, 1080, ...Array.from({ length: 138 }, (_, i) => i * 10)]) {
        if (minute + 60 > 1440) continue;
        draft.from = minute; draft.to = minute + 60;
        if (!applyTimerChange(draft.raw, request()).error) break;
      }
    }
    message = ""; renderDialog(); dialog.showModal();
  }
  roots.forEach(root => root.addEventListener("click", event => {
    const edit = event.target.closest("[data-timer-edit]"), add = event.target.closest("[data-timer-add]");
    if (edit) open(Number(edit.dataset.hc), 0, Number(edit.dataset.timerEdit));
    if (add) open(Number(add.dataset.hc), Number(add.dataset.timerAdd));
  }));
  dialog.addEventListener("change", event => {
    if (!draft || pending) return;
    const el = event.target, field = el.dataset.timerField;
    if (el.dataset.timerDay !== undefined) { const bit = 1 << Number(el.dataset.timerDay); draft.days = el.checked ? draft.days | bit : draft.days & ~bit; }
    else if (field === "day") draft.days = 1 << Number(el.value);
    else if (field === "next") draft.next = el.checked;
    else if (field === "from" || field === "to") draft[field] = Number(el.value);
    // Server errors require a fresh edit when the original basis is stale.
    if (message !== "timer_partial" && message !== "timer_unknown" && !stale()) message = "";
    updateDialogState();
  });
  dialog.addEventListener("click", event => {
    if (event.target.closest("[data-timer-cancel]")) { dialog.close(); return; }
    if (!draft || pending) return;
    if (event.target.closest("[data-timer-remove]")) { draft.remove = true; message = ""; renderDialog(); }
    if (event.target.closest("[data-timer-save]")) {
      if (dialogError()) return;
      const id = sequence = (sequence + 1) >>> 0;
      pending = { id, hc: draft.hc }; message = "";
      sendData("timer_write", JSON.stringify({ id, hc: draft.hc, session: draft.session, revision: draft.revision, ...request() }));
      // A lost application reply must not leave an endless saving state.
      // Never resend automatically: the controller may already have adopted it.
      pendingTimer = setTimeout(() => {
        if (pending?.id !== id) return;
        pending = null; message = "timer_unknown"; render(); updateDialogState();
      }, 120000);
      updateDialogState(); render();
    }
  });
  dialog.addEventListener("close", () => {
    draft = null; message = "";
    const root = roots.find(root => Number(root.dataset.circuit) === opener?.hc);
    const target = root?.querySelector(`[data-timer-edit="${opener.on}"]`) || root?.querySelector(`[data-timer-add="${opener.day}"]`);
    target?.focus();
  });
  window.addEventListener("webui:disconnected", () => {
    fresh = false;
    if (pending) message = "timer_unknown";
    clearTimeout(pendingTimer);
    pending = null;
    render(); updateDialogState();
  });
  window.addEventListener("webui:connected", () => { fresh = false; render(); updateDialogState(); });
  window.addEventListener("webui:message", event => {
    const data = event.detail;
    if (data.type === "km271TimerReply" && pending?.id === data.id && pending.hc === data.hc) {
      if (data.error) { clearTimeout(pendingTimer); message = data.error; pending = null; render(); updateDialogState(); }
    }
    if (data.type !== "km271Timers" || !Array.isArray(data.circuits)) return;
    snapshot = data; fresh = true;
    if (pending) {
      const state = circuit(pending.hc);
      if (state?.request === pending.id && (state.state === 2 || state.state === 3)) {
        const hc = pending.hc, ok = state.state === 2; pending = null;
        clearTimeout(pendingTimer);
        if (ok) {
          render();
          if (draft && dialog.open) dialog.close();
          const status = roots.find(root => Number(root.dataset.circuit) === hc).querySelector(".timer-status");
          status.textContent = text("timer_saved"); status.hidden = false;
          return;
        }
        message = "timer_partial";
      }
    }
    render(); updateDialogState();
  });
  document.getElementById("cfg_lang").addEventListener("change", () => { render(); if (draft) renderDialog(); });
  // Language changes sent by the ESP also localize the static hint.
  new MutationObserver(() => { render(); if (draft) renderDialog(); }).observe(
    roots[0].querySelector("[data-i18n=timer_hint]"), { childList: true, characterData: true, subtree: true });
  render();
}
