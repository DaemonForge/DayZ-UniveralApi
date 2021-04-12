const {LogToFile} = global.config;

const log4js = require('log4js');
let datetime = new Date();
let date = datetime.toISOString().slice(0,10)
let logfilename = "logs/api-warnings-" + date + ".log";
if (LogToFile){
    log4js.configure({
        appenders: { logs: { type: "file", filename: logfilename } },
        categories: { default: { appenders: ["logs"], level: "warn" } }
    });
} 
let logger = log4js.getLogger('logs'); 

module.exports = function(message, type = "info"){

    if (LogToFile && type === "warn") logger.warn(`${message}`);

    if (type === "warn")
        console.log("\x1b[33m", `${message}`,'\x1b[0m')
    else if (type === "info")
        console.log("\x1b[36m", `${message}`,'\x1b[0m')
    else 
        console.log(`${message}`)
}