const {sign, verify} = require('jsonwebtoken');
const { MongoClient } = require("mongodb");
const {createHash, Hash} = require('crypto');
const { isArray ,IncermentAPICount} = require('./utils');
module.exports ={
    ValidateAuth,
    makeAuthToken
}


async  function ValidateAuth (req, res, next) {
    if (req.url.match(/^\/Discord/gi) && req.method === "GET"){
      next();
      return;
    }
    const client = new MongoClient(global.config.DBServer, { useUnifiedTopology: true });
    req.KeyType = "null";
    req.IsServer = false;
    if (TestJWT(req.headers['auth-key'])){
      let data = GetDecode(req.headers['auth-key']);
      await client.connect();
      data = await data;
      if (data.Passed && data.Data.C !== undefined){
        const db = client.db(global.config.MasterDB);
        let collection = db.collection("Clients");
        let query = { ClientId: data.Data.C, Status: "active" };
        let results = collection.find(query);
        let dataarr = await results.toArray(); 
        if ( dataarr[0] !== undefined) {
            let hasAuth = await CheckIsAuthStored(data.Data.GUID, dataarr[0].DB, req.headers['auth-key'])
            if (hasAuth){
              req.ClientInfo = dataarr[0];
              req.KeyType = "client";
              req.GUID = data.Data.GUID;
              req.IsServer = false;
              next(); 
            }
        }
      }
    } else {
        await client.connect();
        const db = client.db(global.config.MasterDB);
        let collection = db.collection("Clients");
        let query = { APIKey: req.headers['auth-key'], Status: "active" };
        let results = collection.find(query);
        let dataarr = await results.toArray(); 
        if (dataarr[0] !== undefined){
            req.ClientInfo = dataarr[0];
            req.KeyType = "server";
            req.IsServer = true;
            next();
        }
    }
    if (req.KeyType === "null" && (req.url.match(/^\/Status$/gi) || req.url.match(/^\/Discord\/Check\/[1-9][0-9]{16}$/gi)) ){
      next();
    } else if (req.KeyType === "null") {
      res.status(401);
      res.json({Status: "Error", Error: `Invalid URL Or Auth`});
    }
    await client.close();
  }

  function TestJWT(key){
    return (key.match(/\./g)||[]).length === 2;

  }
  async function GetDecode(key){

      return verify(key, GetSigningAuth(), function(err, decoded) {
          if (err) {
              if (err.name == "TokenExpiredError"){

              }
              return { Passed: false, Data: {} };
          } else {
              
              return { Passed: true, Data: decoded };
          }
      });
  
  }

  function makeAuthToken(GUID, clientID) {
    const player = { GUID: GUID, C: clientID}; 
    //Token expires in 46.5 minutes, tokens renew every 21-23 Minutes ensuring that if the API is down at the time of the renewal token will last till next retry
    let result = sign(player, GetSigningAuth(), { expiresIn: 2800 }); 
    return result;
 }
  function GetSigningAuth(){
        return global.config.JWTToken;
  }
  function GetKey(){
      return global.config.EncKey
  }


  async function CheckIsAuthStored(guid, DB, auth){
    let isAuth = false;
    const client = new MongoClient(global.config.DBServer, { useUnifiedTopology: true });
      try{
        await client.connect();
        // Connect the client to the server        
        const db = client.db(DB);
        let collection = db.collection("Players");
        let SavedAuth = createHash('sha256').update(auth).digest('base64');
        let query = { GUID: guid, AUTH: SavedAuth };
        let results = collection.find(query);
            if ((await results.count()) != 0){
                isAuth = true;
            }
    } catch(err){
        log("ID " + guid + " err" + err, "warn");
    } finally{
        await client.close();
        return isAuth;
    }
  }