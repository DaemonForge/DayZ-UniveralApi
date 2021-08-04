global.DISCORDSTATUS = "Pending";
const {Router} = require('express');
const {isArray, isObject,NormalizeToGUID} = require('./utils')
const {CheckAuth, CheckPlayerAuth,AuthPlayerGuid,CheckServerAuth} = require("./AuthChecker");
const { MongoClient } = require("mongodb");
const {createHash} = require('crypto');
const {readFileSync, writeFileSync, existsSync, mkdirSync} = require('fs');
const {render} = require('ejs');
const log = require("./log");
const {Client, GuildMember, Guild} = require("discord.js");
const client = new Client();
const fetch = require('node-fetch');
const DefaultTemplates = require("./templates/defaultTemplates.json");
const ejsLint = require('ejs-lint');
const router = Router();

var RateLimit = require('express-rate-limit');
let TheRateLimit = 40;
if (global.config.RequestLimitDiscord !== undefined){
    TheRateLimit =  Math.ceil( global.config.RequestLimitDiscord / 5)
}
var limiter = new RateLimit({
  windowMs: 2*1000, // 20 req/sec
  max: TheRateLimit,
  message:  '{ "Status": "Error", "Error": "RateLimited" }',
  keyGenerator: function (req /*, res*/) {
    return req.headers['CF-Connecting-IP'] || req.headers['x-forwarded-for'] || req.socket.remoteAddress || req.ip;
  },
  onLimitReached: function (req, res, options) {
    let ip = req.headers['CF-Connecting-IP'] || req.headers['x-forwarded-for'] || req.socket.remoteAddress || req.ip;
    log("RateLimit Reached("  + ip + ") you may be under a DDoS Attack or you may need to increase your request limit");
  },
  skip: function (req, res) {
    let ip = req.headers['CF-Connecting-IP'] || req.headers['x-forwarded-for'] || req.socket.remoteAddress || req.ip;
    return (global.config.RateLimitWhiteList !== undefined && ip !== undefined && ip !== null && isArray(global.config.RateLimitWhiteList) && (global.config.RateLimitWhiteList.find(element => element === ip) === ip));
  }
});

// apply rate limiter to all requests
router.use(limiter);

try {
    if (global.config.Discord?.Bot_Token !== "" && global.config.Discord?.Bot_Token !== undefined){
        client.login(global.config.Discord.Bot_Token);
        log("Logging in to discord bot");
    } else {
        log("Discord Bot Token not present you will not be able to use any discord functions", "warn");
        global.DISCORDSTATUS = "Disabled";
    }
} catch (e){
    log("Discord Bot Token is invalid", "warn");
    log(e, "warn");
}

client.on('ready', () => {
    global.DISCORDSTATUS = "Enabled";
    let tag = "NULL";
    if (client && client.user){
        tag = client.user.tag;
    }
    log(`Discord Intergration Ready and is Logged in as ${tag}!`);
  });

if (!existsSync(global.SAVEPATH + 'templates')) mkdirSync(global.SAVEPATH + 'templates');

let LoginTemplate;
//User Facing Code
function LoadLoginTemplate(){
    try{
        LoginTemplate = readFileSync(global.SAVEPATH + "templates/discordLogin.ejs","utf8");
    } catch (e) {
        log("Login Template Missing Creating It Now - " + e);
        LoginTemplate = DefaultTemplates.Login;
        writeFileSync(global.SAVEPATH + "templates/discordLogin.ejs", LoginTemplate);
    }
    try {
        let error = ejsLint(LoginTemplate) ;
        if (error !== undefined){
            LoginTemplate = DefaultTemplates.Login;
            log("============ ERROR IN LOGIN TEMPLATE ================", "warn");
            log(error, "warn");
            log("=====================================================", "warn");
        }
    } catch (e) {
        //console.log(e);
    }
}
LoadLoginTemplate();
let SuccessTemplate;
function LoadSuccessTemplate(){
    try{
        SuccessTemplate = readFileSync(global.SAVEPATH + "templates/discordSuccess.ejs","utf8");
    } catch (e) {
        log("Success Template Missing Creating It Now - " + e);
        SuccessTemplate = DefaultTemplates.Success;
        writeFileSync(global.SAVEPATH + "templates/discordSuccess.ejs", SuccessTemplate);
    }
    try {
        let error = ejsLint(SuccessTemplate) ;
        if (error !== undefined){
            LoginTemplate = DefaultTemplates.Success;
        
            log("=========== ERROR IN SUCCESS TEMPLATE ===============", "warn");
            log(error, "warn");
            log("=====================================================", "warn");
        }
    } catch (e) {
        //console.log(e);
    }
}
LoadSuccessTemplate();
let ErrorTemplate;
function LoadErrorTemplate(){
    try{
        ErrorTemplate = readFileSync(global.SAVEPATH + "templates/discordError.ejs","utf8");
    } catch (e) {
        log("Error Template Missing Creating It Now - " + e);
        ErrorTemplate = DefaultTemplates.Error;
        writeFileSync(global.SAVEPATH + "templates/discordError.ejs", ErrorTemplate);
    }
    try {
        let error = ejsLint(ErrorTemplate) ;
        if (error !== undefined){
            ErrorTemplate = DefaultTemplates.Error;
        
            log("============ ERROR IN ERROR TEMPLATE ================", "warn")
            log(error, "warn")
            log("=====================================================", "warn")
        }
    } catch (e) {
        //console.log(e);
    }

}
LoadErrorTemplate();

router.get('/', (req, res) => {
    res.send(render(ErrorTemplate, {TheError: "Invalid URL", Type: "BadURL"}))
});

router.get('/callback', (req, res) => {
    HandleCallBack(req, res);
});

router.post('/AddRole/:GUID', (req, res) => {
    let GUID = NormalizeToGUID(req.params.GUID);
    AddRole(res,req,  GUID, req.headers['auth-key']);
});

router.post('/RemoveRole/:GUID', (req, res) => {
    let GUID = NormalizeToGUID(req.params.GUID);
    RemoveRole(res,req, GUID,req.headers['auth-key']);
});

router.post('/Get/:GUID', (req, res) => {
    let GUID = NormalizeToGUID(req.params.GUID);
    GetRoles(res,req, GUID, req.headers['auth-key']);
});

router.post('/GetWithPlainId/:ID', (req, res) => {
    let GUID = NormalizeToGUID(req.params.ID);
    GetRoles(res,req, GUID, req.headers['auth-key']);
});

router.post('/Channel/Create', (req, res) => {
    CreateChannel(res, req, req.headers['auth-key']);
});

router.post('/Channel/Delete/:id', (req, res) => {
    DeleteChannel(res, req, req.params.id, req.headers['auth-key']);
});

router.post('/Channel/Edit/:id', (req, res) => {
    EditChannel(res, req, req.params.id, req.headers['auth-key']);
});

router.post('/Channel/Invite/:id', (req, res) => {
    InviteChannel(res, req, req.params.id, req.headers['auth-key']);
});

router.post('/Channel/Send/:id', (req, res) => {
    SendMessageChannel(res, req, req.params.id, req.headers['auth-key']);
});

router.post('/Send/:GUID', (req, res) => {
    let GUID = NormalizeToGUID(req.params.GUID);
    SendMessageUser(res, req, GUID, req.headers['auth-key']);
});

router.post('/Channel/Messages/:id', (req, res) => {
    GetMessagesChannel(res, req, req.params.id, req.headers['auth-key']);
});

router.post('/Check/:ID/', (req, res) => {
    let GUID = NormalizeToGUID(req.params.ID);
    CheckId(res,req, req.params.ID, GUID);
});

//TO REMOVE
router.post('/AddRole/:GUID/:auth', (req, res) => {
    let GUID = NormalizeToGUID(req.params.GUID);
    AddRole(res,req,  GUID, req.params.auth);
});

router.post('/RemoveRole/:GUID/:auth', (req, res) => {
    let GUID = NormalizeToGUID(req.params.GUID);
    RemoveRole(res,req, GUID, req.params.auth);
});

router.post('/Get/:GUID/:auth', (req, res) => {
    let GUID = NormalizeToGUID(req.params.GUID);
    GetRoles(res,req, GUID, req.params.auth);
});

router.post('/GetWithPlainId/:ID/:auth', (req, res) => {
    let GUID = NormalizeToGUID(req.params.ID);
    GetRoles(res,req, GUID, req.params.auth);
});

router.post('/Channel/Create/:auth', (req, res) => {
    CreateChannel(res, req, req.params.auth);
});

router.post('/Channel/Delete/:id/:auth', (req, res) => {
    DeleteChannel(res, req, req.params.id, req.params.auth);
});

router.post('/Channel/Edit/:id/:auth', (req, res) => {
    EditChannel(res, req, req.params.id, req.params.auth);
});

router.post('/Channel/Invite/:id/:auth', (req, res) => {
    InviteChannel(res, req, req.params.id, req.params.auth);
});

router.post('/Channel/Send/:id/:auth', (req, res) => {
    SendMessageChannel(res, req, req.params.id, req.params.auth);
});

router.post('/Channel/Messages/:id/:auth', (req, res) => {
    GetMessagesChannel(res, req, req.params.id, req.params.auth);
});
router.get('/:id', (req, res) => {
    if (LoginTemplate === undefined) LoadLoginTemplate();
    if (ErrorTemplate === undefined) LoadErrorTemplate();
    let id = req.params.id;
    if ( global.config.Discord.Client_Id === "" || global.config.Discord.Client_Secret === ""  || global.config.Discord.Bot_Token === ""  || global.config.Discord.Guild_Id === "" || global.config.Discord.Client_Id === undefined || global.config.Discord.Client_Secret === undefined  || global.config.Discord.Bot_Token === undefined  || global.config.Discord.Guild_Id === undefined )
        res.send(render(ErrorTemplate, {TheError: "Discord Intergration is not setup for this server", Type: "NotSetup"}));
    else if (id.match(/[1-9][0-9]{16,16}/)) 
        res.send(render(LoginTemplate, {SteamId: id, Login_URL: `/discord/login/${id}`}));
    else
        res.send(render(ErrorTemplate, {TheError: "Invalid URL", Type: "BadURL"}));
});

router.get('/login/:id', (req, res) => {
   RenderLogin(req, res);
});


async function RenderLogin(req, res){
    if (ErrorTemplate === undefined) LoadErrorTemplate();
    let id = req.params.id;
    let ip = req.headers['CF-Connecting-IP'] ||  req.headers['x-forwarded-for'] || req.connection.remoteAddress;
    

    let url = encodeURIComponent(`https://${req.headers.host}/discord/callback`); 
    if ( global.config.Discord.Client_Id === "" || global.config.Discord.Client_Secret === ""  || global.config.Discord.Bot_Token === ""  || global.config.Discord.Guild_Id === "" || global.config.Discord.Client_Id === undefined || global.config.Discord.Client_Secret === undefined  || global.config.Discord.Bot_Token === undefined  || global.config.Discord.Guild_Id === undefined ){
        log("User tried to sign up for discord but Intergration is not setup for this server", "warn");
        return res.send(render(ErrorTemplate, {TheError: "Discord Intergration is not setup for this server", Type: "NotSetup"}))
    }
    if (global.config.Discord?.Client_Id === undefined && !id.match(/[1-9][0-9]{16,16}/)){
        log("User tried to sign up for discord but steam id isn't a valid id", "warn");
        return res.send(render(ErrorTemplate, {TheError: "Invalid URL", Type: "BadURL"}))
    }
    if (global.config.Discord.Restrict_Sign_Up === undefined || !global.config.Discord.Restrict_Sign_Up){
        return res.redirect(`https://discordapp.com/api/oauth2/authorize?client_id=${global.config.Discord.Client_Id}&scope=identify&response_type=code&redirect_uri=${url}&state=${id}`);
    }
    if (ip === undefined){
        log("User tried to sign up for discord but ip isn't valid", "warn");
        return res.send(render(ErrorTemplate, {TheError: "Invalid URL", Type: "BadURL"}));
    }
    let responsejson;
    try {
        responsejson = await fetch(`http://ip-api.com/json/${ip}?fields=status,message,countryCode,country,regionName,isp,org,as,proxy,hosting,query`).then((response)=>response.json())
        if (responsejson.status === "success" && !responsejson.proxy && !responsejson.hosting){
            if (global.config.Discord.Restrict_Sign_Up_Countries !== undefined && global.config.Discord.Restrict_Sign_Up_Countries[0] !== undefined){
                let found = (global.config.Discord.Restrict_Sign_Up_Countries.find(element => element == responsejson.countryCode));
                if (global.config.Discord.Restrict_Sign_Up_Countries[0] === 'blacklist' && !found){
                    log(`User signed up under restictive mode - ${id} - ${responsejson.query} - ${responsejson.countryCode} - ${responsejson.regionName} - ${responsejson.isp}`, "warn");
                   return res.redirect(`https://discordapp.com/api/oauth2/authorize?client_id=${global.config.Discord.Client_Id}&scope=identify&response_type=code&redirect_uri=${url}&state=${id}`);
                }
                if (found){
                   log(`User signed up under restictive mode - ${id} - ${responsejson.query} - ${responsejson.countryCode} - ${responsejson.regionName} - ${responsejson.isp}`, "warn");
                    return res.redirect(`https://discordapp.com/api/oauth2/authorize?client_id=${global.config.Discord.Client_Id}&scope=identify&response_type=code&redirect_uri=${url}&state=${id}`);
                }
            }
            if (responsejson.status === "success" ){
               log(`User signed up under restictive mode - ${id} - ${responsejson.query} - ${responsejson.countryCode} - ${responsejson.regionName} - ${responsejson.isp}`, "warn");
               return res.redirect(`https://discordapp.com/api/oauth2/authorize?client_id=${global.config.Discord.Client_Id}&scope=identify&response_type=code&redirect_uri=${url}&state=${id}`);
           }
        }
    } catch (e) {            
        log(`User Failed Singed up under restictive mode - ${id} - ${e}`, "warn");
        return res.send(render(ErrorTemplate, {TheError: `Error Validating the signup proccess - ${e}`, Type: "ValidationError"}));
    }
    log(`User Failed Singed up under restictive mode - ${id} - ${responsejson.query} - ${responsejson.countryCode} - ${responsejson.regionName} - ${responsejson.isp}`, "warn");
    return res.send(render(ErrorTemplate, {TheError: `Error validating the signup proccess`, Type: "ValidationError"}));
}

async function HandleCallBack(req, res){
    if (ErrorTemplate === undefined) LoadErrorTemplate();
    if (SuccessTemplate === undefined) LoadSuccessTemplate();
    const code = req.query.code;
    const state = req.query.state;
    //console.log(req.query)
    if (code === undefined || code === null || state === undefined || state === null){
        log(`HandleCallBack - Invalid Response from Discord`, "warn");
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
        const json = await response.json();
        //console.log(json);
        const discordres = await fetch(`https://discordapp.com/api/users/@me`, {
            method: 'GET',
            headers: {
                Authorization: `Bearer ${json.access_token}`
            }
        });
        let discordjson = await discordres.json();
        //console.log(discordjson)
        discordjson.steamid = state;
        let guild = await client.guilds.fetch(global.config.Discord.Guild_Id);
        let msg = "Unknown Error";
        let errType = "System";
        let player;
        try {
            player = await guild.members.fetch(discordjson.id);
        } catch (e) {
            log(e, "warn")
            msg = "User not found in discord";
            errType = "UserNotFound";
        }
        //let suser = server.member(user);
        //console.log( player )
        //console.log(suser.roles)
        if(player !== undefined && player.roles !== undefined){
            let roles = player._roles
            //console.log(roles)
            msg = "User is missing the role";
            errType = "RoleRequired";
            if (global.config.Discord.BlackList_Role !== "" && global.config.Discord.BlackList_Role !== undefined && roles.find(element => element === global.config.Discord.BlackList_Role) !== undefined){
                msg = "You have a blacklisted role";
                errType = "Blacklisted";
            } else if (global.config.Discord.Required_Role === "" || global.config.Discord.Required_Role === undefined || roles.find(element => element === global.config.Discord.Required_Role) !== undefined){
                msg = "Success";
            } 
        }

        //console.log(msg)
        if (msg === "Success"){
            discordjson.avatar = `https://cdn.discordapp.com/avatars/${discordjson.id}/${discordjson.avatar}`
            //console.log(discordjson)           
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
            if ((await results.count()) == 0){
                query = { GUID: guid };
                const options = { upsert: true };
                const updateDoc = { $set: data, };
                const result = await collection.updateOne(query, updateDoc, options);

                if (result.result.ok == 1){
                    log("Player: " + guid + " Connected to Discord ID: " + discordjson.id);
                    res.send(render(SuccessTemplate, {DiscordId: discordjson.id, DiscordUsername: discordjson.username, DiscordAvatar: discordjson.avatar, DiscordDiscriminator: discordjson.discriminator, SteamId: discordjson.steamid}))
                } else {
                    log("Error when trying to link player: " + guid + " to discord ID: " + discordjson.id, "warn");
                    res.send(render(ErrorTemplate, {TheError: "There was an error linking your discord account", Type: "Database"}));
                }
            } else {
                let dataarr = await results.toArray(); 
                let querydata = dataarr[0]; 
                if ( guid === querydata.GUID){
                    log(`Player: ${guid} try to link to discord ID: ${discordjson.id} already in use with ${querydata.GUID}` , "warn");
                    res.send( render(ErrorTemplate, {TheError: "It seems you already have your discord account Linked", Type: "AlreadyLinked"} ) );
                } else {
                    log(`Player: ${guid} try to link to discord ID: ${discordjson.id} already in use with ${querydata.GUID}` , "warn");
                    res.send( render(ErrorTemplate, { TheError: "You already have your discord linked to another account", Type: "Conflict"} ) );
                }
            }
        } else {
            res.send(render(ErrorTemplate, {TheError: msg, Type: errType}));
        }
    } catch (e){
        log(e, "warn")
        try {
            res.send(render(ErrorTemplate, {TheError: e, Type: "System"}));
        } catch(err){
            log(err, "warn");
            res.send(`<html><head><title>Invalid Link</title></head><body><h1>Error: Invalid Error Templates</h1></body></html>`);
        }
    } finally {
        // Ensures that the client will close when you finish/error
        mongo.close();
    }
}



//API Call Functions

async function AddRole(res, req, GUID, auth){
    if (CheckServerAuth(auth) || (await CheckPlayerAuth(GUID, auth) && global.config.AllowClientWrite)){
        const mongo = new MongoClient(global.config.DBServer, { useUnifiedTopology: true });
        try{
            await mongo.connect();
            // Connect the client to the server
            const db = mongo.db(global.config.DB);
            let collection = db.collection("Players");
            let query = { GUID: GUID };
            let results = collection.find(query);
            let RawData = req.body;
            let Role = RawData.Role;
            if ((await results.count()) == 0){
                log("Error: Discord AddRole Can't find Player with ID " + GUID, "warn");
                res.status(201);
                res.json({Status: "NotFound", Error: `Player with ${GUID} Not Found`, Roles: [], id: "0", Username: "", Discriminator: "", Avatar: "" });
            } else {
                let dataarr = await results.toArray(); 
                let data = dataarr[0]; 
                let resObj;
                if (data.Discord === undefined || data.Discord === {} || data.Discord.id === undefined || data.Discord.id === "" || data.Discord.id === "0" ){
                    log(`Error: Discord AddRole User(${GUID}) doesn't have discord set up`, "warn");
                    resObj = {Status: "NotSetup", Error: `Player Doesn't have discord set up`, Roles: [], id: "0", Username: "", Discriminator: "", Avatar: "" };
                } else {
                    resObj = {Status: "Error", Error: `Couldn't connect to discord`, Roles: [], id: "0", Username: "", Discriminator: "", Avatar: "" }
                    let guild = await client.guilds.fetch(global.config.Discord.Guild_Id);
                    try {
                        let player = await guild.members.fetch(data.Discord.id);
                        let roles = player._roles;
                        resObj = { Status: "Success", Error: "", Roles: roles, id: data.Discord.id, Username: data.Discord.username, Discriminator: data.Discord.discriminator, Avatar: data.Discord.avatar };
                        
                        if (player.roles.cache.has(Role)) {
                            log(`Warning: Discord AddRole User(${GUID}) Already Has Role ${Role}`);
                            resObj.Error = "Already Has Role";
                        } else {
                            let role = await guild.roles.fetch(Role);
                            await player.roles.add(role).catch((e) => log(`Error: ${e}`));
                            resObj.Roles.push(Role);
                            log(`Discord AddRole User(${GUID}) Added ${Role}`);
                        }
                    } catch (e) {
                        log(`Error: Discord AddRole User(${GUID}) not found in discord`, "warn");
                        resObj.Error = "User not found in discord";
                        resObj.Status = "NotFound";
                    }
                }
                res.status(202);
                res.json(resObj);
            }
        }catch(err){
            console.log(err);
            res.status(203);
            res.json({Status: "Error", Error: `${err}`, Roles: [], id: "0", Username: "", Discriminator: "", Avatar: "" });
            log("ERROR: " + err, "warn");
        }finally{
            // Ensures that the client will close when you finish/error
            mongo.close();
        }
    } else {
        res.status(401);
        res.json({Status: "NoAuth", Error: `Invalid Auth Key`, Roles: [], id: "0", Username: "", Discriminator: "", Avatar: "" });
        log("AUTH ERROR: " + req.url, "warn");
    }
}

async function RemoveRole(res, req, GUID, auth){
    if (CheckServerAuth(auth) || (await CheckPlayerAuth(GUID, auth) && global.config.AllowClientWrite)){
        const mongo = new MongoClient(global.config.DBServer, { useUnifiedTopology: true });
        try{
            await mongo.connect();
            // Connect the client to the server
            const db = mongo.db(global.config.DB);
            let collection = db.collection("Players");
            let query = { GUID: GUID };
            let results = collection.find(query);
            let RawData = req.body;
            let Role = RawData.Role;
            if ((await results.count()) == 0){
                log("ERROR: Discord RemoveRole Can't find Player with ID " + GUID, "warn");
                res.status(201);
                res.json({Status: "Error", Error: `Player with ${GUID} Not Found`, Roles: [], id: "0", Username: "", Discriminator: "", Avatar: "" });
            } else {
                let dataarr = await results.toArray(); 
                let data = dataarr[0]; 
                let resObj;
                if (data.Discord === undefined || data.Discord === {} || data.Discord.id === undefined || data.Discord.id === "" || data.Discord.id === "0" ){
                    log(`ERROR: Discord RemoveRole User(${GUID}) Doesn't have discord set up`);
                    resObj = {Status: "NotSetup", Error: `Player Doesn't have discord set up`, Roles: [], id: "0", Username: "", Discriminator: "", Avatar: "" };
                } else {
                    resObj = {Status: "Error", Error: `Couldn't connect to discord`, Roles: [], id: "0", Username: "", Discriminator: "", Avatar: "" };
                    let guild = await client.guilds.fetch(global.config.Discord.Guild_Id);
                    try {
                        let player = await guild.members.fetch(data.Discord.id);
                        let roles = player._roles;
                        resObj = { Status: "Success", Error: "", Roles: roles, id: data.Discord.id, Username: data.Discord.username, Discriminator: data.Discord.discriminator, Avatar: data.Discord.avatar };
                        
                        if (player.roles.cache.has(Role)) {
                            let role = await guild.roles.fetch(Role);
                            await player.roles.remove(role).catch((e) => log(`Error: ${e}`));
                            resObj.Roles = resObj.Roles.filter(item => item !== Role)
                        } else {
                            resObj.Error = "Already didn't have Role";
                        }
                    } catch (e) {
                            resObj.Error = "User not found in discord";
                            resObj.Status = "NotFound";
                    }
                }
                res.status(202);
                res.json(resObj);
            }
        }catch(err){
            console.log(err);
            res.status(203);
            res.json({Status: "Error", Error: `${err}`, Roles: [], id: "0", Username: "", Discriminator: "", Avatar: "" });
            log("ERROR: " + err, "warn");
        }finally{
            // Ensures that the client will close when you finish/error
            mongo.close();
        }
    } else {
        res.status(401);
        res.json({Status: "NoAuth", Error: `Invalid Auth Key`, Roles: [], id: "0", Username: "", Discriminator: "", Avatar: "" });
        log("AUTH ERROR: " + req.url, "warn");
    }

}

async function GetRoles(res, req, GUID, auth){
    if (CheckServerAuth(auth) || (await CheckPlayerAuth(GUID, auth))){
        const mongo = new MongoClient(global.config.DBServer, { useUnifiedTopology: true });
        try{
            await mongo.connect();
            // Connect the client to the server
            const db = mongo.db(global.config.DB);
            let collection = db.collection("Players");
            let query = { GUID: GUID };
            let results = collection.find(query);
            if ((await results.count()) == 0){
                log("Can't find Player with ID " + GUID, "warn");
                res.status(201);
                res.json({Status: "Error", Error: `Player with ${GUID} Not Found`, Roles: [], id: "0", Username: "", Discriminator: "", Avatar: "" });
            } else {
                let dataarr = await results.toArray(); 
                let data = dataarr[0].Discord; 
                let resObj;
                if (data === undefined || data.id === undefined || data.id === "" || data.id === "0" ){
                    resObj = {Status: "NotSetup", Error: `Player Doesn't have discord set up`, Roles: [], id: "0", Username: "", Discriminator: "", Avatar: "" };
                } else {                        
                    resObj = { Status: "Error", Error: "Couldn't connect to discord", Roles: [], id: data.id, Username: data.username, Discriminator: data.discriminator, Avatar: data.avatar };
                    let guild = await client.guilds.fetch(global.config.Discord.Guild_Id);
                    try {
                        let player = await guild.members.fetch(data.id);
                        resObj.Status = "Success"
                        resObj.Error = "";
                        resObj.Roles = player._roles || [];
                    
                        log(`Succefully found discord ID and Roles for ${GUID}`);
                    } catch (e) {
                        log(`Found Discord ID but not roles for ${GUID} `);
                        resObj.Error = "User not found in discord";
                        resObj.Status = "NotFound";
                    }
                }
                res.status(200);
                res.json(resObj);
            }
        }catch(err){
            res.status(203);
            res.json({Status: "Error", Error: `${err}`, Roles: [], id: "0", Username: "", Discriminator: "", Avatar: "" });
            log("ERROR: " + err, "warn");
        }finally{
            // Ensures that the client will close when you finish/error
            mongo.close();
        }
    } else {
        log("AUTH ERROR: " + req.url, "warn");
        res.status(401);
        res.json({Status: "Error", Error: `Invalid Auth Key`, Roles: [], id: "0", Username: "", Discriminator: "", Avatar: "" });
    }

}



async function SendMessageUser(res, req, guid, auth){
    if (CheckServerAuth(auth) || (await CheckPlayerAuth(guid, auth))){
        let RawData = req.body; 
        let guild = client.guilds.fetch(global.config.Discord.Guild_Id);
        let message = RawData.Message;
        let userObj = await GetDiscordObj(guid);
        guild = await guild;
        if (userObj !== undefined && userObj.id !== "0" && userObj.id !== ""){
            try {
                let user = await guild.members.fetch(userObj.id);
                let result = await user.send(message);
                log(`Successfully sent Discord Direct Message to ${guid}`);
                res.status(200);
                res.json({Status: "Success", Error: "", oid: result.id});
            } catch(e) {
                let error = `${e}`;
                if (error === `DiscordAPIError: Cannot send messages to this user`){
                    log(`Error sending message to ${guid} they may block dms - ${e}`,"warn");
                    res.status(200);
                    res.json({Status: "Error", Error: `Cannot send messages to this user, they may have dm's blocked`, oid: "" });
                } else {
                    log(`Error sending message to ${guid} - ${e}`,"warn");
                    res.status(500);
                    res.json({Status: "Error", Error: `${e}`, oid: "" });
                }
            }
        } else {
            log(`Failed to send Discord Direct Message to ${guid} user not configured`);
            res.status(200);
            res.json({Status: "NotFound", Error: "Discord User Found", oid: "" });
        }
    } else {
        res.status(401);
        res.json({Status: "Error", Error: `Invalid Auth`, oid: ""});
        log("AUTH ERROR: " + req.url, "warn");
    }
}


async function CreateChannel(res, req, auth){
    if (CheckServerAuth(auth) || (await CheckAuth(auth) && global.config.AllowClientWrite)){
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
            log("AUTH ERROR: " + req.url, "warn");

        }
    } else {
        res.status(401);
        res.json({Status: "Error", Error: `Invalid Auth`, oid: "0"});
        log("AUTH ERROR: " + req.url, "warn");
    }
}


async function DeleteChannel(res, req, id, auth){
    if (CheckServerAuth(auth) || (await CheckAuth(auth) && global.config.AllowClientWrite)){
        try{
            let RawData = req.body; 
            let guild = await client.guilds.fetch(global.config.Discord.Guild_Id);
            let reason = RawData.Reason;
            try {
                let channel = await guild.channels.cache.get(id);
                if (channel){
                    channel.delete(reason).then(()=>{
                        log(`Deleted channel ${id} for ${reason}`);
                        res.status(200);
                        res.json({Status: "Success", Error: ``, oid: `${id}`});
                    }).catch((e)=>{
                        log(`Error Deleteing channel ${id} for ${reason} Error - ${e}`, "warn");
                        res.status(200);
                        res.json({Status: "Error", Error: `${e}`, oid: `${id}`});
                    })
                } else {
                    log(`Cloudn't delete channel ${id} for ${reason} as it does not exsit`, "warn");
                    res.status(200);
                    res.json({Status: "NotFound", Error: ``, oid: `${id}`});
                }
            } catch (e){
                res.status(200);
                res.json({Status: "NotFound", Error: `${e}`, oid: `${id}`});
                log(`Error Deleteing channel ${id} for ${reason} Error - ${e}`, "warn");
            }
        } catch(e){
            res.status(400);
            res.json({Status: "Error", Error: `${e}`, oid: "0"});
            log(`Error Deleteing channel ${id} for ${reason} Error - ${e}`, "warn");
        }
    } else {
        res.status(401);
        res.json({Status: "Error", Error: `Invalid Auth`, oid: "0"});
        log("AUTH ERROR: " + req.url, "warn");
    }
}

async function EditChannel(res, req, id, auth){
    if (CheckServerAuth(auth) || (await CheckAuth(auth) && global.config.AllowClientWrite)){
        try{
            let RawData = req.body; 
            let guild = await client.guilds.fetch(global.config.Discord.Guild_Id);
            let reason = RawData.Reason;
            let options = RawData.Options;
            try {
                let channel = await guild.channels.cache.get(id);
                if (channel){
                    channel.edit(options, reason).then(()=>{
                        log(`Edited channel ${id} for ${reason}`);
                        res.status(200);
                        res.json({Status: "Success", Error: ``, oid: `${id}`});
                    }).catch((e)=>{
                        log(`Error editing channel ${id} for ${reason} Error - ${e}`, "warn");
                        res.status(200);
                        res.json({Status: "Error", Error: `${e}`, oid: `${id}`});
                    })
                } else {
                    log(`Cloudn't edit channel ${id} for ${reason} as it does not exsit`, "warn");
                    res.status(200);
                    res.json({Status: "NotFound", Error: ``, oid: `${id}`});
                }
            } catch (e){
                res.status(200);
                res.json({Status: "NotFound", Error: `${e}`, oid: `${id}`});
                log(`Error editing channel ${id} for ${reason} Error - ${e}`, "warn");
            }
        } catch(e){
            res.status(400);
            res.json({Status: "Error", Error: `${e}`, oid: "0"});
            log(`Error editing channel ${id} for ${reason} Error - ${e}`, "warn");
        }
    } else {
        res.status(401);
        res.json({Status: "Error", Error: `Invalid Auth`, oid: "0"});
        log("AUTH ERROR: " + req.url, "warn");
    }
}

async function SendMessageChannel(res, req, id, auth){
    let isServerAuth = CheckServerAuth(auth);
    let isClientAuth = false;
    let GUID = "";
    let did = "";
    if (!isServerAuth){
        isClientAuth = (await CheckAuth(auth));
       // console.log(isClientAuth)
        if (isClientAuth){
            GUID = AuthPlayerGuid(auth);
            //console.log(GUID)
            isClientAuth = (CheckPlayerAuth(GUID, auth));
            did = GetDiscordObj(GUID);
            isClientAuth = await isClientAuth;
            //console.log(isClientAuth)
            did = (await GetDiscordObj(GUID)).id;
            //console.log(did)
        }
    }
    if ( isServerAuth || isClientAuth){
        try{
            let RawData = req.body; 
            let guild = await client.guilds.fetch(global.config.Discord.Guild_Id);
            let cid = id;
            let message = {embed: RawData};
            if (RawData.Message !== undefined)
                message = RawData.Message;
            
            try {
                let channel = await guild.channels.cache.get(cid);
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
                        log(`Sent Message to channel ${cid} id: ${msg.id}`);
                        res.status(200);
                        res.json({Status: "Success", Error: ``, oid: `${msg.id}`});
                    } else {
                        log(`Error couldn't send message to channel ${cid} as user ${GUID} doesn't have permissions to send messagaes in the channel`, "warn");
                        res.status(200);
                        res.json({Status: "NoPerms", Error: `User does not have permissions to use this channel`, oid: `0`});
                    }
                } else {
                    log(`Error couldn't send message to channel ${cid} as it does not exsit`, "warn");
                    res.status(200);
                    res.json({Status: "NotFound", Error: `Channel not found in discord`, oid: `0`});
                }
            } catch (e){
                res.status(200);
                res.json({Status: "NotFound", Error: `${e}`, oid: `0`});
                log(`Error couldn't send message to channel ${id} Error - ${e}`, "warn");
            }
        } catch(e){
            res.status(400);
            res.json({Status: "Error", Error: `${e}`, oid: "0"});
            log(`Error couldn't send message to channel  ${id}  Error - ${e}`, "warn");
        }
    } else {
        res.status(401);
        res.json({Status: "Error", Error: `Invalid Auth`, oid: "0"});
        log("AUTH ERROR: " + req.url, "warn");
    }
}


async function GetMessagesChannel(res, req, id, auth){
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
                        perms = (channel.permissionsFor(user).has('VIEW_CHANNEL') && channel.permissionsFor(user).has('READ_MESSAGE_HISTORY')); 
                    }
                    if (isServerAuth || (isClientAuth && perms)){
                        let messages = (await channel.messages.fetch(filter)).array();
                        //console.log(messages);
                        let msgs = messages.map(obj => ({id: obj.id, AuthorId: obj.author.id, Embed: obj.embeds[0], Content: obj.content, ChannelId: obj.channel.id, TimeStamp: obj.createdTimestamp}));
                        res.status(200);
                        res.json({Status: "Success", Error: ``, Messages: msgs});
                    } else {
                        log(`Error couldn't get messages from channel ${cid} as user ${GUID} doesn't have permissions to read message history in the channel`, "warn");
                        res.status(200);
                        res.json({Status: "NoPerms", Error: `User does not have permissions to use this channel`, Messages: []});
                    }
                } else {
                    log(`Error couldn't get messages from channel ${cid} as it does not exsit`, "warn");
                    res.status(200);
                    res.json({Status: "NotFound", Error: `Channel not found in discord`, Messages: []});
                }
            } catch (e){
                res.status(200);
                res.json({Status: "NotFound", Error: `${e}`, Messages: []});
                log(`Error couldn't get messages from channel ${id} Error - ${e}`);
            }
        } catch(e){
            res.status(400);
            res.json({Status: "Error", Error: `${e}`, Messages: []});
            log(`Error couldn't get messages from channel  ${id}  Error - ${e}`);
        }
    } else {
        res.status(401);
        res.json({Status: "Error", Error: `Invalid Auth Key`, Messages: []});
        log("AUTH ERROR: " + req.url, "warn");
    }
}

async function CheckId(res, req, id, guid){
    const mongo = new MongoClient(global.config.DBServer, { useUnifiedTopology: true });
        try{
            await mongo.connect();
            // Connect the client to the server        
            const db = mongo.db(global.config.DB);
            let collection = db.collection("Players");
            let query = { GUID: guid };
            let results = collection.find(query);
            let Logcollection = db.collection("Logs");
            let datetime = new Date();
            let ClientId = GetClientID(req);
            let logobj = { Log: "DiscordStatusCheck", TimeStamp: datetime, GUID: guid, SteamId: id, ClientId: ClientId, Status: "Error" }
            if ((await results.count()) > 0){
                let dataarr = await results.toArray(); 
                let data = dataarr[0]; 
                if (data.Discord?.id !== undefined){
                    res.status(200);
                    logobj.Status = "Success";
                    logobj.Discord = data.Discord.id;
                    res.json({Status: "Success", Error: "" });
                } else {
                    res.status(200);
                    logobj.Status = "NotFound";
                    res.json({Status: "NotFound", Error: "Discord could not be found for user"  });
                }
            } else {
                logobj.Status = "NotFound";
                res.status(200);
                res.json({Status: "NotFound", Error: "No User Found"  });
            }
            //await Logcollection.insertOne(logobj);
            log(`Check status for user: ${guid} - ${datetime.toUTCString()} - ${logobj.Status}`)
        } catch(err){
            log("Error Checking for ID " + guid + " err" + err, "warn");
            res.status(400);
            res.json({Status: "Error", Error: err });
        } finally{
            await mongo.close();
        }
}

async function GetDiscordObj(guid){
    const mongo = new MongoClient(global.config.DBServer, { useUnifiedTopology: true });
    let obj = {
        id:"0",
        Username: "", 
        Discriminator: "", 
        Avatar: ""
    }
    try{
        await mongo.connect();
        // Connect the client to the server
        const db = mongo.db(global.config.DB);
        let collection = db.collection("Players");
        let query = { GUID: guid };
        let results = collection.find(query);
        if ((await results.count()) == 0){
            log("Can't find Player with ID " + guid, "warn"); 
        } else {
            let dataarr = await results.toArray(); 
            let data = dataarr[0]; 
            if (data.Discord === undefined || data.Discord === {} || data.Discord.id === undefined || data.Discord.id === "" || data.Discord.id === "0" ){
            } else {
                obj = data.Discord;
            }
        }
    }catch(err){
        log(`Error Fetching Discord Obj ${err}`, "warn");
    }finally{
        // Ensures that the client will close when you finish/error
        mongo.close();
        return obj;
    }

}

function GetClientID(req){
    let ip = req.headers['CF-Connecting-IP'] || req.headers['x-forwarded-for'] || req.socket.remoteAddress || req.ip;
    let  hash = createHash('sha256');
    let theHash = hash.update(ip).digest('base64');
    return theHash.substr(0,32); //Cutting the last few digets to save a bit of data and make sure people don't mistake it for the GUIDS
}

module.exports = router;