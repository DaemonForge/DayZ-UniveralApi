const { MongoClient } = require("mongodb");
const {readFileSync, writeFileSync, existsSync, mkdirSync} = require('fs');
const logger = global.logger; // take the logger from he global
const client = require("./bot.js");
const {render} = require('ejs');
const DefaultTemplates = require('../templates/defaultTemplates.json');

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
    res.send(render(LoginTemplate, {SteamId: id, Login_URL: `/discord/login/${id}`, Connected: (userObj !== undefined)}));
    
}

async function RenderLogin(req, res){
    let id = req.params.id;
    let GUID = NormalizeToGUID(id);
    if (ErrorTemplate === undefined) LoadErrorTemplate();
    let userObj = await GetDiscordObj(GUID);
    let ip = req.headers['CF-Connecting-IP'] ||  req.headers['x-forwarded-for'] || req.connection.remoteAddress;
    if (userObj !== undefined && (global.config.Discord?.AllowToReRegister !== true) === false){
        return res.send(render(ErrorTemplate, {TheError: "Trying to connect to a Steam ID that already has a Discord connected.", Type: "AlreadyLinked"}))
    }

    let url = encodeURIComponent(`https://${req.headers.host}/discord/callback`); 
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
    if (ErrorTemplate === undefined) LoadErrorTemplate();
    if (SuccessTemplate === undefined) LoadSuccessTemplate();
    const code = req.query.code;
    const state = req.query.state;

    if (code === undefined || code === null || state === undefined || state === null){
        logger.warn(`HandleCallBack - Invalid Response from Discord`);
        res.send(render(ErrorTemplate, {TheError: "Invalid Response from Discord", Type: "Discord"}));
        return;
    }
    const mongo = new MongoClient(global.config.DBServer, { useUnifiedTopology: true });
    try {
        let connect = mongo.connect();
        const response = await fetch(`https://discordapp.com/api/oauth2/token`,{
            method: 'POST',
            headers: {
            },
            body: new URLSearchParams({
                client_id: global.config.Discord.Client_Id,
                client_secret: global.config.Discord.Client_Secret,
                grant_type: 'authorization_code',
                code: code,
                redirect_uri: `https://${req.headers.host}/discord/callback`
            }),
        });
        const json = await response.json();;
        const discordres = await fetch(`https://discordapp.com/api/users/@me`, {
            method: 'GET',
            headers: {
                Authorization: `Bearer ${json.access_token}`
            }
        });
        let discordjson = await discordres.json();
        discordjson.steamid = state;
        let guild = await client.guilds.fetch(global.config.Discord.Guild_Id);
        let msg = "Unknown Error";
        let errType = "System";
        let player;
        try {
            player = await guild.members.fetch(discordjson.id);
        } catch (e) {
            logger.warn(`Error fetching discord member`, { error: e, details: JSON.stringify(e) });
            msg = "User not found in discord";
            errType = "UserNotFound";
        }
        if(player !== undefined && player.roles !== undefined){
            let roles = player._roles

            msg = "User is missing the role";
            errType = "RoleRequired";
            if (global.config.Discord.BlackList_Role !== "" && global.config.Discord.BlackList_Role !== undefined && roles.find(element => element === global.config.Discord.BlackList_Role) !== undefined){
                msg = "You have a blacklisted role";
                errType = "Blacklisted";
            } else if (global.config.Discord.Required_Role === "" || global.config.Discord.Required_Role === undefined || roles.find(element => element === global.config.Discord.Required_Role) !== undefined){
                msg = "Success";
            } 
        }

        if (msg === "Success"){
            discordjson.avatar = `https://cdn.discordapp.com/avatars/${discordjson.id}/${discordjson.avatar}`
           
            let guid = createHash('sha256').update(discordjson.steamid).digest('base64');
            guid = guid.replace(/\+/g, '-'); 
            guid = guid.replace(/\//g, '_');
            await connect;

            let data = {
                GUID: guid,
                Discord: {
                    id: discordjson.id,
                    username: discordjson.username,
                    discriminator: discordjson.discriminator,
                    avatar: discordjson.avatar
                }
            }
            // Connect the client to the server
            const db = mongo.db(global.config.DB);
            let collection = db.collection("Players");
            
            let query = { "Discord.id": discordjson.id };
            let results = collection.find(query);
            if ((await collection.countDocuments(query)) == 0){
                query = { GUID: guid };
                const options = { upsert: true };
                const updateDoc = { $set: data, };
                const result = await collection.updateOne(query, updateDoc, options);

                if ( result.matchedCount === 1 || result.upsertedCount === 1 ){
                    logger.info("Player connected to Discord", { 
                        GUID: guid, 
                        discordId: discordjson.id 
                    });
                    res.send(render(SuccessTemplate, {DiscordId: discordjson.id, DiscordUsername: discordjson.username, DiscordAvatar: discordjson.avatar, DiscordDiscriminator: discordjson.discriminator, SteamId: discordjson.steamid}))
                } else {
                    logger.warn("Error when trying to link player to discord", { 
                        GUID: guid, 
                        discordId: discordjson.id 
                    });
                    res.send(render(ErrorTemplate, {TheError: "There was an error linking your discord account", Type: "Database"}));
                }
            } else {
                let dataarr = await results.toArray(); 
                let querydata = dataarr[0]; 
                if ( guid === querydata.GUID){
                    logger.warn(`Player tried to link to discord ID already in use`, { 
                        GUID: guid, 
                        discordId: discordjson.id, 
                        existingGUID: querydata.GUID 
                    });
                    res.send( render(ErrorTemplate, {TheError: "It seems you already have your discord account Linked", Type: "AlreadyLinked"} ) );
                } else {
                    logger.warn(`Player tried to link to discord ID already in use with another account`, { 
                        GUID: guid, 
                        discordId: discordjson.id, 
                        existingGUID: querydata.GUID 
                    });
                    res.send( render(ErrorTemplate, { TheError: "You already have your discord linked to another account", Type: "Conflict"} ) );
                }
            }
        } else {
            res.send(render(ErrorTemplate, {TheError: msg, Type: errType}));
        }
    } catch (e){
        logger.warn(`Error in HandleCallback`, { 
            error: e, 
            details: JSON.stringify(e) 
        });
        try {
            res.send(render(ErrorTemplate, {TheError: e, Type: "System"}));
        } catch(err){
            logger.warn(`Error rendering error template`, { error: err });
            res.send(`<html><head><title>Invalid Link</title></head><body><h1>Error: Invalid Error Templates</h1></body></html>`);
        }
    } finally {
        // Ensures that the client will close when you finish/error
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