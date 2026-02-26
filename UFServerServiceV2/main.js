const { app, Tray, Menu, shell, BrowserWindow, dialog, ipcMain, nativeImage } = require('electron');
const winston = require('winston');
const path = require('path');
const { pathToFileURL } = require('url');
const { exec } = require('child_process');
const { Writable } = require('stream');
const { readFileSync, writeFileSync, existsSync, mkdirSync } = require('fs');
const net = require('net');

// Global variables – MUST be set before requiring tunnelManager (singleton reads SAVEPATH in constructor)
global.SAVEPATH = `${app.getPath('userData')}/`;
let tunnelManager = null; // Lazy-loaded after app.js initializes global.logger
global.isElectron = true;
global.APIVERSION = process.env.npm_package_version || app.getVersion();
global.rootPath = path.join(__dirname);
let tray = null;
let ConsoleWindow = null;
let logsWindow = null;
let settingsWindow = null;
let globalsWindow = null;
let kbWindow = null;
let modManagerWindow = null;
let indexOptimizerWindow = null;
let indexOptimizerOpening = false;
let modSettingsWindow = null;
let cachedGlobalModel = null;
let cachedKBModel = null;
let cachedModDataModel = null;
let cachedIndexManager = null;
let cachedModSettingsModel = null;

function getGlobalModel() {
  if (!cachedGlobalModel) {
    cachedGlobalModel = require('./models/global');
  }
  return cachedGlobalModel;
}

function getKBModel() {
  if (!cachedKBModel) {
    cachedKBModel = require('./models/kb');
  }
  return cachedKBModel;
}

function getModDataModel() {
  if (!cachedModDataModel) {
    cachedModDataModel = require('./models/modData');
  }
  return cachedModDataModel;
}

function getIndexManager() {
  if (!cachedIndexManager) {
    cachedIndexManager = require('./models/indexManager');
  }
  return cachedIndexManager;
}

function getModSettingsModel() {
  if (!cachedModSettingsModel) {
    cachedModSettingsModel = require('./models/modSettings');
  }
  return cachedModSettingsModel;
}

function getKBController() {
  // Lazy load the KB controller for embedding generation
  return require('./controllers/kb');
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

  // Set initial "Starting Up" menu immediately so users can interact with tray
  setInitialTrayMenu();

  // Load your main service (if required) — this sets global.logger
  const ufService = require('./app');
  createLoggerStream();

  // Require tunnelManager AFTER global.logger exists so createLogger() works normally
  tunnelManager = require('./tunnelManager');

  // Forward tunnel status changes to all renderer windows
  tunnelManager.onStatusChange((status) => {
    BrowserWindow.getAllWindows().forEach((win) => {
      try {
        win.webContents.send('tunnel-status-changed', status);
      } catch (_) { /* window may be destroyed */ }
    });
    // Update tray to reflect tunnel status
    updateTrayMenu();
  });

  // Build context menu with actual status.
  updateTrayMenu();
  setTimeout(updateTrayMenu, 2500);
  setTimeout(updateTrayMenu, 6000);
  setTimeout(updateTrayMenu, 10000);
  setInterval(updateTrayMenu, 15000);

  // Start auto-renewal for proxy token, if configured.
  startProxyAutoRenew();

  // Auto-start Cloudflare tunnel if configured
  startTunnelIfConfigured();

  // Start periodic cloudflared update checks (non-blocking)
  tunnelManager.startUpdateChecks();
});

function createLoggerStream(){
  // Create the Winston stream transport with the writable stream
  const streamTransport = new winston.transports.Stream({ stream: logStream });
  global.logger.add(streamTransport);
}

function checkAndInstallMongoDB() {
  // 0. Check config for remote DB connection
  // If the user has configured a remote DB, we don't need to check for local installation.
  try {
    const configPath = path.join(global.SAVEPATH, 'config.json');
    if (existsSync(configPath)) {
      const configData = JSON.parse(readFileSync(configPath, 'utf8'));
      if (configData.DBServer) {
        // Check if DBServer is NOT localhost/127.0.0.1
        const isLocal = configData.DBServer.includes('localhost') || configData.DBServer.includes('127.0.0.1');
        if (!isLocal) {
          console.log('[Setup] Remote MongoDB configuration detected. Skipping local installation check.');
          return;
        }
      }
    }
  } catch (err) {
    console.error('[Setup] Failed to read config for MongoDB check:', err.message);
    // Proceed with check if config read fails (safe default)
  }

  // 1. First fast check: Is the default MongoDB port open?
  const socket = new net.Socket();
  const cleanup = () => {
    if (socket) socket.destroy();
  };

  socket.setTimeout(500); // Fast timeout
  
  socket.on('connect', () => {
    console.log('[Setup] MongoDB detected running on port 27017.');
    cleanup();
    // It's running, so it's guaranteed installed. No further action needed.
    return;
  });

  socket.on('timeout', () => {
    cleanup();
    checkServiceAndWinget();
  });

  socket.on('error', (err) => {
    cleanup();
    checkServiceAndWinget();
  });

  socket.connect(27017, '127.0.0.1');

  function checkServiceAndWinget() {
    // 2. Second fast check: Does the Windows Service exist?
    // "sc query" is much faster than winget
    exec('sc query "MongoDB"', (err, stdout, stderr) => {
      // If "STATE" is present in output, service exists (even if stopped)
      if (!err && stdout && stdout.includes('STATE')) {
        console.log('[Setup] MongoDB service detected (may be stopped).');
        return;
      }

      // 3. Fallback: Slow winget check (only if port and service checks failed)
      console.log('[Setup] Converting to deep check for MongoDB (winget)...');
      exec('winget list MongoDB.Server', (error, stdout, stderr) => {
        if (error || stdout.indexOf('MongoDB') === -1) {
          dialog.showMessageBox({
            type: 'info',
            buttons: ['Install Server only', 'Install Server & Compass', 'Cancel'],
            title: 'Universal Framework Service',
            message: 'MongoDB is not installed or configured. Would you like to install it now?'
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
          console.log('[Setup] MongoDB detected via winget (installed but likely stopped).');
        }
      });
    });
  }
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

function openLogsWindow() {
  if (logsWindow) {
    logsWindow.restore();
    logsWindow.focus();
    return;
  }

  logsWindow = new BrowserWindow({
    width: 1400,
    height: 800,
    title: 'Log Viewer',
    icon: windowIconImage || resolveAssetPath('public', 'icon.ico'),
    webPreferences: {
      nodeIntegration: false,
      contextIsolation: true,
      sandbox: false,
      preload: path.join(__dirname, 'preload', 'logs.js')
    }
  });

  logsWindow.setMenu(null);
  const logsPath = path.join(__dirname, 'views', 'logs.html');
  const logsUrl = pathToFileURL(logsPath);
  logsUrl.searchParams.set('ts', Date.now().toString());
  logsWindow.loadURL(logsUrl.toString());
  
  // Enable dev tools with F12 or Ctrl+Shift+I
  logsWindow.webContents.on('before-input-event', (event, input) => {
    if (input.key === 'F12' || (input.control && input.shift && input.key === 'I')) {
      logsWindow.webContents.toggleDevTools();
    }
  });
  
  logsWindow.on('closed', () => {
    logsWindow = null;
  });
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
 * Sets an initial tray menu while the service is loading.
 * This allows users to interact with the tray immediately.
 */
function setInitialTrayMenu() {
  const contextMenu = Menu.buildFromTemplate([
    {
      label: 'UF API Service',
      sublabel: 'Status: Starting Up ⏳',
      enabled: false
    },
    {
      label: 'Discord: Loading...',
      sublabel: 'OpenAI: Loading...',
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
      label: '🧾 Log Viewer',
      click: () => {
        openLogsWindow();
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
          label: '⚙️ Mod Settings',
          click: () => {
            openModSettingsWindow();
          }
        },
        {
          label: '📚 KB Manager',
          click: () => {
            openKBWindow();
          }
        },
        {
          label: '🗄️ Data Manager',
          click: () => {
            openModManagerWindow();
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
    
    // Process Tunnel status.
    const tunnelStatus = tunnelManager.getStatus();
    let tunnelAction = null;
    const cfg = loadConfigSync();
    const tunnelConfigured = cfg.Tunnel && cfg.Tunnel.enabled && cfg.Tunnel.token;
    let tunnelMenuItems = [];
    if (tunnelConfigured) {
      let tunnelLabel;
      const hasErrors = tunnelStatus.errors && tunnelStatus.errors.length > 0;
      if (tunnelStatus.running) {
        if (tunnelStatus.connectedAt) {
          tunnelLabel = 'Tunnel: Online 🟢';
        } else if (hasErrors) {
          tunnelLabel = 'Tunnel: Error ⚠️';
        } else {
          tunnelLabel = 'Tunnel: Connecting 🟡';
        }
        tunnelAction = { label: '⏹️ Stop Tunnel', click: () => { tunnelManager.stop(); } };
      } else {
        tunnelLabel = 'Tunnel: Offline 🔴';
        tunnelAction = { label: '▶️ Start Tunnel', click: () => { tunnelManager.start(cfg.Tunnel.token).catch(e => { (global.logger || console).error('[Tunnel] Start failed:', e.message); }); } };
      }
      tunnelMenuItems = [{ label: tunnelLabel, enabled: false }];
    }

    const contextMenu = Menu.buildFromTemplate([
      {
        label: `UF API Service ${apiStatusSubLabel}`,
        sublabel: apiStatusLabel,
        enabled: false
      },
      {
        label: discordStatusLabel,
        sublabel: openaiStatusLabel,
        enabled: false
      },
      ...tunnelMenuItems,
      { type: 'separator' },
      {
        label: '🖥️ Console',
        click: () => {
          OpenConsoleWindow();
        }
      },
      {
        label: '🧾 Log Viewer',
        click: () => {
          openLogsWindow();
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
            label: '⚙️ Mod Settings',
            click: () => {
              openModSettingsWindow();
            }
          },
          {
            label: '📚 KB Manager',
            click: () => {
              openKBWindow();
            }
          },
          {
            label: '🗄️ Data Manager',
            click: () => {
              openModManagerWindow();
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
          ...(tunnelAction ? [tunnelAction] : []),
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

function openKBWindow() {
  if (kbWindow) {
    kbWindow.restore();
    kbWindow.focus();
    return;
  }

  kbWindow = new BrowserWindow({
    width: 1200,
    height: 800,
    title: 'Knowledge Base Manager',
    icon: windowIconImage || resolveAssetPath('public', 'icon.ico'),
    webPreferences: {
      nodeIntegration: false,
      contextIsolation: true,
      sandbox: false,
      preload: path.join(__dirname, 'preload', 'kb.js')
    }
  });

  kbWindow.setMenu(null);
  const kbPath = path.join(__dirname, 'views', 'kb.html');
  const kbUrl = pathToFileURL(kbPath);
  kbUrl.searchParams.set('ts', Date.now().toString());
  kbWindow.loadURL(kbUrl.toString());
  kbWindow.on('closed', () => {
    kbWindow = null;
  });
}

function openModSettingsWindow() {
  if (modSettingsWindow) {
    modSettingsWindow.restore();
    modSettingsWindow.focus();
    return;
  }

  modSettingsWindow = new BrowserWindow({
    width: 1200,
    height: 800,
    title: 'Mod Settings',
    icon: windowIconImage || resolveAssetPath('public', 'icon.ico'),
    webPreferences: {
      nodeIntegration: false,
      contextIsolation: true,
      sandbox: false,
      preload: path.join(__dirname, 'preload', 'modSettings.js')
    }
  });

  modSettingsWindow.setMenu(null);
  const msPath = path.join(__dirname, 'views', 'modSettings.html');
  const msUrl = pathToFileURL(msPath);
  msUrl.searchParams.set('ts', Date.now().toString());
  modSettingsWindow.loadURL(msUrl.toString());

  // Enable DevTools shortcuts in dev mode only (Ctrl+Shift+I or F12)
  if (!app.isPackaged) {
    modSettingsWindow.webContents.on('before-input-event', (event, input) => {
      if (input.key === 'F12' || (input.control && input.shift && input.key === 'I')) {
        modSettingsWindow.webContents.toggleDevTools();
      }
    });
  }

  modSettingsWindow.on('closed', () => {
    modSettingsWindow = null;
  });
}

function openModManagerWindow() {
  if (modManagerWindow) {
    modManagerWindow.restore();
    modManagerWindow.focus();
    return;
  }

  modManagerWindow = new BrowserWindow({
    width: 1400,
    height: 900,
    title: 'Data Manager',
    icon: windowIconImage || resolveAssetPath('public', 'icon.ico'),
    webPreferences: {
      nodeIntegration: false,
      contextIsolation: true,
      sandbox: false,
      preload: path.join(__dirname, 'preload', 'modmanager.js')
    }
  });

  modManagerWindow.setMenu(null);
  const modManagerPath = path.join(__dirname, 'views', 'modmanager.html');
  const modManagerUrl = pathToFileURL(modManagerPath);
  modManagerUrl.searchParams.set('ts', Date.now().toString());
  modManagerWindow.loadURL(modManagerUrl.toString());
  modManagerWindow.on('closed', () => {
    modManagerWindow = null;
  });
}

function openIndexOptimizerWindow() {
  if (indexOptimizerWindow) {
    indexOptimizerWindow.restore();
    indexOptimizerWindow.focus();
    return;
  }
  
  // Prevent double-creation if user clicks menu twice quickly
  if (indexOptimizerOpening) return;
  indexOptimizerOpening = true;

  indexOptimizerWindow = new BrowserWindow({
    width: 1400,
    height: 900,
    title: 'Index Optimizer - Universal Framework Service',
    icon: windowIconImage || resolveAssetPath('public', 'icon.ico'),
    webPreferences: {
      nodeIntegration: false,
      contextIsolation: true,
      sandbox: false,
      preload: path.join(__dirname, 'preload', 'indexOptimizer.js')
    }
  });

  indexOptimizerWindow.setMenu(null);
  const indexOptimizerPath = path.join(__dirname, 'views', 'indexOptimizer.html');
  const indexOptimizerUrl = pathToFileURL(indexOptimizerPath);
  indexOptimizerUrl.searchParams.set('ts', Date.now().toString());
  indexOptimizerWindow.loadURL(indexOptimizerUrl.toString());
  
  indexOptimizerWindow.on('ready-to-show', () => {
    indexOptimizerOpening = false;
  });
  
  indexOptimizerWindow.on('closed', () => {
    indexOptimizerOpening = false;
    indexOptimizerWindow = null;
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

  // Intercept navigation to external URLs — open in system browser instead
  settingsWindow.webContents.on('will-navigate', (event, url) => {
    if (url.startsWith('http://') || url.startsWith('https://')) {
      event.preventDefault();
      shell.openExternal(url);
    }
  });
  settingsWindow.webContents.setWindowOpenHandler(({ url }) => {
    if (url.startsWith('http://') || url.startsWith('https://')) {
      shell.openExternal(url);
    }
    return { action: 'deny' };
  });
   
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
  if (kbWindow) {
    kbWindow.removeAllListeners('close');
    kbWindow.close();
  }
  if (modManagerWindow) {
    modManagerWindow.removeAllListeners('close');
    modManagerWindow.close();
  }
  if (indexOptimizerWindow) {
    indexOptimizerWindow.removeAllListeners('close');
    indexOptimizerWindow.close();
  }
  if (modSettingsWindow) {
    modSettingsWindow.removeAllListeners('close');
    modSettingsWindow.close();
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
  if (kbWindow) {
    kbWindow.removeAllListeners('close');
    kbWindow.close();
  }
  if (modManagerWindow) {
    modManagerWindow.removeAllListeners('close');
    modManagerWindow.close();
  }
  if (indexOptimizerWindow) {
    indexOptimizerWindow.removeAllListeners('close');
    indexOptimizerWindow.close();
  }
  if (modSettingsWindow) {
    modSettingsWindow.removeAllListeners('close');
    modSettingsWindow.close();
  }
  if (ConsoleWindow) {
    ConsoleWindow.removeAllListeners('close');
    ConsoleWindow.close();
  }
  if (globalsWindow) {
    globalsWindow.removeAllListeners('close');
    globalsWindow.close();
  }
  if (kbWindow) {
    kbWindow.removeAllListeners('close');
    kbWindow.close();
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
    if ( data === null) {
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

// ===================== Mod Settings IPC Handlers =====================

ipcMain.handle('modSettings:list', async () => {
  const log = global.logger || console;
  log.info('[ModSettings:IPC] list → invoked');
  try {
    const { listModSettings } = getModSettingsModel();
    const data = await listModSettings();
    log.info('[ModSettings:IPC] list → success', { count: Array.isArray(data) ? data.length : 'n/a' });
    return { success: true, data };
  } catch (err) {
    log.error('[ModSettings:IPC] list → FAILED', { error: err.message, stack: err.stack });
    return { success: false, error: err.message };
  }
});

ipcMain.handle('modSettings:get', async (event, modId) => {
  const log = global.logger || console;
  log.info('[ModSettings:IPC] get → invoked', { modId });
  try {
    if (!modId || typeof modId !== 'string') {
      throw new Error('modId is required.');
    }
    const { getModSettings } = getModSettingsModel();
    const doc = await getModSettings(modId);
    if (!doc) {
      log.warn('[ModSettings:IPC] get → not found', { modId });
      return { success: false, error: 'Mod settings not found.' };
    }
    log.info('[ModSettings:IPC] get → success', { modId, hasTemplate: !!doc.template, templateLen: doc.template?.length });
    return { success: true, data: doc };
  } catch (err) {
    log.error('[ModSettings:IPC] get → FAILED', { modId, error: err.message, stack: err.stack });
    return { success: false, error: err.message };
  }
});

ipcMain.handle('modSettings:loadGlobal', async (event, globalName) => {
  const log = global.logger || console;
  log.info('[ModSettings:IPC] loadGlobal → invoked', { globalName });
  try {
    if (!globalName || typeof globalName !== 'string') {
      throw new Error('globalName is required.');
    }
    const { getGlobalDocument } = getGlobalModel();
    const doc = await getGlobalDocument(globalName);
    if (!doc) {
      log.info('[ModSettings:IPC] loadGlobal → no document found (returning null)', { globalName });
      return { success: true, data: null };
    }
    log.info('[ModSettings:IPC] loadGlobal → success', { globalName, hasData: !!doc.data, keys: doc.data ? Object.keys(doc.data) : [] });
    return { success: true, data: doc };
  } catch (err) {
    log.error('[ModSettings:IPC] loadGlobal → FAILED', { globalName, error: err.message, stack: err.stack });
    return { success: false, error: err.message };
  }
});

ipcMain.handle('modSettings:saveGlobal', async (event, payload) => {
  const log = global.logger || console;
  log.info('[ModSettings:IPC] saveGlobal → invoked', { globalName: payload?.globalName });
  try {
    const globalName = payload?.globalName;
    const data = payload?.data;
    if (!globalName || typeof globalName !== 'string') {
      throw new Error('globalName is required.');
    }
    if (data === null || data === undefined) {
      throw new Error('Data is required.');
    }
    const { saveGlobalDocument } = getGlobalModel();
    await saveGlobalDocument(globalName, data);
    log.info('[ModSettings:IPC] saveGlobal → success', { globalName });
    return { success: true };
  } catch (err) {
    log.error('[ModSettings:IPC] saveGlobal → FAILED', { globalName: payload?.globalName, error: err.message, stack: err.stack });
    return { success: false, error: err.message };
  }
});

ipcMain.handle('modSettings:openExternal', async (event, url) => {
  const log = global.logger || console;
  log.info('[ModSettings:IPC] openExternal → invoked', { url });
  try {
    if (!url || typeof url !== 'string') {
      throw new Error('URL is required.');
    }
    // Only allow http and https URLs
    if (!/^https?:\/\//i.test(url)) {
      throw new Error('Only http/https URLs are allowed.');
    }
    const { shell } = require('electron');
    await shell.openExternal(url);
    log.info('[ModSettings:IPC] openExternal → success', { url });
    return { success: true };
  } catch (err) {
    log.error('[ModSettings:IPC] openExternal → FAILED', { url, error: err.message });
    return { success: false, error: err.message };
  }
});

// ===================== Logs IPC Handlers =====================

// Lazy load MongoDB client for logs
let cachedLogsClient = null;

async function getLogsCollection() {
  const { MongoClient } = require('mongodb');
  if (!cachedLogsClient) {
    cachedLogsClient = new MongoClient(global.config.DBServer);
    await cachedLogsClient.connect();
  }
  return cachedLogsClient.db(global.config.DB).collection('Logs');
}

ipcMain.handle('logs:query', async (event, filters = {}) => {
  try {
    const collection = await getLogsCollection();
    
    const query = {};
    const options = {
      sort: { LoggedDateTime: -1 },
      limit: parseInt(filters.limit) || 50,
      skip: ((parseInt(filters.page) || 1) - 1) * (parseInt(filters.limit) || 50)
    };

    // Search filter (text search in Message field + DayZ mod Log/Action/Item fields)
    if (filters.search) {
      query.$or = [
        { Message: { $regex: filters.search, $options: 'i' } },
        { message: { $regex: filters.search, $options: 'i' } },
        { Log: { $regex: filters.search, $options: 'i' } },
        { Action: { $regex: filters.search, $options: 'i' } },
        { Item: { $regex: filters.search, $options: 'i' } },
        { Target: { $regex: filters.search, $options: 'i' } },
        { GUID: { $regex: filters.search, $options: 'i' } },
        { KilledBy: { $regex: filters.search, $options: 'i' } }
      ];
    }

    // Server ID filter
    if (filters.serverId) {
      query.ServerId = filters.serverId;
    }

    // Level filter - handle case-insensitive field name (Level or level)
    if (filters.levels && Array.isArray(filters.levels) && filters.levels.length > 0) {
      const levelCondition = { 
        $or: [
          { Level: { $in: filters.levels } },
          { level: { $in: filters.levels } }
        ]
      };
      
      // If we already have a search $or, combine with $and
      if (query.$or) {
        query.$and = [
          { $or: query.$or },
          levelCondition
        ];
        delete query.$or;
      } else {
        // Just add the level condition directly
        query.$and = [levelCondition];
      }
    }

    // Client type filter
    if (filters.clientType) {
      query.ClientType = filters.clientType;
    }

    // Log Type filter
    if (filters.logType) {
      query.Type = filters.logType;
    }

    // Date range filter
    if (filters.dateFrom || filters.dateTo) {
      query.LoggedDateTime = {};
      if (filters.dateFrom) {
        query.LoggedDateTime.$gte = new Date(filters.dateFrom);
      }
      if (filters.dateTo) {
        query.LoggedDateTime.$lte = new Date(filters.dateTo);
      }
    }

    // Specific ID lookup
    if (filters._id) {
      const { ObjectId } = require('mongodb');
      try {
        query._id = new ObjectId(filters._id);
        console.log('[logs:query] Looking up by _id:', filters._id);
      } catch (idErr) {
        console.error('[logs:query] Invalid ObjectId:', filters._id, idErr.message);
        return { logs: [], total: 0, error: 'Invalid log ID format' };
      }
    }

    console.log('[logs:query] Final query:', JSON.stringify(query));
    const [logs, total] = await Promise.all([
      collection.find(query, options).toArray(),
      collection.countDocuments(query)
    ]);

    // Convert ObjectId to string for IPC serialization
    const serializedLogs = logs.map(log => ({
      ...log,
      _id: log._id?.toString?.() || String(log._id)
    }));

    return { logs: serializedLogs, total };
  } catch (err) {
    (global.logger || console).error('[LogViewer] Failed to query logs', { error: err.message });
    return { logs: [], total: 0, error: err.message };
  }
});

ipcMain.handle('logs:getServers', async () => {
  try {
    const collection = await getLogsCollection();
    const servers = await collection.distinct('ServerId');
    return servers.filter(s => s); // Filter out null/undefined
  } catch (err) {
    (global.logger || console).error('[LogViewer] Failed to get servers', { error: err.message });
    return [];
  }
});

ipcMain.handle('logs:getTypes', async () => {
  try {
    const collection = await getLogsCollection();
    const types = await collection.distinct('Type');
    return types.filter(t => t); // Filter out null/undefined
  } catch (err) {
    (global.logger || console).error('[LogViewer] Failed to get types', { error: err.message });
    return [];
  }
});

ipcMain.handle('logs:getStats', async (event, filters = {}) => {
  try {
    const collection = await getLogsCollection();
    
    const matchQuery = {};
    
    if (filters.serverId) {
      matchQuery.ServerId = filters.serverId;
    }
    if (filters.clientType) {
      matchQuery.ClientType = filters.clientType;
    }
    if (filters.dateFrom || filters.dateTo) {
      matchQuery.LoggedDateTime = {};
      if (filters.dateFrom) matchQuery.LoggedDateTime.$gte = new Date(filters.dateFrom);
      if (filters.dateTo) matchQuery.LoggedDateTime.$lte = new Date(filters.dateTo);
    }

    const pipeline = [
      { $match: matchQuery },
      {
        $group: {
          _id: { $toLower: { $ifNull: ['$Level', '$level'] } },
          count: { $sum: 1 }
        }
      }
    ];

    const results = await collection.aggregate(pipeline).toArray();
    
    const stats = {
      total: 0,
      info: 0,
      warn: 0,
      error: 0,
      debug: 0
    };

    results.forEach(r => {
      const level = r._id || 'info';
      stats.total += r.count;
      if (level === 'info') stats.info = r.count;
      else if (level === 'warn' || level === 'warning') stats.warn += r.count;
      else if (level === 'error') stats.error = r.count;
      else if (level === 'debug') stats.debug = r.count;
    });

    return stats;
  } catch (err) {
    (global.logger || console).error('[LogViewer] Failed to get stats', { error: err.message });
    return { total: 0, info: 0, warn: 0, error: 0, debug: 0 };
  }
});

ipcMain.handle('logs:delete', async (event, filters = {}) => {
  try {
    const collection = await getLogsCollection();
    
    const query = {};
    if (filters.serverId) query.ServerId = filters.serverId;
    if (filters.dateFrom || filters.dateTo) {
      query.LoggedDateTime = {};
      if (filters.dateFrom) query.LoggedDateTime.$gte = new Date(filters.dateFrom);
      if (filters.dateTo) query.LoggedDateTime.$lte = new Date(filters.dateTo);
    }

    if (Object.keys(query).length === 0) {
      return { success: false, error: 'At least one filter is required to delete logs' };
    }

    const result = await collection.deleteMany(query);
    return { success: true, deletedCount: result.deletedCount };
  } catch (err) {
    (global.logger || console).error('[LogViewer] Failed to delete logs', { error: err.message });
    return { success: false, error: err.message };
  }
});

// ===================== KB IPC Handlers =====================

ipcMain.handle('kb:list', async () => {
  try {
    const { listKBs } = getKBModel();
    const data = await listKBs();
    return { success: true, data };
  } catch (err) {
    (global.logger || console).error('[KBManager] Failed to list KBs', { error: err.message });
    return { success: false, error: err.message };
  }
});

ipcMain.handle('kb:get', async (event, kbId) => {
  try {
    if (!kbId) throw new Error('KB ID is required.');
    const { getKB } = getKBModel();
    const data = await getKB(kbId);
    if (!data) {
      return { success: false, error: 'KB not found.' };
    }
    return { success: true, data };
  } catch (err) {
    (global.logger || console).error('[KBManager] Failed to get KB', { kbId, error: err.message });
    return { success: false, error: err.message };
  }
});

ipcMain.handle('kb:create', async (event, payload) => {
  try {
    const { kbId, name, description, shorterAnswers, extractModel } = payload || {};
    if (!kbId || !name) throw new Error('KB ID and name are required.');
    const { createKB } = getKBModel();
    const data = await createKB(kbId, name, description, { shorterAnswers, extractModel });
    return { success: true, data };
  } catch (err) {
    (global.logger || console).error('[KBManager] Failed to create KB', { error: err.message });
    return { success: false, error: err.message };
  }
});

ipcMain.handle('kb:update', async (event, payload) => {
  try {
    const { kbId, data } = payload || {};
    if (!kbId) throw new Error('KB ID is required.');
    const { updateKB } = getKBModel();
    const success = await updateKB(kbId, data);
    return { success };
  } catch (err) {
    (global.logger || console).error('[KBManager] Failed to update KB', { error: err.message });
    return { success: false, error: err.message };
  }
});

ipcMain.handle('kb:delete', async (event, kbId) => {
  try {
    if (!kbId) throw new Error('KB ID is required.');
    const { deleteKB } = getKBModel();
    const success = await deleteKB(kbId);
    return { success };
  } catch (err) {
    (global.logger || console).error('[KBManager] Failed to delete KB', { error: err.message });
    return { success: false, error: err.message };
  }
});

ipcMain.handle('kb:listDocuments', async (event, kbId) => {
  try {
    if (!kbId) throw new Error('KB ID is required.');
    const { listDocuments } = getKBModel();
    const data = await listDocuments(kbId);
    return { success: true, data };
  } catch (err) {
    (global.logger || console).error('[KBManager] Failed to list documents', { error: err.message });
    return { success: false, error: err.message };
  }
});

ipcMain.handle('kb:getDocument', async (event, payload) => {
  try {
    const { kbId, documentId } = payload || {};
    if (!kbId || !documentId) throw new Error('KB ID and Document ID are required.');
    const { getDocument } = getKBModel();
    const data = await getDocument(kbId, documentId);
    if (!data) {
      return { success: false, error: 'Document not found.' };
    }
    return { success: true, data };
  } catch (err) {
    (global.logger || console).error('[KBManager] Failed to get document', { error: err.message });
    return { success: false, error: err.message };
  }
});

ipcMain.handle('kb:addDocument', async (event, payload) => {
  try {
    const { kbId, name, content, contextHint, fileType } = payload || {};
    if (!kbId || !name || !content) throw new Error('KB ID, name, and content are required.');
    const { addDocument, updateDocumentEmbeddings, splitTextIntoChunks } = getKBModel();
    const kbController = getKBController();
    
    // Add the document
    const result = await addDocument(kbId, name, content, contextHint || '', fileType || 'txt');
    
    // Generate embeddings
    try {
      const chunks = splitTextIntoChunks(content);
      const embeddings = await kbController.generateEmbeddings(chunks);
      await updateDocumentEmbeddings(kbId, result.documentId, embeddings);
      result.hasEmbedding = true;
    } catch (embErr) {
      (global.logger || console).warn('[KBManager] Failed to generate embeddings', { error: embErr.message });
      result.hasEmbedding = false;
    }
    
    return { success: true, data: result };
  } catch (err) {
    (global.logger || console).error('[KBManager] Failed to add document', { error: err.message });
    return { success: false, error: err.message };
  }
});

ipcMain.handle('kb:updateDocument', async (event, payload) => {
  try {
    const { kbId, documentId, name, content, contextHint } = payload || {};
    if (!kbId || !documentId) throw new Error('KB ID and Document ID are required.');
    const { updateDocument, updateDocumentEmbeddings, splitTextIntoChunks, getDocument } = getKBModel();
    const kbController = getKBController();
    
    // Get current document to check if content changed
    const existingDoc = await getDocument(kbId, documentId);
    const contentChanged = content !== null && content !== undefined;
    
    const result = await updateDocument(kbId, documentId, name, content, contextHint);
    
    // Always regenerate embeddings if content was provided (changed)
    if (contentChanged) {
      try {
        (global.logger || console).info('[KBManager] Regenerating embeddings for updated document', { 
          kbId, 
          documentId, 
          name: name || existingDoc?.chunks?.[0]?.name 
        });
        const chunks = splitTextIntoChunks(content);
        const embeddings = await kbController.generateEmbeddings(chunks);
        await updateDocumentEmbeddings(kbId, documentId, embeddings);
        result.hasEmbedding = true;
        result.embeddingsGenerated = embeddings.length;
        (global.logger || console).info('[KBManager] Embeddings regenerated successfully', { 
          kbId, 
          documentId, 
          chunks: embeddings.length 
        });
      } catch (embErr) {
        (global.logger || console).error('[KBManager] Failed to regenerate embeddings', { 
          kbId, 
          documentId, 
          error: embErr.message 
        });
        result.hasEmbedding = false;
        result.embeddingError = embErr.message;
      }
    }
    
    return { success: true, data: result };
  } catch (err) {
    (global.logger || console).error('[KBManager] Failed to update document', { error: err.message });
    return { success: false, error: err.message };
  }
});

ipcMain.handle('kb:deleteDocument', async (event, payload) => {
  try {
    const { kbId, documentId } = payload || {};
    if (!kbId || !documentId) throw new Error('KB ID and Document ID are required.');
    const { deleteDocument } = getKBModel();
    const success = await deleteDocument(kbId, documentId);
    return { success };
  } catch (err) {
    (global.logger || console).error('[KBManager] Failed to delete document', { error: err.message });
    return { success: false, error: err.message };
  }
});

ipcMain.handle('kb:regenerateEmbeddings', async (event, kbId) => {
  try {
    if (!kbId) throw new Error('KB ID is required.');
    const { getDocumentsMissingEmbeddings, updateDocumentEmbeddings } = getKBModel();
    const kbController = getKBController();
    
    const docsMissing = await getDocumentsMissingEmbeddings(kbId);
    if (docsMissing.length === 0) {
      return { success: true, processed: 0, message: 'All documents have embeddings' };
    }

    // Group by documentId
    const byDoc = {};
    for (const doc of docsMissing) {
      if (!byDoc[doc.documentId]) byDoc[doc.documentId] = [];
      byDoc[doc.documentId].push(doc);
    }

    let processed = 0;
    let failed = 0;

    for (const [documentId, chunks] of Object.entries(byDoc)) {
      try {
        chunks.sort((a, b) => a.chunkIndex - b.chunkIndex);
        const contents = chunks.map(c => c.content);
        const embeddings = await kbController.generateEmbeddings(contents);
        await updateDocumentEmbeddings(kbId, documentId, embeddings);
        processed++;
      } catch (err) {
        (global.logger || console).warn('[KBManager] Failed to regenerate embeddings for document', { documentId, error: err.message });
        failed++;
      }
    }

    return { success: true, processed, failed, total: Object.keys(byDoc).length };
  } catch (err) {
    (global.logger || console).error('[KBManager] Failed to regenerate embeddings', { error: err.message });
    return { success: false, error: err.message };
  }
});

ipcMain.handle('kb:listModels', async () => {
  try {
    const kbController = getKBController();
    const models = await kbController.listOpenAIModels();
    return { success: true, models };
  } catch (err) {
    (global.logger || console).error('[KBManager] Failed to list OpenAI models', { error: err.message });
    return { success: false, error: err.message };
  }
});

// Mod Data Manager IPC Handlers
ipcMain.handle('modmanager:list', async () => {
  try {
    const { scanInstalledMods } = getModDataModel();
    const data = await scanInstalledMods();
    (global.logger || console).info('[ModManager] List request processed', { count: Array.isArray(data) ? data.length : 'n/a' });
    return { success: true, data };
  } catch (err) {
    const errorMessage = err?.message || String(err);
    const errorStack = err?.stack || '';
    (global.logger || console).error('[ModManager] Failed to list mods', { 
      error: errorMessage,
      stack: errorStack
    });
    return { success: false, error: errorMessage };
  }
});

ipcMain.handle('modmanager:delete', async (event, modName) => {
  try {
    if (!modName || typeof modName !== 'string') {
      throw new Error('Mod name is required and must be a string');
    }
    const { deleteModData } = getModDataModel();
    const data = await deleteModData(modName);
    (global.logger || console).info('[ModManager] Delete request processed', { modName, deleted: data.totalDeleted });
    return { success: true, data };
  } catch (err) {
    (global.logger || console).error('[ModManager] Failed to delete mod data', { modName, error: err.message });
    return { success: false, error: err.message };
  }
});

// Index Optimizer IPC Handlers
ipcMain.handle('indexOptimizer:getRecommendations', async () => {
  try {
    const indexManager = getIndexManager();
    const recommendations = await indexManager.getRecommendations();
    (global.logger || console).info('[IndexOptimizer] Recommendations retrieved');
    return recommendations;
  } catch (err) {
    (global.logger || console).error('[IndexOptimizer] Failed to get recommendations', { error: err.message });
    throw err;
  }
});

ipcMain.handle('indexOptimizer:getCurrentIndexes', async () => {
  try {
    const indexManager = getIndexManager();
    const indexes = await indexManager.getAllIndexes();
    (global.logger || console).info('[IndexOptimizer] Current indexes retrieved');
    return indexes;
  } catch (err) {
    (global.logger || console).error('[IndexOptimizer] Failed to get current indexes', { error: err.message });
    throw err;
  }
});

ipcMain.handle('indexOptimizer:createIndex', async (event, indexRequest) => {
  try {
    const indexManager = getIndexManager();
    const result = await indexManager.createIndex(
      indexRequest.collection,
      indexRequest.indexSpec,
      indexRequest.options || {}
    );
    (global.logger || console).info('[IndexOptimizer] Index created', {
      collection: indexRequest.collection,
      indexName: result.indexName
    });
    return result;
  } catch (err) {
    (global.logger || console).error('[IndexOptimizer] Failed to create index', {
      collection: indexRequest.collection,
      error: err.message
    });
    throw err;
  }
});

app.on('window-all-closed', (e) => {
  e.preventDefault();
});

// Cleanup on app exit
app.on('will-quit', async () => {
  // Stop cloudflared tunnel & update checks
  try {
    tunnelManager.stopUpdateChecks();
    tunnelManager.stop();
  } catch (err) {
    (global.logger || console).warn('Error stopping tunnel on exit', { error: err.message });
  }

  // Close MongoDB connection from index manager
  try {
    const indexManager = getIndexManager();
    if (indexManager && indexManager.closeConnection) {
      await indexManager.closeConnection();
    }
  } catch (err) {
    (global.logger || console).warn('Error closing IndexManager connection on exit', { error: err.message });
  }
});

/**
 * Register an IPC handler for "get-proxy-domains" and return the available domains
 * from the hard-coded Cloudflare Worker endpoint.
 */
let lastProxyRegistrationTime = 0;

function formatProxyDomainError(error) {
  if (!error) {
    return 'Unknown error while loading proxy domains.';
  }
  const pieces = [];
  if (error.message) {
    pieces.push(error.message);
  }
  if (error.code) {
    pieces.push(`code: ${error.code}`);
  }
  if (!pieces.length) {
    pieces.push(String(error));
  }
  return pieces.join(' | ');
}

ipcMain.handle('get-proxy-domains', async () => {
  if (!fetch) {
    const message = "Fetch implementation unavailable; cannot load proxy domains.";
    console.error(message);
    return { success: false, domains: [], error: message };
  }
  try {
    const response = await fetch("https://ufapi.daemonforge.dev/available");
    if (!response.ok) {
      let bodyText = '';
      try {
        bodyText = await response.text();
      } catch (bodyErr) {
        bodyText = bodyErr?.message ? `Response body unavailable: ${bodyErr.message}` : '';
      }
      const statusText = response.statusText ? ` ${response.statusText}` : '';
      const detail = bodyText ? ` - ${bodyText}` : '';
      throw new Error(`HTTP ${response.status}${statusText}${detail}`);
    }
    const data = await response.json();
    if (!Array.isArray(data)) {
      throw new Error("Received malformed proxy domain list from server.");
    }
    return { success: true, domains: data, error: null };
  } catch (error) {
    const formatted = formatProxyDomainError(error);
    console.error("Error fetching proxy domains:", error);
    return { success: false, domains: [], error: formatted };
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

// ----------------------- External Link Handler -----------------------
ipcMain.handle('open-external', async (event, url) => {
  // Only allow http/https URLs to prevent shell injection
  if (typeof url === 'string' && (url.startsWith('https://') || url.startsWith('http://'))) {
    await shell.openExternal(url);
    return true;
  }
  return false;
});

// ----------------------- Cloudflare Tunnel IPC Handlers -----------------------

/**
 * Auto-start the tunnel on app launch if configured.
 */
function startTunnelIfConfigured() {
  try {
    const cfg = loadConfigSync();
    if (cfg.Tunnel && cfg.Tunnel.enabled && cfg.Tunnel.token && cfg.Tunnel.autoStart) {
      (global.logger || console).info('[Tunnel] Auto-starting cloudflared tunnel...');
      tunnelManager.start(cfg.Tunnel.token).catch(err => {
        (global.logger || console).error('[Tunnel] Auto-start failed:', err.message);
      });
    }
  } catch (err) {
    (global.logger || console).error('[Tunnel] Error checking tunnel config for auto-start:', err.message);
  }
}

ipcMain.handle('tunnel-start', async () => {
  try {
    const cfg = loadConfigSync();
    if (!cfg.Tunnel || !cfg.Tunnel.token) {
      return { success: false, error: 'No tunnel token configured. Add your Cloudflare tunnel token in Settings.' };
    }
    await tunnelManager.start(cfg.Tunnel.token);
    return { success: true };
  } catch (err) {
    return { success: false, error: err.message };
  }
});

ipcMain.handle('tunnel-stop', async () => {
  try {
    tunnelManager.stop();
    return { success: true };
  } catch (err) {
    return { success: false, error: err.message };
  }
});

ipcMain.handle('tunnel-status', async () => {
  return tunnelManager.getStatus();
});

ipcMain.handle('tunnel-download', async () => {
  try {
    if (tunnelManager.isBinaryInstalled()) {
      return { success: true, message: 'cloudflared already installed' };
    }
    await tunnelManager.downloadBinary();
    return { success: true, message: 'cloudflared downloaded successfully' };
  } catch (err) {
    return { success: false, error: err.message };
  }
});

ipcMain.handle('tunnel-check-update', async () => {
  try {
    const result = await tunnelManager.checkForUpdate();
    return { success: true, ...result };
  } catch (err) {
    return { success: false, error: err.message };
  }
});

ipcMain.handle('tunnel-update', async () => {
  try {
    const cfg = loadConfigSync();
    const token = (cfg.Tunnel && cfg.Tunnel.token) || null;
    await tunnelManager.update(token);
    return { success: true };
  } catch (err) {
    return { success: false, error: err.message };
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