let editor = null;
let modules = [];
let currentMod = null;
let originalJson = '';
let isDirty = false;

const LOG_PREFIX = '[GlobalsRenderer]';
const NODE_MODULES_URL = (() => {
  const url = new URL('../node_modules/', window.location.href).toString();
  return url.endsWith('/') ? url : `${url}/`;
})();

const modListEl = document.getElementById('modList');
const searchInputEl = document.getElementById('searchInput');
const statusEl = document.getElementById('statusMessage');
const headerModEl = document.getElementById('currentModLabel');
const saveBtn = document.getElementById('saveBtn');
const reloadBtn = document.getElementById('reloadBtn');
const deleteBtn = document.getElementById('deleteBtn');

const HIDDEN_MODULES = new Set(['universalapistatus']);

if (!modListEl || !searchInputEl || !statusEl || !headerModEl || !saveBtn || !reloadBtn || !deleteBtn) {
  console.error(LOG_PREFIX, 'One or more required DOM elements are missing', {
    hasModList: !!modListEl,
    hasSearch: !!searchInputEl,
    hasStatus: !!statusEl,
    hasHeader: !!headerModEl,
    hasSave: !!saveBtn,
    hasReload: !!reloadBtn,
    hasDelete: !!deleteBtn
  });
}

function log(...args) {
  console.info(LOG_PREFIX, ...args);
}

function logWarn(...args) {
  console.warn(LOG_PREFIX, ...args);
}

function logError(...args) {
  console.error(LOG_PREFIX, ...args);
}

function getBridge() {
  if (!window.globalsApi) {
    logWarn('globalsApi bridge not present on window');
  }
  return window.globalsApi;
}

log('Resolved node modules URL', NODE_MODULES_URL);

function setStatus(type, message) {
  if (!statusEl) {
    console.error(LOG_PREFIX, 'Status element missing', { type, message });
    return;
  }
  statusEl.textContent = message;
  statusEl.dataset.type = type;
  log('Status updated', { type, message });
}

function setDirtyState(dirty) {
  isDirty = !!dirty;
  saveBtn.disabled = !currentMod || !isDirty;
  reloadBtn.disabled = !currentMod;
  deleteBtn.disabled = !currentMod;
  const currentStatus = statusEl?.dataset?.type;
  if (isDirty && currentMod) {
    setStatus('warning', 'Unsaved changes');
  } else if (currentMod) {
    if (currentStatus !== 'error') {
      setStatus('info', 'Ready');
    }
  } else {
    if (currentStatus !== 'error') {
      setStatus('info', 'Select a module');
    }
  }
}

function clearSelection() {
  currentMod = null;
  originalJson = '';
  headerModEl.textContent = 'No module selected';
  if (editor) {
    editor.session.setValue('', -1);
  } else {
    logWarn('clearSelection called before editor ready');
  }
  setDirtyState(false);
}

function renderModuleList(filter = '') {
  const query = filter.trim().toLowerCase();
  if (!modListEl) {
    logError('Module list container not found');
    return;
  }
  modListEl.innerHTML = '';
  const filtered = query
    ? modules.filter(({ mod }) => mod.toLowerCase().includes(query))
    : modules.slice();
  log('Rendering module list', { filter: query, total: modules.length, rendered: filtered.length });

  if (filtered.length === 0) {
    const empty = document.createElement('div');
    empty.className = 'list-empty';
    empty.textContent = query ? 'No matches' : 'No modules yet';
    modListEl.appendChild(empty);
    return;
  }

  filtered.forEach(({ mod }) => {
    const item = document.createElement('button');
    item.type = 'button';
    item.className = 'mod-list-item';
    item.textContent = mod;
    if (mod === currentMod) {
      item.classList.add('selected');
    }
    item.addEventListener('click', () => selectModule(mod));
    modListEl.appendChild(item);
  });
}

async function loadModuleList(selectMod) {
  const bridge = getBridge();
  if (!bridge || typeof bridge.list !== 'function') {
    setStatus('error', 'Globals bridge unavailable. Please restart the app.');
    logError('Bridge list function missing');
    return;
  }
  let response;
  try {
    log('Requesting module list');
    response = await bridge.list();
    log('Module list response', { success: response?.success, count: response?.data?.length, payloadType: typeof response, dataType: typeof response?.data, dataIsArray: Array.isArray(response?.data) });
  } catch (err) {
    logError('Failed to load module list', err);
    setStatus('error', err?.message || 'Failed to load modules');
    return;
  }
  if (!response || response.success !== true) {
    const errorMessage = response?.error || 'Failed to load modules.';
    logWarn('Module list response indicated failure', response);
    modules = [];
    clearSelection();
    setStatus('error', errorMessage);
    renderModuleList(searchInputEl.value || '');
    return;
  }
  if (!Array.isArray(response.data)) {
    logError('Module list data is not an array', { data: response.data });
    modules = [];
    clearSelection();
    setStatus('error', 'Module list payload invalid. See console for details.');
    renderModuleList(searchInputEl.value || '');
    return;
  }
  modules = (response.data || []).filter((item) => {
    const modName = String(item?.mod || '').toLowerCase();
    const shouldInclude = modName && !HIDDEN_MODULES.has(modName);
    if (!shouldInclude) {
      log('Skipping module from list', { mod: item?.mod, reason: 'hidden-or-empty' });
    }
    return shouldInclude;
  });
  log('Modules after load', modules);
  renderModuleList(searchInputEl.value || '');
  setStatus('info', modules.length ? `Loaded ${modules.length} modules` : 'No modules found');
  if (modules.length === 0) {
    logWarn('No modules returned from service');
  }

  if (selectMod && modules.some((item) => item.mod === selectMod)) {
    await selectModule(selectMod, { skipDirtyCheck: true });
  } else if (!currentMod && modules.length > 0) {
    await selectModule(modules[0].mod, { skipDirtyCheck: true });
  } else if (currentMod && !modules.some((item) => item.mod === currentMod)) {
    clearSelection();
  }
}

async function selectModule(mod, options = {}) {
  if (!mod) {
    return;
  }
  if (isDirty && !options.skipDirtyCheck) {
    const confirmLeave = window.confirm('You have unsaved changes. Discard them?');
    if (!confirmLeave) {
      return;
    }
  }
  const bridge = getBridge();
  if (!bridge || typeof bridge.load !== 'function') {
    setStatus('error', 'Globals bridge unavailable. Please restart the app.');
    logError('Bridge load function missing');
    return;
  }
  let response;
  try {
    log('Loading module', { mod });
    response = await bridge.load(mod);
    log('Module load response', { mod, success: response?.success });
  } catch (err) {
    logError('Failed to load module', { mod, error: err });
    setStatus('error', err?.message || 'Failed to load module');
    return;
  }
  if (!response.success || !response.data) {
    setStatus('error', response.error || 'Failed to load module');
    logWarn('Module load returned no data', { mod, response });
    return;
  }

  const json = JSON.stringify(response.data.data ?? {}, null, 2);
  currentMod = response.data.mod;
  headerModEl.textContent = currentMod;
  originalJson = json;
  editor.session.setValue(json, -1);
  editor.clearSelection();
  setDirtyState(false);
  renderModuleList(searchInputEl.value || '');
}

async function saveCurrentModule() {
  if (!currentMod) {
    return;
  }
  if (!editor) {
    setStatus('error', 'Editor not ready. Please retry once the page has finished loading.');
    return;
  }

  let parsed;
  try {
    const currentValue = editor.getValue();
    parsed = currentValue.trim() ? JSON.parse(currentValue) : {};
  } catch (err) {
    const currentValue = editor.getValue();
    const detailedMessage = describeJsonError(err, currentValue);
    setStatus('error', detailedMessage);
    logWarn('JSON parse failed while saving', { mod: currentMod, error: err });
    return;
  }

  const annotations = typeof editor.session?.getAnnotations === 'function'
    ? editor.session.getAnnotations().filter((ann) => ann && ann.type === 'error')
    : [];
  if (annotations.length > 0) {
    const messages = annotations.map((ann) => {
      const line = (ann.row ?? 0) + 1;
      const column = (ann.column ?? 0) + 1;
      const text = ann.text || 'Unknown JSON issue';
      return `line ${line}, column ${column}: ${text}`;
    });
    logWarn('Ace reported JSON issues despite successful parse', { mod: currentMod, annotations: messages });
    setStatus('warning', `JSON warnings:
${messages.join('\n')}`);
  }

  const bridge = getBridge();
  if (!bridge || typeof bridge.save !== 'function') {
    setStatus('error', 'Globals bridge unavailable. Please restart the app.');
    logError('Bridge save function missing');
    return;
  }
  let response;
  try {
    log('Saving module', { mod: currentMod, size: Object.keys(parsed ?? {}).length });
    response = await bridge.save(currentMod, parsed);
    log('Save response', { mod: currentMod, success: response?.success });
  } catch (err) {
    logError('Failed to save module', { mod: currentMod, error: err });
    setStatus('error', err?.message || 'Failed to save module');
    return;
  }
  if (!response.success) {
    setStatus('error', response.error || 'Failed to save module');
    logWarn('Save reported failure', { mod: currentMod, response });
    return;
  }
  originalJson = JSON.stringify(parsed, null, 2);
  setDirtyState(false);
  await loadModuleList(currentMod);
  setStatus('success', 'Saved');
}

function describeJsonError(error, source) {
  if (!error) {
    return 'Unknown JSON error';
  }
  const baseMessage = typeof error.message === 'string' ? error.message : String(error);
  const positionMatch = baseMessage.match(/position\s+(\d+)/i);
  if (positionMatch) {
    const index = Number(positionMatch[1]);
    if (!Number.isNaN(index) && index >= 0) {
      const { line, column } = computeLineColumn(source, index);
      return `${baseMessage} (line ${line}, column ${column})`;
    }
  }
  return baseMessage;
}

function computeLineColumn(text, index) {
  if (typeof text !== 'string' || index <= 0) {
    return { line: 1, column: index + 1 };
  }
  const slice = text.slice(0, index);
  const lines = slice.split(/\r?\n/);
  const line = lines.length;
  const column = (lines.pop() || '').length + 1;
  return { line, column };
}

async function reloadCurrentModule() {
  if (!currentMod) {
    return;
  }
  await selectModule(currentMod, { skipDirtyCheck: false });
}

async function deleteCurrentModule() {
  if (!currentMod) {
    return;
  }
  const confirmDelete = window.confirm(`Delete module "${currentMod}"? This cannot be undone.`);
  if (!confirmDelete) {
    return;
  }
  const bridge = getBridge();
  if (!bridge || typeof bridge.remove !== 'function') {
    setStatus('error', 'Globals bridge unavailable. Please restart the app.');
    logError('Bridge remove function missing');
    return;
  }
  let response;
  try {
    log('Deleting module', { mod: currentMod });
    response = await bridge.remove(currentMod);
    log('Delete response', { mod: currentMod, success: response?.success });
  } catch (err) {
    logError('Failed to delete module', { mod: currentMod, error: err });
    setStatus('error', err?.message || 'Failed to delete module');
    return;
  }
  if (!response.success) {
    setStatus('error', response.error || 'Failed to delete module');
    logWarn('Delete reported failure', { mod: currentMod, response });
    return;
  }
  await loadModuleList();
  clearSelection();
  setStatus('success', 'Module deleted');
}
function ensureAceLoaded() {
  const aceLib = window.ace;
  if (aceLib && typeof aceLib.edit === 'function') {
    log('Ace library found on window', { version: aceLib.version });
    const workerUrl = new URL('../node_modules/ace-builds/src-min-noconflict/worker-json.js', window.location.href).toString();
    const basePath = new URL('../node_modules/ace-builds/src-min-noconflict', window.location.href).toString();
    if (aceLib.config) {
      aceLib.config.set('basePath', basePath);
      aceLib.config.setModuleUrl('ace/mode/json_worker', workerUrl);
    }
    return aceLib;
  }
  throw new Error('Ace library not available in renderer context.');
}

async function initializeEditor() {
  const aceLib = ensureAceLoaded();
  log('Attempting to initialize Ace editor', { available: !!aceLib });
  const workerUrl = `${NODE_MODULES_URL}ace-builds/src-min-noconflict/worker-json.js`;
  const basePath = `${NODE_MODULES_URL}ace-builds/src-min-noconflict`;
  aceLib.config.set('basePath', basePath);
  aceLib.config.setModuleUrl('ace/mode/json_worker', workerUrl);
  const editorElement = document.getElementById('editor');
  if (!editorElement) {
    throw new Error('Editor container element not found.');
  }
  editor = aceLib.edit(editorElement);
  const editorDetails = {
    type: typeof editor,
    constructor: editor?.constructor?.name,
    hasSetTheme: typeof editor?.setTheme,
    keys: editor ? Object.keys(editor) : []
  };
  log('Ace editor instance created', editorDetails);
  if (!editor || typeof editor.setTheme !== 'function') {
    throw new TypeError(`Ace edit returned unexpected value. constructor=${editorDetails.constructor}, hasSetTheme=${editorDetails.hasSetTheme}`);
  }
  editor.setTheme('ace/theme/twilight');
  editor.session.setMode('ace/mode/json');
  editor.session.setUseWrapMode(true);
  editor.session.setTabSize(2);
  editor.session.setUseSoftTabs(true);
  editor.setShowPrintMargin(false);
  editor.renderer.setScrollMargin(10, 10, 0, 0);
  editor.session.on('change', () => {
    if (!currentMod) {
      setDirtyState(false);
      return;
    }
    const value = editor.getValue();
    setDirtyState(value !== originalJson);
  });
  window.addEventListener('resize', () => editor.resize());
  log('Ace editor initialized successfully');
}

async function bootstrap() {
  log('Bootstrap starting');
  try {
    await initializeEditor();
  } catch (err) {
    logError('Failed to initialize editor', err);
    setStatus('error', `Failed to initialize editor: ${err?.message || err}`);
    return;
  }
  setStatus('info', 'Loading modules...');
  await loadModuleList();
  log('Bootstrap finished');
}

searchInputEl.addEventListener('input', () => {
  renderModuleList(searchInputEl.value || '');
});

saveBtn.addEventListener('click', saveCurrentModule);
reloadBtn.addEventListener('click', reloadCurrentModule);
deleteBtn.addEventListener('click', deleteCurrentModule);

if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', bootstrap);
} else {
  bootstrap();
}

window.addEventListener('error', (event) => {
  logError('Uncaught error event', { message: event.message, filename: event.filename, lineno: event.lineno, colno: event.colno, error: event.error });
});

window.addEventListener('unhandledrejection', (event) => {
  logError('Unhandled promise rejection', { reason: event.reason });
});
