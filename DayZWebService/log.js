const log4js = require('log4js');
let datetime = new Date();
let date = datetime.toISOString().slice(0,10)
let logfilename = global.SAVEPATH + "logs/api-warnings-" + date + ".log";
if (global.config?.LogToFile){
    log4js.configure({
        appenders: { logs: { type: "file", filename: logfilename } },
        categories: { default: { appenders: ["logs"], level: "warn" } }
    });
} 
let logger = log4js.getLogger('logs'); 

module.exports = function(message, type = "info"){

    if (global.config?.LogToFile && type === "warn") logger.warn(`${message}`);

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