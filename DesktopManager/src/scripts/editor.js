const collectionid = document.getElementById('collection');
const modid = document.getElementById('mod');
const dialog = document.getElementById("dialog");
const DialogHeader = document.getElementById("DialogHeader");
const DialogText = document.getElementById("DialogText");
const GlobalsSelector = document.getElementById("GlobalsSelector");
const JsonEditorDiv = document.getElementById("JsonEditor");
const JsonEditorWrapper = document.getElementById("JsonEditorWrapper");
const dialogOkay = document.getElementById("dialogOkay");
const copyFrom = document.getElementById("copyFrom");

const pasteTo = document.getElementById("pasteTo");
const Paste = document.getElementById("Paste");

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
        SetUpEditor(ModListArray[0].mod,ModListArray[0]["Data"], )
    }
})

async function SetUpEditor(mod, data){
    let schema = await CheckForSchema(mod);
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

async function OnGlobalsSelectorChange(event){
    ModListArray.forEach(e => {
        if (e.Mod === GlobalsSelector.value){
            SetUpEditor(GlobalsSelector.value, e["Data"]);
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

