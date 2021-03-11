const config = require('./configLoader');

const log4js = require('log4js');
let datetime = new Date();
let date = datetime.toISOString().slice(0,10)
let logfilename = "logs/api-warnings-" + date + ".log";
if (config.LogToFile){
    log4js.configure({
    appenders: { logs: { type: "file", filename: logfilename } },
    categories: { default: { appenders: ["logs"], level: "warn" } }
    });
} 
let logger = log4js.getLogger('logs'); 

module.exports = function(message, type = "info"){
    if (config.LogToFile){
        if (type == "warn"){
            logger.warn(`%c${message}`, `color: red`)
        }
    }
    console.log(`%c${message}`, `color: blue`)
}