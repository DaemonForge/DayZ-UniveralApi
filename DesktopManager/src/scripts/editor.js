const collectionid = document.getElementById('collection');
const modid = document.getElementById('mod');
const dialog = document.getElementById("dialog");
const DialogHeader = document.getElementById("DialogHeader");
const DialogText = document.getElementById("DialogText");
const GlobalsSelector = document.getElementById("GlobalsSelector");
const JsonEditorDiv = document.getElementById("JsonEditor");
const JsonEditorWrapper = document.getElementById("JsonEditorWrapper");
let ModListArray = [];

const Options = {
  mode: 'tree'
};

let editor = new JSONEditor(JsonEditorDiv, Options)

function DeleteAllData(){
	modid.style.animation = null;
    if (modid.value !== ''){
        ipcRenderer.send('OpenConfirmationDialog', {collection: collectionid.options[collectionid.selectedIndex].value, mod: modid.value })
    } else {
        
		modid.offsetWidth;
		modid.style.animation = "border-pulsate 4s";
    }
}

ipcRenderer.on('DeleteResults', function(event, data){
    console.log(data)
	dialog.showModal();
    if (data.status === 1 ){
        DialogHeader.innerHTML = `Deleted ${data.mod} from ${data.col}`;
        if (data.n > 0){
            DialogText.innerHTML = `Deleted ${data.n} elements from ${data.mod}`
        }   else {
            DialogText.innerHTML = `Nothing was deleted`;
        }
    } else {
        DialogHeader.innerHTML = `Something Went Wrong`;
        DialogText.innerHTML = `An error happened when trying to delete ${data.mod} from ${data.col}`
    }
})


ipcRenderer.send('RequestModListGlobals', {});

ipcRenderer.on('ModListGlobals', function(event, data){
    console.log(data)
	ModListArray = data;
    GlobalsSelector.innerHTML = "";
    ModListArray.forEach(e => {

        var option = document.createElement("option");
        option.text = e.Mod;
        option.value = e.Mod;
        GlobalsSelector.add(option);
    })
    if (ModListArray[0] !== undefined){
        editor.set(ModListArray[0]["Data"]);
    }
})
ipcRenderer.on('UpdateModGlobal', function(event, data){
    //console.log(data)
	dialog.showModal();
    if (data.status === 1 ){
        DialogHeader.innerHTML = `Saved ${data.mod}`;
        DialogText.innerHTML = ``;
    } else {
        DialogHeader.innerHTML = `Something Went Wrong`;
        DialogText.innerHTML = `An error happened when trying to save ${data.mod}`
    }
})

function OnGlobalsSelectorChange(event){
    //console.log(GlobalsSelector.value)
    ModListArray.forEach(e => {
        if (e.Mod === GlobalsSelector.value){
            editor.set(e["Data"]);
        }
    })
};

function SaveGlobal(){
    console.log("Save Global " + GlobalsSelector.value)    

    data = { 
        Mod: GlobalsSelector.value,
        Data: editor.get()
    }
    ipcRenderer.send('SaveGlobalMod', data);

}