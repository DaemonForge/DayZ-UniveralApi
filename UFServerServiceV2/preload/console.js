const { contextBridge, ipcRenderer } = require('electron');
console.log('Preload script loaded.');

contextBridge.exposeInMainWorld('api', {
  // Expose a method for the renderer to subscribe to log messages
  onLogMessage: (callback) => ipcRenderer.on('log-message', callback)
});
