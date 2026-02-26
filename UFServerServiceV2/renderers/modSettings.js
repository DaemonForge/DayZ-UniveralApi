/**
 * Mod Settings Renderer
 *
 * This renderer manages the Mod Settings UI. It lists registered mods in the sidebar,
 * and renders the selected mod's custom HTML template inside a sandboxed iframe.
 *
 * The iframe communicates with this renderer via postMessage. The renderer acts as a
 * bridge between the iframe (mod template) and the Electron IPC (which accesses MongoDB).
 *
 * ── postMessage API (iframe → parent) ──
 *
 *  { type: "modSettings:loadGlobal",  requestId: "...", globalName: "MyMod_Config" }
 *  { type: "modSettings:saveGlobal",  requestId: "...", globalName: "MyMod_Config", data: { ... } }
 *  { type: "modSettings:ready" }
 *
 * ── postMessage API (parent → iframe) ──
 *
 *  { type: "modSettings:loadGlobal:response",  requestId: "...", success: true, data: { ... } }
 *  { type: "modSettings:saveGlobal:response",  requestId: "...", success: true }
 *  { type: "modSettings:init", globals: ["MyMod_Config"], modId: "my-mod" }
 */

const LOG_PREFIX = '[ModSettingsRenderer]';

// DOM Elements
const modListEl = document.getElementById('modList');
const searchInputEl = document.getElementById('searchInput');
const statusEl = document.getElementById('statusMessage');
const headerModEl = document.getElementById('currentModLabel');
const reloadBtn = document.getElementById('reloadBtn');
const refreshListBtn = document.getElementById('refreshListBtn');
const editorContainer = document.getElementById('editorContainer');
const placeholderEl = document.getElementById('placeholder');
const metaBlock = document.getElementById('metaBlock');
const metaAuthor = document.getElementById('metaAuthor');

// State
let mods = [];
let currentMod = null;
let currentIframe = null;

function log(...args) { console.info(LOG_PREFIX, ...args); }
function logWarn(...args) { console.warn(LOG_PREFIX, ...args); }
function logError(...args) { console.error(LOG_PREFIX, ...args); }

function getBridge() {
  if (!window.modSettingsApi) {
    logWarn('modSettingsApi bridge not present on window');
  }
  return window.modSettingsApi;
}

function setStatus(type, message) {
  if (!statusEl) return;
  statusEl.textContent = message;
  statusEl.dataset.type = type;
}

// ─── Mod List ───

function renderModList(filter = '') {
  const query = filter.trim().toLowerCase();
  if (!modListEl) return;
  modListEl.innerHTML = '';

  const filtered = query
    ? mods.filter(m => (m.modName || m.modId).toLowerCase().includes(query) || (m.description || '').toLowerCase().includes(query))
    : mods.slice();

  if (filtered.length === 0) {
    const empty = document.createElement('div');
    empty.className = 'list-empty';
    empty.textContent = query ? 'No matches' : 'No mods registered';
    modListEl.appendChild(empty);
    return;
  }

  filtered.forEach((mod) => {
    const item = document.createElement('button');
    item.type = 'button';
    item.className = 'mod-list-item';
    if (currentMod && mod.modId === currentMod.modId) {
      item.classList.add('selected');
    }

    const nameSpan = document.createElement('span');
    nameSpan.className = 'mod-name';
    nameSpan.textContent = mod.modName || mod.modId;
    item.appendChild(nameSpan);

    if (mod.description) {
      const descSpan = document.createElement('span');
      descSpan.className = 'mod-desc';
      descSpan.textContent = mod.description;
      item.appendChild(descSpan);
    }

    item.addEventListener('click', () => selectMod(mod.modId));
    modListEl.appendChild(item);
  });
}

async function loadModList(selectModId) {
  const bridge = getBridge();
  if (!bridge) {
    setStatus('error', 'Bridge unavailable. Restart the app.');
    return;
  }

  try {
    log('Loading mod list');
    const response = await bridge.list();
    log('Mod list response:', JSON.stringify(response));
    if (!response || !response.success) {
      setStatus('error', response?.error || 'Failed to load mods');
      return;
    }
    mods = response.data || [];
    log('Mod list loaded', { count: mods.length });
    renderModList(searchInputEl?.value || '');

    // Auto-select if requested
    if (selectModId && mods.some(m => m.modId === selectModId)) {
      selectMod(selectModId);
    }
  } catch (err) {
    logError('Failed to load mod list', err);
    setStatus('error', err.message || 'Failed to load mods');
  }
}

// ─── Mod Selection & Template Rendering ───

async function selectMod(modId) {
  const bridge = getBridge();
  if (!bridge) return;

  setStatus('info', 'Loading...');
  log('selectMod called', { modId });
  try {
    const response = await bridge.get(modId);
    log('selectMod response:', { modId, success: response?.success, hasTemplate: !!response?.data?.template, templateLen: response?.data?.template?.length });
    if (!response || !response.success) {
      setStatus('error', response?.error || 'Failed to load mod');
      return;
    }

    currentMod = response.data;
    headerModEl.textContent = currentMod.modName || currentMod.modId;
    reloadBtn.disabled = false;

    // Meta tags — show author if present
    if (metaBlock) {
      const hasAuthor = currentMod.author && currentMod.author.length > 0;
      metaBlock.style.display = hasAuthor ? 'flex' : 'none';
      metaAuthor.textContent = hasAuthor ? currentMod.author : '';
      metaAuthor.style.display = hasAuthor ? '' : 'none';
    }

    renderModList(searchInputEl?.value || '');
    renderTemplate(currentMod);
    setStatus('success', 'Loaded');
  } catch (err) {
    logError('Failed to select mod', err);
    setStatus('error', err.message);
  }
}

function renderTemplate(mod) {
  // Remove existing iframe and revoke any un-revoked blob URL
  if (currentIframe) {
    if (currentIframe._blobUrl) {
      URL.revokeObjectURL(currentIframe._blobUrl);
    }
    currentIframe.remove();
    currentIframe = null;
  }

  // Hide placeholder
  if (placeholderEl) placeholderEl.style.display = 'none';

  if (!mod || !mod.template) {
    if (placeholderEl) placeholderEl.style.display = '';
    return;
  }

  // Build the template HTML with the bridge script injected
  const bridgeScript = buildBridgeScript(mod.modId, mod.globals || []);
  const html = injectBridgeIntoTemplate(mod.template, bridgeScript);

  // Create a blob URL for the iframe
  const blob = new Blob([html], { type: 'text/html' });
  const blobUrl = URL.createObjectURL(blob);

  const iframe = document.createElement('iframe');
  iframe.src = blobUrl;
  iframe.sandbox = 'allow-scripts allow-forms';
  iframe.setAttribute('aria-label', `Settings for ${mod.modName || mod.modId}`);

  // Track blob URL for cleanup; revoke after load or when replaced
  iframe._blobUrl = blobUrl;
  iframe.addEventListener('load', () => {
    URL.revokeObjectURL(blobUrl);
    iframe._blobUrl = null;
  });

  editorContainer.appendChild(iframe);
  currentIframe = iframe;
  log('Template rendered in iframe', { modId: mod.modId });
}

/**
 * Build the bridge script that is injected into the mod template's HTML.
 * This provides a `window.UF` API the template can use to load/save globals.
 */
function buildBridgeScript(modId, globals) {
  // This script runs INSIDE the iframe
  return `
<script>
(function() {
  'use strict';

  var _pendingRequests = {};
  var _requestCounter = 0;

  function _generateRequestId() {
    return 'req_' + (++_requestCounter) + '_' + Date.now();
  }

  function _sendRequest(type, payload) {
    return new Promise(function(resolve, reject) {
      var requestId = _generateRequestId();
      _pendingRequests[requestId] = { resolve: resolve, reject: reject };
      var msg = Object.assign({ type: type, requestId: requestId }, payload || {});
      window.parent.postMessage(msg, '*');

      // Timeout after 30s
      setTimeout(function() {
        if (_pendingRequests[requestId]) {
          delete _pendingRequests[requestId];
          reject(new Error('Request timed out: ' + type));
        }
      }, 30000);
    });
  }

  // Listen for responses from parent
  window.addEventListener('message', function(event) {
    var data = event.data;
    if (!data || typeof data.type !== 'string') return;

    // Handle responses
    if (data.requestId && _pendingRequests[data.requestId]) {
      var pending = _pendingRequests[data.requestId];
      delete _pendingRequests[data.requestId];
      if (data.success) {
        pending.resolve(data.data !== undefined ? data.data : true);
      } else {
        pending.reject(new Error(data.error || 'Unknown error'));
      }
      return;
    }

    // Handle init message
    if (data.type === 'modSettings:init') {
      if (typeof window.onUFReady === 'function') {
        window.onUFReady(data);
      }
    }
  });

  /**
   * UF Bridge API — available inside mod settings templates.
   *
   * @namespace UF
   * @property {string} modId - The mod's unique identifier
   * @property {string[]} globals - Global names this mod declared
   */
  window.UF = {
    modId: ${JSON.stringify(modId)},
    globals: ${JSON.stringify(globals)},

    /**
     * Load a global's JSON data from the database.
     * @param {string} globalName - The global name (e.g. "MyMod_Config")
     * @returns {Promise<object|null>} The global data or null if not found
     */
    loadGlobal: function(globalName) {
      return _sendRequest('modSettings:loadGlobal', { globalName: globalName });
    },

    /**
     * Save JSON data to a global in the database.
     * @param {string} globalName - The global name
     * @param {object} data - The data to save
     * @returns {Promise<boolean>} true on success
     */
    saveGlobal: function(globalName, data) {
      return _sendRequest('modSettings:saveGlobal', { globalName: globalName, data: data });
    },

    /**
     * Notify the host that the template is ready.
     * Call this after your template has finished initializing.
     */
    ready: function() {
      window.parent.postMessage({ type: 'modSettings:ready' }, '*');
    }
  };

  // ─── External Link Interception ───
  // Intercept clicks on <a> tags with external URLs and forward them
  // to the parent renderer for confirmation + default browser opening.
  document.addEventListener('click', function(e) {
    var anchor = e.target.closest ? e.target.closest('a[href]') : null;
    if (!anchor) {
      // Fallback for browsers without .closest
      var el = e.target;
      while (el && el.tagName !== 'A') el = el.parentElement;
      if (el && el.href) anchor = el;
    }
    if (!anchor) return;

    var href = anchor.getAttribute('href') || '';
    // Only intercept http/https links (skip anchors, javascript:, etc.)
    var lc = href.toLowerCase();
    if (lc.indexOf('http://') === 0 || lc.indexOf('https://') === 0) {
      e.preventDefault();
      e.stopPropagation();
      window.parent.postMessage({ type: 'modSettings:openLink', url: href }, '*');
    }
  }, true);

  // Auto-fire ready on DOMContentLoaded if not already fired
  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', function() { window.UF.ready(); });
  } else {
    window.UF.ready();
  }
})();
<\/script>`;
}

/**
 * Inject the bridge script into the template HTML.
 * Inserts before </head> if present, otherwise before </body> or at the end.
 */
function injectBridgeIntoTemplate(template, bridgeScript) {
  if (template.includes('</head>')) {
    return template.replace('</head>', bridgeScript + '\n</head>');
  }
  if (template.includes('</body>')) {
    return template.replace('</body>', bridgeScript + '\n</body>');
  }
  return template + '\n' + bridgeScript;
}

// ─── Save Confirmation Modal ───

const saveConfirmOverlay = document.getElementById('saveConfirmOverlay');
const saveConfirmDesc = document.getElementById('saveConfirmDesc');
const saveBatchList = document.getElementById('saveBatchList');
const saveConfirmOk = document.getElementById('saveConfirmOk');
const saveConfirmCancel = document.getElementById('saveConfirmCancel');

// ─── Batch Save Confirmation ───
// When a template fires multiple saveGlobal() calls (e.g. from a saveAll()),
// we collect them over a short debounce window, then show ONE modal listing
// all the globals with expandable JSON inspection.

let _saveBatch = [];          // { globalName, data, resolve }
let _saveBatchTimer = null;
const SAVE_BATCH_DELAY = 300; // ms to wait for more saves before showing modal

/**
 * Queue a save for batch confirmation.
 * Returns a Promise<boolean> — true if operator confirmed, false if cancelled.
 */
function confirmSave(globalName, data) {
  return new Promise((resolve) => {
    _saveBatch.push({ globalName, data, resolve });
    // Reset the debounce timer each time a new save arrives
    if (_saveBatchTimer) clearTimeout(_saveBatchTimer);
    _saveBatchTimer = setTimeout(_showBatchSaveModal, SAVE_BATCH_DELAY);
  });
}

/** Build and show the batch modal with all queued saves. */
function _showBatchSaveModal() {
  _saveBatchTimer = null;
  if (_saveBatch.length === 0) return;

  const count = _saveBatch.length;
  const modLabel = currentMod?.modName || currentMod?.modId || 'A mod';

  saveConfirmDesc.textContent = count === 1
    ? `${modLabel} wants to save 1 global:`
    : `${modLabel} wants to save ${count} globals:`;

  // Build the expandable list
  saveBatchList.innerHTML = '';
  _saveBatch.forEach((item, idx) => {
    const card = document.createElement('div');
    card.className = 'batch-item';

    // Compute size label
    let jsonStr;
    try { jsonStr = JSON.stringify(item.data, null, 2); } catch { jsonStr = String(item.data); }
    const sizeKB = (new Blob([jsonStr]).size / 1024).toFixed(1);

    // Header row (click to expand)
    const header = document.createElement('div');
    header.className = 'batch-item-header';
    header.innerHTML = `<span class="batch-item-arrow">&#9654;</span>`
      + `<span class="batch-item-name">${_escHtml(item.globalName)}</span>`
      + `<span class="batch-item-size">${sizeKB} KB</span>`;
    header.addEventListener('click', () => card.classList.toggle('expanded'));
    card.appendChild(header);

    // Body (JSON preview)
    const body = document.createElement('div');
    body.className = 'batch-item-body';
    const pre = document.createElement('pre');
    pre.className = 'batch-item-json';
    pre.textContent = jsonStr;
    body.appendChild(pre);
    card.appendChild(body);

    saveBatchList.appendChild(card);
  });

  saveConfirmOverlay.style.display = 'flex';
}

function _escHtml(s) {
  const d = document.createElement('div');
  d.textContent = s || '';
  return d.innerHTML;
}

function closeBatchSave(approved) {
  saveConfirmOverlay.style.display = 'none';
  // Resolve every promise in the batch with the same result
  const batch = _saveBatch.slice();
  _saveBatch = [];
  for (const item of batch) {
    item.resolve(approved);
  }
}

saveConfirmOk?.addEventListener('click', () => closeBatchSave(true));
saveConfirmCancel?.addEventListener('click', () => closeBatchSave(false));

// Close on Escape key
saveConfirmOverlay?.addEventListener('keydown', (e) => {
  if (e.key === 'Escape') closeBatchSave(false);
});

// Close on overlay background click
saveConfirmOverlay?.addEventListener('click', (e) => {
  if (e.target === saveConfirmOverlay) closeBatchSave(false);
});

// ─── External Link Confirmation Modal ───

const linkConfirmOverlay = document.getElementById('linkConfirmOverlay');
const linkConfirmUrl = document.getElementById('linkConfirmUrl');
const linkConfirmOk = document.getElementById('linkConfirmOk');
const linkConfirmCancel = document.getElementById('linkConfirmCancel');

let _pendingLinkResolve = null;

/**
 * Show a confirmation modal before opening an external link.
 * Returns a Promise<boolean> — true if operator approved.
 */
function confirmOpenLink(url) {
  return new Promise((resolve) => {
    linkConfirmUrl.textContent = url;
    linkConfirmUrl.title = url;
    _pendingLinkResolve = resolve;
    linkConfirmOverlay.style.display = 'flex';
  });
}

function closeLinkConfirm(result) {
  linkConfirmOverlay.style.display = 'none';
  if (_pendingLinkResolve) {
    const resolve = _pendingLinkResolve;
    _pendingLinkResolve = null;
    resolve(result);
  }
}

linkConfirmOk?.addEventListener('click', () => closeLinkConfirm(true));
linkConfirmCancel?.addEventListener('click', () => closeLinkConfirm(false));

linkConfirmOverlay?.addEventListener('keydown', (e) => {
  if (e.key === 'Escape') closeLinkConfirm(false);
});

linkConfirmOverlay?.addEventListener('click', (e) => {
  if (e.target === linkConfirmOverlay) closeLinkConfirm(false);
});

// ─── postMessage Handler (iframe → parent) ───

window.addEventListener('message', async (event) => {
  const data = event.data;
  if (!data || typeof data.type !== 'string') return;
  if (!data.type.startsWith('modSettings:')) return;

  log('postMessage received from iframe:', { type: data.type, requestId: data.requestId, globalName: data.globalName });

  const bridge = getBridge();
  if (!bridge) {
    logWarn('Bridge unavailable when handling iframe message', { type: data.type });
    sendToIframe(event.source, data.type + ':response', data.requestId, false, null, 'Bridge unavailable');
    return;
  }

  try {
    switch (data.type) {
      case 'modSettings:loadGlobal': {
        log('iframe requested loadGlobal', { globalName: data.globalName });
        const result = await bridge.loadGlobal(data.globalName);
        log('loadGlobal IPC result:', JSON.stringify(result));
        if (result && result.success) {
          // Unwrap the global document — send just the .data payload so templates
          // receive their config object directly instead of { id, mod, data }.
          const payload = result.data?.data ?? null;
          log('loadGlobal sending to iframe:', { requestId: data.requestId, hasPayload: payload !== null });
          sendToIframe(event.source, data.type + ':response', data.requestId, true, payload);
        } else {
          logWarn('loadGlobal failed', { globalName: data.globalName, error: result?.error });
          sendToIframe(event.source, data.type + ':response', data.requestId, false, null, result?.error || 'Load failed');
        }
        break;
      }

      case 'modSettings:saveGlobal': {
        log('iframe requested saveGlobal', { globalName: data.globalName });

        // Show confirmation modal before writing
        const confirmed = await confirmSave(data.globalName, data.data);
        if (!confirmed) {
          log('saveGlobal cancelled by operator', { globalName: data.globalName });
          sendToIframe(event.source, data.type + ':response', data.requestId, false, null, 'Save cancelled by operator');
          setStatus('warning', `Save cancelled for ${data.globalName}`);
          break;
        }

        const result = await bridge.saveGlobal(data.globalName, data.data);
        if (result && result.success) {
          sendToIframe(event.source, data.type + ':response', data.requestId, true);
          setStatus('success', `Saved ${data.globalName}`);
        } else {
          sendToIframe(event.source, data.type + ':response', data.requestId, false, null, result?.error || 'Save failed');
          setStatus('error', `Failed to save ${data.globalName}`);
        }
        break;
      }

      case 'modSettings:ready': {
        log('iframe reports ready', { modId: currentMod?.modId });
        // Send init data to the iframe
        if (event.source && currentMod) {
          event.source.postMessage({
            type: 'modSettings:init',
            modId: currentMod.modId,
            globals: currentMod.globals || [],
          }, '*');
        }
        break;
      }

      case 'modSettings:openLink': {
        const url = data.url;
        log('iframe requested openLink', { url });
        if (!url || typeof url !== 'string' || !url.match(/^https?:\/\//i)) {
          logWarn('openLink blocked — invalid URL', { url });
          break;
        }
        // Show confirmation before opening
        const approved = await confirmOpenLink(url);
        if (approved) {
          log('openLink approved by operator', { url });
          try {
            await bridge.openExternal(url);
          } catch (err) {
            logError('openExternal failed', err);
          }
        } else {
          log('openLink cancelled by operator', { url });
        }
        break;
      }

      default:
        logWarn('Unknown message type from iframe', data.type);
    }
  } catch (err) {
    logError('Error handling iframe message', err);
    sendToIframe(event.source, data.type + ':response', data.requestId, false, null, err.message);
  }
});

function sendToIframe(source, type, requestId, success, data, error) {
  if (!source) {
    logWarn('sendToIframe: source is null/undefined, cannot send response', { type, requestId });
    return;
  }
  const msg = { type, requestId, success };
  if (data !== undefined && data !== null) msg.data = data;
  if (error) msg.error = error;
  try {
    log('sendToIframe:', { type, requestId, success, hasData: data !== undefined && data !== null, error: error || null });
    source.postMessage(msg, '*');
  } catch (err) {
    logError('Failed to send message to iframe', err);
  }
}

// ─── Event Listeners ───

searchInputEl?.addEventListener('input', () => {
  renderModList(searchInputEl.value);
});

refreshListBtn?.addEventListener('click', () => {
  const selectedId = currentMod?.modId;
  loadModList(selectedId);
});

reloadBtn?.addEventListener('click', () => {
  if (currentMod) {
    selectMod(currentMod.modId);
  }
});

// ─── Init ───

log('Initializing...');
loadModList();
