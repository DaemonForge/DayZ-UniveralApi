const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('globalsApi', {
  list: () => ipcRenderer.invoke('globals:list'),
  load: (mod) => ipcRenderer.invoke('globals:load', mod),
  save: (mod, data) => ipcRenderer.invoke('globals:save', { mod, data }),
  remove: (mod) => ipcRenderer.invoke('globals:delete', mod)
});
