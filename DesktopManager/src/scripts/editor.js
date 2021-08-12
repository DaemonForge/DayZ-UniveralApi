const collectionid = document.getElementById('collection');
const modid = document.getElementById('mod');
const dialog = document.getElementById("dialog");
const DialogHeader = document.getElementById("DialogHeader");
const DialogText = document.getElementById("DialogText");
const GlobalsSelector = document.getElementById("GlobalsSelector");
const ModListList = document.getElementById("ModList");
const JsonEditorDiv = document.getElementById("JsonEditor");
const JsonEditorWrapper = document.getElementById("JsonEditorWrapper");
const dialogOkay = document.getElementById("dialogOkay");
const copyFrom = document.getElementById("copyFrom");

const DatabaseSelector = document.getElementById("DatabaseSelector");
const SearchButton = document.getElementById("SearchButton");
const SearchBar = document.getElementById("SearchBar");

const pasteTo = document.getElementById("pasteTo");
const Paste = document.getElementById("Paste");

let ModListArray = [];
let LastId = "";
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
    ModListList.innerHTML = "";
    ModListArray.forEach(e => {
        let option = document.createElement('option');
        option.value = e.Mod; 
        ModListList.appendChild(option);
    })
    LastId = "";
    SetUpEditor("", {},DatabaseSelector.value);
})

async function SetUpEditor(mod, data, database){
    let schema;
    if (mod !== "" && database === "Globals"){
        schema = await CheckForSchema(mod);
    }
    delete editor;
    JsonEditorDiv.innerHTML = "";
    if (schema){
        let TheOptions = {
            mode: 'tree',
            schema: schema
        }
        editor = new JSONEditor(JsonEditorDiv, TheOptions);
    } else {
        editor = new JSONEditor(JsonEditorDiv, Options)
    }
    editor.set(data);
}

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

ipcRenderer.on('ReceiveDatabaseData', function(event, data){
    if (data?.Results[0] !== undefined){
        LastId = data.ID;
        SetUpEditor("", data.Results[0], DatabaseSelector.value);
    }
})

async function OnDatabaseSelectorChange(){
    if (DatabaseSelector.value == "Globals"){
        SearchBar.style.display = "none";
        SearchButton.style.display = "none";
        LastId = "";
        SetUpEditor("", {}, DatabaseSelector.value);
    } else if (DatabaseSelector.value == "Players"){
        SearchBar.placeholder = "SteamID/GUID"
        SearchBar.style.display = "block";
        SearchButton.style.display = "block";
        LastId = "";
        SetUpEditor("", {}, DatabaseSelector.value);
        GlobalsSelector.value = "";
    } else {
        SearchBar.placeholder = "Object ID"
        SearchBar.style.display = "block";
        SearchButton.style.display = "block";
        LastId = "";
        SetUpEditor("", {}, DatabaseSelector.value);
        GlobalsSelector.value = "";
    }

}

async function SearchDatabase(){
    if ( GlobalsSelector.value !== "" && SearchBar.value !== "") {
        ipcRenderer.send('RequestFromDatabase', {ID: SearchBar.value, Collection: DatabaseSelector.value,Mod: GlobalsSelector.value});
    }
}
async function OnModSelectorChange(){
    if (DatabaseSelector.value == "Globals"){
        if ("" === GlobalsSelector.value){
            LastId = "";
            SetUpEditor("", {}, DatabaseSelector.value);
        }
        ModListArray.forEach(e => {
            if (e.Mod === GlobalsSelector.value){
                LastId = "";
                SetUpEditor(GlobalsSelector.value, e["Data"], DatabaseSelector.value);
            }
        })
    } else {    
        if (SearchBar.value !== "" && GlobalsSelector.value !== ""){
            ipcRenderer.send('RequestFromDatabase', {ID: SearchBar.value, Collection: DatabaseSelector.value, Mod: GlobalsSelector.value});
        }
    }
};
function SaveData(){
    if (DatabaseSelector.value === "Globals"){
        SaveGlobal();
    } else {
        SaveOtherData()
    }
}

function SaveOtherData(){
    if (LastId !== ""){
        ipcRenderer.send('SaveOtherData', {Collection: DatabaseSelector.value, ID: LastId, Mod: GlobalsSelector.value, Data: editor.get()});
    }
}

function SaveGlobal(){
    console.log("Save Global " + GlobalsSelector.value)    

    data = { 
        Mod: GlobalsSelector.value,
        Data: editor.get()
    }
    ipcRenderer.send('SaveGlobalMod', data);

}


function CopyJson(){
    let json = editor.get();
    let jsonText = JSON.stringify(json, undefined, 2);
    copyFrom.value = jsonText;
    /* Select the text field */
    copyFrom.select();
    copyFrom.setSelectionRange(0, 999999); /* For mobile devices */
  
    /* Copy the text inside the text field */
    document.execCommand("copy");
  }


function PasteJson(){
	dialog.showModal();
    DialogHeader.innerHTML = `Paste JSON Data Here`;
    DialogText.innerHTML = ``;
    dialogOkay.innerHTML = 'Cancel';
    Paste.style.display = 'inline';
    pasteTo.style.display = 'inline';
}


function PasteContent(){
	dialog.close();
    DialogHeader.innerHTML = ``;
    DialogText.innerHTML = ``;
    dialogOkay.innerHTML = 'Okay';
    Paste.style.display = 'none';
    pasteTo.style.display = 'none';
    try {
        let content = JSON.parse(pasteTo.value);
        editor.set(content);
        console.log(content)
        pasteTo.value = "";
    } catch (e){
        console.log(e)
        DialogHeader.innerHTML = `Error`;
        DialogText.innerHTML = `${e}`;
        pasteTo.value = "";
        dialog.showModal();
    }
}

async function CheckForSchema(modtag){
    let TheSchema = await fetch(`https://raw.githubusercontent.com/daemonforge/DayZ-UniveralApi/stable/Schemas/${modtag}.json`).then(response => {
        return response.json().catch( e => console.log(e));
    }).catch( e => console.log(e))
    //console.log(TheSchema);
    return TheSchema;
}

