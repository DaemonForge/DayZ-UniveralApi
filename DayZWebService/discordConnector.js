const {Router, response} = require('express');
const { MongoClient } = require("mongodb");
const {createHash} = require('crypto');
const {readFileSync, writeFileSync, existsSync, mkdirSync} = require('fs');
const {render} = require('ejs');
const log = require("./log");
const config = require('./configLoader');
const {Client, GuildMember, Guild} = require("discord.js");
const client = new Client();
const fetch = require('node-fetch');
const DefaultTemplates = require("./templates/defaultTemplates.json");
const ejsLint = require('ejs-lint');
const CheckAuth = require("./AuthChecker");
const router = Router();
try {
    if (config.Discord_Bot_Token !== "" && config.Discord_Bot_Token !== undefined){
        client.login(config.Discord_Bot_Token);
        console.log("Logging in to discord bot");
    } else {
        log("Discord Bot Token not present you will not be able to use any discord functions");
    }
} catch (e){
    log("Discord Bot Token is invalid", "warn");
    log(e, "warn");
}


if (!existsSync('templates')) mkdirSync('templates');

let LoginTemplate;
//User Facing Code
function LoadLoginTemplate(){
    try{
        LoginTemplate = readFileSync("./templates/discordLogin.ejs","utf8");
    } catch (e) {
        log("Login Template Missing Creating It Now - " + e);
        LoginTemplate = DefaultTemplates.Login;
        writeFileSync("./templates/discordLogin.ejs", LoginTemplate);
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
        console.log(e);
    }
}
LoadLoginTemplate();
let SuccessTemplate;
function LoadSuccessTemplate(){
    try{
        SuccessTemplate = readFileSync("./templates/discordSuccess.ejs","utf8");
    } catch (e) {
        log("Success Template Missing Creating It Now - " + e);
        SuccessTemplate = DefaultTemplates.Success;
        writeFileSync("./templates/discordSuccess.ejs", SuccessTemplate);
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
        console.log(e);
    }
}
LoadSuccessTemplate();
let ErrorTemplate;
function LoadErrorTemplate(){
    try{
        ErrorTemplate = readFileSync("./templates/discordError.ejs","utf8");
    } catch (e) {
        log("Error Template Missing Creating It Now - " + e);
        ErrorTemplate = DefaultTemplates.Error;
        writeFileSync("./templates/discordError.ejs", ErrorTemplate);
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
        console.log(e);
    }

}
LoadErrorTemplate();

router.get('/', (req, res) => {
    res.send(render(ErrorTemplate, {TheError: "Invalid URL", Type: "BadURL"}))
});

router.get('/callback', (req, res) => {
    HandleCallBack(req, res);
});

router.post('/AddRole/:GUID/:auth', (req, res) => {
    AddRole(res,req,  req.params.GUID, req.params.auth);
});

router.post('/RemoveRole/:GUID/:auth', (req, res) => {
    RemoveRole(res,req, req.params.GUID, req.params.auth);
});

router.post('/Get/:GUID/:auth', (req, res) => {
    GetRoles(res,req, req.params.GUID, req.params.auth);
});

router.post('/GetWithPlainId/:ID/:auth', (req, res) => {
    let guid = createHash('sha256').update(req.params.ID).digest('base64');
    guid = guid.replace(/\+/g, '-'); 
    guid = guid.replace(/\//g, '_');
    GetRoles(res,req, guid, req.params.auth);
});

router.post('/Check/:ID/', (req, res) => {
    let guid = createHash('sha256').update(req.params.ID).digest('base64');
    guid = guid.replace(/\+/g, '-'); 
    guid = guid.replace(/\//g, '_');
    CheckId(res,req, req.params.ID, guid);
});

router.get('/:id', (req, res) => {
    if (LoginTemplate === undefined) LoadLoginTemplate();
    if (ErrorTemplate === undefined) LoadErrorTemplate();
    let id = req.params.id;
    if ( config.Discord_Client_Id === "" || config.Discord_Client_Secret === ""  || config.Discord_Bot_Token === ""  || config.Discord_Guild_Id === "" || config.Discord_Client_Id === undefined || config.Discord_Client_Secret === undefined  || config.Discord_Bot_Token === undefined  || config.Discord_Guild_Id === undefined )
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
    if ( config.Discord_Client_Id === "" || config.Discord_Client_Secret === ""  || config.Discord_Bot_Token === ""  || config.Discord_Guild_Id === "" || config.Discord_Client_Id === undefined || config.Discord_Client_Secret === undefined  || config.Discord_Bot_Token === undefined  || config.Discord_Guild_Id === undefined ){
        return res.send(render(ErrorTemplate, {TheError: "Discord Intergration is not setup for this server", Type: "NotSetup"}))
    }
    if (config.Discord_Client_Id === undefined && !id.match(/[1-9][0-9]{16,16}/)){
        return res.send(render(ErrorTemplate, {TheError: "Invalid URL", Type: "BadURL"}))
    }
    if (config.Discord_Restrict_Sign_Up === undefined || !config.Discord_Restrict_Sign_Up){
        return res.redirect(`https://discordapp.com/api/oauth2/authorize?client_id=${config.Discord_Client_Id}&scope=identify&response_type=code&redirect_uri=${url}&state=${id}`);
    }
    if (ip === undefined){
        return res.send(render(ErrorTemplate, {TheError: "Invalid URL", Type: "BadURL"}));
    }
    let responsejson;
    try {
        responsejson = await fetch(`http://ip-api.com/json/${ip}?fields=status,message,countryCode,country,regionName,isp,org,as,proxy,hosting,query`).then((response)=>response.json())
        if (responsejson.status === "success" && !responsejson.proxy && !responsejson.hosting){
            if (config.Discord_Restrict_Sign_Up_Countries !== undefined && config.Discord_Restrict_Sign_Up_Countries[0] !== undefined){
                let found = (config.Discord_Restrict_Sign_Up_Countries.find(element => element == responsejson.countryCode));
                if (config.Discord_Restrict_Sign_Up_Countries[0] === 'blacklist' && !found){
                    log(`User signed up under restictive mode - ${id} - ${responsejson.query} - ${responsejson.countryCode} - ${responsejson.regionName} - ${responsejson.isp}`);
                   return res.redirect(`https://discordapp.com/api/oauth2/authorize?client_id=${config.Discord_Client_Id}&scope=identify&response_type=code&redirect_uri=${url}&state=${id}`);
                }
                if (found){
                   log(`User signed up under restictive mode - ${id} - ${responsejson.query} - ${responsejson.countryCode} - ${responsejson.regionName} - ${responsejson.isp}`);
                    return res.redirect(`https://discordapp.com/api/oauth2/authorize?client_id=${config.Discord_Client_Id}&scope=identify&response_type=code&redirect_uri=${url}&state=${id}`);
                }
            }
            if (responsejson.status === "success" ){
               log(`User signed up under restictive mode - ${id} - ${responsejson.query} - ${responsejson.countryCode} - ${responsejson.regionName} - ${responsejson.isp}`);
               return res.redirect(`https://discordapp.com/api/oauth2/authorize?client_id=${config.Discord_Client_Id}&scope=identify&response_type=code&redirect_uri=${url}&state=${id}`);
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
        res.send(render(ErrorTemplate, {TheError: "Invalid Response from Discord", Type: "Discord"}));
        return;
    }
    const mongo = new MongoClient(config.DBServer, { useUnifiedTopology: true });
    try {
        let connect = mongo.connect();
        const response = await fetch(`https://discordapp.com/api/oauth2/token`,{
            method: 'POST',
            headers: {
            },
            body: new URLSearchParams({
                client_id: config.Discord_Client_Id,
                client_secret: config.Discord_Client_Secret,
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
        let guild = await client.guilds.fetch(config.Discord_Guild_Id);
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
            if (config.Discord_BlackList_Role !== "" && config.Discord_BlackList_Role !== undefined && roles.find(element => element === config.Discord_BlackList_Role) !== undefined){
                msg = "You have a blacklisted role";
                errType = "Blacklisted";
            } else if (config.Discord_Required_Role === "" || config.Discord_Required_Role === undefined || roles.find(element => element === config.Discord_Required_Role) !== undefined){
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
            const db = mongo.db(config.DB);
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
    if ((auth === config.ServerAuth) || (await CheckPlayerAuth(GUID, auth) && config.AllowClientWrite)){
        const mongo = new MongoClient(config.DBServer, { useUnifiedTopology: true });
        try{
            await mongo.connect();
            // Connect the client to the server
            const db = mongo.db(config.DB);
            let collection = db.collection("Players");
            let query = { GUID: GUID };
            let results = collection.find(query);
            let RawData = req.body;
            let Role = RawData.Role;
            if ((await results.count()) == 0){
                log("Error: Discord AddRole Can't find Player with ID " + GUID, "warn");
                res.status(201);
                res.json({Status: "Error", Error: `Player with ${GUID} Not Found`, Roles: [], id: "0", Username: "", Discriminator: "", Avatar: "" });
            } else {
                let dataarr = await results.toArray(); 
                let data = dataarr[0]; 
                let resObj;
                if (data.Discord === undefined || data.Discord === {} || data.Discord.id === undefined || data.Discord.id === "" || data.Discord.id === "0" ){
                    log(`Error: Discord AddRole User(${GUID}) doesn't have discord set up`, "warn");
                    resObj = {Status: "NotSetup", Error: `Player Doesn't have discord set up`, Roles: [], id: "0", Username: "", Discriminator: "", Avatar: "" };
                } else {
                    resObj = {Status: "Error", Error: `Couldn't connect to discord`, Roles: [], id: "0", Username: "", Discriminator: "", Avatar: "" }
                    let guild = await client.guilds.fetch(config.Discord_Guild_Id);
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
        res.json({Status: "Error", Error: `Invalid Auth Key`, Roles: [], id: "0", Username: "", Discriminator: "", Avatar: "" });
        log("AUTH ERROR: " + req.url, "warn");
    }
}

async function RemoveRole(res, req, GUID, auth){
    if ((auth === config.ServerAuth) || (await CheckPlayerAuth(GUID, auth) && config.AllowClientWrite)){
        const mongo = new MongoClient(config.DBServer, { useUnifiedTopology: true });
        try{
            await mongo.connect();
            // Connect the client to the server
            const db = mongo.db(config.DB);
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
                    let guild = await client.guilds.fetch(config.Discord_Guild_Id);
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
        res.json({Status: "Error", Error: `Invalid Auth Key`, Roles: [], id: "0", Username: "", Discriminator: "", Avatar: "" });
        log("AUTH ERROR: " + req.url, "warn");
    }

}

async function GetRoles(res, req, GUID, auth){
    if ((auth === config.ServerAuth) || (await CheckPlayerAuth(GUID, auth))){
        const mongo = new MongoClient(config.DBServer, { useUnifiedTopology: true });
        try{
            await mongo.connect();
            // Connect the client to the server
            const db = mongo.db(config.DB);
            let collection = db.collection("Players");
            let query = { GUID: GUID };
            let results = collection.find(query);
            if ((await results.count()) == 0){
                log("Can't find Player with ID " + GUID, "warn");
                res.status(201);
                res.json({Status: "Error", Error: `Player with ${GUID} Not Found`, Roles: [], id: "0", Username: "", Discriminator: "", Avatar: "" });
            } else {
                let dataarr = await results.toArray(); 
                let data = dataarr[0]; 
                let resObj;
                if (data.Discord === undefined || data.Discord === {} || data.Discord.id === undefined || data.Discord.id === "" || data.Discord.id === "0" ){
                    resObj = {Status: "NotSetup", Error: `Player Doesn't have discord set up`, Roles: [], id: "0", Username: "", Discriminator: "", Avatar: "" };
                } else {
                    resObj = {Status: "Error", Error: `Couldn't connect to discord`, Roles: [], id: "0", Username: "", Discriminator: "", Avatar: "" };
                    let guild = await client.guilds.fetch(config.Discord_Guild_Id);
                    try {
                        let player = await guild.members.fetch(data.Discord.id);
                        let roles = player._roles;
                        resObj = { Status: "Success", Error: "", Roles: roles, id: data.Discord.id, Username: data.Discord.username, Discriminator: data.Discord.discriminator, Avatar: data.Discord.avatar };
                    
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


async function CheckPlayerAuth(guid, auth){
    let isAuth = false;
    const client = new MongoClient(config.DBServer, { useUnifiedTopology: true });
    if ((await CheckAuth(auth))){
        try{
            await client.connect();
            // Connect the client to the server        
            const db = client.db(config.DB);
            let collection = db.collection("Players");
            let SavedAuth = createHash('sha256').update(auth).digest('base64');
            let query = { GUID: guid, AUTH: SavedAuth };
            let results = collection.find(query);
                if ((await results.count()) != 0){
                    res.status(203);
                }
        } catch(err){
            log("ID " + guid + " err" + err, "warn");
        } finally{
            await client.close();
            return isAuth;
        }
    }
    return isAuth;
}

async function CheckId(res,req, id, guid){
    const client = new MongoClient(config.DBServer, { useUnifiedTopology: true });
        try{
            await client.connect();
            // Connect the client to the server        
            const db = client.db(config.DB);
            let collection = db.collection("Players");
            let query = { GUID: guid };
            let results = collection.find(query);
            if ((await results.count()) > 0){
                let dataarr = await results.toArray(); 
                let data = dataarr[0]; 
                if (data.Discord?.id !== undefined){
                    res.status(200);
                    res.json({Status: "Success", Error: "" });
                } else {
                    res.status(200);
                    res.json({Status: "NoDiscord", Error: "Discord found for user"  });
                }
            } else {
                res.status(200);
                res.json({Status: "NoUser", Error: "No User Found"  });
            }
        } catch(err){
            log("Error Checking for ID " + guid + " err" + err, "warn");
            res.status(200);
            res.json({Status: "Error", Error: err });
        } finally{
            await client.close();
        }
}


module.exports = router;


