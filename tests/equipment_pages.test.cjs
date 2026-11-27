// Production navigation/help with a DOM double; no layout or hardware proof.
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const path = require('node:path');
test('received underfloor limit constrains the slider without replacing the reported text or sending', () => {
  const source = {textContent:'60'};
  const reported = {textContent:'75'};
  // Native range inputs clamp their thumb when max changes; the separate
  // received-value text must still describe the actual reported setting.
  const input = {value:'75',get max() { return this.limit; },set max(value) { this.limit=value; this.value=String(Math.min(Number(this.value),Number(value))); }};
  let observer;
  const context = {
    document: {
      getElementById: id => id === 'p02_hc2_max_limit' ? source : id === 'p02_hc2_max_temp' ? input : reported,
      querySelectorAll: () => [],
    },
    MutationObserver: class { constructor(callback) { observer=callback; } observe() {} },
    sendValue: () => assert.fail('Limit update must not transmit'),
  };
  vm.createContext(context);
  const production = fs.readFileSync(path.join(__dirname,'../web/js/user.js'),'utf8');
  vm.runInContext(production.split('function initMobileNavigation()')[0]+'\ninitControlSliders();',context);
  assert.equal(input.max,'60');
  assert.equal(input.value,'60');
  assert.equal(reported.textContent,'75');
  source.textContent='90';
  observer([{target:source}]);
  assert.equal(input.max,'90');
  assert.equal(reported.textContent,'75');
});
class Element {
  constructor() { this.attrs = {}; this.listeners = {}; this.dataset = {}; this.style = {}; this.hidden = false; }
  addEventListener(name, callback) { this.listeners[name] = callback; }
  setAttribute(key, value) { this.attrs[key] = value; }
  getAttribute(key) { return this.attrs[key]; }
  focus() { this.focused = true; }
  getBoundingClientRect() { return {left: 300, top: 10, bottom: 40, width: 300, height: 100}; }
  contains(target) { return this === target; }
  closest() { return null; }
}
function setup() {
  const doc = new Element(), win = new Element(), card = new Element(), button = new Element(), nav = new Element();
  const panels = [new Element(), new Element(), new Element()];
  const tabs = panels.map((p,i) => { const t = new Element(); t.attrs['aria-controls'] = 'pane'+i; return t; });
  nav.querySelectorAll = () => tabs;
  button.id = 'setting_help'; button.closest = () => card;
  const description = {textContent: 'Deutsche Erklärung'};
  const lang = new Element();
  doc.getElementById = id => id === 'cfg_lang' ? lang : id === 'setting_help_text' ? description : panels[Number(id.replace('pane',''))];
  doc.querySelectorAll = selector => selector === '[data-equipment-tabs]' ? [nav] : [button];
  doc.createElement = () => new Element();
  let tooltip; doc.body = {append: el => { tooltip = el; }};
  const timers = new Map(); let serial = 0;
  const context = vm.createContext({document: doc, window: Object.assign(win,{innerWidth:390,innerHeight:700}), setTimeout: fn => {timers.set(++serial, fn); return serial;}, clearTimeout: id => timers.delete(id), sendData: () => assert.fail('Local UI action sent a command')});
  vm.runInContext(fs.readFileSync(path.join(__dirname,'../web/js/user.js'),'utf8'), context);
  context.initEquipmentPages();
  return {tabs, panels, button, card, doc, win, tooltip, description, lang, timers};
}
test('tabs select only their own panels and support arrow/home/end without commands', () => {
  const {tabs,panels} = setup();
  assert.deepEqual(panels.map(p=>p.hidden), [false,true,true]);
  tabs[1].listeners.click(); assert.deepEqual(panels.map(p=>p.hidden),[true,false,true]);
  tabs[1].listeners.keydown({key:'End',preventDefault(){}});
  assert.deepEqual(panels.map(p=>p.hidden),[true,true,false]);
  tabs[2].listeners.keydown({key:'ArrowRight',preventDefault(){}});
  assert.deepEqual(panels.map(p=>p.hidden),[false,true,true]);
  assert.deepEqual(tabs.map(t=>t.tabIndex),[0,-1,-1]);
});
test('help opens on click/focus/hover, follows translated text and closes on escape/scroll', () => {
  const x=setup();
  assert.equal(x.tooltip.hidden,true);
  x.button.listeners.click(); assert.equal(x.tooltip.textContent,'Deutsche Erklärung');
  assert.equal(x.tooltip.hidden,false); assert.ok(parseInt(x.tooltip.style.left)>=12);
  x.doc.listeners.keydown({key:'Escape'}); assert.equal(x.tooltip.hidden,true);
  x.description.textContent='English explanation'; x.card.listeners.focusin();
  assert.equal(x.tooltip.textContent,'English explanation');
  x.win.listeners.scroll(); assert.equal(x.tooltip.hidden,true);
  x.card.listeners.pointerenter({pointerType:'mouse'});
  [...x.timers.values()].at(-1)(); assert.equal(x.tooltip.hidden,false);
  x.lang.listeners.change(); assert.equal(x.tooltip.hidden,true);
});
