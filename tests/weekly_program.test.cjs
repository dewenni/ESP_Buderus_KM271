// node --test tests/weekly_program.test.cjs; no browser, broker or UART.
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const path = require('node:path');
const code = fs.readFileSync(path.join(__dirname, '../web/js/weekly_program.js'), 'utf8');
const fixtures = require('./fixtures/km271_timers.json').cases;
const plain = value => JSON.parse(JSON.stringify(value));
const rawHex = hex => [...Buffer.from(hex, 'hex')];
const c = vm.createContext({}); vm.runInContext(code, c);
const decode = raw => plain(c.decodeTimerProgram(raw));
const apply = (raw, change) => plain(c.applyTimerChange(raw, change));
const empty = () => Array.from({ length: 42 }, () => [0xc2, 0x90]).flat();

test('all recorded before/after snapshots decode, with positions and 24:00 preserved', () => {
  for (const fixture of fixtures) for (const state of ['before', 'after']) for (const hex of fixture[state]) {
    const raw = rawHex(hex), before = raw.slice(), schedule = decode(raw);
    assert.equal(schedule.error, undefined, `${fixture.name} ${state}`);
    assert.deepEqual(raw, before);
    for (const p of schedule.periods) { assert.equal(raw[p.on * 2] & 31, 1); assert.equal(raw[p.off * 2] & 31, 0); assert.ok(p.end > p.start); }
  }
  const raw = empty(); raw.splice(0, 4, 0xc1, 143, 0xc0, 144);
  assert.equal(decode(raw).periods[0].end, 10080);
  assert.deepEqual(plain(c.timerSegments({ start: 10020, end: 10380 }, 0)), [{ from: 0, to: 300 }]);
});

test('edit preserves neighbours and physical positions, including cross-block and special time bytes', () => {
  const raw = rawHex(fixtures[0].before[0]);
  const result = apply(raw, { op: 'edit', on: 2, off: 3, days: 2, from: 160, to: 1010, next: false });
  assert.equal(result.error, undefined);
  assert.deepEqual(result.raw.slice(4, 8), [0x21, 0x10, 0x20, 0x65]);
  for (let i = 0; i < 84; ++i) if (i < 4 || i > 7) assert.equal(result.raw[i], raw[i]);
  const removed = apply(raw, { op: 'remove', on: 2, off: 3 });
  assert.deepEqual(removed.raw.slice(4, 8), [0xc2, 0x90, 0xc2, 0x90]);
  assert.equal(apply(raw, { op: 'remove', on: 0, off: 3 }).error, 'timer_stale');
});

test('bounds, conflicts, midnight, full capacity and multi-day allocation', () => {
  let raw = empty();
  for (const from of [0, 480, 960]) {
    const result = apply(raw, { op: 'add', days: 127, from, to: from + 60, next: false });
    assert.equal(result.error, undefined); raw = result.raw;
  }
  assert.equal(decode(raw).used, 42);
  assert.equal(apply(raw, { op: 'add', days: 1, from: 180, to: 240, next: false }).error, 'timer_capacity');
  for (const change of [{ days: 0 }, { from: 1 }, { to: 1441 }, { from: -10 }, { next: 'false' }])
    assert.equal(apply(empty(), { op: 'add', days: 1, from: 360, to: 480, next: false, ...change }).error, 'timer_order');
  raw = apply(empty(), { op: 'add', days: 64, from: 1430, to: 300, next: true }).raw;
  assert.equal(apply(raw, { op: 'add', days: 1, from: 290, to: 360, next: false }).error, 'timer_overlap');
  assert.equal(apply(raw, { op: 'add', days: 1, from: 300, to: 360, next: false }).error, 'timer_overlap');
  assert.equal(apply(raw, { op: 'add', days: 1, from: 310, to: 360, next: false }).error, undefined);
});

test('non-pairable, redundant, odd, invalid and contradictory points are never silently normalised', () => {
  for (const points of [[1, 36], [1, 36, 1, 40], [1, 36, 0, 36], [0xe1, 36, 0, 40], [1, 144, 0, 40], [2, 36, 0, 40]]) {
    const raw = empty(); raw.splice(0, points.length, ...points);
    assert.equal(decode(raw).error, 'timer_invalid');
    assert.equal(apply(raw, { op: 'add', days: 1, from: 0, to: 60, next: false }).error, 'timer_invalid');
  }
});

// Minimal DOM of the actual UI contract. It models native controls and events,
// not CSS, browser focus/tooltip layout or serial adoption.
class Element {
  constructor(tag = 'div', cls = '') { this.tag = tag; this.className = cls; this.children = []; this.dataset = {}; this.listeners = {}; this.style = {}; this.attrs = {}; this.textContent = ''; this.disabled = false; this.hidden = false; this.value = ''; }
  append(...children) { this.children.push(...children); }
  replaceChildren(...children) { this.children = children; }
  setAttribute(k, v) { this.attrs[k] = v; }
  focus() {}
  addEventListener(k, fn) { this.listeners[k] = fn; }
  matches(selector) {
    return selector.split(',').some(s => {
      s = s.trim(); const enabled = s.endsWith(':not(:disabled)'); s = s.replace(':not(:disabled)', '');
      if (enabled && this.disabled) return false;
      if (s.startsWith('.')) return this.className.split(' ').includes(s.slice(1));
      const attr = /^\[data-([\w-]+)(?:="([^"]*)")?\]$/.exec(s);
      if (attr) { const k = attr[1].replace(/-([a-z])/g, (_, x) => x.toUpperCase()); return k in this.dataset && (attr[2] === undefined || this.dataset[k] === attr[2]); }
      return this.tag === s;
    });
  }
  querySelectorAll(s) { return this.children.flatMap(child => [...(child.matches(s) ? [child] : []), ...child.querySelectorAll(s)]); }
  querySelector(s) { return this.querySelectorAll(s)[0] || null; }
  closest(s) { return this.matches(s) ? this : null; }
  set innerHTML(html) {
    this.children = [];
    for (const match of html.matchAll(/<(button|input|select|p)\b([^>]*)(?:>([\s\S]*?)<\/\1>)?/g)) {
      const el = new Element(match[1]); const attrs = match[2];
      el.className = /class="([^"]*)"/.exec(attrs)?.[1] || '';
      for (const a of attrs.matchAll(/data-([\w-]+)(?:="([^"]*)")?/g)) el.dataset[a[1].replace(/-([a-z])/g, (_, x) => x.toUpperCase())] = a[2] || '';
      el.disabled = /\bdisabled\b/.test(attrs); el.checked = /\bchecked\b/.test(attrs);
      if (el.tag === 'select') el.value = /value="([^"]*)"\s+selected/.exec(match[3])?.[1] || /value="([^"]*)"/.exec(match[3])?.[1] || '';
      this.children.push(el);
    }
  }
}
function ui() {
  const roots = [1, 2].map(hc => { const r = new Element(); r.dataset.circuit = String(hc); for (const cls of ['timer-status', 'timer-chart', 'timer-capacity']) r.append(new Element('div', cls)); return r; });
  const dialog = new Element('dialog'), article = new Element('article'), lang = new Element('select'); dialog.append(article); lang.value = '0';
  dialog.showModal = () => { dialog.open = true; }; dialog.close = () => { dialog.open = false; dialog.listeners.close(); };
  const handlers = {}, sent = [], timers = new Map(); let timerId = 0;
  const context = vm.createContext({
    document: { querySelectorAll: () => roots, getElementById: id => id === 'cfg_lang' ? lang : dialog, createElement: tag => new Element(tag) },
    window: { addEventListener: (key, fn) => handlers[key] = fn }, ws: { readyState: 1 }, WebSocket: { OPEN: 1 }, isGitHubPages: () => false,
    MutationObserver: class { observe() {} },
    sendData: (element, value) => sent.push({ element, ...JSON.parse(value) }), setTimeout: fn => { timers.set(++timerId, fn); return timerId; }, clearTimeout: id => timers.delete(id)
  });
  vm.runInContext(fs.readFileSync(path.join(__dirname, '../web/js/lang.js'), 'utf8'), context);
  vm.runInContext(code, context); context.initWeeklyPrograms();
  const data = { type: 'km271Timers', session: 77, connected: true, busy: false, circuits: [1, 2].map(hc => ({ hc, enabled: true, valid: 16383, revision: 1, request: 0, state: 0, raw: rawHex(fixtures[0].before[hc - 1]) })) };
  const receive = snapshot => handlers['webui:message']({ detail: plain(snapshot) }); receive(data);
  const click = (host, target) => { assert.ok(target); host.listeners.click({ target }); };
  const action = selector => click(dialog, dialog.querySelector(selector));
  const edit = () => click(roots[0], roots[0].querySelector('[data-timer-edit="0"]'));
  return { context, roots, dialog, lang, data, sent, handlers, timers, receive, click, action, edit };
}

test('only bars and Plus; opening, changing and cancelling do not transmit', () => {
  const f = ui();
  assert.equal(f.roots[0].querySelectorAll('button').length, 14); // seven periods, seven Plus
  assert.equal(f.roots[0].querySelector('details'), null);
  assert.equal(f.roots[0].querySelectorAll('.timer-raster').length, 7);
  f.edit(); assert.equal(f.dialog.open, true); assert.equal(f.sent.length, 0);
  const select = f.dialog.querySelector('[data-timer-field="from"]'); select.value = '370'; f.dialog.listeners.change({ target: select });
  f.action('[data-timer-cancel]'); assert.equal(f.sent.length, 0);
  f.click(f.roots[0], f.roots[0].querySelector('[data-timer-add="0"]')); f.action('[data-timer-cancel]'); assert.equal(f.sent.length, 0);
});

test('save waits for final request ACK snapshot; accepted/partial replies cannot show success', () => {
  const f = ui(); f.edit(); f.action('[data-timer-save]');
  assert.equal(f.sent.length, 1); assert.equal(f.sent[0].element, 'timer_write'); assert.equal(f.sent[0].session, 77);
  assert.equal(f.dialog.querySelector('[data-timer-save]').disabled, true);
  f.receive({ type: 'km271TimerReply', hc: 1, id: f.sent[0].id, accepted: true }); assert.equal(f.dialog.open, true);
  f.data.busy = true; f.data.circuits[0].request = f.sent[0].id; f.data.circuits[0].state = 1; f.data.circuits[0].revision = 2;
  f.receive(f.data); assert.equal(f.dialog.open, true);
  f.data.busy = false; f.data.circuits[0].state = 2; f.receive(f.data);
  assert.equal(f.dialog.open, false); assert.equal(f.roots[0].querySelector('.timer-status').textContent, 'Gespeichert');
});

test('stale, disconnected, incomplete and failed data cannot be submitted', () => {
  const f = ui(); f.edit(); f.data.circuits[0].revision++; f.receive(f.data); f.action('[data-timer-save]'); assert.equal(f.sent.length, 0);
  f.action('[data-timer-cancel]'); f.edit(); f.action('[data-timer-save]'); assert.equal(f.sent.length, 1);
  f.data.circuits[0].request = f.sent[0].id; f.data.circuits[0].state = 3; f.data.circuits[0].valid = 0; f.receive(f.data);
  assert.match(f.dialog.querySelector('.timer-dialog-feedback').textContent, /Ein Teil/); f.action('[data-timer-save]'); assert.equal(f.sent.length, 1);
  f.action('[data-timer-cancel]'); assert.equal(f.roots[0].querySelector('[data-timer-add="0"]').disabled, true);
  f.handlers['webui:disconnected'](); f.handlers['webui:connected'](); assert.equal(f.roots[0].querySelector('[data-timer-add="0"]').disabled, true);
});

test('disconnect or lost application reply reports unknown without resending', () => {
  for (const interrupt of ['disconnect', 'timeout']) {
    const f = ui(); f.edit(); f.action('[data-timer-save]');
    if (interrupt === 'disconnect') f.handlers['webui:disconnected'](); else [...f.timers.values()][0]();
    assert.match(f.dialog.querySelector('.timer-dialog-feedback').textContent, /Ergebnis unbekannt/);
    f.action('[data-timer-save]'); assert.equal(f.sent.length, 1);
  }
});
