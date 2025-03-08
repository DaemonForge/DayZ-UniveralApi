const client = require("./bot.js");
const {User, GuildMember, Guild} = require("discord.js");
const {ReMapMessage} = require("./dsUtils");
const logger = global.logger;

async function CreateChannel(req, res){
        try{
            let RawData = req.body; 
            let guild = await client.guilds.fetch(global.config.Discord.Guild_Id);
            let options = RawData.Options;
            let channel = await guild.channels.create(RawData.Name, options)
            let id = channel.id;
                
            res.status(201);
            res.json({Status: "Success", Error: ``, oid: `${id}`});

        } catch(e){
            res.status(400);
            res.json({Status: "Error", Error: `${e}`, oid: "0"});
            logger.warn("Error creating channel", { error: e });
        }
}


async function DeleteChannel(req, res){
    let id = req.params.id;
        try{
            let RawData = req.body; 
            let guild = await client.guilds.fetch(global.config.Discord.Guild_Id);
            let reason = RawData.Reason;
            try {
                let channel = await guild.channels.cache.get(id);
                if (channel){
                    channel.delete(reason).then(()=>{
                        logger.info(`Deleted channel`, { channelId: id, reason });
                        res.status(200);
                        res.json({Status: "Success", Error: ``, oid: `${id}`});
                    }).catch((e)=>{
                        logger.warn(`Error Deleting channel`, { channelId: id, reason, error: e });
                        res.status(200);
                        res.json({Status: "Error", Error: `${e}`, oid: `${id}`});
                    })
                } else {
                    logger.warn(`Couldn't delete channel - does not exist`, { channelId: id, reason });
                    res.status(200);
                    res.json({Status: "NotFound", Error: ``, oid: `${id}`});
                }
            } catch (e){
                res.status(200);
                res.json({Status: "NotFound", Error: `${e}`, oid: `${id}`});
                logger.warn(`Error Deleting channel`, { channelId: id, reason, error: e });
            }
        } catch(e){
            res.status(400);
            res.json({Status: "Error", Error: `${e}`, oid: "0"});
            logger.warn(`Error Deleting channel`, { channelId: id, error: e });
        }
}

async function EditChannel(req, res){
    let id = req.params.id;
        try{
            let RawData = req.body; 
            let guild = await client.guilds.fetch(global.config.Discord.Guild_Id);
            let reason = RawData.Reason;
            let options = RawData.Options;
            try {
                let channel = await guild.channels.cache.get(id);
                if (channel){
                    channel.edit(options, reason).then(()=>{
                        logger.info(`Edited channel`, { channelId: id, reason });
                        res.status(200);
                        res.json({Status: "Success", Error: ``, oid: `${id}`});
                    }).catch((e)=>{
                        logger.warn(`Error editing channel`, { channelId: id, reason, error: e });
                        res.status(200);
                        res.json({Status: "Error", Error: `${e}`, oid: `${id}`});
                    })
                } else {
                    logger.warn(`Couldn't edit channel - does not exist`, { channelId: id, reason });
                    res.status(200);
                    res.json({Status: "NotFound", Error: ``, oid: `${id}`});
                }
            } catch (e){
                res.status(200);
                res.json({Status: "NotFound", Error: `${e}`, oid: `${id}`});
                logger.warn(`Error editing channel`, { channelId: id, reason, error: e });
            }
        } catch(e){
            res.status(400);
            res.json({Status: "Error", Error: `${e}`, oid: "0"});
            logger.warn(`Error editing channel`, { channelId: id, error: e });
        }
}

async function SendMessageChannel(req, res){
    let id = req.params.id;
    let auth = req.headers['auth-key'];
    let isServerAuth = CheckServerAuth(auth);
    let isClientAuth = false;
    let GUID = "";
    let did = "";
    if (!isServerAuth){
        isClientAuth = (await CheckAuth(auth));
        if (isClientAuth){
            GUID = AuthPlayerGuid(auth);
            isClientAuth = (CheckPlayerAuth(GUID, auth));
            did = GetDiscordObj(GUID);
            isClientAuth = await isClientAuth;
            did = (await did).id;
        }
    }
    if ( isServerAuth || isClientAuth){
        try{
            let data = req.body; 
            let guild = await client.guilds.fetch(global.config.Discord.Guild_Id);
            let cid = id;
            let message = { embeds: [data] };
            if (data.Message !== undefined)
                message = data.Message;
            try {
                if (!cid) throw new Error("Channel ID is not defined");
                let channel = guild.channels.cache.get(cid);
                let user = await guild.members.fetch(did);
                //console.log(channel)
                if (channel && user){
                    let perms = false;
                    if (isClientAuth){
                        perms = (channel.permissionsFor(user).has('VIEW_CHANNEL') && channel.permissionsFor(user).has('SEND_MESSAGES'));
                    }
                    //console.log(perms)
                    if (isServerAuth || (isClientAuth && perms)){
                        let msg = await channel.send(message);
                        logger.info(`Sent Message to channel ${cid} id: ${msg.id}`);
                        res.status(200).json({Status: "Success", Error: ``, oid: `${msg.id}`});
                    } else {
                        logger.warn(`Error couldn't send message to channel ${cid} as user ${GUID} doesn't have permissions to send messagaes in the channel`, { GUID, channelId: cid });
                        res.status(200).json({Status: "NoPerms", Error: `User does not have permissions to use this channel`, oid: `0`});
                    }
                } else {
                    logger.warn(`Error couldn't send message to channel ${cid} as it does not exist`, { channelId: cid });
                    res.status(200).json({Status: "NotFound", Error: `Channel not found in discord`, oid: `0`});
                }
            } catch (e){
                logger.warn(`Error couldn't send message to channel ${id}`, { error: e });
                return res.status(200).json({Status: "NotFound", Error: `${e}`, oid: `0`});
            }
        } catch(e){
            logger.warn(`Error couldn't send message to channel ${id}`, { error: e });
            return res.status(400).json({Status: "Error", Error: `${e}`, oid: "0"});
        }
    } else {
        logger.warn("AUTH ERROR: " + req.url);
        return res.status(401).json({Status: "Error", Error: `Invalid Auth`, oid: "0"});
    }
}


async function GetMessagesChannel(req, res){
    let id = req.params.id;
    let auth = req.headers['auth-key'];
    let isServerAuth = CheckServerAuth(auth);
    let isClientAuth = false;
    let GUID = "";
    let did = "";
    if (!isServerAuth){
        isClientAuth = (await CheckAuth(auth));
       // console.log(isClientAuth)
        if (isClientAuth){
            GUID = AuthPlayerGuid(auth);
            isClientAuth = (CheckPlayerAuth(GUID, auth));
            did = (await GetDiscordObj(GUID)).id;
            isClientAuth = await isClientAuth;
        }
    }
    if ( isServerAuth || isClientAuth){
        try{
            let RawData = req.body; 
            let guild = await client.guilds.fetch(global.config.Discord.Guild_Id);
            let cid = id;
            let filter = {};
            if (RawData.Limit !== undefined && RawData.Limit > 0){ filter.limit = RawData.Limit; }
            if (RawData.Before !== undefined && RawData.Before !== ""){filter.before = RawData.Before;}
            if (RawData.After !== undefined && RawData.After !== ""){filter.after = RawData.After;}
            try {
                let channel = await guild.channels.cache.get(cid);
                let user = await guild.members.fetch(did);
                //console.log(channel)
                if (channel && user){
                    let perms = false;
                    if (isClientAuth){
                        perms = (channel.permissionsFor(user).has('VIEW_CHANNEL') && channel.permissionsFor(user).has('SEND_MESSAGES') && channel.permissionsFor(user).has('READ_MESSAGE_HISTORY')); 
                    }
                    if (isServerAuth || (isClientAuth && perms)){
                        let messages = (await channel.messages.fetch(filter)).array();
                        //console.log(messages);
                        let msgs = await Promise.all( messages.map(ReMapMessage));
                        res.status(200);
                        res.json({Status: "Success", Error: ``, Messages: msgs});
                    } else {
                        logger.warn(`Error couldn't get messages from channel ${cid} as user ${GUID} doesn't have permissions to read message history in the channel`, { GUID, channelId: cid });
                        res.status(200);
                        res.json({Status: "NoPerms", Error: `User does not have permissions to use this channel`, Messages: []});
                    }
                } else {
                    logger.warn(`Error couldn't get messages from channel ${cid} as it does not exist`, { channelId: cid });
                    res.status(200);
                    res.json({Status: "NotFound", Error: `Channel not found in discord`, Messages: []});
                }
            } catch (e){
                res.status(200);
                res.json({Status: "NotFound", Error: `${e}`, Messages: []});
                logger.warn(`Error couldn't get messages from channel ${id}`, { error: e });
            }
        } catch(e){
            res.status(400);
            res.json({Status: "Error", Error: `${e}`, Messages: []});
            logger.warn(`Error couldn't get messages from channel ${id}`, { error: e });
        }
    } else {
        res.status(401);
        res.json({Status: "Error", Error: `Invalid Auth Key`, Messages: []});
        logger.warn("AUTH ERROR: " + req.url);
    }
}


/**
 * Creates an invite for a Discord channel.
 * 
 * @async
 * @function InviteChannel
 * @param {Object} req - Express request object
 * @param {number} [Options.maxAge=86400] - Maximum age of the invite in seconds (defaults to 24 hours)
 * @param {number} [Options.maxUses=0] - Maximum number of uses (0 for unlimited)
 * @param {boolean} [Options.unique=true] - Whether the invite should be unique
 * @param {string} [Options.reason='API requested invite'] - Reason for creating the invite
 * @param {Object} res - Express response object
 * 
 * @returns {Promise<void>} Sends a JSON response with the status and invite details
 * @returns {Object} 201 - Success response with invite code and URL
 * @returns {Object} 200 - Error response when the channel doesn't support invites
 * @returns {Object} 200 - NotFound response when channel doesn't exist
 * @returns {Object} 400 - Error response for request parsing or other errors
 * 
 * @throws {Error} If there's an issue fetching the guild or creating the invite
 */
async function InviteChannel(req, res) {
    let id = req.params.id;
    try {
        let RawData = req.body;
        let guild = await client.guilds.fetch(global.config.Discord.Guild_Id);
        let options = RawData.Options || {};
        
        try {
            let channel = await guild.channels.cache.get(id);
            if (channel) {
                // Check if channel supports invites (text, voice, etc.)
                if (channel.isText() || channel.isVoice()) {
                    const invite = await channel.createInvite({
                        maxAge: options.maxAge || 86400, // 24 hours by default
                        maxUses: options.maxUses || 0, // unlimited uses by default
                        unique: options.unique !== undefined ? options.unique : true,
                        reason: options.reason || 'API requested invite'
                    });
                    
                    logger.info(`Created invite for channel`, { channelId: id, inviteCode: invite.code });
                    res.status(201).json({
                        Status: "Success", 
                        Error: ``, 
                        oid: `${invite.code}`,
                        url: invite.url
                    });
                } else {
                    logger.warn(`Can't create invite for this channel type`, { channelId: id });
                    res.status(200).json({
                        Status: "Error", 
                        Error: "This channel type doesn't support invites", 
                        oid: id
                    });
                }
            } else {
                logger.warn(`Couldn't create invite - channel doesn't exist`, { channelId: id });
                res.status(200).json({Status: "NotFound", Error: ``, oid: `${id}`});
            }
        } catch (e) {
            res.status(200).json({Status: "Error", Error: `${e}`, oid: `${id}`});
            logger.warn(`Error creating invite for channel`, { channelId: id, error: e });
        }
    } catch (e) {
        res.status(400).json({Status: "Error", Error: `${e}`, oid: "0"});
        logger.warn(`Error creating invite for channel`, { channelId: id, error: e });
    }
}

module.exports = {CreateChannel, DeleteChannel, EditChannel, SendMessageChannel, GetMessagesChannel, InviteChannel};