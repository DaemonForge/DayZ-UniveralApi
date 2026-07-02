const { MongoClient } = require("mongodb");
const {readFileSync, writeFileSync, existsSync, mkdirSync} = require('fs');

const {createLogger, NormalizeToGUID} = require('../utils');
const {createHash} = require('crypto');
const {GetDiscordObj} = require('./dsUtils');
const logger = createLogger(global.logger, 'discord');
const client = require("./bot.js");
const {render} = require('ejs');
const DefaultTemplates = require('../templates/defaultTemplates.json');
const { log } = require("console");

//Create Template Folder if it doesn't exist
if (!existsSync(global.SAVEPATH + 'templates')) mkdirSync(global.SAVEPATH + 'templates');

//Load Signup Templates
let LoginTemplate;
let ErrorTemplate;
let SuccessTemplate;
LoadLoginTemplate();
LoadSuccessTemplate();
LoadErrorTemplate();


async function SendLoginPage(res, id, guid){
    let userObj = await GetDiscordObj(guid);
    console.log(userObj);
    res.send(render(LoginTemplate, {SteamId: id, Login_URL: `/Discord/login/${id}`, Connected: (userObj !== undefined && userObj !== null)}));
    
}

// A SteamID64 is a 17-digit number (first digit non-zero).
const STEAMID64_REGEX = /^[1-9][0-9]{16}$/;

/**
 * Resolves the public host for the OAuth redirect_uri. Prefers the operator's
 * configured LetsEncrypt domain so the callback isn't derived from a
 * client-supplied Host header; falls back to the request Host otherwise.
 */
function getCallbackHost(req) {
    const leDomain = global.config.LetsEncypt?.Enabled ? global.config.LetsEncypt?.Domain : "";
    return leDomain || req.headers.host;
}

async function RenderLogin(req, res){
    let id = req.params.id;
    if (ErrorTemplate === undefined) LoadErrorTemplate();
    // Validate the Steam ID before it is reflected into the OAuth state / login
    // URL. Rejects malformed input rather than echoing it back.
    if (!STEAMID64_REGEX.test(id)) {
        logger.warn("Discord login attempted with an invalid Steam ID", { id });
        return res.send(render(ErrorTemplate, {TheError: "Invalid URL", Type: "BadURL"}));
    }
    let GUID = NormalizeToGUID(id);
    let userObj = await GetDiscordObj(GUID);
    let ip = req.headers['cf-connecting-ip'] ||  req.headers['x-forwarded-for'] || req.connection.remoteAddress;
    if (userObj !== undefined && (global.config.Discord?.AllowToReRegister !== true) === false){
        return res.send(render(ErrorTemplate, {TheError: "Trying to connect to a Steam ID that already has a Discord connected.", Type: "AlreadyLinked"}))
    }

    let url = encodeURIComponent(`https://${getCallbackHost(req)}/discord/callback`);
    if ( global.config.Discord.Client_Id === "" || global.config.Discord.Client_Secret === ""  || global.config.Discord.Bot_Token === ""  || global.config.Discord.Guild_Id === "" || global.config.Discord.Client_Id === undefined || global.config.Discord.Client_Secret === undefined  || global.config.Discord.Bot_Token === undefined  || global.config.Discord.Guild_Id === undefined ){
        logger.warn("User tried to sign up for discord but Intergration is not setup for this server");
        return res.send(render(ErrorTemplate, {TheError: "Discord Intergration is not setup for this server", Type: "NotSetup"}))
    }
    if (global.config.Discord?.Client_Id === undefined && !id.match(/[1-9][0-9]{16,16}/)){
        logger.warn("User tried to sign up for discord but steam id isn't a valid id");
        return res.send(render(ErrorTemplate, {TheError: "Invalid URL", Type: "BadURL"}))
    }
    if (global.config.Discord.Restrict_Sign_Up === undefined || !global.config.Discord.Restrict_Sign_Up){
        return res.redirect(`https://discordapp.com/api/oauth2/authorize?client_id=${global.config.Discord.Client_Id}&scope=identify&response_type=code&redirect_uri=${url}&state=${id}`);
    }
    if (ip === undefined){
        logger.warn("User tried to sign up for discord but ip isn't valid");
        return res.send(render(ErrorTemplate, {TheError: "Invalid URL", Type: "BadURL"}));
    }
    let responsejson;
    try {
        responsejson = await fetch(`http://ip-api.com/json/${ip}?fields=status,message,countryCode,country,regionName,isp,org,as,proxy,hosting,query`).then((response)=>response.json())
        if (responsejson.status === "success" && !responsejson.proxy && !responsejson.hosting){
            if (global.config.Discord.Restrict_Sign_Up_Countries !== undefined && global.config.Discord.Restrict_Sign_Up_Countries[0] !== undefined){
                let found = (global.config.Discord.Restrict_Sign_Up_Countries.find(element => element == responsejson.countryCode));
                if (global.config.Discord.Restrict_Sign_Up_Countries[0] === 'blacklist' && !found){
                    logger.warn(`User signed up under restictive mode`, { 
                        steamId: id, 
                        ip: responsejson.query, 
                        country: responsejson.countryCode, 
                        region: responsejson.regionName, 
                        isp: responsejson.isp 
                    });
                   return res.redirect(`https://discordapp.com/api/oauth2/authorize?client_id=${global.config.Discord.Client_Id}&scope=identify&response_type=code&redirect_uri=${url}&state=${id}`);
                }
                if (found){
                   logger.warn(`User signed up under restictive mode`, { 
                       steamId: id, 
                       ip: responsejson.query, 
                       country: responsejson.countryCode, 
                       region: responsejson.regionName, 
                       isp: responsejson.isp 
                   });
                    return res.redirect(`https://discordapp.com/api/oauth2/authorize?client_id=${global.config.Discord.Client_Id}&scope=identify&response_type=code&redirect_uri=${url}&state=${id}`);
                }
            }
            if (responsejson.status === "success" ){
               logger.warn(`User signed up under restictive mode`, { 
                   steamId: id, 
                   ip: responsejson.query, 
                   country: responsejson.countryCode, 
                   region: responsejson.regionName, 
                   isp: responsejson.isp 
               });
               return res.redirect(`https://discordapp.com/api/oauth2/authorize?client_id=${global.config.Discord.Client_Id}&scope=identify&response_type=code&redirect_uri=${url}&state=${id}`);
           }
        }
    } catch (e) {        
        logger.warn(`User Failed Singed up under restictive mode`, { 
            steamId: id, 
            error: e, 
            stack: e.stack 
        });
        return res.send(render(ErrorTemplate, {TheError: `Error Validating the signup proccess - ${e}`, Type: "ValidationError"}));
    }
    logger.warn(`User Failed Singed up under restictive mode`, { 
        steamId: id, 
        ip: responsejson.query, 
        country: responsejson.countryCode, 
        region: responsejson.regionName, 
        isp: responsejson.isp 
    });
    return res.send(render(ErrorTemplate, {TheError: `Error validating the signup proccess`, Type: "ValidationError"}));
}



async function HandleCallBack(req, res){
    logger.info("Discord OAuth callback received", { hasCode: !!req.query.code, hasState: !!req.query.state });
    if (ErrorTemplate === undefined) LoadErrorTemplate();
    if (SuccessTemplate === undefined) LoadSuccessTemplate();
    const code = req.query.code;
    const state = req.query.state;
    logger.debug("Parsed parameters", { code, state });

    if (code === undefined || code === null || state === undefined || state === null){
        logger.warn("HandleCallBack - Invalid Response from Discord");
        res.send(render(ErrorTemplate, {TheError: "Invalid Response from Discord", Type: "Discord"}));
        return;
    }
    const mongo = new MongoClient(global.config.DBServer);
    try {
        let connect = mongo.connect();
        logger.debug("Requesting token from Discord");
        const response = await fetch(`https://discord.com/api/oauth2/token`,{
            method: 'POST',
            headers: {
                "Content-Type": "application/x-www-form-urlencoded"
            },
            body: new URLSearchParams({
                client_id: global.config.Discord.Client_Id,
                client_secret: global.config.Discord.Client_Secret,
                grant_type: 'authorization_code',
                code: code,
                // Must match the redirect_uri used in RenderLogin's authorize step.
                redirect_uri: `https://${getCallbackHost(req)}/discord/callback`
            }),
        });
        const json = await response.json();
        logger.debug("Received token response", { tokenResponse: json });
        const discordres = await fetch(`https://discord.com/api/users/@me`, {
            method: 'GET',
            headers: {
                Authorization: `Bearer ${json.access_token}`
            }
        });
        let discordjson = await discordres.json();
        logger.debug("Received Discord user details", discordjson);
        discordjson.steamid = state;
        let guild = await client.guilds.fetch(global.config.Discord.Guild_Id);
        logger.debug("Fetched discord guild", { guildId: global.config.Discord.Guild_Id });
        let msg = `Unknown Error, possible that call back isn't configured correctly should be "https://${getCallbackHost(req)}/discord/callback"`;
        let errType = "System";
        let player;
        try {
            player = await guild.members.fetch(discordjson.id);
            logger.debug("Fetched discord member", { memberId: discordjson.id });
        } catch (e) {
            logger.warn("Error fetching discord member", { error: e, details: JSON.stringify(e) });
            msg = "User not found in discord";
            errType = "UserNotFound";
        }
        if (player && player.roles && player.roles.cache) {
            let roles = player.roles.cache;
            msg = "User is missing the role";
            errType = "RoleRequired";
            if (global.config.Discord.BlackList_Role && roles.some(r => r.id === global.config.Discord.BlackList_Role)){
                msg = "You have a blacklisted role";
                errType = "Blacklisted";
            } else if (!global.config.Discord.Required_Role || roles.some(r => r.id === global.config.Discord.Required_Role)){
                msg = "Success";
            }
            logger.debug("Role check result", { msg, errType, roles: roles.map(r => r.id) });
        }

        if (msg === "Success"){
            discordjson.avatar = `https://cdn.discordapp.com/avatars/${discordjson.id}/${discordjson.avatar}`;
            let guid = createHash('sha256').update(discordjson.steamid).digest('base64');
            guid = guid.replace(/\+/g, '-'); 
            guid = guid.replace(/\//g, '_');
            await connect;
            logger.debug("Connected to MongoDB", { guid });

            let data = {
                GUID: guid,
                Discord: {
                    id: discordjson.id,
                    globalName: discordjson.global_name,
                    username: discordjson.username,
                    avatar: discordjson.avatar
                }
            }
            // Connect the client to the server
            const db = mongo.db(global.config.DB);
            let collection = db.collection("Players");
            
            let query = { "Discord.id": discordjson.id };
            let results = collection.find(query);
            const count = await collection.countDocuments(query);
            logger.debug("Checking existing discord record", { discordId: discordjson.id, count });
            if (count == 0){
                query = { GUID: guid };
                const options = { upsert: true };
                const updateDoc = { $set: data, };
                const result = await collection.updateOne(query, updateDoc, options);
                logger.debug("MongoDB updateOne result", { result });
                if ( result.matchedCount === 1 || result.upsertedCount === 1 ){
                    logger.info("Player connected to Discord", { GUID: guid, discordId: discordjson.id });
                    res.send(render(SuccessTemplate, {DiscordId: discordjson.id, DiscordUsername: discordjson.username, DiscordAvatar: discordjson.avatar, DiscordName: discordjson.global_name, SteamId: discordjson.steamid}));
                } else {
                    logger.warn("Error when trying to link player to discord", { GUID: guid, discordId: discordjson.id });
                    res.send(render(ErrorTemplate, {TheError: "There was an error linking your discord account", Type: "Database"}));
                }
            } else {
                let dataarr = await results.toArray(); 
                let querydata = dataarr[0]; 
                logger.debug("Existing discord record found", { existingGUID: querydata.GUID, newGUID: guid });
                if ( guid === querydata.GUID){
                    logger.warn("Player tried to link discord already in use", { GUID: guid, discordId: discordjson.id, existingGUID: querydata.GUID });
                    res.send(render(ErrorTemplate, {TheError: "It seems you already have your discord account Linked", Type: "AlreadyLinked"}));
                } else {
                    logger.warn("Player tried to link discord already in use with another account", { GUID: guid, discordId: discordjson.id, existingGUID: querydata.GUID });
                    res.send(render(ErrorTemplate, { TheError: "You already have your discord linked to another account", Type: "Conflict"}));
                }
            }
        } else {
            logger.debug("Discord user does not meet role requirements", { msg, errType });
            res.send(render(ErrorTemplate, {TheError: msg, Type: errType}));
        }
    } catch (error){
        logger.warn("Error in HandleCallback", { error, details: JSON.stringify(error) });
        try {
            res.send(render(ErrorTemplate, {TheError: error, Type: "System"}));
        } catch(err){
            console.log(err);
            logger.warn("Error rendering error template", err);
            res.send(`<html><head><title>Invalid Link</title></head><body><h1>Error: Invalid Error Templates</h1></body></html>`);
        }
    } finally {
        logger.debug("Closing MongoDB connection");
        mongo.close();
    }
}







//User Facing Code
function LoadLoginTemplate(){
    try{
        LoginTemplate = readFileSync(global.SAVEPATH + "templates/discordLogin.ejs","utf8");
    } catch (e) {
        logger.info("Login Template Missing Creating It Now", { error: e });
        LoginTemplate = DefaultTemplates.Login;
        writeFileSync(global.SAVEPATH + "templates/discordLogin.ejs", LoginTemplate);
    }
    try {
        let error = ejsLint(LoginTemplate) ;
        if (error !== undefined){
            LoginTemplate = DefaultTemplates.Login;
            logger.warn("============ ERROR IN LOGIN TEMPLATE ================");
            logger.warn("Template Error", { error });
            logger.warn("=====================================================");
        }
    } catch (e) {
        //console.log(e);
    }
}

function LoadSuccessTemplate(){
    try{
        SuccessTemplate = readFileSync(global.SAVEPATH + "templates/discordSuccess.ejs","utf8");
    } catch (e) {
        logger.info("Success Template Missing Creating It Now", { error: e });
        SuccessTemplate = DefaultTemplates.Success;
        writeFileSync(global.SAVEPATH + "templates/discordSuccess.ejs", SuccessTemplate);
    }
    try {
        let error = ejsLint(SuccessTemplate) ;
        if (error !== undefined){
            LoginTemplate = DefaultTemplates.Success;
        
            logger.warn("=========== ERROR IN SUCCESS TEMPLATE ===============");
            logger.warn("Template Error", { error });
            logger.warn("=====================================================");
        }
    } catch (e) {
        //console.log(e);
    }
}
function LoadErrorTemplate(){
    try{
        ErrorTemplate = readFileSync(global.SAVEPATH + "templates/discordError.ejs","utf8");
    } catch (e) {
        logger.info("Error Template Missing Creating It Now", { error: e });
        ErrorTemplate = DefaultTemplates.Error;
        writeFileSync(global.SAVEPATH + "templates/discordError.ejs", ErrorTemplate);
    }
    try {
        let error = ejsLint(ErrorTemplate) ;
        if (error !== undefined){
            ErrorTemplate = DefaultTemplates.Error;
        
            logger.warn("============ ERROR IN ERROR TEMPLATE ================");
            logger.warn("Template Error", { error });
            logger.warn("=====================================================");
        }
    } catch (e) {
        //console.log(e);
    }

}

async function renderRootErrorTemplate(req, res){
    res.send(render((await GetErrorTemplate()), {TheError: "Invalid URL", Type: "BadURL"}));
}


async function GetErrorTemplate(){
    if (ErrorTemplate === undefined) await LoadErrorTemplate();
    return ErrorTemplate;
}

async function GetSuccessTemplate(){
    if (SuccessTemplate === undefined) await LoadSuccessTemplate();
    return SuccessTemplate;
}

async function GetLoginTemplate(){
    if (LoginTemplate === undefined) await LoadLoginTemplate();
    return LoginTemplate;
}

module.exports = {renderRootErrorTemplate,  RenderLogin, HandleCallBack, SendLoginPage, GetLoginTemplate, GetSuccessTemplate, GetErrorTemplate};