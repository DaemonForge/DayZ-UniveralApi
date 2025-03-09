const { app, Tray, Menu, shell } = require('electron');
const path = require('path');
const { exec } = require('child_process');
const { dialog } = require('electron');
global.SAVEPATH = `${app.getPath('userData')}/`;
global.isElectron = true;
global.APIVERSION = process.env.npm_package_version || app.getVersion();
let tray = null;
// Define the folder path where the files are saved
const filesFolderPath = `${path.join(app.getPath('userData'))}/`; // update this path if needed

app.on('ready', () => {
    checkAndInstallMongoDB();
    // Create the system tray icon
    tray = new Tray(path.join(__dirname, 'public', 'universalFrameworklogo.png'));
    
    const contextMenu = Menu.buildFromTemplate([
        {
            label: 'Restart',
            click: () => {
                app.relaunch();
                app.exit();
            }
        },
        {
            label: 'Stop',
            click: () => {
                app.quit();
            }
        },
        {
            label: 'Files',
            submenu: [
                {
                    label: 'Configs',
                    click: () => {
                        shell.openPath(filesFolderPath);
                    }
                }
            ]
        }
    ]);
    
    tray.setToolTip('UF Service');
    tray.setContextMenu(contextMenu);
    const ufService = require('./app'); 
});

function checkAndInstallMongoDB() {
    // This is a simplified check; in a real scenario, you might check for a running service or the existence of the executable.
    //winget install -e --id MongoDB.Server
    exec('winget list MongoDB.Server', (error, stdout, stderr) => {
      if (error || stdout.indexOf('MongoDB') === -1) {
        // MongoDB not found, prompt user
        dialog.showMessageBox({
          type: 'info',
          buttons: ['Install', 'Cancel'],
          title: 'Universal Framework Service',
          message: 'MongoDB is not installed. Would you like to install it now?'
        }).then(result => {
          if (result.response === 0) { // User chose 'Install'
            // Run the winget install command
            exec('winget install --id MongoDB.Server', (err, out, errOut) => {
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
    if (url === `https://localhost:${global.config.Port}/Status` || url === `https://localhost/Status`) {
      // Verification logic.
      //console.log("preventing cert error")
      event.preventDefault()
      callback(true)
    } else {
      callback(false)
    }
  })
  