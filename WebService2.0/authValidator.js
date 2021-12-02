const {
  sign,
  verify
} = require('jsonwebtoken');
const {
  MongoClient
} = require("mongodb");
const {
  createHash,
  Hash
} = require('crypto');
const {
  isArray,
  GetClientInfoById,
  IncermentAPICount,
  byteSize
} = require('./utils');
module.exports = {
  ValidateAuth,
  makeAuthToken
}


async function ValidateAuth(req, res, next) {
  //console.log(`${req.method} | ${req.url}`);
  if (req.url.match(/^\/Discord/gi) && req.method === "GET") {
    next();
    return;
  }
  req.KeyType = "null";
  req.IsServer = false;
  if (TestJWT(req.headers['auth-key'])) {
    let data = GetDecode(req.headers['auth-key']);
    data = await data;
    if (data.Passed && data.Data.C !== undefined) {
      let Cdata = await GetClientInfoById(data.Data.C);
      if (Cdata !== undefined) {
        let hasAuth = await CheckIsAuthStored(data.Data.GUID, Cdata.DB, req.headers['auth-key'])
        if (hasAuth) {
          req.ClientInfo = Cdata;
          req.KeyType = "client";
          req.GUID = data.Data.GUID;
          req.IsServer = false;
          next();
        }
      }
    }
  } else {
    const client = new MongoClient(global.config.DBServer, {
      useUnifiedTopology: true
    });
    let dataarr;
    try {
      await client.connect();
      const db = client.db(global.config.MasterDB);
      let collection = db.collection("Clients");
      let query = {
        APIKey: req.headers['auth-key'],
        Status: "active"
      };
      let results = collection.find(query);
      dataarr = await results.toArray();
    } catch (err) {
      console.log(err);
    } finally {
      client.close();
    }
    if (dataarr !== undefined && dataarr[0] !== undefined) {
      req.ClientInfo = dataarr[0];
      req.KeyType = "server";
      req.IsServer = true;
      next();
    }
  }
  if (req.KeyType === "null" && (req.url.match(/^\/Status$/gi) || req.url.match(/^\/Discord\/Check\/[1-9][0-9]{16}$/gi))) {
    next();
  } else if (req.KeyType === "null" && req.url.match(/^\/[A-Za-z]{3,32}\/Player\/PublicLoad\/[A-Za-z_\-0-9]{42,45}\=\/[A-Za-z\_0-9]{1,128}$/g)) {
    next();
  } else if (req.KeyType === "null") {
    res.status(401);
    res.json({
      Status: "Error",
      Error: `Invalid URL Or Auth`
    });
  }
}

function TestJWT(key) {
  return (key.match(/\./g) || []).length === 2;

}
async function GetDecode(key) {

  return verify(key, GetSigningAuth(), function (err, decoded) {
    if (err) {
      if (err.name == "TokenExpiredError") {

      }
      return {
        Passed: false,
        Data: {}
      };
    } else {

      return {
        Passed: true,
        Data: decoded
      };
    }
  });

}

function makeAuthToken(GUID, clientID) {
  const player = {
    GUID: GUID,
    C: clientID
  };
  //Token expires in 46.5 minutes, tokens renew every 21-23 Minutes ensuring that if the API is down at the time of the renewal token will last till next retry
  let result = sign(player, GetSigningAuth(), {
    expiresIn: 2800
  });
  return result;
}

function GetSigningAuth() {
  return global.config.JWTToken;
}

function GetKey() {
  return global.config.EncKey
}


async function CheckIsAuthStored(guid, DB, auth) {
  let isAuth = false;
  const client = new MongoClient(global.config.DBServer, {
    useUnifiedTopology: true
  });
  try {
    await client.connect();
    // Connect the client to the server        
    const db = client.db(DB);
    let collection = db.collection("Players");
    let SavedAuth = createHash('sha256').update(auth).digest('base64');
    let query = {
      GUID: guid,
      AUTH: SavedAuth
    };
    let results = collection.find(query);
    if ((await results.count()) != 0) {
      isAuth = true;
    }
  } catch (err) {
    log("ID " + guid + " err" + err, "warn");
  } finally {
    await client.close();
    return isAuth;
  }
}