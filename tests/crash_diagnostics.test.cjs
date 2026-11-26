const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const source = fs.readFileSync('web/js/crash_diagnostics.js', 'utf8');

function fixture() {
  const handlers = {}, elements = {}, requests = [], downloads = [];
  const element = id => elements[id] ||= { value: '0', open: false, disabled: false, textContent: '',
    addEventListener(event, callback) { this[event] = callback; } };
  const context = vm.createContext({
    Uint8Array, TextEncoder, Blob, setTimeout, clearTimeout,
    user_translations: new Proxy({}, { get: (_, key) => ({ de: key, en: key }) }),
    document: { getElementById: element, body: { append() {} }, createElement: () => ({ click() { downloads.push(this.download); }, remove() {} }) },
    window: { addEventListener: (type, fn) => { handlers[type] = fn; } },
    WebSocket: { OPEN: 1 }, ws: { readyState: 1 }, isGitHubPages: () => false,
    prompt: () => 'service', confirm: () => true,
    URL: { createObjectURL: () => 'blob:test', revokeObjectURL() {} },
    sendData: (element, value) => requests.push(JSON.parse(value)),
  });
  vm.runInContext(source + '\ninitCrashDiagnostics();', context);
  return { context, elements, requests, downloads, handlers,
    respond(data) { const req = requests.at(-1); handlers['webui:message']({ detail: { type: 'crashDiagnostics', id: req.id, boot: 42, size: 2, state: 'crash_available', ...data } }); } };
}
const tick = () => new Promise(resolve => setImmediate(resolve));

test('rejects mismatched, oversized, empty or malformed chunks', () => {
  const { context } = fixture();
  const valid = { boot: 42, size: 2, offset: 0, data: '00ff' };
  context.message = valid;
  assert.deepEqual(Array.from(vm.runInContext('crashDecodeChunk(message, 0, 2, 42)', context)), [0, 255]);
  for (const change of [{ boot: 43 }, { size: 3 }, { offset: 1 }, { data: '' }, { data: '0x' }, { data: '000102' }]) {
    context.message = { ...valid, ...change };
    assert.throws(() => vm.runInContext('crashDecodeChunk(message, 0, 2, 42)', context), /crash_changed/);
  }
});

test('download waits for matching replies and completes only after all bytes', async () => {
  const f = fixture();
  const operation = f.elements.p12_crash_download.click();
  assert.equal(f.requests[0].op, 'info');
  assert.equal(f.requests[0].password, '73657276696365');
  f.handlers['webui:message']({ detail: { type: 'crashDiagnostics', id: 999 } });
  await tick();
  assert.equal(f.requests.length, 1);
  f.respond({ sha256: 'abc' }); await tick();
  assert.equal(f.requests.at(-1).op, 'read');
  assert.equal(f.downloads.length, 0);
  f.respond({ offset: 0, data: '0102' }); await operation;
  assert.deepEqual(f.downloads, ['buderus-coredump-abc.bin']);
  assert.equal(f.requests.some(request => request.op === 'erase'), false);
});

test('disconnect aborts a partial transfer without creating a file', async () => {
  const f = fixture();
  const operation = f.elements.p12_crash_download.click();
  f.respond({ size: 4 }); await tick();
  f.respond({ size: 4, offset: 0, data: '0102' }); await tick();
  f.context.ws.readyState = 3;
  f.handlers['webui:disconnected'](); await operation;
  assert.equal(f.downloads.length, 0);
  assert.equal(f.elements.p12_crash_download.disabled, true);
});

test('cancelled deletion sends no command or password', async () => {
  const f = fixture(); f.context.confirm = () => false;
  await f.elements.p12_crash_erase.click();
  assert.equal(f.requests.length, 0);
});
