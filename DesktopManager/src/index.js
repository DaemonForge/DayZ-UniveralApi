const { app, BrowserWindow, ipcMain, Menu, Tray , globalShortcut, shell, dialog} = require('electron');
const ejse = require('ejs-electron');
const { MongoClient } = require("mongodb");
var iconpath = `${__dirname}/icon.ico`; // pa


// Handle creating/removing shortcuts on Windows when installing/uninstalling.
if (require('electron-squirrel-startup')) { // eslint-disable-line global-require
  app.quit();
}
ejse.data('theme', 'auto');
ejse.data('logs', global.logs);
ejse.data('config', global.config);
ejse.data('projects', [{name: "Project 1", img: "someimage", path: "/path/"}, {name: "Project 2", img: "someimage", path: "/path/"}, {name: "Project 3", img: "someimage", path: "/path/"}])

let isQuiting;
let appIcon;
let showState = true;
global.logs = [];
const createWindow = () => {
   
  // Create the browser window.
  global.mainWindow = new BrowserWindow({
    width: 800,
    height: 550,
    title: "Universal API Manager",
    darkTheme: true,
    icon: iconpath,
    webPreferences: {
      nodeIntegration: true,
      contextIsolation: false,
    }
  });
  global.mainWindow.setMenuBarVisibility(false);
  // and load the index.html of the app.
  global.mainWindow.loadURL('file://' + __dirname + '/views/index.ejs');

  appIcon = new Tray(iconpath)
  let contextMenu = Menu.buildFromTemplate([
      {
          label: 'Show/Hide', click: function () {
            if (showState){
              global.mainWindow.hide()
            } else {
              global.mainWindow.show()
            }
            showState = !showState;
          }
      },
      {
          label: 'Shutdown', click: function () {
              isQuiting = true;
              global.mainWindow.destroy()
              appIcon.destroy();
              app.quit();
              app.exit();
          }
      },
      {
        label: 'Restart', click: function () {
          app.relaunch()
          app.exit()
        }
     }
  ])

  appIcon.setContextMenu(contextMenu)
  // Open the DevTools.
  //global.mainWindow.webContents.openDevTools();

  
  global.mainWindow.on('close', function (event) {
    if (!isQuiting) {
      //console.log(global.logs)
      event.preventDefault();
      global.mainWindow.hide();
      event.returnValue = false;
      showState = false;
    } else {
      app.quit();
    }
  });

  global.mainWindow.webContents.on('new-window', function(e, url) {
    e.preventDefault();
    shell.openExternal(url);
  });
};

app.on('before-quit', function () {
  isQuiting = true;
});


// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
// Some APIs can only be used after this event occurs.
app.on('ready', createWindow);

// Quit when all windows are closed, except on macOS. There, it's common
// for applications and their menu bar to stay active until the user quits
// explicitly with Cmd + Q.
app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') {
    app.quit();
  }
});

app.on('activate', () => {
  // On OS X it's common to re-create a window in the app when the
  // dock icon is clicked and there are no other windows open.
  if (BrowserWindow.getAllWindows().length === 0) {
    createWindow();
  }
});
app.on('certificate-error', (event, webContents, url, error, certificate, callback) => {
  if (url === 'https://localhost/Status') {
    // Verification logic.
    //console.log("preventing cert error")
    event.preventDefault()
    callback(true)
  } else {
    callback(false)
  }
})

ipcMain.on('message', (event, arg) => {
  console.log(arg) // prints "ping"
  tmp = arg;
})
ipcMain.on('OpenTemplatesFolder', (event, arg) => {
  shell.openPath(`${__dirname}\\..\\..\\..\\templates\\`) // Show the given file in a file manager. If possible, select the file.
})
ipcMain.on('RestartApp', (event, arg) => {
  app.relaunch()
  app.exit()
})
ipcMain.on('OpenLogsFolder', (event, arg) => {
  shell.openPath(`${__dirname}\\..\\..\\..\\logs\\`) // Show the given file in a file manager. If possible, select the file.
})


ipcMain.on('OpenConfirmationDialog', (event, arg) => {
  //console.log(arg);
  //console.log(arg.mod)
  //console.log(arg.collection)
  let options  = {
      buttons: ["Cancel",`Delete all "${arg.collection}" data for "${arg.mod}"`],
      message: `Are you sure?`,
      title: `Are you sure?`
  }
  //aSynchronous modal - stay alway on top
  dialog.showMessageBox(options).then( (response) => {
      if (response.response === 1){
        deleteModData(arg.collection, arg.mod)
      } else {

      }
  });
})

ipcMain.on('SaveGlobalMod', (event, arg) => {
  SaveGlobalMod(arg)

})


async function SaveGlobalMod(data){
  
  const client = new MongoClient(global.config.DBServer, { useUnifiedTopology: true });
  try {
    await client.connect();
    const db = client.db(global.config.DB);
    let collection = db.collection("Globals");
    let query = { "Mod": data.Mod };
      let result = await collection.updateOne(query, {"$set": data}, { upsert: false });
      console.log(result.result);
      let response = { n: result.result.n, status: result.result.ok, mod: data.Mod, col: "Gobals"}
      global.mainWindow.send("UpdateModGlobal", response)

  } catch (err) {
    console.log(err)
  } finally {
    
    client.close();
  }


  GetModList();
}

ipcMain.on('RequestModListGlobals', (event, arg) => {
  GetModList();
})

let https = require('./WebServer/app');

async function deleteModData(col, mod){
  const client = new MongoClient(global.config.DBServer, { useUnifiedTopology: true });
  try {
    await client.connect();
    const db = client.db(global.config.DB);
    let query = {};
    let collection = db.collection(col);
    if (col === 'Players'){
      query[mod] = { "$exists": true };
      let result = await collection.updateMany(query, {"$unset": JSON.parse(`{ "${mod}": "" }`)});
      //console.log(result.result);
      let response = { n: result.result.n, status: result.result.ok, mod: mod, col: col}
      global.mainWindow.send("DeleteResults", response)
    } else if (col === 'Objects'){
      query.Mod = mod;
      let result = await collection.deleteMany(query);
      //console.log(result.result);
      let response = { n: result.result.n, status: result.result.ok, mod: mod, col: col}
      global.mainWindow.send("DeleteResults", response)
    }

  } catch (err) {
    console.log(err)
  } finally {
    
    client.close();
  }
}
async function GetModList(){
  const client = new MongoClient(global.config.DBServer, { useUnifiedTopology: true });
  try {
    await client.connect();
    const db = client.db(global.config.DB);
    let query = { "Mod": { "$exists": true }};
    let collection = db.collection("Globals");
    let result = collection.find(query);
    let count = await result.count()
    //console.log(count);
    let response = [];
    await result.forEach(e => {
      if (e.Mod !== "UniversalApiStatus"){ 
        response.push(e);
      }
    })

    global.mainWindow.send("ModListGlobals", response)

  } catch (err) {
    console.log(err)
  } finally {
    
    client.close();
  }
}