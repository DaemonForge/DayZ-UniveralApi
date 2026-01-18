// Preload script for Index Optimizer
const { contextBridge, ipcRenderer } = require('electron');

// Expose protected methods that allow the renderer process to use
// the ipcRenderer without exposing the entire object
contextBridge.exposeInMainWorld('indexOptimizer', {
    getRecommendations: () => ipcRenderer.invoke('indexOptimizer:getRecommendations'),
    getCurrentIndexes: () => ipcRenderer.invoke('indexOptimizer:getCurrentIndexes'),
    createIndex: (indexRequest) => ipcRenderer.invoke('indexOptimizer:createIndex', indexRequest)
});
