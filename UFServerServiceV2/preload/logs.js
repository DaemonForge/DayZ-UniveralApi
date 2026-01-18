const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('logsApi', {
  // Query logs with filters
  query: (filters) => ipcRenderer.invoke('logs:query', filters),
  
  // Get list of unique server IDs
  getServers: () => ipcRenderer.invoke('logs:getServers'),
  
  // Get list of unique log types
  getTypes: () => ipcRenderer.invoke('logs:getTypes'),
  
  // Get stats (counts by level)
  getStats: (filters) => ipcRenderer.invoke('logs:getStats', filters),
  
  // Delete logs (optional admin feature)
  deleteLogs: (filters) => ipcRenderer.invoke('logs:delete', filters),
  
  // Subscribe to live log updates
  onNewLog: (callback) => ipcRenderer.on('logs:new', callback),
  
  // Unsubscribe from live updates
  offNewLog: () => ipcRenderer.removeAllListeners('logs:new')
});
