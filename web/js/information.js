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
