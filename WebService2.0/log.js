
module.exports = function(message, type = "info"){

    //For Desktop Version, Easier to maintain one version of the API
    if (global.mainWindow !== undefined) global.mainWindow.send("log",{type: type, message: message})
    if (global.logs !== undefined) global.logs.push({type: type, message: message});

    if (type === "warn")
        console.log("\x1b[33m", `${message}`,'\x1b[0m')
    else if (type === "info")
        console.log("\x1b[36m", `${message}`,'\x1b[0m')
    else 
        console.log(`${message}`)
}