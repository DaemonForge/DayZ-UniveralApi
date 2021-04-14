const collectionid = document.getElementById('collection');
const modid = document.getElementById('mod');
const dialog = document.getElementById("dialog");
const DialogHeader = document.getElementById("DialogHeader");
const DialogText = document.getElementById("DialogText");

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