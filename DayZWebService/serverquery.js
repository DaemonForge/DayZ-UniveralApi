const {query} = require('gamedig');

const {Router} = require('express');


const {CheckAuth, CheckServerAuth} = require('./AuthChecker')
const log = require("./log");


const router = Router();

router.post('/Status/:ip/:port/:auth', (req, res)=>{
    GetServerStatus(req, res, req.params.ip, req.params.port, req.params.auth);
});

async function QueryServer(ip, port){
    try{
        let data = query({
            type: 'dayz',
            host: ip,
            port: port
        }).then((state) =>{
            //console.log(state);
            let keywords = state.raw.tags.split(',');
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
        try {
            let response =  await QueryServer(ip, port);
            if (response.error === undefined) {
                let statusobj = {
                    Status: response.status,
                    Error: "", 
                    IP: response.ip,
                    GamePort: response.game_port,
                    QueryPort: response.query_port,
                    Name: response.name,
                    Version: response.version,
                    Players: response.players,
                    Queue: response.queue,
                    MaxPlayers: response.max_players,
                    Time: response.time,
                    Map: response.map,
                    Password: response.password ? 1 : 0,
                    FirstPerson: response.first_person ? 1 : 0
                }
                res.status(200);
                return res.json(statusobj);
            }
        } catch (e) {
            log(e, "warn");
        } finally {
            res.status(200);
            return res.json( {
                IP: ip,
                Error: response.error, 
                GamePort: -1,
                QueryPort: parseInt(port),
                Status: "Offline",
                Name: "",
                Version: "",
                Players: 0,
                Queue: 0,
                MaxPlayers: 0,
                Time: "",
                Map: "",
                Password: 0,
                FirstPerson: 0
            });
        }
    } else {
        res.status(401);
        res.json({Status: "NoAuth", Error: "" });
    }
}

module.exports = router;