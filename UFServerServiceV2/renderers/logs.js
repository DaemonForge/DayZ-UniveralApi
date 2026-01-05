// Log Viewer Renderer
// Handles UI interactions and data display

let currentPage = 1;
let pageSize = 50;
let totalLogs = 0;
let totalPages = 1;
let currentFilters = {};
let liveMode = true;
let refreshInterval = null;

// DOM Elements
const elements = {
  searchInput: null,
  serverFilter: null,
  levelInfo: null,
  levelWarn: null,
  levelError: null,
  levelDebug: null,
  typeServer: null,
  typeClient: null,
  dateFrom: null,
  dateTo: null,
  refreshBtn: null,
  clearFiltersBtn: null,
  liveMode: null,
  exportBtn: null,
  logTableBody: null,
  totalLogs: null,
  infoCount: null,
  warnCount: null,
  errorCount: null,
  showingStart: null,
  showingEnd: null,
  showingTotal: null,
  currentPage: null,
  totalPages: null,
  prevPage: null,
  nextPage: null,
  pageSize: null,
  detailModal: null,
  closeModal: null,
  logDetailContent: null,
  loadingOverlay: null
};

// Initialize
document.addEventListener('DOMContentLoaded', () => {
  initElements();
  initEventListeners();
  loadServers();
  loadLogs();
  startLiveUpdates();
});

function initElements() {
  elements.searchInput = document.getElementById('searchInput');
  elements.serverFilter = document.getElementById('serverFilter');
  elements.levelInfo = document.getElementById('levelInfo');
  elements.levelWarn = document.getElementById('levelWarn');
  elements.levelError = document.getElementById('levelError');
  elements.levelDebug = document.getElementById('levelDebug');
  elements.typeServer = document.getElementById('typeServer');
  elements.typeClient = document.getElementById('typeClient');
  elements.dateFrom = document.getElementById('dateFrom');
  elements.dateTo = document.getElementById('dateTo');
  elements.refreshBtn = document.getElementById('refreshBtn');
  elements.clearFiltersBtn = document.getElementById('clearFiltersBtn');
  elements.liveMode = document.getElementById('liveMode');
  elements.exportBtn = document.getElementById('exportBtn');
  elements.logTableBody = document.getElementById('logTableBody');
  elements.totalLogs = document.getElementById('totalLogs');
  elements.infoCount = document.getElementById('infoCount');
  elements.warnCount = document.getElementById('warnCount');
  elements.errorCount = document.getElementById('errorCount');
  elements.showingStart = document.getElementById('showingStart');
  elements.showingEnd = document.getElementById('showingEnd');
  elements.showingTotal = document.getElementById('showingTotal');
  elements.currentPage = document.getElementById('currentPage');
  elements.totalPages = document.getElementById('totalPages');
  elements.prevPage = document.getElementById('prevPage');
  elements.nextPage = document.getElementById('nextPage');
  elements.pageSize = document.getElementById('pageSize');
  elements.detailModal = document.getElementById('detailModal');
  elements.closeModal = document.getElementById('closeModal');
  elements.logDetailContent = document.getElementById('logDetailContent');
  elements.loadingOverlay = document.getElementById('loadingOverlay');
}

function initEventListeners() {
  // Search with debounce
  let searchTimeout;
  elements.searchInput.addEventListener('input', () => {
    clearTimeout(searchTimeout);
    searchTimeout = setTimeout(() => {
      currentPage = 1;
      loadLogs();
    }, 300);
  });

  // Filters
  elements.serverFilter.addEventListener('change', () => {
    currentPage = 1;
    loadLogs();
  });

  [elements.levelInfo, elements.levelWarn, elements.levelError, elements.levelDebug,
   elements.typeServer, elements.typeClient].forEach(el => {
    el.addEventListener('change', () => {
      currentPage = 1;
      loadLogs();
    });
  });

  elements.dateFrom.addEventListener('change', () => {
    currentPage = 1;
    loadLogs();
  });

  elements.dateTo.addEventListener('change', () => {
    currentPage = 1;
    loadLogs();
  });

  // Quick date buttons
  document.querySelectorAll('.quick-date-btn').forEach(btn => {
    btn.addEventListener('click', () => {
      document.querySelectorAll('.quick-date-btn').forEach(b => b.classList.remove('active'));
      btn.classList.add('active');
      setDateRange(btn.dataset.range);
      currentPage = 1;
      loadLogs();
    });
  });

  // Actions
  elements.refreshBtn.addEventListener('click', loadLogs);
  elements.clearFiltersBtn.addEventListener('click', clearFilters);
  elements.exportBtn.addEventListener('click', exportLogs);

  // Live mode toggle
  elements.liveMode.addEventListener('change', (e) => {
    liveMode = e.target.checked;
    if (liveMode) {
      startLiveUpdates();
    } else {
      stopLiveUpdates();
    }
  });

  // Pagination
  elements.prevPage.addEventListener('click', () => {
    if (currentPage > 1) {
      currentPage--;
      loadLogs();
    }
  });

  elements.nextPage.addEventListener('click', () => {
    if (currentPage < totalPages) {
      currentPage++;
      loadLogs();
    }
  });

  elements.pageSize.addEventListener('change', (e) => {
    pageSize = parseInt(e.target.value);
    currentPage = 1;
    loadLogs();
  });

  // Modal
  elements.closeModal.addEventListener('click', closeDetailModal);
  elements.detailModal.addEventListener('click', (e) => {
    if (e.target === elements.detailModal) {
      closeDetailModal();
    }
  });

  // Event delegation for details buttons (avoids inline onclick which is blocked by CSP)
  elements.logTableBody.addEventListener('click', (e) => {
    const detailsBtn = e.target.closest('.details-btn');
    if (detailsBtn) {
      const row = detailsBtn.closest('tr');
      const logId = row?.dataset?.logId;
      if (logId) {
        showLogDetails(logId);
      }
    }
  });

  // Keyboard shortcuts
  document.addEventListener('keydown', (e) => {
    if (e.key === 'Escape') {
      closeDetailModal();
    }
    if (e.key === 'r' && (e.ctrlKey || e.metaKey)) {
      e.preventDefault();
      loadLogs();
    }
  });

  // Live log updates
  window.logsApi.onNewLog((event, log) => {
    if (liveMode && currentPage === 1) {
      prependLog(log);
      updateStats();
    }
  });
}

function setDateRange(range) {
  const now = new Date();
  let from = null;

  switch (range) {
    case '1h':
      from = new Date(now.getTime() - 60 * 60 * 1000);
      break;
    case '6h':
      from = new Date(now.getTime() - 6 * 60 * 60 * 1000);
      break;
    case '24h':
      from = new Date(now.getTime() - 24 * 60 * 60 * 1000);
      break;
    case '7d':
      from = new Date(now.getTime() - 7 * 24 * 60 * 60 * 1000);
      break;
    case 'all':
    default:
      elements.dateFrom.value = '';
      elements.dateTo.value = '';
      return;
  }

  elements.dateFrom.value = formatDateForInput(from);
  elements.dateTo.value = formatDateForInput(now);
}

function formatDateForInput(date) {
  return date.toISOString().slice(0, 16);
}

function buildFilters() {
  const filters = {
    page: currentPage,
    limit: pageSize
  };

  // Search
  const search = elements.searchInput.value.trim();
  if (search) {
    filters.search = search;
  }

  // Server
  const server = elements.serverFilter.value;
  if (server) {
    filters.serverId = server;
  }

  // Levels - include both uppercase (DayZ mod) and lowercase (service logger) variants
  const levels = [];
  if (elements.levelInfo.checked) levels.push('info', 'INFO', 'VERBOSE');
  if (elements.levelWarn.checked) levels.push('warn', 'warning', 'WARN', 'WARNING');
  if (elements.levelError.checked) levels.push('error', 'ERROR');
  if (elements.levelDebug.checked) levels.push('debug', 'DEBUG');
  if (levels.length > 0 && levels.length < 12) {
    filters.levels = levels;
  }

  // Client types
  const types = [];
  if (elements.typeServer.checked) types.push('Server');
  if (elements.typeClient.checked) types.push('Client');
  if (types.length === 1) {
    filters.clientType = types[0];
  }

  // Date range
  if (elements.dateFrom.value) {
    filters.dateFrom = new Date(elements.dateFrom.value).toISOString();
  }
  if (elements.dateTo.value) {
    filters.dateTo = new Date(elements.dateTo.value).toISOString();
  }

  currentFilters = filters;
  return filters;
}

async function loadServers() {
  try {
    const servers = await window.logsApi.getServers();
    elements.serverFilter.innerHTML = '<option value="">All Servers</option>';
    servers.forEach(server => {
      const option = document.createElement('option');
      option.value = server;
      option.textContent = server;
      elements.serverFilter.appendChild(option);
    });
  } catch (error) {
    console.error('Failed to load servers:', error);
  }
}

async function loadLogs() {
  showLoading();
  
  try {
    const filters = buildFilters();
    const result = await window.logsApi.query(filters);
    
    totalLogs = result.total;
    totalPages = Math.ceil(totalLogs / pageSize) || 1;
    
    renderLogs(result.logs);
    updatePagination();
    await updateStats();
  } catch (error) {
    console.error('Failed to load logs:', error);
    showError('Failed to load logs');
  } finally {
    hideLoading();
  }
}

function renderLogs(logs) {
  if (!logs || logs.length === 0) {
    elements.logTableBody.innerHTML = `
      <tr>
        <td colspan="6">
          <div class="empty-state">
            <div class="empty-state-icon">📋</div>
            <div class="empty-state-text">No logs found</div>
            <div class="empty-state-subtext">Try adjusting your filters or check back later</div>
          </div>
        </td>
      </tr>
    `;
    return;
  }

  const searchTerm = elements.searchInput.value.trim().toLowerCase();
  
  elements.logTableBody.innerHTML = logs.map(log => {
    const timestamp = formatTimestamp(log.LoggedDateTime);
    const level = (log.Level || log.level || 'info').toLowerCase();
    const serverId = log.ServerId || '-';
    const clientType = log.ClientType || '-';
    // Support both service logs (Message) and DayZ mod logs (Log field + context)
    const message = log.Message || log.message || formatDayZLog(log);
    // Convert ObjectId to string - handles both string and ObjectId object formats
    const logId = typeof log._id === 'string' ? log._id : (log._id?.toString?.() || log._id?.$oid || String(log._id));
    
    // Highlight search matches
    const displayMessage = searchTerm 
      ? highlightText(message, searchTerm) 
      : escapeHtml(message);
    
    return `
      <tr data-log-id="${logId}">
        <td class="timestamp">${timestamp}</td>
        <td><span class="level-badge ${level}">${level}</span></td>
        <td class="server-id">${escapeHtml(serverId)}</td>
        <td><span class="type-badge ${clientType.toLowerCase()}">${clientType}</span></td>
        <td class="message-cell" title="${escapeHtml(message)}">${displayMessage}</td>
        <td>
          <button class="details-btn">View</button>
        </td>
      </tr>
    `;
  }).join('');
}

function prependLog(log) {
  const existingEmpty = elements.logTableBody.querySelector('.empty-state');
  if (existingEmpty) {
    elements.logTableBody.innerHTML = '';
  }

  const timestamp = formatTimestamp(log.LoggedDateTime);
  const level = (log.Level || log.level || 'info').toLowerCase();
  const serverId = log.ServerId || '-';
  const clientType = log.ClientType || '-';
  // Support both service logs (Message) and DayZ mod logs (Log field + context)
  const message = log.Message || log.message || formatDayZLog(log);
  // Convert ObjectId to string - handles both string and ObjectId object formats
  const logId = typeof log._id === 'string' ? log._id : (log._id?.toString?.() || log._id?.$oid || String(log._id));

  const tr = document.createElement('tr');
  tr.dataset.logId = logId;
  tr.innerHTML = `
    <td class="timestamp">${timestamp}</td>
    <td><span class="level-badge ${level}">${level}</span></td>
    <td class="server-id">${escapeHtml(serverId)}</td>
    <td><span class="type-badge ${clientType.toLowerCase()}">${clientType}</span></td>
    <td class="message-cell" title="${escapeHtml(message)}">${escapeHtml(message)}</td>
    <td>
      <button class="details-btn">View</button>
    </td>
  `;
  
  // Add highlight animation
  tr.style.backgroundColor = 'rgba(14, 99, 156, 0.3)';
  elements.logTableBody.insertBefore(tr, elements.logTableBody.firstChild);
  
  setTimeout(() => {
    tr.style.transition = 'background-color 1s ease';
    tr.style.backgroundColor = '';
  }, 100);

  // Remove oldest if over limit
  const rows = elements.logTableBody.querySelectorAll('tr');
  if (rows.length > pageSize) {
    rows[rows.length - 1].remove();
  }
  
  totalLogs++;
  updatePagination();
}

async function updateStats() {
  try {
    const statsFilters = { ...currentFilters };
    delete statsFilters.page;
    delete statsFilters.limit;
    
    const stats = await window.logsApi.getStats(statsFilters);
    
    elements.totalLogs.textContent = stats.total || 0;
    elements.infoCount.textContent = stats.info || 0;
    elements.warnCount.textContent = stats.warn || 0;
    elements.errorCount.textContent = stats.error || 0;
  } catch (error) {
    console.error('Failed to load stats:', error);
  }
}

function updatePagination() {
  const start = totalLogs === 0 ? 0 : (currentPage - 1) * pageSize + 1;
  const end = Math.min(currentPage * pageSize, totalLogs);
  
  elements.showingStart.textContent = start;
  elements.showingEnd.textContent = end;
  elements.showingTotal.textContent = totalLogs;
  elements.currentPage.textContent = currentPage;
  elements.totalPages.textContent = totalPages;
  
  elements.prevPage.disabled = currentPage <= 1;
  elements.nextPage.disabled = currentPage >= totalPages;
}

function clearFilters() {
  elements.searchInput.value = '';
  elements.serverFilter.value = '';
  elements.levelInfo.checked = true;
  elements.levelWarn.checked = true;
  elements.levelError.checked = true;
  elements.levelDebug.checked = true;
  elements.typeServer.checked = true;
  elements.typeClient.checked = true;
  elements.dateFrom.value = '';
  elements.dateTo.value = '';
  
  document.querySelectorAll('.quick-date-btn').forEach(b => b.classList.remove('active'));
  document.querySelector('.quick-date-btn[data-range="all"]').classList.add('active');
  
  currentPage = 1;
  loadLogs();
}

// Log cache for details view
let logCache = {};

window.showLogDetails = async function(logId) {
  console.log('[showLogDetails] Called with logId:', logId, 'type:', typeof logId);
  let logData = null;
  
  // Try to get from current query results
  try {
    console.log('[showLogDetails] Querying for _id:', logId);
    const result = await window.logsApi.query({ _id: logId });
    console.log('[showLogDetails] Query result:', result);
    if (result.logs && result.logs.length > 0) {
      logData = result.logs[0];
    } else if (result.error) {
      console.error('[showLogDetails] Query error:', result.error);
    } else {
      console.warn('[showLogDetails] No logs returned for ID:', logId);
    }
  } catch (e) {
    console.error('[showLogDetails] Failed to fetch log details:', e);
  }
  
  if (logData) {
    console.log('[showLogDetails] Showing modal with data');
    elements.logDetailContent.textContent = JSON.stringify(logData, null, 2);
    elements.detailModal.classList.remove('hidden');
  } else {
    console.error('[showLogDetails] No log data found for ID:', logId);
    alert('Failed to load log details. ID: ' + logId);
  }
};

function closeDetailModal() {
  elements.detailModal.classList.add('hidden');
}

async function exportLogs() {
  try {
    showLoading();
    
    // Get all logs matching current filters
    const filters = { ...currentFilters };
    filters.limit = 10000; // Max export
    filters.page = 1;
    
    const result = await window.logsApi.query(filters);
    
    // Create CSV
    const headers = ['Timestamp', 'Level', 'Server', 'Type', 'Message', 'ClientId'];
    const rows = result.logs.map(log => [
      log.LoggedDateTime,
      log.Level || log.level || '',
      log.ServerId || '',
      log.ClientType || '',
      `"${(log.Message || log.message || '').replace(/"/g, '""')}"`,
      log.ClientId || ''
    ]);
    
    const csv = [headers.join(','), ...rows.map(r => r.join(','))].join('\n');
    
    // Download
    const blob = new Blob([csv], { type: 'text/csv' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `logs-export-${new Date().toISOString().slice(0, 10)}.csv`;
    a.click();
    URL.revokeObjectURL(url);
  } catch (error) {
    console.error('Export failed:', error);
    alert('Failed to export logs');
  } finally {
    hideLoading();
  }
}

function startLiveUpdates() {
  if (refreshInterval) return;
  
  // Auto-refresh every 30 seconds when in live mode
  refreshInterval = setInterval(() => {
    if (liveMode && currentPage === 1) {
      loadLogs();
    }
  }, 30000);
}

function stopLiveUpdates() {
  if (refreshInterval) {
    clearInterval(refreshInterval);
    refreshInterval = null;
  }
}

// Utility functions
function formatTimestamp(dateStr) {
  if (!dateStr) return '-';
  const date = new Date(dateStr);
  return date.toLocaleString('en-US', {
    year: 'numeric',
    month: '2-digit',
    day: '2-digit',
    hour: '2-digit',
    minute: '2-digit',
    second: '2-digit',
    hour12: false
  });
}

function escapeHtml(text) {
  if (!text) return '';
  const div = document.createElement('div');
  div.textContent = text;
  return div.innerHTML;
}

function highlightText(text, term) {
  if (!text || !term) return escapeHtml(text);
  const escaped = escapeHtml(text);
  const regex = new RegExp(`(${escapeRegExp(term)})`, 'gi');
  return escaped.replace(regex, '<span class="highlight">$1</span>');
}

function escapeRegExp(string) {
  return string.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
}

// Format DayZ mod logs that use Log field instead of Message
function formatDayZLog(log) {
  if (!log.Log) return '';
  
  const parts = [log.Log];
  
  // Add context based on log type
  if (log.GUID) parts.push(`Player: ${log.GUID.substring(0, 8)}...`);
  if (log.Action) parts.push(`Action: ${log.Action}`);
  if (log.Item) parts.push(`Item: ${log.Item}`);
  if (log.Target) parts.push(`Target: ${log.Target}`);
  if (log.KilledBy) parts.push(`By: ${log.KilledBy}`);
  if (log.KilledByGUID) parts.push(`Killer: ${log.KilledByGUID.substring(0, 8)}...`);
  if (log.Distance) parts.push(`Distance: ${log.Distance.toFixed(1)}m`);
  if (log.Speed !== undefined && log.Speed !== null) parts.push(`Speed: ${log.Speed}`);
  if (log.InTransport) parts.push('(In Vehicle)');
  
  return parts.join(' | ');
}

function showLoading() {
  elements.loadingOverlay.classList.remove('hidden');
}

function hideLoading() {
  elements.loadingOverlay.classList.add('hidden');
}

function showError(message) {
  // Could implement a toast notification here
  console.error(message);
}

// Cleanup on window unload
window.addEventListener('beforeunload', () => {
  stopLiveUpdates();
  window.logsApi.offNewLog();
});
