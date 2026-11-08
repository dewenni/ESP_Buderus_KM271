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
