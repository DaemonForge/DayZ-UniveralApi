// Embedded Index Optimizer for Data Manager
// This is a lighter version of indexOptimizer.js designed to run inside a modal

const indexOptimizerBtn = document.getElementById('indexOptimizerBtn');
const indexOptimizerModal = document.getElementById('indexOptimizerModal');
const closeIndexOptimizerBtn = document.getElementById('closeIndexOptimizerBtn');
const indexOptimizerContent = document.getElementById('indexOptimizerContent');

let currentRecommendations = null;
let currentIndexes = null;
let selectedRecommendations = new Set();

// Event listeners
if (indexOptimizerBtn) {
  indexOptimizerBtn.addEventListener('click', openIndexOptimizer);
}

if (closeIndexOptimizerBtn) {
  closeIndexOptimizerBtn.addEventListener('click', closeIndexOptimizer);
}

// Close modal when clicking outside
if (indexOptimizerModal) {
  indexOptimizerModal.addEventListener('click', (e) => {
    if (e.target === indexOptimizerModal) {
      closeIndexOptimizer();
    }
  });
}

async function openIndexOptimizer() {
  if (!indexOptimizerModal) return;
  
  indexOptimizerModal.style.display = 'flex';
  await loadIndexOptimizerContent();
}

function closeIndexOptimizer() {
  if (indexOptimizerModal) {
    indexOptimizerModal.style.display = 'none';
  }
}

async function loadIndexOptimizerContent() {
  try {
    showLoading();
    const data = await window.indexOptimizer.getRecommendations();
    
    currentRecommendations = data.recommendations;
    currentIndexes = data.existingIndexes;
    
    renderIndexOptimizer(data);
  } catch (err) {
    console.error('Failed to load Index Optimizer:', err);
    showError('Failed to load Index Optimizer: ' + err.message);
  }
}

function renderIndexOptimizer(data) {
  const stats = data.stats;
  const recommendations = data.recommendations;
  
  let html = `
    <div class="index-optimizer-container">
      <div class="index-stats-bar">
        <div class="stat-card">
          <div class="stat-label">Uptime</div>
          <div class="stat-value">${stats.uptimeHours}h</div>
        </div>
        <div class="stat-card">
          <div class="stat-label">Queries Tracked</div>
          <div class="stat-value">${stats.totalQueries.toLocaleString()}</div>
        </div>
        <div class="stat-card">
          <div class="stat-label">Collections</div>
          <div class="stat-value">${stats.collections}</div>
        </div>
        <div class="stat-card">
          <div class="stat-label">Recommendations</div>
          <div class="stat-value">${countRecommendations(recommendations)}</div>
        </div>
      </div>
      
      <div class="index-actions-bar">
        <button id="refreshIndexes" class="btn-secondary">🔄 Refresh</button>
        <button id="viewCurrentIndexes" class="btn-secondary">📋 View Current Indexes</button>
        <button id="createIndexes" class="btn-primary" disabled>✅ Create Selected</button>
      </div>
      
      <div id="indexContentArea" class="index-content-area">
        ${renderRecommendations(recommendations)}
      </div>
    </div>
  `;
  
  indexOptimizerContent.innerHTML = html;
  
  // Attach event listeners
  setupIndexOptimizerEventListeners();
}

function countRecommendations(recommendations) {
  let count = 0;
  for (const recs of Object.values(recommendations)) {
    count += recs.length;
  }
  return count;
}

function renderRecommendations(recommendations) {
  const hasRecommendations = recommendations && 
    Object.values(recommendations).some(recs => recs && recs.length > 0);
  
  if (!hasRecommendations) {
    return `
      <div class="empty-state">
        <div class="empty-icon">📊</div>
        <h3>No recommendations yet</h3>
        <p>Query patterns will be analyzed as the service runs.<br>
        Come back after the server has been running for a while.</p>
      </div>
    `;
  }
  
  let html = '<div class="recommendations-list">';
  
  for (const [collection, recs] of Object.entries(recommendations)) {
    if (!recs || recs.length === 0) continue;
    
    html += `
      <div class="collection-section">
        <div class="collection-header">
          <span class="collection-name">${escapeHtml(collection)}</span>
          <span class="collection-badge">${recs.length} recommendation${recs.length > 1 ? 's' : ''}</span>
        </div>
    `;
    
    recs.forEach((rec, index) => {
      const recId = `${collection}-${index}`;
      const priorityClass = rec.priorityScore >= 70 ? 'high' : rec.priorityScore >= 40 ? 'medium' : 'low';
      const indexSpecStr = JSON.stringify(rec.indexSpec, null, 2);
      
      html += `
        <div class="recommendation-card" data-rec-id="${recId}">
          <div class="recommendation-header">
            <label class="checkbox-label">
              <input type="checkbox" data-collection="${escapeHtml(collection)}" data-rec-index="${index}">
              <span class="priority-badge ${priorityClass}">Priority: ${rec.priorityScore}</span>
            </label>
            <div class="recommendation-stats">
              <span>📊 ${rec.queryCount} queries</span>
              <span>⚡ ${rec.avgExecutionTime}ms avg</span>
              <span>🔄 ${rec.queriesPerHour}/hr</span>
            </div>
          </div>
          <div class="index-spec">
            <strong>Index:</strong> <code>${escapeHtml(indexSpecStr)}</code>
          </div>
          <div class="recommendation-reason">${escapeHtml(rec.reason)}</div>
        </div>
      `;
    });
    
    html += '</div>';
  }
  
  html += '</div>';
  return html;
}

function renderCurrentIndexes(indexes) {
  let html = '<div class="current-indexes-list">';
  
  if (!indexes || Object.keys(indexes).length === 0) {
    return '<div class="empty-state"><p>No indexes found</p></div>';
  }
  
  for (const [collection, idxList] of Object.entries(indexes)) {
    html += `
      <div class="collection-section">
        <div class="collection-header">
          <span class="collection-name">${escapeHtml(collection)}</span>
          <span class="collection-badge">${idxList.length} index${idxList.length > 1 ? 'es' : ''}</span>
        </div>
    `;
    
    idxList.forEach(idx => {
      const fieldsStr = JSON.stringify(idx.key);
      const badges = [];
      if (idx.unique) badges.push('<span class="index-badge unique">Unique</span>');
      if (idx.sparse) badges.push('<span class="index-badge">Sparse</span>');
      if (idx.expireAfterSeconds) badges.push(`<span class="index-badge">TTL: ${idx.expireAfterSeconds}s</span>`);
      
      html += `
        <div class="index-card">
          <div class="index-name">${escapeHtml(idx.name)}</div>
          <div class="index-fields">${escapeHtml(fieldsStr)}</div>
          <div class="index-badges">${badges.join('')}</div>
        </div>
      `;
    });
    
    html += '</div>';
  }
  
  html += '</div>';
  return html;
}

function setupIndexOptimizerEventListeners() {
  const refreshBtn = document.getElementById('refreshIndexes');
  const viewCurrentBtn = document.getElementById('viewCurrentIndexes');
  const createBtn = document.getElementById('createIndexes');
  
  if (refreshBtn) {
    refreshBtn.addEventListener('click', loadIndexOptimizerContent);
  }
  
  if (viewCurrentBtn) {
    viewCurrentBtn.addEventListener('click', showCurrentIndexes);
  }
  
  if (createBtn) {
    createBtn.addEventListener('click', createSelectedIndexes);
  }
  
  // Checkbox listeners
  document.querySelectorAll('#indexContentArea input[type="checkbox"]').forEach(checkbox => {
    checkbox.addEventListener('change', updateSelectedIndexes);
  });
}

function updateSelectedIndexes(event) {
  const checkbox = event.target;
  const recId = `${checkbox.dataset.collection}-${checkbox.dataset.recIndex}`;
  
  if (checkbox.checked) {
    selectedRecommendations.add(recId);
  } else {
    selectedRecommendations.delete(recId);
  }
  
  const createBtn = document.getElementById('createIndexes');
  if (createBtn) {
    createBtn.disabled = selectedRecommendations.size === 0;
  }
}

async function showCurrentIndexes() {
  try {
    const contentArea = document.getElementById('indexContentArea');
    if (!contentArea) return;
    
    contentArea.innerHTML = '<div class="loading">Loading current indexes...</div>';
    
    if (!currentIndexes) {
      const data = await window.indexOptimizer.getCurrentIndexes();
      currentIndexes = data;
    }
    
    contentArea.innerHTML = renderCurrentIndexes(currentIndexes);
    
    // Add back button
    const backBtn = document.createElement('button');
    backBtn.className = 'btn-secondary';
    backBtn.textContent = '← Back to Recommendations';
    backBtn.style.marginTop = '20px';
    backBtn.addEventListener('click', () => loadIndexOptimizerContent());
    contentArea.appendChild(backBtn);
  } catch (err) {
    console.error('Failed to load current indexes:', err);
    showError('Failed to load current indexes: ' + err.message);
  }
}

async function createSelectedIndexes() {
  if (selectedRecommendations.size === 0) return;
  
  const indexRequests = [];
  
  for (const recId of selectedRecommendations) {
    const [collection, indexStr] = recId.split('-');
    const index = parseInt(indexStr);
    const rec = currentRecommendations[collection][index];
    
    if (rec) {
      indexRequests.push({
        collection,
        indexSpec: rec.indexSpec,
        options: {}
      });
    }
  }
  
  if (indexRequests.length === 0) return;
  
  try {
    const contentArea = document.getElementById('indexContentArea');
    contentArea.innerHTML = `
      <div class="progress-info">
        <h3>Creating ${indexRequests.length} index${indexRequests.length > 1 ? 'es' : ''}...</h3>
        <div class="progress-bar">
          <div class="progress-fill" style="width: 0%"></div>
        </div>
        <p id="progressStatus">Starting...</p>
      </div>
    `;
    
    const progressFill = contentArea.querySelector('.progress-fill');
    const progressStatus = document.getElementById('progressStatus');
    const results = [];
    
    for (let i = 0; i < indexRequests.length; i++) {
      const req = indexRequests[i];
      const progress = ((i + 1) / indexRequests.length) * 100;
      
      if (progressFill) progressFill.style.width = `${progress}%`;
      if (progressStatus) progressStatus.textContent = `Creating index ${i + 1} of ${indexRequests.length}...`;
      
      try {
        const result = await window.indexOptimizer.createIndex(req);
        results.push({ ...req, success: true, result });
      } catch (err) {
        results.push({ ...req, success: false, error: err.message });
      }
    }
    
    // Show results
    showIndexCreationResults(results);
    selectedRecommendations.clear();
    
  } catch (err) {
    console.error('Failed to create indexes:', err);
    showError('Failed to create indexes: ' + err.message);
  }
}

function showIndexCreationResults(results) {
  const successCount = results.filter(r => r.success).length;
  const failCount = results.filter(r => !r.success).length;
  
  let html = `
    <div class="results-summary">
      <h3>Index Creation Complete</h3>
      <p>${successCount} created successfully, ${failCount} failed</p>
    </div>
    <div class="results-list">
  `;
  
  results.forEach(r => {
    const icon = r.success ? '✅' : '❌';
    const className = r.success ? 'success' : 'error';
    const message = r.success ? 'Created successfully' : r.error;
    
    html += `
      <div class="result-item ${className}">
        <div class="result-icon">${icon}</div>
        <div class="result-info">
          <div class="result-collection">${escapeHtml(r.collection)}</div>
          <div class="result-message">${escapeHtml(message)}</div>
          <div class="result-spec">${escapeHtml(JSON.stringify(r.indexSpec))}</div>
        </div>
      </div>
    `;
  });
  
  html += `
    </div>
    <button id="backToRecommendations" class="btn-primary" style="margin-top: 20px;">Back to Recommendations</button>
  `;
  
  const contentArea = document.getElementById('indexContentArea');
  if (contentArea) {
    contentArea.innerHTML = html;
    
    const backBtn = document.getElementById('backToRecommendations');
    if (backBtn) {
      backBtn.addEventListener('click', () => loadIndexOptimizerContent());
    }
  }
}

function showLoading() {
  if (indexOptimizerContent) {
    indexOptimizerContent.innerHTML = '<div class="loading">⏳ Loading...</div>';
  }
}

function showError(message) {
  if (indexOptimizerContent) {
    indexOptimizerContent.innerHTML = `<div class="error">❌ ${escapeHtml(message)}</div>`;
  }
}

function escapeHtml(text) {
  if (text === null || text === undefined) return '';
  const div = document.createElement('div');
  div.textContent = String(text);
  return div.innerHTML;
}
