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
  ipcRenderer.send('SaveConfig', editor.get())

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

function UpdateAndRestart(){
  ipcRenderer.send('UpdateAndRestart', {})
}
function CloseDialog(){
  
  if (dialogOkay !== undefined){
    dialogOkay.innerHTML = 'Okay';
  }
  if (pasteTo !== undefined){
    pasteTo.style.display = 'none';
    pasteTo.value = '';
  }
  if (Paste !== undefined){
    Paste.style.display = 'none';
  }
	dialog.close();
}
