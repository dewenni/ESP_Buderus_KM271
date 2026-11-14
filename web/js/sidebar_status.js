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
