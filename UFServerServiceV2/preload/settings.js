const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('api', {
  getConfig: () => ipcRenderer.invoke('get-config'),
  saveConfig: (newConfig) => ipcRenderer.invoke('save-config', newConfig),
  onAttemptClose: (callback) => ipcRenderer.on('attempt-close', callback),
  forceClose: () => ipcRenderer.send('force-close'),
  restartApp: () => ipcRenderer.send('restart-app'),
  getProxyDomains: () => ipcRenderer.invoke('get-proxy-domains'),
  registerProxy: (selectedDomain) => ipcRenderer.invoke('register-proxy', selectedDomain),
  // Cloudflare Tunnel
  tunnelStart: () => ipcRenderer.invoke('tunnel-start'),
  tunnelStop: () => ipcRenderer.invoke('tunnel-stop'),
  tunnelStatus: () => ipcRenderer.invoke('tunnel-status'),
  tunnelDownload: () => ipcRenderer.invoke('tunnel-download'),
  tunnelCheckUpdate: () => ipcRenderer.invoke('tunnel-check-update'),
  tunnelUpdate: () => ipcRenderer.invoke('tunnel-update'),
  onTunnelStatusChanged: (callback) => ipcRenderer.on('tunnel-status-changed', (event, status) => callback(status)),
  openExternal: (url) => ipcRenderer.invoke('open-external', url),
  checkDomainStatus: (domain) => ipcRenderer.invoke('check-domain-status', domain)
});