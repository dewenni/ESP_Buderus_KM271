// Request-paced transfer over the existing authenticated WebSocket. No ESP-side
// copy of the dump, no polling and no persistent browser storage of credentials.
function crashDecodeChunk(message, offset, size, boot) {
  if (message.boot !== boot || message.size !== size || message.offset !== offset ||
      typeof message.data !== "string" || !/^(?:[0-9a-f]{2}){1,384}$/.test(message.data) ||
      offset + message.data.length / 2 > size) throw new Error("crash_changed");
  return Uint8Array.from(message.data.match(/../g), value => parseInt(value, 16));
}

function initCrashDiagnostics() {
  const get = id => document.getElementById(id);
  const panel = get("p12_crash_panel"), status = get("p12_crash_status"), summary = get("p12_crash_summary");
  const buttons = ["info", "download", "erase"].map(name => get("p12_crash_" + name));
  // Server status keys are explicit so the translation checker can verify them.
  const states = {
    crash_auth: user_translations.crash_auth,
    crash_available: user_translations.crash_available,
    crash_busy: user_translations.crash_busy,
    crash_changed: user_translations.crash_changed,
    crash_downloaded: user_translations.crash_downloaded,
    crash_erased: user_translations.crash_erased,
    crash_error: user_translations.crash_error,
    crash_invalid: user_translations.crash_invalid,
    crash_no_partition: user_translations.crash_no_partition,
    crash_none: user_translations.crash_none,
  };
  const text = key => (states[key] || user_translations[key])[get("cfg_lang").value === "1" ? "en" : "de"];
  const connected = () => typeof ws !== "undefined" && ws && ws.readyState === WebSocket.OPEN && !isGitHubPages();
  let state, pending, sequence = 0, busy = false, result = "";
  function render() {
    status.textContent = result ? text(result) : state ? text(state.state) + (state.size ? ` (${state.size} B)` : "") : "";
    buttons[0].disabled = busy || !connected();
    buttons[1].disabled = busy || !connected() || state?.state !== "crash_available";
    buttons[2].disabled = busy || !connected() || !state || ["crash_none", "crash_no_partition"].includes(state.state);
  }
  function request(op, password = "", extra = {}) {
    return new Promise((resolve, reject) => {
      if (!connected() || pending) { reject(new Error("crash_error")); return; }
      const id = ++sequence;
      const timer = setTimeout(() => { pending = null; reject(new Error("crash_error")); }, 10000);
      pending = { id, resolve, reject, timer };
      sendData("crash_diagnostics", JSON.stringify({ id, op, password, ...extra }));
    });
  }
  window.addEventListener("webui:message", event => {
    const message = event.detail;
    if (message.type !== "crashDiagnostics" || !pending || message.id !== pending.id) return;
    const active = pending;
    pending = null;
    clearTimeout(active.timer);
    if (message.error) active.reject(new Error(message.error));
    else { state = message; active.resolve(message); }
  });
  window.addEventListener("webui:disconnected", () => {
    if (pending) { clearTimeout(pending.timer); pending.reject(new Error("crash_error")); pending = null; }
    state = null;
    summary.hidden = true;
    summary.textContent = "";
    render();
  });
  async function refresh() {
    if (busy || !connected()) return;
    busy = true; result = ""; render();
    try { await request("status"); }
    catch (error) { result = error.message; }
    finally { busy = false; render(); }
  }
  panel.addEventListener("toggle", () => { if (panel.open) refresh(); });
  window.addEventListener("webui:connected", () => { if (panel.open) refresh(); else render(); });
  get("cfg_lang").addEventListener("change", render);
  buttons.forEach((button, index) => button.addEventListener("click", async () => {
    if (busy || !connected()) return;
    if (index === 2 && !confirm(text("crash_confirm"))) return;
    let password = prompt(text("crash_password"));
    if (password === null) return;
    password = Array.from(new TextEncoder().encode(password), byte => byte.toString(16).padStart(2, "0")).join("");
    busy = true; result = ""; render();
    try {
      const boot = state?.boot;
      if (index === 2) {
        await request("erase", password, { boot });
        summary.hidden = true; summary.textContent = ""; result = "crash_erased";
      } else {
        const info = await request("info", password);
        summary.textContent = info.task ? `${info.reason || ""}\nTask: ${info.task}\nPC: 0x${info.pc.toString(16)}\nELF SHA256: ${info.sha256}\nBacktrace: ${(info.backtrace || []).map(pc => "0x" + pc.toString(16)).join(" ")}${info.corrupted ? "\n" + text("crash_trace_corrupt") : ""}` : "";
        summary.hidden = !summary.textContent;
        if (index === 1) {
          if (info.state !== "crash_available" || !Number.isInteger(info.size) || info.size <= 0 || info.size > 65536)
            throw new Error("crash_changed");
          const chunks = [];
          for (let offset = 0; offset < info.size;) {
            const message = await request("read", password, { boot: info.boot, offset });
            const chunk = crashDecodeChunk(message, offset, info.size, info.boot);
            chunks.push(chunk); offset += chunk.length;
            status.textContent = `${text("crash_download")}: ${Math.round(offset / info.size * 100)} %`;
          }
          const url = URL.createObjectURL(new Blob(chunks, { type: "application/octet-stream" }));
          const link = document.createElement("a");
          link.href = url; link.download = `buderus-coredump-${info.sha256 || "unknown"}.bin`;
          document.body.append(link); link.click(); link.remove();
          setTimeout(() => URL.revokeObjectURL(url), 1000);
          result = "crash_downloaded";
        }
      }
    } catch (error) { result = Object.hasOwn(user_translations, error.message) ? error.message : "crash_error"; }
    finally { password = ""; busy = false; render(); }
  }));
  render();
}
