/**
 * Knowledge Base Manager Renderer
 * Handles UI interactions for KB management
 */

const LOG_PREFIX = '[KBRenderer]';
const MAX_CHAR_RECOMMEND = 16000;
const MAX_CHAR_REQUIRED = 22000;

let editor = null;
let currentKB = null;
let currentDoc = null;
let kbList = [];
let docList = [];
let originalContent = '';
let isDirty = false;
let pendingDelete = null;
let availableModels = [];

// DOM Elements
const elements = {
    kbList: document.getElementById('kbList'),
    searchKB: document.getElementById('searchKB'),
    kbDetailsPanel: document.getElementById('kbDetailsPanel'),
    docEditorPanel: document.getElementById('docEditorPanel'),
    emptyState: document.getElementById('emptyState'),
    kbName: document.getElementById('kbName'),
    kbDescription: document.getElementById('kbDescription'),
    docCount: document.getElementById('docCount'),
    shorterAnswersStatus: document.getElementById('shorterAnswersStatus'),
    extractModel: document.getElementById('extractModel'),
    documentsList: document.getElementById('documentsList'),
    docNameInput: document.getElementById('docNameInput'),
    docContextInput: document.getElementById('docContextInput'),
    sizeWarning: document.getElementById('sizeWarning'),
    sizeWarningText: document.getElementById('sizeWarningText'),
    toastContainer: document.getElementById('toastContainer')
};

// Utility Functions
function log(...args) { console.info(LOG_PREFIX, ...args); }
function logError(...args) { console.error(LOG_PREFIX, ...args); }

function getBridge() {
    if (!window.kbApi) {
        logError('kbApi bridge not present on window');
    }
    return window.kbApi;
}

function showToast(message, type = 'info') {
    const toast = document.createElement('div');
    toast.className = `toast toast-${type}`;
    toast.textContent = message;
    elements.toastContainer.appendChild(toast);
    
    setTimeout(() => {
        toast.classList.add('fade-out');
        setTimeout(() => toast.remove(), 300);
    }, 3000);
}

function showModal(modalId) {
    const modal = document.getElementById(modalId);
    if (modal) {
        modal.classList.remove('hidden');
    }
}

function hideModal(modalId) {
    const modal = document.getElementById(modalId);
    if (modal) {
        modal.classList.add('hidden');
        
        // Clear pending re-upload state when upload modal is closed
        if (modalId === 'uploadDocModal' && pendingDelete && pendingDelete.type === 'reupload') {
            pendingDelete = null;
            // Clear form fields
            document.getElementById('uploadFile').value = '';
            document.getElementById('uploadDocName').value = '';
            document.getElementById('uploadContextHint').value = '';
        }
    }
}

function formatDate(dateStr) {
    if (!dateStr) return 'N/A';
    const date = new Date(dateStr);
    return date.toLocaleDateString() + ' ' + date.toLocaleTimeString();
}

function formatCharCount(count) {
    if (count >= 1000) {
        return (count / 1000).toFixed(1) + 'k';
    }
    return count.toString();
}

// Panel Management
function showPanel(panel) {
    elements.kbDetailsPanel.classList.add('hidden');
    elements.docEditorPanel.classList.add('hidden');
    elements.emptyState.classList.add('hidden');
    
    if (panel === 'kb') {
        elements.kbDetailsPanel.classList.remove('hidden');
    } else if (panel === 'editor') {
        elements.docEditorPanel.classList.remove('hidden');
    } else {
        elements.emptyState.classList.remove('hidden');
    }
}

// KB List Rendering
function renderKBList(filter = '') {
    const query = filter.trim().toLowerCase();
    const filtered = kbList.filter(kb => 
        !query || 
        kb.name.toLowerCase().includes(query) || 
        kb.kbId.toLowerCase().includes(query)
    );

    elements.kbList.innerHTML = '';

    if (filtered.length === 0) {
        elements.kbList.innerHTML = '<div class="empty-list">No Knowledge Bases found</div>';
        return;
    }

    filtered.forEach(kb => {
        const item = document.createElement('div');
        item.className = 'kb-item' + (currentKB?.kbId === kb.kbId ? ' active' : '');
        item.innerHTML = `
            <div class="kb-item-name">${escapeHtml(kb.name)}</div>
            <div class="kb-item-id">${escapeHtml(kb.kbId)}</div>
            <div class="kb-item-meta">${kb.documentCount || 0} documents</div>
        `;
        item.addEventListener('click', () => selectKB(kb.kbId));
        elements.kbList.appendChild(item);
    });
}

function escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
}

// KB Operations
async function loadKBList() {
    const bridge = getBridge();
    if (!bridge) return;

    try {
        const result = await bridge.list();
        if (result.success) {
            kbList = result.data || [];
            renderKBList(elements.searchKB.value);
        } else {
            showToast('Failed to load KBs: ' + result.error, 'error');
        }
    } catch (err) {
        logError('Failed to load KB list', err);
        showToast('Failed to load Knowledge Bases', 'error');
    }
}

async function selectKB(kbId) {
    if (isDirty) {
        if (!confirm('You have unsaved changes. Discard them?')) {
            return;
        }
        isDirty = false;
    }

    const bridge = getBridge();
    if (!bridge) return;

    try {
        const result = await bridge.get(kbId);
        if (result.success) {
            currentKB = result.data;
            currentDoc = null;
            
            elements.kbName.textContent = currentKB.name;
            elements.kbDescription.textContent = currentKB.description || '';
            elements.docCount.textContent = currentKB.documentCount || 0;
            elements.shorterAnswersStatus.textContent = currentKB.shorterAnswers ? 'On' : 'Off';
            elements.extractModel.textContent = currentKB.extractModel || 'gpt-5-mini';

            await loadDocuments();
            showPanel('kb');
            renderKBList(elements.searchKB.value);
        } else {
            showToast('Failed to load KB: ' + result.error, 'error');
        }
    } catch (err) {
        logError('Failed to select KB', err);
        showToast('Failed to load Knowledge Base', 'error');
    }
}

async function createKB() {
    const kbId = document.getElementById('newKBId').value.trim();
    const name = document.getElementById('newKBName').value.trim();
    const description = document.getElementById('newKBDesc').value.trim();
    const shorterAnswers = document.getElementById('newKBShorterAnswers').checked;
    const extractModel = document.getElementById('newKBExtractModel').value.trim() || 'gpt-5-mini';

    if (!kbId || !name) {
        showToast('KB ID and Name are required', 'error');
        return;
    }

    if (!/^[a-zA-Z0-9_]+$/.test(kbId)) {
        showToast('KB ID must contain only letters, numbers, and underscores', 'error');
        return;
    }

    const bridge = getBridge();
    if (!bridge) return;

    try {
        const result = await bridge.create({
            kbId,
            name,
            description,
            shorterAnswers,
            extractModel
        });

        if (result.success) {
            hideModal('createKBModal');
            showToast('Knowledge Base created successfully', 'success');
            
            // Clear form and reset dropdown to first model
            document.getElementById('newKBId').value = '';
            document.getElementById('newKBName').value = '';
            document.getElementById('newKBDesc').value = '';
            document.getElementById('newKBShorterAnswers').checked = false;
            const dropdown = document.getElementById('newKBExtractModel');
            if (dropdown.options.length > 0) {
                dropdown.selectedIndex = 0;
            }

            await loadKBList();
            selectKB(kbId);
        } else {
            showToast('Failed to create KB: ' + result.error, 'error');
        }
    } catch (err) {
        logError('Failed to create KB', err);
        showToast('Failed to create Knowledge Base', 'error');
    }
}

async function updateKBSettings() {
    if (!currentKB) return;

    const name = document.getElementById('editKBName').value.trim();
    const description = document.getElementById('editKBDesc').value.trim();
    const shorterAnswers = document.getElementById('editKBShorterAnswers').checked;
    const extractModel = document.getElementById('editKBExtractModel').value.trim();

    const bridge = getBridge();
    if (!bridge) return;

    try {
        const result = await bridge.update(currentKB.kbId, {
            name,
            description,
            shorterAnswers,
            extractModel
        });

        if (result.success) {
            hideModal('kbSettingsModal');
            showToast('Settings updated', 'success');
            await loadKBList();
            await selectKB(currentKB.kbId);
        } else {
            showToast('Failed to update settings: ' + result.error, 'error');
        }
    } catch (err) {
        logError('Failed to update KB settings', err);
        showToast('Failed to update settings', 'error');
    }
}

async function deleteKB() {
    if (!currentKB) return;

    const bridge = getBridge();
    if (!bridge) return;

    try {
        const result = await bridge.remove(currentKB.kbId);
        if (result.success) {
            hideModal('deleteConfirmModal');
            showToast('Knowledge Base deleted', 'success');
            currentKB = null;
            currentDoc = null;
            showPanel('empty');
            await loadKBList();
        } else {
            showToast('Failed to delete KB: ' + result.error, 'error');
        }
    } catch (err) {
        logError('Failed to delete KB', err);
        showToast('Failed to delete Knowledge Base', 'error');
    }
}

// Document Operations
async function loadDocuments() {
    if (!currentKB) return;

    const bridge = getBridge();
    if (!bridge) return;

    try {
        const result = await bridge.listDocuments(currentKB.kbId);
        if (result.success) {
            docList = result.data || [];
            renderDocumentsList();
        } else {
            showToast('Failed to load documents: ' + result.error, 'error');
        }
    } catch (err) {
        logError('Failed to load documents', err);
    }
}

function renderDocumentsList() {
    elements.documentsList.innerHTML = '';

    if (docList.length === 0) {
        elements.documentsList.innerHTML = `
            <div class="empty-list">
                <p>No documents in this Knowledge Base yet.</p>
                <p>Upload a document or create a new text document to get started.</p>
            </div>
        `;
        return;
    }

    // Editable file types (can open in editor)
    const editableTypes = ['txt', 'md', 'json'];
    
    docList.forEach(doc => {
        const item = document.createElement('div');
        item.className = 'document-item';
        
        const embeddingStatus = doc.hasEmbedding 
            ? '<span class="status-badge success" title="Embeddings ready">✓</span>'
            : '<span class="status-badge warning" title="Embeddings pending">⏳</span>';
        
        const fileType = (doc.fileType || 'txt').toLowerCase();
        const isEditable = editableTypes.includes(fileType);
        
        // Show edit button for editable types, re-upload button for others
        const actionButton = isEditable
            ? `<button class="btn small" data-action="edit" data-id="${doc.documentId}" title="Edit">✏️</button>`
            : `<button class="btn small" data-action="reupload" data-id="${doc.documentId}" title="Re-upload to replace">📤</button>`;

        item.innerHTML = `
            <div class="doc-info">
                <div class="doc-name">${escapeHtml(doc.name)} ${embeddingStatus}</div>
                <div class="doc-meta">
                    <span class="doc-type">${fileType}</span>
                    <span class="doc-size">${formatCharCount(doc.charCount)} chars</span>
                    ${doc.chunks > 1 ? `<span class="doc-chunks">${doc.chunks} chunks</span>` : ''}
                </div>
                ${doc.contextHint ? `<div class="doc-context">${escapeHtml(doc.contextHint)}</div>` : ''}
            </div>
            <div class="doc-actions">
                ${actionButton}
                <button class="btn small danger" data-action="delete" data-id="${doc.documentId}" title="Delete">🗑️</button>
            </div>
        `;

        if (isEditable) {
            item.querySelector('[data-action="edit"]').addEventListener('click', (e) => {
                e.stopPropagation();
                editDocument(doc.documentId);
            });
        } else {
            item.querySelector('[data-action="reupload"]').addEventListener('click', (e) => {
                e.stopPropagation();
                reuploadDocument(doc);
            });
        }

        item.querySelector('[data-action="delete"]').addEventListener('click', (e) => {
            e.stopPropagation();
            confirmDeleteDocument(doc);
        });

        elements.documentsList.appendChild(item);
    });
}

async function editDocument(documentId) {
    if (!currentKB) return;

    const bridge = getBridge();
    if (!bridge) return;

    try {
        const result = await bridge.getDocument(currentKB.kbId, documentId);
        if (result.success) {
            currentDoc = result.data;
            
            elements.docNameInput.value = currentDoc.name || '';
            elements.docContextInput.value = currentDoc.contextHint || '';
            
            // Initialize or update editor
            if (editor) {
                editor.setValue(currentDoc.content || '', -1);
                setEditorMode(currentDoc.fileType);
            }
            
            originalContent = currentDoc.content || '';
            isDirty = false;
            updateSizeWarning(originalContent.length);
            
            showPanel('editor');
        } else {
            showToast('Failed to load document: ' + result.error, 'error');
        }
    } catch (err) {
        logError('Failed to edit document', err);
        showToast('Failed to load document', 'error');
    }
}

function newDocument() {
    currentDoc = null;
    elements.docNameInput.value = '';
    elements.docContextInput.value = '';
    
    if (editor) {
        editor.setValue('', -1);
        setEditorMode('txt');
    }
    
    originalContent = '';
    isDirty = false;
    updateSizeWarning(0);
    
    showPanel('editor');
}

async function saveDocument() {
    if (!currentKB) return;

    const name = elements.docNameInput.value.trim();
    const contextHint = elements.docContextInput.value.trim();
    const content = editor ? editor.getValue() : '';

    if (!name) {
        showToast('Document name is required', 'error');
        return;
    }

    if (!content.trim()) {
        showToast('Document content cannot be empty', 'error');
        return;
    }

    const bridge = getBridge();
    if (!bridge) return;

    try {
        let result;
        if (currentDoc && currentDoc.documentId) {
            // Update existing document
            result = await bridge.updateDocument(currentKB.kbId, currentDoc.documentId, {
                name,
                content,
                contextHint
            });
        } else {
            // Create new document
            const ext = name.includes('.') ? name.split('.').pop().toLowerCase() : 'txt';
            result = await bridge.addDocument(currentKB.kbId, {
                name,
                content,
                contextHint,
                fileType: ext
            });
        }

        if (result.success) {
            showToast('Document saved successfully', 'success');
            originalContent = content;
            isDirty = false;
            
            if (result.data?.documentId) {
                currentDoc = { ...currentDoc, documentId: result.data.documentId };
            }
            
            await loadDocuments();
            await loadKBList(); // Update document count
        } else {
            showToast('Failed to save document: ' + result.error, 'error');
        }
    } catch (err) {
        logError('Failed to save document', err);
        showToast('Failed to save document', 'error');
    }
}

function confirmDeleteDocument(doc) {
    pendingDelete = { type: 'document', data: doc };
    document.getElementById('deleteConfirmText').textContent = 
        `Are you sure you want to delete "${doc.name}"? This action cannot be undone.`;
    showModal('deleteConfirmModal');
}

// Re-upload document (for non-editable file types like PDF, DOCX)
function reuploadDocument(doc) {
    // Pre-fill the upload modal with the document's info
    document.getElementById('uploadDocName').value = doc.name || '';
    document.getElementById('uploadContextHint').value = doc.contextHint || '';
    
    // Store the document ID to replace on upload
    pendingDelete = { type: 'reupload', data: doc };
    
    showModal('uploadDocModal');
    showToast(`Select a file to replace "${doc.name}"`, 'info');
}

async function deleteDocument(documentId) {
    if (!currentKB) return;

    const bridge = getBridge();
    if (!bridge) return;

    try {
        const result = await bridge.deleteDocument(currentKB.kbId, documentId);
        if (result.success) {
            hideModal('deleteConfirmModal');
            showToast('Document deleted', 'success');
            await loadDocuments();
            await loadKBList();
        } else {
            showToast('Failed to delete document: ' + result.error, 'error');
        }
    } catch (err) {
        logError('Failed to delete document', err);
        showToast('Failed to delete document', 'error');
    }
}

async function uploadDocument() {
    if (!currentKB) return;

    const fileInput = document.getElementById('uploadFile');
    const nameInput = document.getElementById('uploadDocName');
    const contextInput = document.getElementById('uploadContextHint');

    if (!fileInput.files || fileInput.files.length === 0) {
        showToast('Please select a file to upload', 'error');
        return;
    }

    const file = fileInput.files[0];
    const name = nameInput.value.trim() || file.name;
    const contextHint = contextInput.value.trim();

    const bridge = getBridge();
    if (!bridge) return;

    const progressEl = document.getElementById('uploadProgress');
    progressEl.classList.remove('hidden');
    
    // Check if this is a re-upload (replacing an existing document)
    const isReupload = pendingDelete && pendingDelete.type === 'reupload';
    const oldDocId = isReupload ? pendingDelete.data.documentId : null;

    try {
        // If re-uploading, delete the old document first
        if (isReupload && oldDocId) {
            await bridge.deleteDocument(currentKB.kbId, oldDocId);
        }
        
        // Read file content
        const content = await readFileAsText(file);
        
        const ext = file.name.includes('.') ? file.name.split('.').pop().toLowerCase() : 'txt';
        
        const result = await bridge.addDocument(currentKB.kbId, {
            name,
            content,
            contextHint,
            fileType: ext
        });

        if (result.success) {
            hideModal('uploadDocModal');
            showToast(isReupload ? 'Document replaced successfully' : 'Document uploaded successfully', 'success');
            
            // Clear form and pending state
            fileInput.value = '';
            nameInput.value = '';
            contextInput.value = '';
            pendingDelete = null;
            
            await loadDocuments();
            await loadKBList();

            // Show split info if applicable
            if (result.splitInfo?.needsSplit) {
                showToast(`Document was split into ${result.data.chunks} chunks for better search performance`, 'info');
            } else if (result.splitInfo?.recommendSplit) {
                showToast('Consider splitting large documents for better search results', 'info');
            }
        } else {
            showToast('Failed to upload document: ' + result.error, 'error');
        }
    } catch (err) {
        logError('Failed to upload document', err);
        showToast('Failed to upload document', 'error');
    } finally {
        progressEl.classList.add('hidden');
    }
}

function readFileAsText(file) {
    return new Promise((resolve, reject) => {
        const reader = new FileReader();
        reader.onload = (e) => resolve(e.target.result);
        reader.onerror = (e) => reject(e);
        reader.readAsText(file);
    });
}

async function regenerateEmbeddings() {
    if (!currentKB) return;

    const bridge = getBridge();
    if (!bridge) return;

    showToast('Regenerating embeddings...', 'info');

    try {
        const result = await bridge.regenerateEmbeddings(currentKB.kbId);
        if (result.success) {
            if (result.processed === 0) {
                showToast('All documents already have embeddings', 'info');
            } else {
                showToast(`Regenerated embeddings for ${result.processed} documents`, 'success');
            }
            await loadDocuments();
        } else {
            showToast('Failed to regenerate embeddings: ' + result.error, 'error');
        }
    } catch (err) {
        logError('Failed to regenerate embeddings', err);
        showToast('Failed to regenerate embeddings', 'error');
    }
}

// Editor Functions
function setEditorMode(fileType) {
    if (!editor) return;

    const modeMap = {
        'md': 'ace/mode/markdown',
        'json': 'ace/mode/json',
        'xml': 'ace/mode/xml',
        'txt': 'ace/mode/text'
    };

    const mode = modeMap[fileType] || 'ace/mode/text';
    editor.session.setMode(mode);
}

function updateSizeWarning(charCount) {
    if (charCount > MAX_CHAR_REQUIRED) {
        elements.sizeWarning.classList.remove('hidden');
        elements.sizeWarning.classList.add('error');
        elements.sizeWarningText.textContent = 
            `Document has ${formatCharCount(charCount)} characters. It will be automatically split into chunks for embedding. Consider splitting into separate documents for better organization.`;
    } else if (charCount > MAX_CHAR_RECOMMEND) {
        elements.sizeWarning.classList.remove('hidden');
        elements.sizeWarning.classList.remove('error');
        elements.sizeWarningText.textContent = 
            `Document has ${formatCharCount(charCount)} characters. Consider splitting it into smaller documents for better search results.`;
    } else {
        elements.sizeWarning.classList.add('hidden');
    }
}

// Initialize Editor
function initEditor() {
    if (typeof ace === 'undefined') {
        log('ACE editor not loaded, retrying...');
        setTimeout(initEditor, 100);
        return;
    }

    editor = ace.edit('docEditor');
    editor.setTheme('ace/theme/twilight');
    editor.session.setMode('ace/mode/text');
    editor.setOptions({
        fontSize: '14px',
        showPrintMargin: false,
        wrap: true,
        tabSize: 2,
        useSoftTabs: true
    });

    editor.session.on('change', () => {
        const content = editor.getValue();
        isDirty = content !== originalContent;
        updateSizeWarning(content.length);
    });

    log('Editor initialized');
}

// Event Listeners
function setupEventListeners() {
    // Search
    elements.searchKB.addEventListener('input', () => {
        renderKBList(elements.searchKB.value);
    });

    // Create KB buttons
    document.getElementById('createKBBtn').addEventListener('click', () => showModal('createKBModal'));
    document.getElementById('createKBBtnEmpty').addEventListener('click', () => showModal('createKBModal'));
    document.getElementById('confirmCreateKB').addEventListener('click', createKB);

    // Shorter answers toggle
    document.getElementById('newKBShorterAnswers').addEventListener('change', (e) => {
        document.getElementById('extractModelGroup').style.display = e.target.checked ? 'block' : 'none';
    });
    
    // Edit shorter answers toggle
    document.getElementById('editKBShorterAnswers').addEventListener('change', (e) => {
        document.getElementById('editExtractModelGroup').style.display = e.target.checked ? 'block' : 'none';
    });

    // KB Settings
    document.getElementById('kbSettingsBtn').addEventListener('click', () => {
        if (!currentKB) return;
        document.getElementById('editKBName').value = currentKB.name || '';
        document.getElementById('editKBDesc').value = currentKB.description || '';
        document.getElementById('editKBShorterAnswers').checked = currentKB.shorterAnswers || false;
        
        // Show/hide extract model dropdown based on shorter answers state
        document.getElementById('editExtractModelGroup').style.display = currentKB.shorterAnswers ? 'block' : 'none';
        
        // Select the current model in the dropdown
        const dropdown = document.getElementById('editKBExtractModel');
        const modelValue = currentKB.extractModel || 'gpt-5-mini';
        for (let i = 0; i < dropdown.options.length; i++) {
            if (dropdown.options[i].value === modelValue) {
                dropdown.selectedIndex = i;
                break;
            }
        }
        
        showModal('kbSettingsModal');
    });
    document.getElementById('confirmUpdateKB').addEventListener('click', updateKBSettings);

    // Delete KB
    document.getElementById('deleteKBBtn').addEventListener('click', () => {
        if (!currentKB) return;
        pendingDelete = { type: 'kb', data: currentKB };
        document.getElementById('deleteConfirmText').textContent = 
            `Are you sure you want to delete the Knowledge Base "${currentKB.name}"? All documents will be permanently deleted.`;
        showModal('deleteConfirmModal');
    });

    // Confirm delete
    document.getElementById('confirmDelete').addEventListener('click', () => {
        if (!pendingDelete) return;
        if (pendingDelete.type === 'kb') {
            deleteKB();
        } else if (pendingDelete.type === 'document') {
            deleteDocument(pendingDelete.data.documentId);
        }
        pendingDelete = null;
    });

    // Document actions
    document.getElementById('uploadDocBtn').addEventListener('click', () => showModal('uploadDocModal'));
    document.getElementById('newDocBtn').addEventListener('click', newDocument);
    document.getElementById('confirmUpload').addEventListener('click', uploadDocument);

    // Editor actions
    document.getElementById('backToDocsBtn').addEventListener('click', () => {
        if (isDirty) {
            if (!confirm('You have unsaved changes. Discard them?')) {
                return;
            }
        }
        isDirty = false;
        currentDoc = null;
        showPanel('kb');
    });
    document.getElementById('saveDocBtn').addEventListener('click', saveDocument);

    // Modal close buttons
    document.querySelectorAll('.modal-close, [data-modal]').forEach(btn => {
        btn.addEventListener('click', (e) => {
            const modalId = e.currentTarget.dataset.modal;
            if (modalId) {
                hideModal(modalId);
            }
        });
    });

    // Close modal on backdrop click
    document.querySelectorAll('.modal').forEach(modal => {
        modal.addEventListener('click', (e) => {
            if (e.target === modal) {
                hideModal(modal.id);
            }
        });
    });

    // Keyboard shortcuts
    document.addEventListener('keydown', (e) => {
        // Ctrl+S to save
        if (e.ctrlKey && e.key === 's') {
            e.preventDefault();
            if (!elements.docEditorPanel.classList.contains('hidden')) {
                saveDocument();
            }
        }
        // Escape to close modals
        if (e.key === 'Escape') {
            document.querySelectorAll('.modal:not(.hidden)').forEach(modal => {
                hideModal(modal.id);
            });
        }
    });

    log('Event listeners set up');
}

// Initialize
async function init() {
    log('Initializing KB Manager');
    
    setupEventListeners();
    initEditor();
    
    // Load available models for dropdowns
    await loadAvailableModels();
    
    await loadKBList();
    showPanel('empty');
    
    log('KB Manager initialized');
}

// Load available OpenAI models for dropdowns
async function loadAvailableModels() {
    const bridge = getBridge();
    if (!bridge) return;

    try {
        log('Loading available OpenAI models...');
        const result = await bridge.listModels();
        
        if (result.success && result.models) {
            availableModels = result.models;
            log('Loaded models:', availableModels.length);
            populateModelDropdowns();
        } else {
            log('Failed to load models, using defaults');
            // Use default model
            availableModels = [{ id: 'gpt-5-mini', name: 'gpt-5-mini' }];
            populateModelDropdowns();
        }
    } catch (err) {
        logError('Failed to load models', err);
        availableModels = [{ id: 'gpt-5-mini', name: 'gpt-5-mini' }];
        populateModelDropdowns();
    }
}

// Populate all model dropdowns with available models
function populateModelDropdowns() {
    const dropdowns = [
        document.getElementById('newKBExtractModel'),
        document.getElementById('editKBExtractModel')
    ];

    dropdowns.forEach(dropdown => {
        if (!dropdown) return;
        
        const currentValue = dropdown.value;
        dropdown.innerHTML = '';
        
        availableModels.forEach(model => {
            const option = document.createElement('option');
            option.value = model.id;
            option.textContent = model.name;
            if (model.id === currentValue || (currentValue === '' && model.id === 'gpt-5-mini')) {
                option.selected = true;
            }
            dropdown.appendChild(option);
        });
    });
}

// Start when DOM is ready
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
} else {
    init();
}
