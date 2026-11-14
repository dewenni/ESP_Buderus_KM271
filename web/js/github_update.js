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
