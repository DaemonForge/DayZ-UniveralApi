let mods = [];
let currentDeleteMod = null;

const LOG_PREFIX = '[ModManagerRenderer]';

const modGridEl = document.getElementById('modGrid');
const searchInputEl = document.getElementById('searchInput');
const statusEl = document.getElementById('statusMessage');
const refreshBtn = document.getElementById('refreshBtn');

const confirmModal = document.getElementById('confirmModal');
const confirmModNameEl = document.getElementById('confirmModName');
const confirmDataSummaryEl = document.getElementById('confirmDataSummary');
const confirmInput = document.getElementById('confirmInput');
const cancelBtn = document.getElementById('cancelBtn');
const confirmDeleteBtn = document.getElementById('confirmDeleteBtn');

const progressModal = document.getElementById('progressModal');
const progressText = document.getElementById('progressText');

const resultsModal = document.getElementById('resultsModal');
const resultsTitle = document.getElementById('resultsTitle');
const resultsContent = document.getElementById('resultsContent');
const closeResultsBtn = document.getElementById('closeResultsBtn');

if (!modGridEl || !searchInputEl || !statusEl || !refreshBtn) {
  console.error(LOG_PREFIX, 'One or more required DOM elements are missing');
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
  if (!window.modManagerApi) {
    logWarn('modManagerApi bridge not present on window');
  }
  return window.modManagerApi;
}

function setStatus(type, message) {
  if (!statusEl) {
    console.error(LOG_PREFIX, 'Status element missing', { type, message });
    return;
  }
  statusEl.textContent = message;
  statusEl.dataset.type = type;
  log('Status updated', { type, message });
}

function formatNumber(num) {
  return num.toLocaleString();
}

function renderModGrid(filter = '') {
  const query = filter.trim().toLowerCase();
  
  if (!Array.isArray(mods)) {
    logError('mods is not an array', typeof mods);
    modGridEl.innerHTML = '<div class="error">Failed to load mods</div>';
    return;
  }

  const filtered = query
    ? mods.filter(m => m.modName.toLowerCase().includes(query))
    : mods;

  if (filtered.length === 0) {
    modGridEl.innerHTML = query
      ? '<div class="empty-state">No mods match your search</div>'
      : '<div class="empty-state">No mods found<br><small>No mod data detected in the database</small></div>';
    return;
  }

  modGridEl.innerHTML = filtered
    .map(mod => {
      const collectionEntries = Object.entries(mod.collections || {});
      const collectionsHtml = collectionEntries
        .map(([collection, count]) => `
          <div class="collection-row">
            <span class="collection-name">${collection}</span>
            <span class="collection-count">${formatNumber(count)}</span>
          </div>
        `)
        .join('');

      return `
        <div class="mod-card" data-mod="${escapeHtml(mod.modName)}">
          <div class="mod-card-header">
            <h3 class="mod-name">${escapeHtml(mod.modName)}</h3>
            <div class="mod-total">${formatNumber(mod.totalDocuments)} documents</div>
          </div>
          <div class="mod-card-body">
            <div class="collections-list">
              ${collectionsHtml}
            </div>
          </div>
          <div class="mod-card-footer">
            <button class="btn danger delete-btn" data-mod="${escapeHtml(mod.modName)}">
              🗑️ Delete All Data
            </button>
          </div>
        </div>
      `;
    })
    .join('');

  // Attach click handlers to delete buttons (stop propagation so card click doesn't fire)
  document.querySelectorAll('.delete-btn').forEach(btn => {
    btn.addEventListener('click', (e) => {
      e.stopPropagation();
      const modName = e.target.getAttribute('data-mod');
      showDeleteConfirmation(modName);
    });
  });

  // Attach click handlers to mod cards for viewing details
  document.querySelectorAll('.mod-card').forEach(card => {
    card.addEventListener('click', (e) => {
      const modName = card.getAttribute('data-mod');
      showModDetails(modName);
    });
  });

  log('Rendered mod grid', { total: mods.length, filtered: filtered.length });
}

function escapeHtml(text) {
  const div = document.createElement('div');
  div.textContent = text;
  return div.innerHTML;
}

function showModDetails(modName) {
  const modData = mods.find(m => m.modName === modName);
  if (!modData) {
    logError('Mod not found for details', modName);
    return;
  }

  const collectionsHtml = Object.entries(modData.collections || {})
    .map(([collection, count]) => `
      <div class="detail-row">
        <span class="detail-label">${collection}</span>
        <span class="detail-value">${formatNumber(count)} ${count === 1 ? 'document' : 'documents'}</span>
      </div>
    `)
    .join('');

  resultsTitle.textContent = `📊 ${escapeHtml(modName)}`;
  resultsContent.innerHTML = `
    <div class="mod-details-view">
      <div class="detail-total">
        <strong>Total Documents: ${formatNumber(modData.totalDocuments)}</strong>
      </div>
      <div class="detail-collections">
        <h4>Data Distribution:</h4>
        ${collectionsHtml}
      </div>
      <div class="detail-info">
        <p>💡 Click the delete button on the card to remove all data for this mod.</p>
      </div>
    </div>
  `;
  
  resultsModal.style.display = 'flex';
  log('Showing details for', modName);
}

function showDeleteConfirmation(modName) {
  const modData = mods.find(m => m.modName === modName);
  if (!modData) {
    logError('Mod not found for deletion', modName);
    return;
  }

  currentDeleteMod = modData;
  
  confirmModNameEl.textContent = modName;
  
  const collectionsHtml = Object.entries(modData.collections || {})
    .map(([collection, count]) => `
      <div class="summary-row">
        <span class="summary-label">${collection}:</span>
        <span class="summary-value">${formatNumber(count)} ${count === 1 ? 'document' : 'documents'}</span>
      </div>
    `)
    .join('');
  
  confirmDataSummaryEl.innerHTML = `
    <div class="summary-total">
      <strong>Total: ${formatNumber(modData.totalDocuments)} ${modData.totalDocuments === 1 ? 'document' : 'documents'}</strong>
    </div>
    <div class="summary-details">
      ${collectionsHtml}
    </div>
  `;

  confirmInput.value = '';
  confirmDeleteBtn.disabled = true;
  
  confirmModal.style.display = 'flex';
  setTimeout(() => confirmInput.focus(), 100);
  
  log('Showing delete confirmation for', modName);
}

function hideConfirmModal() {
  confirmModal.style.display = 'none';
  currentDeleteMod = null;
  confirmInput.value = '';
  confirmDeleteBtn.disabled = true;
}

function showProgressModal() {
  progressModal.style.display = 'flex';
  progressText.textContent = 'Deleting mod data...';
}

function hideProgressModal() {
  progressModal.style.display = 'none';
}

function showResultsModal(success, summary) {
  if (success) {
    resultsTitle.textContent = '✅ Deletion Complete';
    
    const collectionsHtml = Object.entries(summary.collections || {})
      .filter(([, count]) => count > 0)
      .map(([collection, count]) => `
        <div class="result-row">
          <span class="result-label">${collection}:</span>
          <span class="result-value">${formatNumber(count)} deleted</span>
        </div>
      `)
      .join('');

    const errorsHtml = (summary.errors && summary.errors.length > 0)
      ? `
        <div class="result-errors">
          <h4>⚠️ Errors:</h4>
          ${summary.errors.map(err => `
            <div class="error-item">
              <strong>${err.collection}:</strong> ${err.error}
            </div>
          `).join('')}
        </div>
      `
      : '';

    resultsContent.innerHTML = `
      <div class="result-summary">
        <p class="result-mod-name">Deleted data for: <strong>${escapeHtml(summary.modName)}</strong></p>
        <div class="result-total">
          <strong>Total Deleted: ${formatNumber(summary.totalDeleted)} ${summary.totalDeleted === 1 ? 'document' : 'documents'}</strong>
        </div>
        <div class="result-details">
          ${collectionsHtml}
        </div>
        ${errorsHtml}
      </div>
    `;
  } else {
    resultsTitle.textContent = '❌ Deletion Failed';
    resultsContent.innerHTML = `
      <div class="result-error">
        <p>${escapeHtml(summary.error || 'Unknown error occurred')}</p>
      </div>
    `;
  }
  
  resultsModal.style.display = 'flex';
}

function hideResultsModal() {
  resultsModal.style.display = 'none';
}

async function loadMods() {
  const bridge = getBridge();
  if (!bridge) {
    setStatus('error', 'Bridge not available');
    return;
  }

  setStatus('info', 'Loading mods...');
  log('Requesting mod list...');

  try {
    const result = await bridge.listMods();
    log('List result received', result);

    if (!result.success) {
      throw new Error(result.error || 'Failed to load mods');
    }

    mods = result.data || [];
    renderModGrid();
    
    if (mods.length === 0) {
      setStatus('info', 'No mods found');
    } else {
      setStatus('success', `Found ${mods.length} ${mods.length === 1 ? 'mod' : 'mods'}`);
    }
  } catch (err) {
    logError('Failed to load mods', err);
    setStatus('error', `Failed to load: ${err.message}`);
    modGridEl.innerHTML = `<div class="error">Failed to load mods: ${escapeHtml(err.message)}</div>`;
  }
}

async function deleteMod(modName) {
  const bridge = getBridge();
  if (!bridge) {
    logError('Bridge not available');
    return;
  }

  hideConfirmModal();
  showProgressModal();
  
  log('Deleting mod data for', modName);

  try {
    const result = await bridge.deleteMod(modName);
    log('Delete result received', result);

    hideProgressModal();

    if (!result.success) {
      throw new Error(result.error || 'Failed to delete mod data');
    }

    showResultsModal(true, result.data);
    
    // Reload mods after deletion
    await loadMods();
  } catch (err) {
    logError('Failed to delete mod', err);
    hideProgressModal();
    showResultsModal(false, { error: err.message });
  }
}

// Event Listeners
refreshBtn.addEventListener('click', () => {
  log('Refresh button clicked');
  loadMods();
});

searchInputEl.addEventListener('input', (e) => {
  renderModGrid(e.target.value);
});

confirmInput.addEventListener('input', (e) => {
  const input = e.target.value.trim();
  const matches = currentDeleteMod && input === currentDeleteMod.modName;
  confirmDeleteBtn.disabled = !matches;
  log('Confirm input changed', { input, matches });
});

confirmInput.addEventListener('keydown', (e) => {
  if (e.key === 'Enter' && !confirmDeleteBtn.disabled) {
    confirmDeleteBtn.click();
  } else if (e.key === 'Escape') {
    cancelBtn.click();
  }
});

cancelBtn.addEventListener('click', () => {
  log('Cancel button clicked');
  hideConfirmModal();
});

confirmDeleteBtn.addEventListener('click', () => {
  if (currentDeleteMod) {
    log('Confirm delete button clicked for', currentDeleteMod.modName);
    deleteMod(currentDeleteMod.modName);
  }
});

closeResultsBtn.addEventListener('click', () => {
  log('Close results button clicked');
  hideResultsModal();
});

// Close modals when clicking outside
confirmModal.addEventListener('click', (e) => {
  if (e.target === confirmModal) {
    hideConfirmModal();
  }
});

resultsModal.addEventListener('click', (e) => {
  if (e.target === resultsModal) {
    hideResultsModal();
  }
});

// Load mods on init
log('Initializing mod manager renderer');
loadMods();
