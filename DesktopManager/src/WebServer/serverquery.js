const { query } = require('gamedig');

const { Router } = require('express');


const { CheckAuth, CheckServerAuth } = require('./AuthChecker')
const log = require("./log");
const {isArray,GenerateLimiter} = require('./utils');


const router = Router();


router.use(GenerateLimiter(global.config.RequestLimitServerQuery || 200 ,5));




/**
 *  DayZ Server SteamQuery
 *  Post: /ServerQuery/Status/[IP]/[PORT]
 *  
 *  Description: This runs a Steam Query to the specified IP and port to get some 
 *    basic server information, like player count and Queue
 * 
 *  Returns: `{ 
 *               "Status": "|STATUSOFSERVER|", 
 *               "Error": "|ANYERRORMESSAGE|",
 *               "IP": "|Servers IP|",
 *               "GamePort": |GAME PORT|,
 *               "QueryPort": |STEAMQUERYPORT|,
 *               "Name": "|SERVERNAME|",
 *               "ServerVersion": "|SERVERSVERSION|",
 *               "Players": |#OFPLAYERSONLINE|,
 *               "QueuePlayers": |#OFPLAYERSINQUEUE|,
 *               "MaxPlayers": |MAX#OFPLAYERCANBEONSERVER|,
 *               "GameTime": "|CURRENTINGAMETIME|",
 *               "GameMap": "|MAPSERVERISRUNNING|",
 *               "Password": |1?0HASPASSWORD|,
 *               "FirstPerson": |1?0ISFIRSTPERSON|
 *            }`
 * 
 */
router.post('/Status/:ip/:port', (req, res)=>{
    GetServerStatus(req, res, req.params.ip, req.params.port, req.headers['auth-key']);
});


async function QueryServer(ip, port){
    try{
        let data = query( {
            type: 'dayz',
            host: ip,
            port: port,
            requestRules: true
        } ).then((state) => {
            let keywords = state.raw.tags;
            return {
                ip: state.connect.split(':')[0],
                query_port: parseInt(port),
                game_port: parseInt(state.connect.split(':')[1]),
                status: "Online",
                name: state.name,
                version: state.raw.version,
                players: state.raw.numplayers,
                queue: parseInt(keywords.find(tag => tag.includes('lqs')).replace('lqs', '')),
                max_players: state.maxplayers,
                time: keywords.find(tag => tag.includes(':')),
                first_person: keywords.some(tag => tag.includes('no3rd')),
                map: state.map,
                time_acceleration: parseFloat((keywords.find(tag => tag.includes('etm')) || '12').replace('etm', '')) + ", " + parseFloat((keywords.find(tag => tag.includes('entm')) || '1').replace('entm', '')),
                day_time_acceleration: parseFloat((keywords.find(tag => tag.includes('etm')) || '12').replace('etm', '')),
                night_time_acceleration: parseFloat((keywords.find(tag => tag.includes('entm')) || '1').replace('entm', '')),
                password: state.password,
                battleye: keywords.some(tag => tag.includes('battleye')),
                vac: state.raw.secure == 1,
                public_hive: !keywords.some(tag => tag.includes('privHive')),
                dlc_enabled: keywords.some(tag => tag.includes('isDLC')),
                ping: state.ping
            }
        }
        ).catch((error) => {
            log(error, "warn");
            return {ip: ip, query_port: parseInt(port), status: "offline", error: "Server is offline or wrong ip/query port"};
        });
        theData = await data;
        return theData;
    } catch (error){
        log(error, "warn");
        return {ip: ip, query_port: parseInt(port), status: "offline", error: "Server is offline or wrong ip/query port"};
    }
};



async function GetServerStatus(req, res, ip, port, auth){
    if (CheckServerAuth(auth) || ( await CheckAuth(auth) ) ){
        let response;
        let isSent = false;
        try {
            response =  await QueryServer(ip, port);
            if (response.error === undefined) {
                let statusobj = {
                    Status: response.status,
                    Error: "", 
                    IP: response.ip,
                    GamePort: response.game_port,
                    QueryPort: response.query_port,
                    Name: response.name,
                    ServerVersion: response.version,
                    Players: response.players,
                    QueuePlayers: response.queue,
                    MaxPlayers: response.max_players,
                    GameTime: response.time,
                    GameMap: response.map,
                    Password: response.password ? 1 : 0,
                    FirstPerson: response.first_person ? 1 : 0
                }
                isSent = true;
                log("Server Status Check requested for " + response.ip + ":" + response.query_port);
                res.status(200);
                res.json(statusobj);

                return;
            }
        } catch (e) {
            log(e, "warn");
        }
            if(isSent) return;
             res.status(200);
             res.json( {
                Status: "Offline",
                Error: response.error || "Error Unknown", 
                IP: ip,
                GamePort: -1,
                QueryPort: parseInt(port),
                Name: "",
                ServerVersion: "",
                Players: 0,
                QueuePlayers: 0,
                MaxPlayers: 0,
                GameTime: "",
                GameMap: "",
                Password: 0,
                FirstPerson: 0
            } );
            return;
    } else {
        res.status(401);
        log("Invalid Authkey Server Query");
        res.json({Status: "Error", Error: "Invalid Auth" });
        return;
    }
}

module.exports = router;
