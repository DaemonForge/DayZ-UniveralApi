const client = require("./bot.js");
const {User, GuildMember, Guild} = require("discord.js");

const {NormalizeToGUID, createLogger} = require('../utils');
const logger = createLogger(global.logger, 'discord');
const {GetDiscordObj, GetClientID} = require('./dsUtils');
const {playerExists} = require('../models/player');


//API Call Functions
async function AddRole(req, res) {
    if (!client) {
        logger.error("Discord client not initialized");
        return res.status(500).send({ Status: "Error", Error: "Discord client not initialized" });
    }
    let GUID = NormalizeToGUID(req.params.GUID);
        try{
            let dsInfo = GetDiscordObj(GUID);
            let guild = await client.guilds.fetch(global.config.Discord.Guild_Id);
            dsInfo = await dsInfo;
            let RawData = req.body;
            let Role = RawData.Role;
            let resObj;
            if (dsInfo === undefined || dsInfo.id === "0" ){
                logger.info(`Error: Discord AddRole - User doesn't have discord set up`, { GUID });
                resObj = {Status: "NotSetup", Error: `Player Doesn't have discord set up`, Roles: [], VoiceChannel: "",id: "0", Username: "", GlobalName: "", Avatar: "" };
            } else {
                resObj = { Status: "Error", Error: "Couldn't connect to discord", Roles: [], VoiceChannel: "", id: dsInfo.id, Username: dsInfo.username, GlobalName: dsInfo.globalName, Avatar: dsInfo.avatar };
                try {
                    let player = await guild.members.fetch(dsInfo.id);
                    resObj.Status = "Success";
                    resObj.Error = "";
                    resObj.VoiceChannel = player.voice.channel.id || "";
                    resObj.Roles = player._roles || [];
                        
                    if (player.roles.cache.has(Role)) {
                        logger.info(`Discord AddRole - User already has role`, { GUID, role: Role });
                        resObj.Error = "Already Has Role";
                    } else {
                        let role = await guild.roles.fetch(Role);
                        await player.roles.add(role).catch((e) => logger.error(`Error adding role`, { error: e }));
                        resObj.Roles.push(Role);
                        logger.info(`Discord AddRole - Added role to user`, { GUID, role: Role });
                    }
                } catch (e) {
                    console.log(e);
                    logger.info(`Discord AddRole - User not found in discord`, { GUID, error: e });
                    resObj.Error = "User not found in discord";
                    resObj.Status = "NotFound";
                }
            }
            res.status(200);
            res.json(resObj);
        }catch(err){
            console.log(err);
            res.status(200);
            res.json({Status: "Error", Error: `${err}`, Roles: [], VoiceChannel: "", id: "0", Username: "", GlobalName: "", Avatar: "" });
            logger.warn("Error in AddRole", { error: err });
        }
}

async function RemoveRole(req, res){
    let GUID = NormalizeToGUID(req.params.GUID);
        try{
            let resObj = {};
            if ((await playerExists(GUID)) === false){
                logger.info(`Error: Discord RemoveRole - Player doesn't exist`, { GUID });
                return res.status(200).json({Status: "NotFound", Error: `Player Doesn't have discord set up`, Roles: [], VoiceChannel: "", id: "0", Username: "", GlobalName: "", Avatar: "" });
            } 
            let dsInfo = GetDiscordObj(GUID);
            let guild = await client.guilds.fetch(global.config.Discord.Guild_Id);
            dsInfo = await dsInfo;
            if (dsInfo === undefined || dsInfo.id === undefined || dsInfo.id === "0" ){
                logger.info(`Discord RemoveRole - User doesn't have discord set up`, { GUID });
                return res.status(200).json(resObj = {Status: "NotSetup", Error: `Player Doesn't have discord set up`, Roles: [], VoiceChannel: "", id: "0", Username: "", GlobalName: "", Avatar: "" });
            } 
            let RawData = req.body;
            let Role = RawData.Role;
            resObj = { Status: "Error", Error: "Couldn't connect to discord", Roles: [], VoiceChannel: "", id: dsInfo.id, Username: dsInfo.username, GlobalName: dsInfo.globalName, Avatar: dsInfo.avatar };
            try {
                let player = await guild.members.fetch(dsInfo.id);
                resObj.Status = "Success";
                resObj.Error = "";
                resObj.VoiceChannel = player.voice.channel.id || "";
                resObj.Roles = player._roles || [];
                if (player.roles.cache.has(Role)) {
                    let role = await guild.roles.fetch(Role);
                    await player.roles.remove(role).catch((e) => logger.error(`Error removing role`, { error: e }));
                    resObj.Roles = resObj.Roles.filter(item => item !== Role)
                    logger.info(`Discord RemoveRole - Removed role from user`, { GUID, role: Role });
                } else {
                    logger.warn(`Discord RemoveRole - User didn't have role`, { GUID, role: Role });
                    resObj.Error = "Already didn't have Role";
                }
            }catch (e) {
                logger.warn(`Discord RemoveRole - User not found in discord`, { GUID, error: e });
                resObj.Error = "User not found in discord";
                resObj.Status = "NotFound";
            }
            return res.status(200).json(resObj);
        }catch(err){
            logger.warn("Error in RemoveRole", err);
            return res.status(200).json({Status: "Error", Error: `${err}`, Roles: [], VoiceChannel: "", id: "0", Username: "", GlobalName: "", Avatar: "" });
        }
}

async function GetUserAndRoles(req, res){
    let GUID = NormalizeToGUID(req.params.GUID);
    logger.info("GetUserAndRoles request", { GUID });
    try{
        let dsInfo = await GetDiscordObj(GUID);
        if (dsInfo?.id  === undefined || dsInfo.id === "0" ){
            logger.info("Can't find Player in database", { GUID });
            res.status(200);
            res.json({Status: "NotSetup", Error: `Player with ${GUID} Not Found`, Roles: [], VoiceChannel: "", id: "0", Username: "", GlobalName: "", Avatar: "" });
        } else {

            let resObj;
            if (dsInfo === undefined || dsInfo.id === undefined || dsInfo.id === "" || dsInfo.id === "0" ){
                resObj = {Status: "NotSetup", Error: `Player Doesn't have discord set up`, Roles: [], VoiceChannel: "", id: "0", Username: "", GlobalName: "", Avatar: "" };
                logger.debug("Discord object is invalid", { GUID, dsInfo });
            } else {                        
                resObj = { Status: "Error", Error: "Couldn't connect to discord", Roles: [], VoiceChannel: "", id: dsInfo.id, Username: dsInfo.username, GlobalName: dsInfo.globalName, Avatar: dsInfo.avatar };
                let guild = await client.guilds.fetch(global.config.Discord.Guild_Id);
                logger.debug("Fetched guild", { guildId: guild.id });
                try {
                    let player = await guild.members.fetch(dsInfo.id);
                    resObj.VoiceChannel = player.voice.channel?.id || "";
                    resObj.Status = "Success";
                    resObj.Error = "";
                    resObj.Roles = player._roles || [];
                    logger.info("Successfully found discord ID and roles", { GUID });
                    logger.debug("Player details", { GUID, roles: resObj.Roles, voiceChannel: resObj.VoiceChannel });
                } catch (e) {
                    logger.info("Found Discord ID but not roles", { GUID, error: e });
                    logger.debug("Error fetching player roles", { GUID, error: e });
                    resObj.Error = "User not found in discord";
                    resObj.Status = "NotFound";
                }
            }
            res.status(200);
            res.json(resObj);
        }
    }catch(err){
        logger.warn("Error in GetUserAndRoles", { GUID, error: err });
        res.status(200);
        res.json({Status: "Error", Error: `${err}`, Roles: [], VoiceChannel: "", id: "0", Username: "", GlobalName: "", Avatar: "" });
    }
}


async function PlayerVoiceGetChannel(req, res){
    let GUID = NormalizeToGUID(req.params.GUID);
    try {
        let dsInfo = GetDiscordObj(GUID);
        let guild = await client.guilds.fetch(global.config.Discord.Guild_Id);
        dsInfo = await dsInfo;
        if (dsInfo !== undefined && dsInfo.id !== undefined &&  dsInfo.id !== "0"){
            try {
                let player = await guild.members.fetch(dsInfo.id);
                let result = player.voice.channel.id;
                if (result !== undefined && result !== null) {
                    res.json({Status: "Success", Error: "", oid: result})
                } else {
                    res.json({Status: "NotFound", Error: "Player is not in a channel on discord", oid: ""})
                }
            } catch (e) {
                logger.warn(`Can't get discord voice channel, User not found in discord`, { GUID, error: e });
                res.json({Status: "NotFound", Error: `Player not a member of the discord server`, oid: ""})
            }
        } else {
            logger.warn(`Discord User not found in discord`, { GUID });
            res.json({Status: "NotSetup", Error: `Player not setup`, oid: ""})
        }
    } catch (e) {
        logger.warn(`Error getting discord voice channel`, { GUID, error: e });
        res.json({Status: "Error", Error: `${e}`});
    }
}


async function PlayerVoiceMute(req, res){
    let GUID = NormalizeToGUID(req.params.GUID);
        let dsInfo = GetDiscordObj(GUID);
        let RawData = req.body;
        let State = (RawData.State === 1);
        let guild = await client.guilds.fetch(global.config.Discord.Guild_Id);
        dsInfo = await dsInfo;
        if (dsInfo !== undefined && dsInfo.id !== undefined &&  dsInfo.id !== "0"){
            try {
                let player = await guild.members.fetch(dsInfo.id);
                try {
                    if (player.voice.channel.id !== null && player.voice.channel.id !== undefined){
                        let result = await player.voice.setMute(State);
                        logger.info(`Muted Discord User in a channel`, { GUID, channelId: player.voice.channel.id, muteState: State });
                        res.json({Status: "Success", Error: ""})
                    } else {
                        logger.warn(`Can't Mute Discord User - Not in a channel on discord`, { GUID });
                        res.json({Status: "NotFound", Error: "Player is not in a channel on discord"})
                    }
                }
                catch (e) {
                    logger.warn(`Discord User not in a channel on discord`, { GUID, error: e });
                    res.json({Status: "NotFound", Error: "Player is not in a channel on discord"})
                }
            } catch (e) {
                logger.warn(`Discord User not found in discord`, { GUID, error: e });
                res.json({Status: "NotFound", Error: `Player not a member of the discord server`})
            }
        } else {
            logger.warn(`Discord User not found in discord`, { GUID });
            res.json({Status: "NotSetup", Error: `Player not setup`})
        }
}
/**
 * Kicks a player from their voice channel in Discord
 * @param {Object} res - Express response object
 * @param {Object} req - Express request object with GUID parameter and optional reason text
 */
async function PlayerVoiceKick(req, res){
    // Normalize the player's GUID from the request parameters
    let GUID = NormalizeToGUID(req.params.GUID);
        // Get Discord info for the player
        let dsInfo = GetDiscordObj(GUID);
        // Get the reason for kicking from the request body
        let RawData = req.body;
        let Reason = RawData.Text || "";
        // Fetch the Discord guild/server
        let guild = await client.guilds.fetch(global.config.Discord.Guild_Id);
        dsInfo = await dsInfo;
        // Check if the player has Discord information
        if (dsInfo !== undefined && dsInfo.id !== undefined &&  dsInfo.id !== "0"){
            try {
                // Try to fetch the member from the guild
                let player = await guild.members.fetch(dsInfo.id);
                try {
                    // Check if the player is in a voice channel
                    if (player.voice.channel.id !== null && player.voice.channel.id !== undefined){
                        // Kick the player from the voice channel
                        let result = await player.voice.kick(Reason);
                        res.json({Status: "Success", Error: ""})
                    } else {
                        // Player is not in a voice channel
                        res.json({Status: "NotFound", Error: "Player is not in a channel on discord"})
                    }
                }
                catch (e) {
                    logger.warn(`Discord User not in a channel on discord`, { GUID, error: e });
                    res.json({Status: "NotFound", Error: "Player is not a channel on discord"})
                }
            } catch (e) {
                logger.warn(`Discord User not found in discord`, { GUID, error: e });
                res.json({Status: "NotFound", Error: `Player not a member of the discord server`})
            }
        } else {
            logger.warn(`Discord User not found in discord`, { GUID });
            res.json({Status: "NotSetup", Error: `Player not setup`})
        }
    } 

async function ChannelVoiceMove(req, res){
    let GUID = NormalizeToGUID(req.params.GUID);
    let ChannelId = req.params.id;
        let dsInfo = GetDiscordObj(GUID);
        let guild = await client.guilds.fetch(global.config.Discord.Guild_Id);
        dsInfo = await dsInfo;
        if (dsInfo !== undefined && dsInfo.id !== undefined &&  dsInfo.id !== "0"){
            try {
                let player = await guild.members.fetch(dsInfo.id);
                let channel = await guild.channels.cache.get(ChannelId);
                if (channel === undefined || channel === null){
                    logger.warn(`Discord User Channel doesn't exist`, { GUID, ChannelId });
                    res.json({Status: `Error`, Error: `Channel doesn't exsit`});
                    return;
                }
                if (req.isServer || (channel.permissionsFor(player).has('VIEW_CHANNEL') && channel.permissionsFor(player).has('CONNECT') )) {
                    try {
                        let oldchannel = player.voice.channelID;
                        let result = await player.voice.setChannel(ChannelId);
                        logger.info(`Moved Discord User to new channel`, { GUID, fromChannel: oldchannel, toChannel: ChannelId });
                        res.json({Status: "Success", Error: ""})
                    }
                    catch (e) {
                        logger.warn(`Can't Move Discord User - Not in a channel on discord`, { GUID, error: e });
                        res.json({Status: "NotFound", Error: "Player is not in a channel on discord"})
                    }
                } else {
                    logger.warn(`Discord User tried to move to channel but doesn't have permissions`, { GUID, ChannelId });
                    res.json({Status: "NoPerms", Error: `Player doesn't have permissions to join the channel'`})
                }
            } catch (e) {
                console.log(e);
                logger.warn(`Discord User not found in discord`, { GUID, error: e });
                res.json({Status: "NotFound", Error: `Player not a member of the discord server`})
            }
        } else {
            logger.warn(`Discord User not found in discord`, { GUID });
            res.json({Status: "NotSetup", Error: `Player not setup`})
        }
}

async function SendMessageUser(req, res){
    let guid = NormalizeToGUID(req.params.GUID);
        let RawData = req.body; 
        let guild = client.guilds.fetch(global.config.Discord.Guild_Id);
        let message = RawData.Message;
        let userObj = await GetDiscordObj(guid);
        guild = await guild;
        if (userObj !== undefined && userObj !== null && userObj.id !== "0"){
            try {
                let did = userObj.id;
                let user = await client.users.fetch(did);
                let dm = await user.createDM();
                let result = await dm.send(message);
                logger.info(`Successfully sent Discord Direct Message`, { guid, messageId: result?.id });
                res.status(200);
                let oid = result?.id || "";
                res.json({Status: "Success", Error: "", oid: `${oid}`});
            } catch(e) {
                let error = `${e}`;
                if (error === `DiscordAPIError: Cannot send messages to this user`){
                    logger.warn(`Error sending message - user may block DMs`, { guid, error });
                    res.status(200);
                    res.json({Status: "Error", Error: `Cannot send messages to this user, they may have dm's blocked`, oid: "" });
                } else {
                    logger.warn(`Error sending message`, { guid, error });
                    res.status(500);
                    res.json({Status: "Error", Error: `${e}`, oid: "" });
                }
            }
        } else {
            logger.info(`Failed to send Discord Direct Message - user not configured`, { guid });
            res.status(200);
            res.json({Status: "NotSetup", Error: "Discord User Found", oid: "" });
        }
}

async function SetNicknameUser(req, res){
        let guid = NormalizeToGUID(req.params.GUID);
        let RawData = req.body; 
        let guild = client.guilds.fetch(global.config.Discord.Guild_Id);
        let nickname = RawData.Nickname;
        let userObj = await GetDiscordObj(guid);
        guild = await guild;
        if (userObj !== undefined && userObj.id !== "0"){
            try {
                if (nickname !== undefined && nickname !== ""){
                    let user = await guild.members.fetch(userObj.id);
                    let result = await user.setNickname(nickname);
                    logger.info(`Successfully changed nickname`, { guid, nickname });
                    res.status(200);
                    res.json({Status: "Success", Error: ""});
                } else {
                    logger.info(`Error can't change nickname to empty string`, { guid });
                    res.status(200);
                    res.json({Status: "Error", Error: "Can't change nickname to Empty String"});
                }
            } catch(e) {
                logger.warn(`Error changing nickname`, { guid, error: e });
                console.log(e);
                res.status(500);
                res.json({Status: "Error", Error: `${e}`});
            }
        } else {
            logger.info(`Failed changing nickname - user not configured`, { guid });
            res.status(200);
            res.json({Status: "NotSetup", Error: "Discord User Found", oid: "" });
        }
}



async function CheckId(req, res){
    let guid = NormalizeToGUID(req.params.GUID);
        try{
            let dsInfo = await GetDiscordObj(GUID);
            let datetime = new Date();
            let ClientId = GetClientID(req);
            let logobj = { Log: "DiscordStatusCheck", TimeStamp: datetime, GUID: guid, SteamId: id, ClientId: ClientId, Status: "Error" }
            if (dsInfo !== undefined && dsInfo.id !== "0"){
                    logobj.Status = "Success";
                    logobj.Discord = dsInfo.id;
                    res.status(200).json({Status: "Success", Error: "" });
            } else {
                logobj.Status = "NotFound";
                res.status(200).json({Status: "NotFound", Error: "No User Found"  });
            }
            logger.info(`Check status for user: ${guid} - ${datetime.toUTCString()} - ${logobj.Status}`, logobj);
        } catch(err){
            logger.warn("Error Checking for ID " + guid, { error: err });
            res.status(400).json({Status: "Error", Error: err });
        }
}

async function CheckIdHasRole(req, res) {
    let guid = NormalizeToGUID(req.params.GUID);
    let roleid = req.params.ROLEID;
    let datetime = new Date();
    let ClientId = GetClientID(req);
    // Initialize log object (SteamId uses dsInfo.steamId if available, else null)
    let logobj = { 
        Log: "DiscordRoleCheck", 
        TimeStamp: datetime, 
        GUID: guid, 
        SteamId: null, 
        ClientId: ClientId, 
        Status: "Error" 
    };

    try {
        // Use the normalized guid
        let dsInfo = await GetDiscordObj(guid);
        if (dsInfo && dsInfo.id !== "0") {
            logobj.Discord = dsInfo.id;
            // Optionally set SteamId if available
            logobj.SteamId = dsInfo.steamId || null;
            let guild = await client.guilds.fetch(global.config.Discord.Guild_Id);
            let resObj = { Status: "Error", Error: "Unknown Error" };
            try {
                let player = await guild.members.fetch(dsInfo.id);
                // Use Array.includes() for clarity
                let roles = player._roles || [];
                if (roles.includes(roleid)) {
                    resObj.Status = "Success";
                    resObj.Error = "";
                    logobj.Status = "Success";
                } else {
                    resObj.Status = "NotFound";
                    resObj.Error = "User does not have role";
                    logobj.Status = "NotFound";
                }
                logger.info(`Successfully found discord ID and roles for ${guid}`);
            } catch (e) {
                logger.info(`Found Discord ID but could not fetch roles for ${guid}`);
                resObj.Status = "NotFound";
                resObj.Error = "User not found in discord";
                logobj.Status = "NotFound";
            }
            res.json(resObj);
        } else if (dsInfo) {
            // Discord info exists but the ID is invalid ("0")
            logobj.Status = "NotFound";
            res.status(200).json({ Status: "NotFound", Error: "Discord could not be found for user" });
        } else {
            // No Discord info found at all
            logobj.Status = "NotFound";
            res.status(200).json({ Status: "NotFound", Error: "No User Found" });
        }
        logger.info(`Check status for user: ${guid} - ${datetime.toUTCString()} - ${logobj.Status}`);
    } catch (err) {
        logger.warn("Error checking for ID " + guid, { error: err });
        res.status(400).json({ Status: "Error", Error: err });
    } finally {
        await mongo.close();
    }
}


module.exports = { AddRole, RemoveRole, GetUserAndRoles, PlayerVoiceGetChannel, PlayerVoiceMute, PlayerVoiceKick, ChannelVoiceMove, SendMessageUser, SetNicknameUser, CheckId, CheckIdHasRole };