// renderer process
const { ipcRenderer } = require('electron');
const {writeFileSync} = require('fs')

let logsElement = document.getElementById("logs");

ipcRenderer.on('log', function (event,test) {
    //console.log(test);
    if (test.type === "" || !logsElement) return;
    logsElement.innerHTML+= `<span class="${test.type}">${test.message}</span>\r\n`;
    logsElement.scrollTop = logsElement.scrollHeight - logsElement.clientHeight;
});

function OpenTemplatesFolder(){
  ipcRenderer.send("OpenTemplatesFolder", {});
}

function OpenLogsFolder(){
  ipcRenderer.send("OpenLogsFolder", {});
}
function OpenMenu() {
    document.getElementById("myDropdown").classList.toggle("show");
  }

function RestartApp() {
  ipcRenderer.send("RestartApp", {});
}
  

async function SaveConfig() {
  try{
    global.config = editor.get();
    console.log(global.config);
    writeFileSync("./config.json", JSON.stringify(global.config, undefined, 4))
    console.log("File Saved");
    
  } catch(e) {
    console.log(e)
  }
}
window.onclick = function(event) {
    if (!event.target.matches('.menubtn') && !event.target.matches('.menuicon') ) {
      var dropdowns = document.getElementsByClassName("menudropdown-content");
      var i;
      for (i = 0; i < dropdowns.length; i++) {
        var openDropdown = dropdowns[i];
        if (openDropdown.classList.contains('show')) {
          openDropdown.classList.remove('show');
        }
      }
    }
  }

  
function CloseDialog(){
	dialog.close();
}
