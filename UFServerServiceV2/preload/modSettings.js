const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('modSettingsApi', {
  /** List all registered mod settings pages (metadata only) */
  list: () => ipcRenderer.invoke('modSettings:list'),

  /** Get the full mod settings document (including template HTML) for a given modId */
  get: (modId) => ipcRenderer.invoke('modSettings:get', modId),

  /** Load a global by mod name — returns the global JSON data */
  loadGlobal: (globalName) => ipcRenderer.invoke('modSettings:loadGlobal', globalName),

  /** Save a global by mod name — writes new JSON data */
  saveGlobal: (globalName, data) => ipcRenderer.invoke('modSettings:saveGlobal', { globalName, data }),

  /** Open an external URL in the user's default browser */
  openExternal: (url) => ipcRenderer.invoke('modSettings:openExternal', url),
});
