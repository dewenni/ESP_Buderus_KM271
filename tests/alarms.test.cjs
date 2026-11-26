// No dependencies: node --test tests/alarms.test.cjs
const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const context = vm.createContext({});
vm.runInContext(fs.readFileSync(path.join(__dirname, '../web/js/alarms.js'), 'utf8'), context);
const decode = snapshot => JSON.parse(JSON.stringify(context.decodeAlarmSnapshot(snapshot)));
const view = (snapshot, connected = true) => JSON.parse(JSON.stringify(context.alarmView(snapshot, connected)));
const empty = () => ({ title: 'Kein Fehler', raw: 'Kein Fehler', data: [0, 0, 0, 0, 0, 0, 0] });
const slot = data => ({ title: 'Fühler defekt', raw: 'Originaltext', data });
const snapshot = () => ({ enabled: true, connected: true, current: 15, slots: Array.from({ length: 4 }, empty) });

test('no received data is unknown; four received empty slots are clear', () => {
  assert.equal(view(null).state, 'loading');
  const state = snapshot();
  assert.equal(view(state).state, 'clear');
  state.current = 0;
  assert.equal(view(state).state, 'loading');
});

test('history keeps controller order and adds both relative day offsets', () => {
  const state = snapshot();
  state.slots[0] = slot([8, 23, 59, 3, 0, 1, 2]);
  state.slots[2] = slot([2, 16, 24, 0, 16, 25, 0]);
  const entries = decode(state);
  assert.deepEqual(entries.map(entry => entry.index), [0, 2]);
  assert.deepEqual(entries[0].came, [23, 59, 5]);
  assert.deepEqual(entries[0].gone, [0, 1, 2]);
  assert.equal(entries[0].open, false);
  assert.equal(view(state).state, 'clear');
});

test('0xff marks an open alarm; end-day bytes must not affect its arrival', () => {
  const state = snapshot();
  state.slots[0] = slot([8, 16, 26, 3, 255, 255, 255]);
  assert.deepEqual(decode(state)[0].came, [16, 26, 3]);
  assert.equal(decode(state)[0].gone, null);
  assert.equal(view(state).state, 'active');
  assert.equal(view(state).count, 1);
  state.slots[1] = slot([20, 17, 0, 0, 255, 0, 0]);
  assert.equal(view(state).count, 2);
});

test('a missing slot cannot certify zero alarms', () => {
  const state = snapshot();
  state.current = 7;
  state.slots[3] = null;
  assert.equal(view(state).state, 'partial');
  assert.equal(view(state).received, 3);
  state.slots[0] = slot([8, 16, 26, 0, 255, 0, 0]);
  assert.equal(view(state).state, 'active');
  assert.equal(view(state).received, 3);
});

test('cached alarms survive disconnect but do not become current on reconnect', () => {
  const state = snapshot();
  state.slots[2] = slot([8, 16, 26, 0, 255, 0, 0]);
  assert.equal(view(state, false).state, 'offline');
  assert.equal(view(state, false).entries.length, 1);
  state.connected = false;
  assert.equal(view(state).state, 'offline');
  state.connected = true;
  state.current = 1;
  assert.equal(view(state).state, 'partial');
  assert.equal(view(state).entries.length, 0);
  state.current = 5;
  assert.equal(view(state).state, 'active');
});

test('disabled alarm reception never reports clear or an active cached alarm', () => {
  const state = snapshot();
  state.slots[0] = slot([8, 16, 26, 0, 255, 0, 0]);
  state.enabled = false;
  assert.equal(view(state).state, 'disabled');
  assert.deepEqual(view(state).entries, []);
});

test('unknown error codes and invalid clock values stay distinguishable', () => {
  const state = snapshot();
  state.slots[0] = slot([217, 255, 99, 255, 255, 255, 255]);
  const entry = decode(state)[0];
  assert.equal(entry.code, 217);
  assert.equal(entry.open, true);
  assert.deepEqual(entry.came, [255, 99, 255]);
});

test('malformed or truncated snapshots cannot overwrite a valid state', () => {
  const changes = [state => state.slots.pop(), state => state.slots[0] = null,
    state => state.slots[0].data.pop(), state => state.slots[0].data[0] = -1,
    state => state.slots[0].data[0] = 256, state => state.current = 16,
    state => state.enabled = 'true', state => state.slots[0].title = null];
  for (const change of changes) {
    const state = snapshot(); change(state);
    assert.equal(decode(state), null);
  }
});
