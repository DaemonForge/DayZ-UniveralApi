const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('modManagerApi', {
  listMods: () => ipcRenderer.invoke('modmanager:list'),
  deleteMod: (modName) => ipcRenderer.invoke('modmanager:delete', modName)
});

// Index Optimizer API
contextBridge.exposeInMainWorld('indexOptimizer', {
  getRecommendations: () => ipcRenderer.invoke('indexOptimizer:getRecommendations'),
  getCurrentIndexes: () => ipcRenderer.invoke('indexOptimizer:getCurrentIndexes'),
  createIndex: (indexRequest) => ipcRenderer.invoke('indexOptimizer:createIndex', indexRequest)
});
