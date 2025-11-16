const { app, Tray, Menu, shell, BrowserWindow, dialog, ipcMain, nativeImage } = require('electron');
const winston = require('winston');
const path = require('path');
const { pathToFileURL } = require('url');
const { exec } = require('child_process');
const { Writable } = require('stream');
const { readFileSync, writeFileSync, existsSync, mkdirSync } = require('fs');

// Global variables
global.SAVEPATH = `${app.getPath('userData')}/`;
global.isElectron = true;
global.APIVERSION = process.env.npm_package_version || app.getVersion();
global.rootPath = path.join(__dirname);
let tray = null;
let ConsoleWindow = null;
let settingsWindow = null;
let globalsWindow = null;
let cachedGlobalModel = null;

function getGlobalModel() {
  if (!cachedGlobalModel) {
    cachedGlobalModel = require('./models/global');
  }
  return cachedGlobalModel;
}


const fetch = (() => {
  if (typeof globalThis.fetch === 'function') {
    return globalThis.fetch.bind(globalThis);
  }
  try {
    const nodeFetch = require('node-fetch');
    return nodeFetch.default || nodeFetch;
  } catch (error) {
    console.error('Failed to load fetch implementation:', error);
    return undefined;
  }
})();

function resolveAssetPath(...segments) {
  const fallback = path.join(__dirname, ...segments);
  if (!app.isPackaged) {
    return fallback;
  }

  const resourceCandidate = path.join(process.resourcesPath, ...segments);
  if (existsSync(resourceCandidate)) {
    return resourceCandidate;
  }

  const asarCandidate = path.join(process.resourcesPath, 'app.asar', ...segments);
  if (existsSync(asarCandidate)) {
    return asarCandidate;
  }

  return fallback;
}

function loadIconImage(fileName) {
  const iconPath = resolveAssetPath('public', fileName);
  const image = nativeImage.createFromPath(iconPath);
  return image && !image.isEmpty() ? image : null;
}

const windowIconImage = loadIconImage('icon.ico');
const trayMenuIcon = loadIconImage('icon32x32.png');
const trayIconImage = windowIconImage || loadIconImage('universalFrameworklogo.ico');

const https = require('https');
const httpsAgent = new https.Agent({
  rejectUnauthorized: false,
});

// Create a proper writable stream that Winston can use to send log messages to the renderer
const logStream = new Writable({
  write(chunk, encoding, callback) {
    const message = chunk.toString();
    // Send the log message to all open renderer windows
    BrowserWindow.getAllWindows().forEach((win) => {
      win.webContents.send('log-message', message);
    });
    callback();
  }
});


app.on('ready', () => {
  checkAndInstallMongoDB();

  // Create the system tray icon
  const traySource = trayIconImage || resolveAssetPath('public', 'icon.ico');
  tray = new Tray(traySource);
  tray.setToolTip('Universal Framework');

  // Load your main service (if required)
  const ufService = require('./app');
  createLoggerStream();
  // Build initial context menu.
  updateTrayMenu();
  setTimeout(updateTrayMenu, 2500);
  setTimeout(updateTrayMenu, 6000);
  setTimeout(updateTrayMenu, 10000);
  setInterval(updateTrayMenu, 15000);

  // Start auto-renewal for proxy token, if configured.
  startProxyAutoRenew();
});

function createLoggerStream(){
  // Create the Winston stream transport with the writable stream
  const streamTransport = new winston.transports.Stream({ stream: logStream });
  global.logger.add(streamTransport);
}

function checkAndInstallMongoDB() {
  exec('winget list MongoDB.Server', (error, stdout, stderr) => {
    if (error || stdout.indexOf('MongoDB') === -1) {
      dialog.showMessageBox({
        type: 'info',
        buttons: ['Install Server only', 'Install Server & Compass', 'Cancel'],
        title: 'Universal Framework Service',
        message: 'MongoDB is not installed. Would you like to install it now?'
      }).then(result => {
        if (result.response === 0) {
          exec('winget install --id MongoDB.Server', (err, out, errOut) => {
            if (err) {
              console.error('Error installing MongoDB:', err);
            } else {
              console.log('MongoDB installation initiated:', out);
            }
          });
        } else if (result.response === 1) {
          exec('winget install -e --id MongoDB.Server;winget install -e --id MongoDB.Compass.Community', (err, out, errOut) => {
            if (err) {
              console.error('Error installing MongoDB:', err);
            } else {
              console.log('MongoDB installation initiated:', out);
            }
          });
        }
      });
    } else {
      console.log('MongoDB is already installed.');
    }
  });
}

app.on('certificate-error', (event, webContents, url, error, certificate, callback) => {
  if (url === `https://localhost:${global.config.Port}/Status?noLog=1` || url === `https://localhost/Status?noLog=1`) {
    event.preventDefault();
    callback(true);
  } else {
    callback(false);
  }
});

function OpenConsoleWindow() {
  if (ConsoleWindow) {
    ConsoleWindow.restore();
    ConsoleWindow.focus();
    return;
  }
  ConsoleWindow = new BrowserWindow({
    width: 980,
    height: 392,
    title: "Universal Framework Console", // sets the window title
    icon: windowIconImage || resolveAssetPath('public', 'icon.ico'),
    webPreferences: {
      nodeIntegration: false,
      contextIsolation: true,
      preload: path.join(__dirname, 'preload', 'console.js')
    }
  });
  ConsoleWindow.setMenu(null);
  
  // When the window finishes loading, send the cached log history.
  ConsoleWindow.webContents.on('did-finish-load', () => {
    global.logHistory.forEach((msg) => {
      ConsoleWindow.webContents.send('log-message', msg);
    });
  });

     
  // When the window is truly closed (app quit), then clean up.
  ConsoleWindow.on('closed', () => {
    ConsoleWindow = null;
  });

  ConsoleWindow.loadFile(path.join(__dirname, 'views', 'console.html'));
}

function fetchAPIStatus(callback) {
  const options = {
    hostname: 'localhost',
    port: global.config.Port,
    path: '/Status?noLog=1',
    method: 'GET',
    rejectUnauthorized: false // ignore certificate errors (self-signed, etc.)
  };

  const req = https.request(options, res => {
    let data = '';
    res.on('data', chunk => { data += chunk; });
    res.on('end', () => {
      try {
        const json = JSON.parse(data);
        callback(null, json);
      } catch (e) {
        callback(e);
      }
    });
  });

  req.on('error', (err) => {
    callback(err);
  });
  req.end();
}


/**
 * Updates the tray context menu with the current API status.
 */
function updateTrayMenu() {
  fetchAPIStatus((err, statusData) => {
    let apiStatusLabel = "Status: Starting Up";
    let apiStatusEmoji = "❓";
    let apiStatusSubLabel = "";
    
    // Process API status.
    if (err || (statusData.Error !== "noerror" && statusData.Error !== "NoAuth")) {
      apiStatusLabel = "Status: Error";
      apiStatusEmoji = "⚠️";
    } else {
      apiStatusEmoji = (statusData.Status === "Success") ? "🟢" : "🔴";
      apiStatusLabel = `Status: ${(statusData.Status === "Success") ? "Online" : statusData.Status} ${apiStatusEmoji}`;
      apiStatusSubLabel = `V${statusData.Version}`;
    }
    
    // Process Discord status.
    if (typeof global.DISCORDSTATUS === "undefined") {
      global.DISCORDSTATUS = "Disabled";
    }
    let discordEmoji = "❓";
    switch (global.DISCORDSTATUS) {
      case "Online":
        discordEmoji = "🟢";
        break;
      case "Disabled":
        discordEmoji = "⚫";
        break;
      case "Disconnected":
        discordEmoji = "🔴";
        break;
      case "Error":
        discordEmoji = "⚠️";
        break;
      default:
        discordEmoji = "❓";
    }
    const discordStatusLabel = `Discord: ${global.DISCORDSTATUS} ${discordEmoji}`;
    
    // Process OpenAI status.
    let openaiStatus = "Unknown";
    let openaiEmoji = "❓";
    if (statusData && typeof statusData.OpenAI === 'string') {
      openaiStatus = statusData.OpenAI;
      switch (statusData.OpenAI) {
        case "Online":
          openaiEmoji = "🟢";
          break;
        case "Disabled":
          openaiEmoji = "⚫";
          break;
        case "Error":
          openaiEmoji = "⚠️";
          break;
        default:
          openaiEmoji = "❓";
      }
    }
    const openaiStatusLabel = `OpenAI: ${openaiStatus} ${openaiEmoji}`;
    
    const contextMenu = Menu.buildFromTemplate([
      {
        label: `UF API Service ${apiStatusSubLabel}`,
        sublabel: apiStatusLabel,
        enabled: false,
        icon: trayMenuIcon || undefined
      },
      {
        label: discordStatusLabel,
        sublabel: openaiStatusLabel,
        enabled: false
      },
      { type: 'separator' },
      {
        label: '🖥️ Console',
        click: () => {
          OpenConsoleWindow();
        }
      },
      {
        label: '🔄 Restart',
        click: () => {
          if (settingsWindow) {
            settingsWindow.removeAllListeners('close');
            settingsWindow.close();
          }
          app.relaunch();
          app.exit();
        }
      },
      { type: 'separator' },
      { label: "❤️ Donate", click: () => { shell.openExternal('https://github.com/sponsors/DaemonF0rge'); } },
      {
        label: '⚙️ Options',
        submenu: [
          {
            label: '⚙️ Settings',
            click: () => {
              openSettingsWindow();
            }
          },
          {
            label: '📝Globals Editor',
            click: () => {
              openGlobalsWindow();
            }
          },
          {
            label: '📁 Logs',
            click: () => {
              shell.openPath(path.join(global.SAVEPATH,'logs'));
            }
          },
          {
            label: '📁 Discord Templates',
            click: () => {
              shell.openPath(path.join(global.SAVEPATH,'templates'));
            }
          },
          {
            label: '🛑 Stop',
            click: () => {
              if (settingsWindow) {
                settingsWindow.removeAllListeners('close');
                settingsWindow.close();
              }
              app.quit();
            }
          }
        ]
      }
    ]);
    tray.setContextMenu(contextMenu);
  });
}

function openGlobalsWindow() {
  if (globalsWindow) {
    globalsWindow.restore();
    globalsWindow.focus();
    return;
  }

  globalsWindow = new BrowserWindow({
    width: 1100,
    height: 720,
    title: 'Globals Editor',
    icon: windowIconImage || resolveAssetPath('public', 'icon.ico'),
    webPreferences: {
      nodeIntegration: false,
      contextIsolation: true,
      sandbox: false,
      preload: path.join(__dirname, 'preload', 'globals.js')
    }
  });

  globalsWindow.setMenu(null);
  const globalsPath = path.join(__dirname, 'views', 'globals.html');
  const globalsUrl = pathToFileURL(globalsPath);
  globalsUrl.searchParams.set('ts', Date.now().toString());
  globalsWindow.loadURL(globalsUrl.toString());
  globalsWindow.on('closed', () => {
    globalsWindow = null;
  });
}

function openSettingsWindow() {
  if (settingsWindow) {
    settingsWindow.restore();
    settingsWindow.focus();
    return;
  }
  
  settingsWindow = new BrowserWindow({
    width: 800,
    height: 600,
    title: "Universal Framework Settings",
    icon: windowIconImage || resolveAssetPath('public', 'icon.ico'),
    webPreferences: {
      nodeIntegration: false,
      contextIsolation: true,
      preload: path.join(__dirname, 'preload', 'settings.js')
    }
  });
  
  settingsWindow.setMenu(null);

  settingsWindow.loadFile(path.join(__dirname, 'views', 'settings.html'));
  //settingsWindow.webContents.openDevTools();
   
    // When the window is truly closed (app quit), then clean up.
    settingsWindow.on('closed', () => {
      
      settingsWindow = null;
    });
    // When creating the settings window
    settingsWindow.on('close', (e) => {
      console.log("Close event triggered");
      // Prevent immediate close
      e.preventDefault();
      // Tell the renderer that a close was attempted.
      settingsWindow.webContents.send('attempt-close');
    });
}

// IPC handler to return the current configuration
ipcMain.handle('get-config', (event) => {
  const configPath = path.join(global.SAVEPATH, 'config.json');
  try {
    const configDir = path.dirname(configPath);
    if (!existsSync(configDir)) {
      mkdirSync(configDir, { recursive: true });
    }
    const configData = readFileSync(configPath, 'utf-8');
    return JSON.parse(configData);
  } catch (err) {
    console.error("Error reading config:", err);
    if (global.config) {
      return global.config;
    }
    return null;
  }
});

// IPC handler to save the new configuration
ipcMain.handle('save-config', (event, newConfig) => {
  const configPath = path.join(global.SAVEPATH, 'config.json');
  try {
    const configDir = path.dirname(configPath);
    if (!existsSync(configDir)) {
      mkdirSync(configDir, { recursive: true });
    }
    writeFileSync(configPath, JSON.stringify(newConfig, null, 2));
    global.config = newConfig;
    return { success: true };
  } catch (err) {
    console.error("Error saving config:", err);
    return { success: false, error: err };
  }
});
ipcMain.on('force-close', () => {
  // Remove the close event handler to avoid an infinite loop
  if (settingsWindow) {
    settingsWindow.removeAllListeners('close');
    settingsWindow.close();
  }
  if (ConsoleWindow) {
    ConsoleWindow.removeAllListeners('close');
    ConsoleWindow.close();
  }
  if (globalsWindow) {
    globalsWindow.removeAllListeners('close');
    globalsWindow.close();
  }
});
ipcMain.on('restart-app', () => {
  if (settingsWindow) {
    settingsWindow.removeAllListeners('close');
    settingsWindow.close();
  }
  if (ConsoleWindow) {
    ConsoleWindow.removeAllListeners('close');
    ConsoleWindow.close();
  }
  if (globalsWindow) {
    globalsWindow.removeAllListeners('close');
    globalsWindow.close();
  }
  app.relaunch();
  app.exit();
});

ipcMain.handle('globals:list', async () => {
  try {
    const { listGlobals } = getGlobalModel();
    const data = await listGlobals();
    (global.logger || console).info('[GlobalsEditor] List request processed', { count: Array.isArray(data) ? data.length : 'n/a' });
    return { success: true, data };
  } catch (err) {
    (global.logger || console).error('[GlobalsEditor] Failed to list globals', { error: err.message });
    return { success: false, error: err.message };
  }
});

ipcMain.handle('globals:load', async (event, mod) => {
  try {
    if (!mod || typeof mod !== 'string') {
      throw new Error('Module name is required.');
    }
    const { getGlobalDocument } = getGlobalModel();
    const document = await getGlobalDocument(mod);
    if (!document) {
      return { success: false, error: 'Module not found.' };
    }
    return { success: true, data: document };
  } catch (err) {
    (global.logger || console).error('[GlobalsEditor] Failed to load module', { mod, error: err.message });
    return { success: false, error: err.message };
  }
});

ipcMain.handle('globals:save', async (event, payload) => {
  try {
    const mod = payload?.mod;
    const data = payload?.data;
    if (!mod || typeof mod !== 'string') {
      throw new Error('Module name is required.');
    }
    if (typeof data !== 'object' || data === null) {
      throw new Error('Data must be a JSON object or array.');
    }
    const { saveGlobalDocument } = getGlobalModel();
    await saveGlobalDocument(mod, data);
    return { success: true };
  } catch (err) {
    (global.logger || console).error('[GlobalsEditor] Failed to save module', { mod: payload?.mod, error: err.message });
    return { success: false, error: err.message };
  }
});

ipcMain.handle('globals:delete', async (event, mod) => {
  try {
    if (!mod || typeof mod !== 'string') {
      throw new Error('Module name is required.');
    }
    const { deleteGlobal } = getGlobalModel();
    const removed = await deleteGlobal(mod);
    if (!removed) {
      return { success: false, error: 'Module not found.' };
    }
    return { success: true };
  } catch (err) {
    (global.logger || console).error('[GlobalsEditor] Failed to delete module', { mod, error: err.message });
    return { success: false, error: err.message };
  }
});

app.on('window-all-closed', (e) => {
  e.preventDefault();
});
/**
 * Register an IPC handler for "get-proxy-domains" and return the available domains
 * from the hard-coded Cloudflare Worker endpoint.
 */
let lastProxyRegistrationTime = 0;

ipcMain.handle('get-proxy-domains', async (event) => {
  if (!fetch) {
    console.error("Fetch implementation unavailable; cannot load proxy domains.");
    return [];
  }
  try {
    const response = await fetch("https://ufapi.daemonforge.dev/available");
    if (!response.ok) {
      throw new Error("HTTP error " + response.status);
    }
    const data = await response.json();
    return data; // Expected to be an array of domain strings.
  } catch (error) {
    console.error("Error fetching proxy domains:", error);
    return [];
  }
});

ipcMain.handle('register-proxy', async (event, selectedDomain) => {
  if (!fetch) {
    throw new Error("Fetch implementation unavailable; cannot register proxy.");
  }
  try {
    const now = Date.now();
    if (now - lastProxyRegistrationTime < 60000) {
      throw new Error("Rate limited: Please wait at least 60 seconds between registrations.");
    }
    lastProxyRegistrationTime = now;
    const response = await fetch("https://ufapi.daemonforge.dev/register", {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ domain: selectedDomain })
    });
    if (!response.ok) {
      throw new Error("Failed to register proxy subdomain. HTTP " + response.status);
    }
    const data = await response.json();
    return data; // Expected to return { subdomain, token }
  } catch (error) {
    console.error("Error in 'register-proxy' handler:", error);
    throw error;
  }
});

// ----------------------- Proxy Auto-Renew Functions -----------------------
function loadConfigSync() {
  const configPath = path.join(global.SAVEPATH, 'config.json');
  try {
    const configDir = path.dirname(configPath);
    if (!existsSync(configDir)) {
      mkdirSync(configDir, { recursive: true });
    }
    return JSON.parse(readFileSync(configPath, 'utf-8'));
  } catch (err) {
    console.error("Error reading config:", err);
    return {};
  }
}

async function renewProxyToken() {
  if (!fetch) {
    console.error("Fetch implementation unavailable; skipping proxy token renewal.");
    return;
  }
  const config = loadConfigSync();
  if (!config.Proxy || !config.Proxy.subdomain || !config.Proxy.token || !config.Proxy.autoRenew) {
    return;
  }
  // Use the hard-coded keepalive endpoint of the proxy.
  const keepaliveUrl = "https://ufapi.daemonforge.dev/keepalive";
  try {
    const res = await fetch(keepaliveUrl, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ subdomain: config.Proxy.subdomain, token: config.Proxy.token })
    });
    if (res.ok) {
      global.logger.info("Proxy token renewed successfully.");
      config.Proxy.lastRenew = new Date().toISOString();
      writeFileSync(path.join(global.SAVEPATH, 'config.json'), JSON.stringify(config, null, 2));
    } else {
      const text = await res.text();
      global.logger.error("Failed to renew proxy token: " + text);
    }
  } catch (e) {
    global.logger.error("Error renewing proxy token: " + e.message);
  }
}

function startProxyAutoRenew() {
  renewProxyToken();
  setInterval(renewProxyToken, 24 * 60 * 60 * 1000);
}