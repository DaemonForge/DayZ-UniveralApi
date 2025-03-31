const { Router } = require('express');
const {requireServerAuth, requirePlayerOrServerAuth} = require('../auth/utils')
const {CheckIdHasRole, CheckId, AddRole, RemoveRole, GetUserAndRoles, PlayerVoiceMute, PlayerVoiceKick, ChannelVoiceMove, SendMessageUser, PlayerVoiceGetChannel, SetNicknameUser} = require('./user');
const {CreateChannel, DeleteChannel, EditChannel, InviteChannel, SendMessageChannel, GetMessagesChannel} = require('./channels');
const {GenerateLimiter, NormalizeToGUID} = require('../utils');
const {renderRootErrorTemplate, HandleCallBack,RenderLogin, GetLoginTemplate, GetErrorTemplate, SendLoginPage} = require('./login');
const {render} = require('ejs');

const {createLogger} = require('../utils');
const logger = createLogger(global.logger, 'discord');

const router = Router();

router.use((req, res, next) => {
    if(global.DISCORDSTATUS === "Error"){
        return res.json({Status: "Error", Error: "Discord Error"});
    } else if(global.DISCORDSTATUS === "Disabled"){
        return  res.json({Status: "NotSetup", Error: "Discord Disabled"});
    } else if(global.DISCORDSTATUS === "Disconnected"){
        return res.json({Status: "Error", Error: "Discord Disconnected"});
    } else if(global.DISCORDSTATUS === "Pending"){
        return res.json({Status: "Error", Error: "Discord Pending"});
    } else {
        next();
    }
});



// apply rate limiter to all requests
router.use(GenerateLimiter(global.config.RequestLimitQuery || 400, 10));

/**
 *  Add Role to User
 *  Post: Discord/AddRole/[GUID]
 *  
 *  Description: This will get the discord object associated with the GUID
 *                 and then add a new role and return an updated user Object
 * 
 *  Accepts: `{ "Role": "|ROLETOADD|" }`
 * 
 *  Returns: `{
 *               Status: "|STATUSOFREQUEST|", 
 *               Error: "|ANYERRORMESSAGE|", 
 *               Roles: ["|ARRAYOFROLES|"], 
 *               VoiceChannel: "|CONNECTEDVOICECHANNEL|", 
 *               id: "|DISCORDID|", 
 *               Username: "|USERNAME|", 
 *               GlobalName: "|GlobalName|", 
 *               Avatar: "|LINKTOAVATAR|" 
 *             }`
 * 
 */
router.post('/AddRole/:GUID', requireServerAuth, AddRole);

/**
 *  Remove Role to User
 *  Post: Discord/RemoveRole/[GUID]
 *  
 *  Description: This will get the discord object associated with the GUID
 *                 and then remove a role and return an updated user Object
 * 
 *  Accepts: `{ "Role": "|ROLETOREMOVE|" }`
 * 
 *  Returns: `{
 *               Status: "|STATUSOFREQUEST|", 
 *               Error: "|ANYERRORMESSAGE|", 
 *               Roles: ["|ARRAYOFROLES|"], 
 *               VoiceChannel: "|CONNECTEDVOICECHANNEL|", 
 *               id: "|DISCORDID|", 
 *               Username: "|USERNAME|", 
 *               GlobalName: "|GlobalName|", 
 *               Avatar: "|LINKTOAVATAR|" 
 *             }`
 * 
 */
router.post('/RemoveRole/:GUID', requireServerAuth, RemoveRole);

/**
 *  Get Discord User
 *  Post: Discord/Get/[GUID]
 *  
 *  Description: This will get the discord object associated with the GUID
 * 
 *  Returns: `{
 *               Status: "|STATUSOFREQUEST|", 
 *               Error: "|ANYERRORMESSAGE|", 
 *               Roles: ["|ARRAYOFROLES|"], 
 *               VoiceChannel: "|CONNECTEDVOICECHANNEL|", 
 *               id: "|DISCORDID|", 
 *               Username: "|USERNAME|", 
 *               GlobalName: "|GlobalName|", 
 *               Avatar: "|LINKTOAVATAR|" 
 *             }`
 * 
 */
router.post('/Get/:GUID',requirePlayerOrServerAuth, GetUserAndRoles);


/**
 *  Mute Discord User
 *  Post: Discord/Mute/[GUID]
 *  
 *  Description: Mutes the user in discord if they are connected to a voice channel
 * 
 *  Accepts: `{ "State": |1-Mute,0-UnMute| }`
 * 
 *  Returns: `{
 *               Status: "|STATUSOFREQUEST|", 
 *               Error: "|ANYERRORMESSAGE|"
 *            }`
 * 
 */
router.post('/Mute/:GUID', requireServerAuth,PlayerVoiceMute);


/**
 *  Kick Discord User
 *  Post: Discord/Kick/[GUID]
 *  
 *  Description: Kicks a user from a voice channel
 * 
 *  Accepts: `{ "Text": "|REASONFORKICK|" }`
 * 
 *  Returns: `{
 *               Status: "|STATUSOFREQUEST|", 
 *               Error: "|ANYERRORMESSAGE|"
 *            }`
 * 
 */
router.post('/Kick/:GUID', requireServerAuth ,PlayerVoiceKick);


/**
 *  Move Discord User to new Channel
 *  Post: Discord/Move/[GUID]/[ChannelId]
 *  
 *  Description: Moves player to specified Channel
 * 
 *  Returns: `{
 *               Status: "|STATUSOFREQUEST|", 
 *               Error: "|ANYERRORMESSAGE|"
 *            }`
 * 
 */
router.post('/Move/:GUID/:id', requireServerAuth , ChannelVoiceMove);


/**
 *  Send a DM from Bot
 *  Post: Discord/Send/[GUID]
 *  
 *  Description: Sends a DM to the user based on there GUID
 * 
 *  Accepts: `{ "Message": "|MESSAGETOSEND|" }`
 * 
 *  Returns: `{
 *               Status: "|STATUSOFREQUEST|", 
 *               Error: "|ANYERRORMESSAGE|",
 *               oid: "|IDOFMESSAGE|"
 *            }`
 * 
 */
router.post('/Send/:GUID', requireServerAuth , SendMessageUser);


/**
 *  Get Voice Channel
 *  Post: Discord/GetChannel/[GUID]
 *  
 *  Description: Checks if the user is connected to a voice channel and returns the channel id
 * 
 *  Returns: `{
 *               Status: "|STATUSOFREQUEST|", 
 *               Error: "|ANYERRORMESSAGE|",
 *               oid: "|IDOFCHANNELCONNECTEDTO|"
 *            }`
 * 
 */
 router.post('/GetChannel/:GUID', requirePlayerOrServerAuth, PlayerVoiceGetChannel);


/**
 *  Set Nickname
 *  Post: Discord/SetNickname/[GUID]
 *  
 *  Description: Checks if the user is connected to a voice channel and returns the channel id
 * 
 *  Accepts: `{ "Nickname": "|NewNickname|" }`
 * 
 *  Returns: `{
 *               Status: "|STATUSOFREQUEST|", 
 *               Error: "|ANYERRORMESSAGE|"
 *            }`
 * 
 */
 router.post('/SetNickname/:GUID', requireServerAuth, SetNicknameUser);

/**
 *  Check User
 *  Post: Discord/Check/[GUID]
 *  
 *  Description: Allows you to check if a user has a discord attached to their GUID
 *                 without authentication
 * 
 *  Returns: `{
 *               Status: "|STATUSOFREQUEST|", 
 *               Error: "|ANYERRORMESSAGE|"
 *            }`
 * 
 */
router.post('/Check/:GUID/', CheckId);
router.post('/CheckRole/:GUID/:ROLEID', CheckIdHasRole);

/**
 * Channel Related Endpoints
 *
 * These endpoints handle Channel related functions
 *
 */
router.post('/Channel/Create', requireServerAuth, CreateChannel);
router.post('/Channel/Delete/:id', requireServerAuth, DeleteChannel);
router.post('/Channel/Edit/:id', requireServerAuth, EditChannel);
router.post('/Channel/Invite/:id', requireServerAuth, InviteChannel);
router.post('/Channel/Send/:id',requirePlayerOrServerAuth, SendMessageChannel);
router.post('/Channel/Messages/:id', requirePlayerOrServerAuth, GetMessagesChannel);




/**
 * Sign Up Page
 *
 * This endpoints handle the signup/connection process for players connecting there steam IDs to Discord
 *
 */
router.get('/', renderRootErrorTemplate);

router.get('/login/:id', RenderLogin);

router.get('/callback', HandleCallBack);


router.get('/:id', async (req, res) => {
    let LoginTemplate = await GetLoginTemplate();
    let ErrorTemplate = await GetErrorTemplate();
    let id = req.params.id;
    let GUID = NormalizeToGUID(id);
    if ( global.config.Discord.Client_Id === "" || global.config.Discord.Client_Secret === ""  || global.config.Discord.Bot_Token === ""  || global.config.Discord.Guild_Id === "" || global.config.Discord.Client_Id === undefined || global.config.Discord.Client_Secret === undefined  || global.config.Discord.Bot_Token === undefined  || global.config.Discord.Guild_Id === undefined )
        res.send(render(ErrorTemplate, {TheError: "Discord Intergration is not setup for this server", Type: "NotSetup"}));
    else if (id.match(/[1-9][0-9]{16,16}/)) {
        SendLoginPage(res,id,GUID)
    } else
        res.send(render(ErrorTemplate, {TheError: "Invalid URL", Type: "BadURL"}));
});

module.exports = router;
