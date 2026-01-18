// Renderer for Index Optimizer
// Note: ipcRenderer is accessed via preload bridge (window.indexOptimizer)

let currentRecommendations = null;
let currentIndexes = null;
let selectedRecommendations = new Set();

// DOM Elements
const uptimeValue = document.getElementById('uptimeValue');
const queriesValue = document.getElementById('queriesValue');
const recommendationsValue = document.getElementById('recommendationsValue');
const collectionsValue = document.getElementById('collectionsValue');

const refreshBtn = document.getElementById('refreshBtn');
const optimizeBtn = document.getElementById('optimizeBtn');
const viewCurrentBtn = document.getElementById('viewCurrentBtn');

const recommendationsContainer = document.getElementById('recommendationsContainer');
const currentIndexesContainer = document.getElementById('currentIndexesContainer');

const confirmModal = document.getElementById('confirmModal');
const confirmContent = document.getElementById('confirmContent');
const confirmCreate = document.getElementById('confirmCreate');
const confirmCancel = document.getElementById('confirmCancel');

const progressModal = document.getElementById('progressModal');
const progressFill = document.getElementById('progressFill');
const progressText = document.getElementById('progressText');

const resultsModal = document.getElementById('resultsModal');
const resultsContent = document.getElementById('resultsContent');
const resultsClose = document.getElementById('resultsClose');

// Initialize
document.addEventListener('DOMContentLoaded', () => {
    setupEventListeners();
    loadRecommendations();
});

function setupEventListeners() {
    // Tab switching
    document.querySelectorAll('.tab-btn').forEach(btn => {
        btn.addEventListener('click', () => switchTab(btn.dataset.tab));
    });

    // Buttons
    refreshBtn.addEventListener('click', loadRecommendations);
    optimizeBtn.addEventListener('click', showConfirmModal);
    viewCurrentBtn.addEventListener('click', () => switchTab('current'));

    // Modal actions
    confirmCreate.addEventListener('click', createSelectedIndexes);
    confirmCancel.addEventListener('click', () => confirmModal.classList.remove('show'));
    resultsClose.addEventListener('click', () => {
        resultsModal.classList.remove('show');
        loadRecommendations(); // Refresh after creating indexes
    });
}

function switchTab(tabName) {
    // Update tab buttons
    document.querySelectorAll('.tab-btn').forEach(btn => {
        btn.classList.toggle('active', btn.dataset.tab === tabName);
    });

    // Update tab content
    document.querySelectorAll('.tab-content').forEach(content => {
        content.classList.toggle('active', content.id === `${tabName}-tab`);
    });

    // Load data if switching to current indexes
    if (tabName === 'current') {
        loadCurrentIndexes();
    }
}

async function loadRecommendations() {
    try {
        showLoading(recommendationsContainer);
        refreshBtn.disabled = true;

        const data = await window.indexOptimizer.getRecommendations();
        
        currentRecommendations = data.recommendations;
        currentIndexes = data.existingIndexes;

        // Update stats
        updateStats(data.stats, data.recommendations);

        // Render recommendations
        renderRecommendations(data.recommendations);

        refreshBtn.disabled = false;
    } catch (err) {
        console.error('Failed to load recommendations:', err);
        showError(recommendationsContainer, 'Failed to load recommendations: ' + err.message);
        refreshBtn.disabled = false;
    }
}

async function loadCurrentIndexes() {
    try {
        showLoading(currentIndexesContainer);

        if (!currentIndexes) {
            const data = await window.indexOptimizer.getCurrentIndexes();
            currentIndexes = data;
        }

        renderCurrentIndexes(currentIndexes);
    } catch (err) {
        console.error('Failed to load current indexes:', err);
        showError(currentIndexesContainer, 'Failed to load current indexes: ' + err.message);
    }
}

function updateStats(stats, recommendations) {
    uptimeValue.textContent = `${stats.uptimeHours}h`;
    queriesValue.textContent = stats.totalQueries.toLocaleString();
    
    // Count total recommendations
    let totalRecs = 0;
    for (const recs of Object.values(recommendations)) {
        totalRecs += recs.length;
    }
    recommendationsValue.textContent = totalRecs;
    collectionsValue.textContent = stats.collections;

    // Enable/disable optimize button
    optimizeBtn.disabled = totalRecs === 0;
}

function renderRecommendations(recommendations) {
    selectedRecommendations.clear();

    // Check if there are any recommendations
    const hasRecommendations = Object.values(recommendations).some(recs => recs.length > 0);

    if (!hasRecommendations) {
        recommendationsContainer.innerHTML = `
            <div class="empty-state">
                <div class="empty-state-icon">✨</div>
                <h3>No Recommendations Yet</h3>
                <p>Query patterns will be analyzed as the service runs.<br>
                Come back after the server has been running for a while.</p>
            </div>
        `;
        return;
    }

    let html = '';

    for (const [collection, recs] of Object.entries(recommendations)) {
        if (recs.length === 0) continue;

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
                        <div class="index-spec">${escapeHtml(indexSpecStr)}</div>
                        <div class="priority-badge priority-${priorityClass}">
                            Priority: ${rec.priorityScore}
                        </div>
                    </div>
                    <div class="recommendation-details">
                        <div class="detail-item">
                            <span class="detail-label">Queries:</span>
                            <span class="detail-value">${rec.queryCount}</span>
                        </div>
                        <div class="detail-item">
                            <span class="detail-label">Avg Time:</span>
                            <span class="detail-value">${rec.avgExecutionTime}ms</span>
                        </div>
                        <div class="detail-item">
                            <span class="detail-label">Rate:</span>
                            <span class="detail-value">${rec.queriesPerHour}/hr</span>
                        </div>
                        <div class="detail-item">
                            <span class="detail-label">Fields:</span>
                            <span class="detail-value">${rec.fields.length}</span>
                        </div>
                    </div>
                    <div class="recommendation-reason">
                        💡 ${escapeHtml(rec.reason)}
                    </div>
                    <div class="recommendation-actions">
                        <label class="recommendation-checkbox">
                            <input type="checkbox" data-collection="${escapeHtml(collection)}" data-rec-index="${index}">
                            <span>Select for creation</span>
                        </label>
                    </div>
                </div>
            `;
        });

        html += `</div>`;
    }

    recommendationsContainer.innerHTML = html;

    // Add checkbox listeners
    recommendationsContainer.querySelectorAll('input[type="checkbox"]').forEach(checkbox => {
        checkbox.addEventListener('change', updateSelectedRecommendations);
    });
}

function renderCurrentIndexes(indexes) {
    let html = '';

    for (const [collection, idxList] of Object.entries(indexes)) {
        html += `
            <div class="collection-section">
                <div class="collection-header">
                    <span class="collection-name">${escapeHtml(collection)}</span>
                    <span class="collection-badge">${idxList.length} index${idxList.length > 1 ? 'es' : ''}</span>
                </div>
                <div class="index-list">
        `;

        idxList.forEach(idx => {
            const fieldsStr = JSON.stringify(idx.key);
            const badges = [];
            if (idx.unique) badges.push('<span class="index-badge unique">Unique</span>');
            if (idx.sparse) badges.push('<span class="index-badge">Sparse</span>');
            if (idx.expireAfterSeconds) badges.push(`<span class="index-badge">TTL: ${idx.expireAfterSeconds}s</span>`);

            html += `
                <div class="index-card">
                    <div class="index-info">
                        <div class="index-name">${escapeHtml(idx.name)}</div>
                        <div class="index-fields">${escapeHtml(fieldsStr)}</div>
                        <div class="index-badges">${badges.join('')}</div>
                    </div>
                </div>
            `;
        });

        html += `
                </div>
            </div>
        `;
    }

    currentIndexesContainer.innerHTML = html;
}

function updateSelectedRecommendations(event) {
    const checkbox = event.target;
    const recId = `${checkbox.dataset.collection}-${checkbox.dataset.recIndex}`;
    
    if (checkbox.checked) {
        selectedRecommendations.add(recId);
    } else {
        selectedRecommendations.delete(recId);
    }

    optimizeBtn.disabled = selectedRecommendations.size === 0;
}

function showConfirmModal() {
    if (selectedRecommendations.size === 0) {
        // Select all by default
        recommendationsContainer.querySelectorAll('input[type="checkbox"]').forEach(cb => {
            cb.checked = true;
            selectedRecommendations.add(`${cb.dataset.collection}-${cb.dataset.recIndex}`);
        });
    }

    const indexRequests = getSelectedIndexRequests();

    let html = `
        <p>This will create <strong>${indexRequests.length}</strong> index${indexRequests.length > 1 ? 'es' : ''} across your collections:</p>
        <div style="margin-top: 15px;">
    `;

    indexRequests.forEach(req => {
        html += `
            <div class="result-item" style="background: rgba(102, 126, 234, 0.1);">
                <div class="result-icon">📊</div>
                <div class="result-info">
                    <div class="result-collection">${escapeHtml(req.collection)}</div>
                    <div class="result-message">${escapeHtml(JSON.stringify(req.indexSpec))}</div>
                </div>
            </div>
        `;
    });

    html += '</div>';
    confirmContent.innerHTML = html;
    confirmModal.classList.add('show');
}

function getSelectedIndexRequests() {
    const requests = [];

    selectedRecommendations.forEach(recId => {
        const [collection, indexStr] = recId.split('-');
        const index = parseInt(indexStr);
        const rec = currentRecommendations[collection][index];
        
        requests.push({
            collection,
            indexSpec: rec.indexSpec,
            options: {}
        });
    });

    return requests;
}

async function createSelectedIndexes() {
    confirmModal.classList.remove('show');
    progressModal.classList.add('show');

    const requests = getSelectedIndexRequests();
    const results = [];

    for (let i = 0; i < requests.length; i++) {
        const req = requests[i];
        const progress = ((i + 1) / requests.length) * 100;
        
        progressFill.style.width = `${progress}%`;
        progressText.textContent = `Creating index ${i + 1} of ${requests.length} on ${req.collection}...`;

        try {
            const result = await window.indexOptimizer.createIndex(req);
            results.push({ ...req, success: true, result });
        } catch (err) {
            results.push({ ...req, success: false, error: err.message });
        }
    }

    progressModal.classList.remove('show');
    showResults(results);
}

function showResults(results) {
    let html = '';

    results.forEach(result => {
        const icon = result.success ? '✅' : '❌';
        const className = result.success ? 'success' : 'error';
        const message = result.success 
            ? `Index created: ${result.result.indexName}` 
            : `Error: ${result.error}`;

        html += `
            <div class="result-item ${className}">
                <div class="result-icon">${icon}</div>
                <div class="result-info">
                    <div class="result-collection">${escapeHtml(result.collection)}</div>
                    <div class="result-message">${escapeHtml(message)}</div>
                </div>
            </div>
        `;
    });

    resultsContent.innerHTML = html;
    resultsModal.classList.add('show');
}

function showLoading(container) {
    container.innerHTML = `
        <div class="loading-state">
            <div class="spinner"></div>
            <p>Loading...</p>
        </div>
    `;
}

function showError(container, message) {
    container.innerHTML = `
        <div class="empty-state">
            <div class="empty-state-icon">⚠️</div>
            <h3>Error</h3>
            <p>${escapeHtml(message)}</p>
        </div>
    `;
}

function escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
}
