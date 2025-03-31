const { app, Tray, Menu, shell, BrowserWindow, dialog, ipcMain } = require('electron');
const winston = require('winston');
const path = require('path');
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
  tray = new Tray(path.join(__dirname, 'public', 'icon.ico'));
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
    icon: path.join(__dirname, 'public', 'icon.ico'), // Use .ico for Windows, or .png if preferred
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
        icon: path.join(__dirname, 'public', 'icon32x32.png')
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

function openSettingsWindow() {
  if (settingsWindow) {
    settingsWindow.restore();
    settingsWindow.focus();
    return;
  }
  
  settingsWindow = new BrowserWindow({
    width: 800,
    height: 600,
    title: "Unviersal Framework Settings",
    icon: path.join(__dirname, 'public', 'icon.ico'), // Use .ico for Windows, or .png if preferred
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
    const configData = readFileSync(configPath, 'utf-8');
    return JSON.parse(configData);
  } catch (err) {
    console.error("Error reading config:", err);
    return null;
  }
});

// IPC handler to save the new configuration
ipcMain.handle('save-config', (event, newConfig) => {
  const configPath = path.join(global.SAVEPATH, 'config.json');
  try {
    writeFileSync(configPath, JSON.stringify(newConfig, null, 2));
    return { success: true };
  } catch (err) {
    console.error("Error saving config:", err);
    return { success: false, error: err };
  }
});
ipcMain.on('force-close', () => {
  // Remove the close event handler to avoid an infinite loop
  settingsWindow.removeAllListeners('close');
  settingsWindow.close();
});
ipcMain.on('restart-app', () => {
  settingsWindow.removeAllListeners('close');
  settingsWindow.close();
  app.relaunch();
  app.exit();
});

app.on('window-all-closed', (e) => {
  e.preventDefault();
});