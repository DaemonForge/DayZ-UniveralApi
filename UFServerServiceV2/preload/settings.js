// preload/settings.js
const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('api', {
  getConfig: () => ipcRenderer.invoke('get-config'),
  saveConfig: (newConfig) => ipcRenderer.invoke('save-config', newConfig),
  onAttemptClose: (callback) => ipcRenderer.on('attempt-close', callback),
  forceClose: () => ipcRenderer.send('force-close'),
  restartApp: () => ipcRenderer.send('restart-app')
});