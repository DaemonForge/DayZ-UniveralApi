const { Client, GatewayIntentBits } = require("discord.js");
const {createLogger} = require('../utils');
const logger = createLogger(global.logger, 'discord');
global.DISCORDSTATUS = "Pending";

const intents = [
    GatewayIntentBits.Guilds,
    GatewayIntentBits.GuildMembers,
    GatewayIntentBits.DirectMessages,
    GatewayIntentBits.DirectMessageReactions,
    GatewayIntentBits.GuildVoiceStates,
    GatewayIntentBits.GuildInvites,
    GatewayIntentBits.GuildMessages,
    GatewayIntentBits.GuildMessageReactions
];
const client = new Client({ intents });

try {
    if (global.config.Discord?.Bot_Token !== "" && global.config.Discord?.Bot_Token !== undefined){
        client.login(global.config.Discord.Bot_Token);
        logger.info("Logging in to discord bot");
    } else {
        logger.warn("Discord Bot Token not present you will not be able to use any discord functions");
        global.DISCORDSTATUS = "Disabled";
    }
} catch (e){
    logger.warn("Discord Bot Token is invalid", { error: e });
    global.DISCORDSTATUS = "Error";
}

// Use 'clientReady' instead of deprecated 'ready' event (deprecated in discord.js v14, removed in v15)
client.on('clientReady', () => {
    logger.info(`Discord Bot Ready! ${client.user.tag}`, { username: client.user.tag });
    global.DISCORDSTATUS = "Online";
});

client.on('disconnect', () => {
    logger.warn(`Bot Disconnected!`, { event: 'disconnect' });
    global.DISCORDSTATUS = "Disconnected";
});

client.on('error', error => {
    logger.error(`Discord Bot Error`, { error: error.message, stack: error.stack });
    global.DISCORDSTATUS = "Error";
});

client.on('warn', warning => {
    logger.warn(`Discord Bot Warning`, { warning });
});

module.exports = client;
