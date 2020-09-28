const config = require('./configLoader');

const log4js = require('log4js');
var datetime = new Date();
var date = datetime.toISOString().slice(0,10)
var logfilename = "logs/api-warnings-" + date + ".log";
if (config.LogToFile){
    log4js.configure({
    appenders: { logs: { type: "file", filename: logfilename } },
    categories: { default: { appenders: ["logs"], level: "warn" } }
    });
} 
var logger = log4js.getLogger('logs'); 

module.exports = function(message, type = "info"){
    if (config.LogToFile){
        if (type == "warn"){
            logger.warn(message)
        }
    }
    console.log(message)
}