
const {isArray, isObject,NormalizeToGUID,GenerateLimiter} = require('../utils')
const { MongoClient } = require("mongodb");
const {getPlayerModData,playerExists} = require('../models/player');
const {createHash} = require('crypto');


async function GetGUIDFromDiscordId(dsid){
    const mongo = new MongoClient(global.config.DBServer);
    let guid = "";
    try{
        await mongo.connect();
        // Connect the client to the server
        const db = mongo.db(global.config.DB);
        let collection = db.collection("Players");
        let query = { "Discord.id": dsid };
        let results = collection.find(query);
        if ((await collection.countDocuments(query)) == 0){
            logger.warn("Can't find Player with Discord ID " + dsid);
        } else {
            let dataarr = await results.toArray(); 
            let data = dataarr[0]; 
            guid = data.GUID || "";
        }
    }catch(err){
        logger.warn(`Error Fetching Player Obj`, { error: err });
    }finally{
        // Ensures that the client will close when you finish/error
        mongo.close();
        return guid;
    }
}

async function GetDiscordObj(guid){
    try{
        if ((await playerExists(guid))){
            let data = await getPlayerModData(guid, "Discord");
            return data;
        } else {
            return undefined;
        }
    }catch(err){
        logger.warn("Error Fetching Discord Obj", { error: err });
        return null;
    }
}

function GetClientID(req){
    let ip = req.headers['CF-Connecting-IP'] || req.headers['x-forwarded-for'] || req.socket.remoteAddress || req.ip;
    let  hash = createHash('sha256');
    let theHash = hash.update(ip).digest('base64');
    return theHash.substring(0,32); //Cutting the last few digets to save a bit of data and make sure people don't mistake it for the GUIDS
}


async function ReMapMessage(obj) {
    let guid = await GetGUIDFromDiscordId(obj.author.id);
    let RepliedTo = obj?.reference?.messageID || "";
    return {id: obj.id, AuthorId: obj.author.id, AuthorGUID: guid, RepliedTo: RepliedTo, Embed: obj.embeds[0], Content: obj.content, ChannelId: obj.channel.id, TimeStamp: obj.createdTimestamp}
    
}

module.exports = {GetGUIDFromDiscordId,GetDiscordObj,GetClientID,ReMapMessage}  //Exporting the functions to be used in other files