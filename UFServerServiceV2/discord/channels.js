const client = require("./bot.js");
const { User, GuildMember, Guild } = require("discord.js");
const { ReMapMessage } = require("./dsUtils");
const { createLogger } = require("../utils");
const logger = createLogger(global.logger, "discord");

async function CreateChannel(req, res) {
    let RawData = req.body;
    try {
        let guild = await client.guilds.fetch(global.config.Discord.Guild_Id);
        let options = RawData.Options;
        let channel = await guild.channels.create(RawData.Name, options);
        let id = channel.id;

        res.status(201);
        res.json({ Status: "Success", Error: "", oid: `${id}` });
    } catch (e) {
        res.status(400);
        res.json({ Status: "Error", Error: `${e.message}`, oid: "0" });
        logger.error(`Error creating channel "${RawData?.Name}": ${e.message}`, { error: e, rawData: RawData });
    }
}

async function DeleteChannel(req, res) {
    let id = req.params.id;
    let RawData = req.body;
    try {
        let guild = await client.guilds.fetch(global.config.Discord.Guild_Id);
        let reason = RawData.Reason;
        try {
            let channel = guild.channels.cache.get(id);
            if (channel) {
                channel.delete(reason)
                    .then(() => {
                        logger.info(`Deleted channel ${id} successfully`, { channelId: id, reason });
                        res.status(200);
                        res.json({ Status: "Success", Error: "", oid: `${id}` });
                    })
                    .catch((e) => {
                        logger.error(`Error deleting channel ${id}: ${e.message}`, { error: e, channelId: id, reason });
                        res.status(200);
                        res.json({ Status: "Error", Error: `${e.message}`, oid: `${id}` });
                    });
            } else {
                logger.warn(`Channel ${id} not found for deletion`, { channelId: id, reason });
                res.status(200);
                res.json({ Status: "NotFound", Error: "", oid: `${id}` });
            }
        } catch (e) {
            logger.error(`Error accessing channel ${id} for deletion: ${e.message}`, { error: e, channelId: id, reason });
            res.status(200);
            res.json({ Status: "NotFound", Error: `${e.message}`, oid: `${id}` });
        }
    } catch (e) {
        logger.error(`Error deleting channel ${id}: ${e.message}`, { error: e, channelId: id });
        res.status(400);
        res.json({ Status: "Error", Error: `${e.message}`, oid: "0" });
    }
}

async function EditChannel(req, res) {
    let id = req.params.id;
    let RawData = req.body;
    try {
        let guild = await client.guilds.fetch(global.config.Discord.Guild_Id);
        let reason = RawData.Reason;
        let options = RawData.Options;
        try {
            let channel = guild.channels.cache.get(id);
            if (channel) {
                channel.edit(options, reason)
                    .then(() => {
                        logger.info(`Edited channel ${id} successfully`, { channelId: id, reason, options });
                        res.status(200);
                        res.json({ Status: "Success", Error: "", oid: `${id}` });
                    })
                    .catch((e) => {
                        logger.error(`Error editing channel ${id}: ${e.message}`, { error: e, channelId: id, reason, options });
                        res.status(200);
                        res.json({ Status: "Error", Error: `${e.message}`, oid: `${id}` });
                    });
            } else {
                logger.warn(`Channel ${id} not found for editing`, { channelId: id, reason });
                res.status(200);
                res.json({ Status: "NotFound", Error: "", oid: `${id}` });
            }
        } catch (e) {
            logger.error(`Error accessing channel ${id} for editing: ${e.message}`, { error: e, channelId: id, reason });
            res.status(200);
            res.json({ Status: "NotFound", Error: `${e.message}`, oid: `${id}` });
        }
    } catch (e) {
        logger.error(`Error editing channel ${id}: ${e.message}`, { error: e, channelId: id });
        res.status(400);
        res.json({ Status: "Error", Error: `${e.message}`, oid: "0" });
    }
}

async function SendMessageChannel(req, res) {
    let id = req.params.id;
    let auth = req.headers["auth-key"];
    let isServerAuth = CheckServerAuth(auth);
    let isClientAuth = false;
    let GUID = "";
    let did = "";
    if (!isServerAuth) {
        isClientAuth = await CheckAuth(auth);
        if (isClientAuth) {
            GUID = AuthPlayerGuid(auth);
            isClientAuth = CheckPlayerAuth(GUID, auth);
            did = await GetDiscordObj(GUID);
            did = did.id;
        }
    }
    if (isServerAuth || isClientAuth) {
        try {
            let data = req.body;
            let guild = await client.guilds.fetch(global.config.Discord.Guild_Id);
            let cid = id;
            let message = data.Message !== undefined ? data.Message : { embeds: [data] };
            try {
                if (!cid) throw new Error("Channel ID is not defined");
                let channel = guild.channels.cache.get(cid);
                let user = await guild.members.fetch(did);
                if (channel && user) {
                    let perms = false;
                    if (isClientAuth) {
                        perms =
                            channel.permissionsFor(user).has("VIEW_CHANNEL") &&
                            channel.permissionsFor(user).has("SEND_MESSAGES");
                    }
                    if (isServerAuth || (isClientAuth && perms)) {
                        let msg = await channel.send(message);
                        logger.info(`Sent message to channel ${cid} successfully`, { channelId: cid, messageId: msg.id });
                        res.status(200).json({ Status: "Success", Error: "", oid: `${msg.id}` });
                    } else {
                        logger.warn(`Insufficient permissions for user ${GUID} to send message in channel ${cid}`, { GUID, channelId: cid });
                        res.status(200).json({ Status: "NoPerms", Error: "User does not have permissions to use this channel", oid: "0" });
                    }
                } else {
                    logger.warn(`Channel ${cid} not found or user ${did} not found`, { channelId: cid, userId: did });
                    res.status(200).json({ Status: "NotFound", Error: "Channel not found in discord", oid: "0" });
                }
            } catch (e) {
                logger.error(`Error sending message to channel ${id}: ${e.message}`, { error: e, channelId: id });
                return res.status(200).json({ Status: "NotFound", Error: `${e.message}`, oid: "0" });
            }
        } catch (e) {
            logger.error(`Error sending message to channel ${id}: ${e.message}`, { error: e, channelId: id });
            return res.status(400).json({ Status: "Error", Error: `${e.message}`, oid: "0" });
        }
    } else {
        logger.warn("AUTH ERROR: Invalid auth key", { url: req.url, authKey: auth });
        return res.status(401).json({ Status: "Error", Error: "Invalid Auth", oid: "0" });
    }
}

async function GetMessagesChannel(req, res) {
    let id = req.params.id;
    let auth = req.headers["auth-key"];
    let isServerAuth = CheckServerAuth(auth);
    let isClientAuth = false;
    let GUID = "";
    let did = "";
    if (!isServerAuth) {
        isClientAuth = await CheckAuth(auth);
        if (isClientAuth) {
            GUID = AuthPlayerGuid(auth);
            isClientAuth = CheckPlayerAuth(GUID, auth);
            did = (await GetDiscordObj(GUID)).id;
        }
    }
    if (isServerAuth || isClientAuth) {
        try {
            let RawData = req.body;
            let guild = await client.guilds.fetch(global.config.Discord.Guild_Id);
            let cid = id;
            let filter = {};
            if (RawData.Limit !== undefined && RawData.Limit > 0) { filter.limit = RawData.Limit; }
            if (RawData.Before !== undefined && RawData.Before !== "") { filter.before = RawData.Before; }
            if (RawData.After !== undefined && RawData.After !== "") { filter.after = RawData.After; }
            try {
                let channel = guild.channels.cache.get(cid);
                let user = await guild.members.fetch(did);
                if (channel && user) {
                    let perms = false;
                    if (isClientAuth) {
                        perms =
                            channel.permissionsFor(user).has("VIEW_CHANNEL") &&
                            channel.permissionsFor(user).has("SEND_MESSAGES") &&
                            channel.permissionsFor(user).has("READ_MESSAGE_HISTORY");
                    }
                    if (isServerAuth || (isClientAuth && perms)) {
                        let messages = (await channel.messages.fetch(filter)).array();
                        let msgs = await Promise.all(messages.map(ReMapMessage));
                        res.status(200);
                        res.json({ Status: "Success", Error: "", Messages: msgs });
                    } else {
                        logger.warn(`Insufficient permissions for user ${GUID} to read messages in channel ${cid}`, { GUID, channelId: cid });
                        res.status(200);
                        res.json({ Status: "NoPerms", Error: "User does not have permissions to use this channel", Messages: [] });
                    }
                } else {
                    logger.warn(`Channel ${cid} not found for message fetch`, { channelId: cid });
                    res.status(200);
                    res.json({ Status: "NotFound", Error: "Channel not found in discord", Messages: [] });
                }
            } catch (e) {
                logger.error(`Error fetching messages from channel ${id}: ${e.message}`, { error: e, channelId: id });
                res.status(200);
                res.json({ Status: "NotFound", Error: `${e.message}`, Messages: [] });
            }
        } catch (e) {
            logger.error(`Error getting messages from channel ${id}: ${e.message}`, { error: e, channelId: id });
            res.status(400);
            res.json({ Status: "Error", Error: `${e.message}`, Messages: [] });
        }
    } else {
        logger.warn("AUTH ERROR: Invalid auth key for message fetch", { url: req.url });
        res.status(401);
        res.json({ Status: "Error", Error: "Invalid Auth Key", Messages: [] });
    }
}

/**
 * Creates an invite for a Discord channel.
 */
async function InviteChannel(req, res) {
    let id = req.params.id;
    try {
        let RawData = req.body;
        let guild = await client.guilds.fetch(global.config.Discord.Guild_Id);
        let options = RawData.Options || {};

        try {
            let channel = guild.channels.cache.get(id);
            if (channel) {
                // Check if channel supports invites (text, voice, etc.)
                if (channel.isText() || channel.isVoice()) {
                    const invite = await channel.createInvite({
                        maxAge: options.maxAge || 86400, // 24 hours by default
                        maxUses: options.maxUses || 0,      // unlimited uses by default
                        unique: options.unique !== undefined ? options.unique : true,
                        reason: options.reason || "API requested invite"
                    });

                    logger.info(`Created invite for channel ${id}`, { channelId: id, inviteCode: invite.code });
                    res.status(201).json({
                        Status: "Success",
                        Error: "",
                        oid: `${invite.code}`,
                        url: invite.url
                    });
                } else {
                    logger.warn(`Channel ${id} does not support invites`, { channelId: id });
                    res.status(200).json({
                        Status: "Error",
                        Error: "This channel type doesn't support invites",
                        oid: id
                    });
                }
            } else {
                logger.warn(`Channel ${id} not found for invite creation`, { channelId: id });
                res.status(200).json({ Status: "NotFound", Error: "", oid: `${id}` });
            }
        } catch (e) {
            logger.error(`Error creating invite for channel ${id}: ${e.message}`, { error: e, channelId: id });
            res.status(200).json({ Status: "Error", Error: `${e.message}`, oid: `${id}` });
        }
    } catch (e) {
        logger.error(`Error in InviteChannel for channel ${id}: ${e.message}`, { error: e, channelId: id });
        res.status(400).json({ Status: "Error", Error: `${e.message}`, oid: "0" });
    }
}

module.exports = { CreateChannel, DeleteChannel, EditChannel, SendMessageChannel, GetMessagesChannel, InviteChannel };
