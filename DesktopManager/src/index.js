const { app, autoUpdater, remote, BrowserWindow, ipcMain, Menu, Tray , globalShortcut, shell, dialog} = require('electron');
const ejse = require('ejs-electron');
const { MongoClient } = require("mongodb");
var iconpath = `${__dirname}/icon.ico`; // pa
const {writeFileSync} = require('fs');
let {createHash} = require('crypto');

const server = "https://hazel.daemonforge.dev"
const feed = `${server}/update/${process.platform}/${app.getVersion()}`

autoUpdater.setFeedURL({url: feed, serverType: 'json'});

global.PENDINGUPDATE = false;
global.SAVEPATH = (app || remote.app).getPath('userData') + "/";
global.APIVERSION = process.env.npm_package_version || app.getVersion();

// Handle creating/removing shortcuts on Windows when installing/uninstalling.
if (require('electron-squirrel-startup')) { // eslint-disable-line global-require
  app.quit();
}
ejse.data('theme', 'auto');
ejse.data('logs', global.logs);
ejse.data('config', global.config);
ejse.data('pendingupdate', global.PENDINGUPDATE);

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
          if (global.PENDINGUPDATE){
            autoUpdater.quitAndInstall();
          } else {
            app.relaunch()
            app.exit()
          }
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
  if (url === `https://localhost:${global.config.Port}/Status` || url === `https://localhost/Status`) {
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
  shell.openPath(global.SAVEPATH + 'templates\\') // Show the given file in a file manager.
})
ipcMain.on('RestartApp', (event, arg) => {
  
  if (global.PENDINGUPDATE){
    autoUpdater.quitAndInstall();
  } else {
    app.relaunch()
    app.exit()
  }
})
ipcMain.on('OpenLogsFolder', (event, arg) => {
  shell.openPath(global.SAVEPATH + 'logs\\') // Show the given file in a file manager.
})


ipcMain.on('OpenConfirmationDialog', (event, arg) => {
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

ipcMain.on('RequestFromDatabase', (event, arg) => {
  RequestFromDatabase(arg)

})

ipcMain.on('SaveConfig', (event, arg) => {
  SaveConfig(arg)

})

async function RequestFromDatabase(arg){
  
  const mongo = new MongoClient(global.config.DBServer, { useUnifiedTopology: true });
  try {
    await mongo.connect();
    const db = mongo.db(global.config.DB);
    let collection = db.collection(arg.Collection);
    let query = {};
    let returnValue = "Data";
    if (arg.Collection === "Players"){
      query["GUID"] = NormalizeToGUID(arg.ID);
      returnValue = arg.Mod;
    } else {
      query["Mod"] = arg.Mod 
      query["ObjectId"] = arg.ID;
    }
    let result = collection.find(query);
    let count = await result.count();
    if (count > 0){
      let res = [];
      await result.forEach(e => {
        res.push(e[returnValue])
      });
      let response = { status: true, Results: res, ID: arg.ID, Mod: arg.Mod, col: arg.Collection}
      global.mainWindow.send("ReceiveDatabaseData", response);
    } else {
      
      global.mainWindow.send("ReceiveDatabaseData", { n: 0, status: false, Results: [], ID: arg.ID, Mod: arg.Mod, col: arg.Collection})
    }
  } catch (err) {
    console.log(err);
  } finally {
      mongo.close();
  }
}

async function SaveOtherData(arg){
  
  const client = new MongoClient(global.config.DBServer, { useUnifiedTopology: true });
  try {
    await client.connect();
    const db = client.db(global.config.DB);
    let collection = db.collection(arg.Collection);
    let query = {};
    let returnValue = "Data";
    if (arg.Collection === "Players"){
      query["GUID"] = NormalizeToGUID(arg.ID);
      returnValue = arg.Mod;
    } else {
      query["Mod"] = arg.Mod 
      query["ObjectId"] = arg.ID;
    }
    let data = {};
    data[returnValue] = arg.Data;
    let result = await collection.updateOne(query, {"$set": data}, { upsert: false });
    console.log(result.result);

  } catch (err) {
    console.log(err)
  } finally {
    client.close();
  }
}


ipcMain.on('SaveOtherData', (event, arg) => {
  SaveOtherData(arg)

})
  function NormalizeToGUID(idorguid){
    if (idorguid.match(/[1-9][0-9]{16}/g)){
        idorguid = createHash('sha256').update(idorguid).digest('base64');
        idorguid = idorguid.replace(/\+/g, '-'); 
        idorguid = idorguid.replace(/\//g, '_');
    }
    return idorguid;
  }
async function SaveConfig(data){
  try{
    global.config = data;
    //console.log(global.config);
    writeFileSync(global.SAVEPATH + "config.json", JSON.stringify(data, undefined, 4))
    console.log("File Saved"); 
  } catch(e) {
    console.log(e)
  }
}

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

function StartCheckingForUpdates(){
  if (global.config.CheckForNewVersion){
    autoUpdater.checkForUpdates();

    setInterval(() => {
      
      autoUpdater.checkForUpdates();
  
    }, 1200000); //Changed to 20 minutes
    
  }
}

ipcMain.on('UpdateAndRestart', (event, arg) => {
  isQuiting = true;
  autoUpdater.quitAndInstall();
  global.PENDINGUPDATE = false;
})

autoUpdater.on('update-downloaded', (event, releaseNotes, releaseName) => {
  if (global.config.AutoUpdate){
    if (global.mainWindow !== undefined) global.mainWindow.send("log",{type: "info", message: 'A new version has been downloaded. AutoUpdate Enabled Restarting the WebService.'})
    if (global.logs !== undefined) global.logs.push({type: "info", message: 'A new version has been downloaded. AutoUpdate Enabled Restarting the WebService.'});
    
    setTimeout(() => {
      isQuiting = true;
      autoUpdater.quitAndInstall();
      
    }, 10000); // 10 seconds after warning message restart and install new version.
  } else {
    const dialogOpts = {
      type: 'info',
      buttons: ['Restart', 'Later'],
      title: 'Application Update',
      message: process.platform === 'win32' ? releaseNotes : releaseName,
      detail: 'A new version has been downloaded. Restart the application to apply the updates.'
    }
    global.PENDINGUPDATE = true;
    //For Desktop Version, Easier to maintain one version of the API
    if (global.mainWindow !== undefined) global.mainWindow.send("log",{type: "info", message: 'A new version has been downloaded. Restart the application to apply the updates.'})
    if (global.logs !== undefined) global.logs.push({type: "info", message: 'A new version has been downloaded. Restart the application to apply the updates.'});
    dialog.showMessageBox(dialogOpts).then((returnValue) => {
      if (returnValue.response === 0) {
        global.PENDINGUPDATE = false;
        isQuiting = true;
        autoUpdater.quitAndInstall();
      }
    })
  }
})
autoUpdater.on('update-available', ()=> {
  if (global.mainWindow !== undefined) global.mainWindow.send("log",{type: "info", message: `A new update is availbe downloading it now`})
  if (global.logs !== undefined) global.logs.push({type: "info", message: `A new update is availbe downloading it now`});
});
autoUpdater.on('update-not-available', ()=> {
  /*if (global.mainWindow !== undefined) global.mainWindow.send("log",{type: "info", message: `No new updates availbe`})
  if (global.logs !== undefined) global.logs.push({type: "info", message: `No new updates availbe`});*/
});

autoUpdater.on('checking-for-update', () => {
  /*if (global.mainWindow !== undefined) global.mainWindow.send("log",{type: "info", message: `Checking for update ${feed}`})
  if (global.logs !== undefined) global.logs.push({type: "info", message: `Checking for update ${feed}`});*/
});

autoUpdater.on('error', message => {
  if (global.mainWindow !== undefined) global.mainWindow.send("log",{type: "warn", message: `Error Checking for updates ${feed} - ${message}`})
  if (global.logs !== undefined) global.logs.push({type: "warn", message: `Error Checking for updates ${feed} - ${message}`});
})

setTimeout(StartCheckingForUpdates, 6000);



let https = require('./WebServer/app');

https(true);