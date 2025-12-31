const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('kbApi', {
    // KB Management
    list: () => ipcRenderer.invoke('kb:list'),
    get: (kbId) => ipcRenderer.invoke('kb:get', kbId),
    create: (data) => ipcRenderer.invoke('kb:create', data),
    update: (kbId, data) => ipcRenderer.invoke('kb:update', { kbId, data }),
    remove: (kbId) => ipcRenderer.invoke('kb:delete', kbId),

    // Document Management
    listDocuments: (kbId) => ipcRenderer.invoke('kb:listDocuments', kbId),
    getDocument: (kbId, documentId) => ipcRenderer.invoke('kb:getDocument', { kbId, documentId }),
    addDocument: (kbId, data) => ipcRenderer.invoke('kb:addDocument', { kbId, ...data }),
    updateDocument: (kbId, documentId, data) => ipcRenderer.invoke('kb:updateDocument', { kbId, documentId, ...data }),
    deleteDocument: (kbId, documentId) => ipcRenderer.invoke('kb:deleteDocument', { kbId, documentId }),
    regenerateEmbeddings: (kbId) => ipcRenderer.invoke('kb:regenerateEmbeddings', kbId),

    // OpenAI Models
    listModels: () => ipcRenderer.invoke('kb:listModels')
});
