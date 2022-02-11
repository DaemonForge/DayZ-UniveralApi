
const { MongoClient } = require("mongodb");
const {writeFileSync} = require('fs');
const ConfigPath = "config.json";
const log = require("./log");
let {createHash} = require('crypto');
const RateLimit = require('express-rate-limit');

module.exports ={
    dynamicSortMultiple,
    dynamicSort,
    isObject,
    isArray,
    isEmpty,
    makeAuthToken,
    makeObjectId,
    RemoveBadProperties,
    versionCompare,
    InstallIndexes,
    CheckIndexes,
    CheckRecentVersion,
    NormalizeToGUID,
    ExtractAuthKey,
    CleanRegEx,
    GenerateLimiter,
    promisedProperties
}



function promisedProperties(object) {

  let promisedProperties = [];
  const objectKeys = Object.keys(object);

  objectKeys.forEach((key) => promisedProperties.push(object[key]));

  return Promise.all(promisedProperties)
    .then((resolvedValues) => {
      return resolvedValues.reduce((resolvedObject, property, index) => {
        resolvedObject[objectKeys[index]] = property;
        return resolvedObject;
      }, object);
    });

}

function dynamicSortMultiple( props ) {
    /*
     * save the arguments object as it will be overwritten
     * note that arguments object is an array-like object
     * consisting of the names of the properties to sort by
     */
    return function (obj1, obj2) {
        var i = 0, result = 0, numberOfProperties = props.length;
        /* try getting a different result from 0 (equal)
         * as long as we have extra properties to compare
         */
        while(result === 0 && i < numberOfProperties) {
            result = dynamicSort(props[i])(obj1, obj2);
            i++;
        }
        return result;
    }
}
function dynamicSort(property) {
    var sortOrder = 1;
    if(property[0] === "-") {
        sortOrder = -1;
        property = property.substr(1);
    }
    return function (a,b) {
        /* next line works with strings and numbers, 
         * and you may want to customize it to your needs
         */
        var result = (a[property] < b[property]) ? -1 : (a[property] > b[property]) ? 1 : 0;
        return result * sortOrder;
    }
}

function isObject(a) {
    return (!!a) && (a.constructor === Object);
};


function isArray(a) {
    return (!!a) && (a.constructor === Array);
};
function isEmpty(obj){
    return (obj && Object.keys(obj).length === 0 && obj.constructor === Object)
}
function makeAuthToken() {
    let result           = '';
    let characters       = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-.!~';
    let charactersLength = characters.length;
    for ( let i = 0; i < 48; i++ ) {
       result += characters.charAt(Math.floor(Math.random() * charactersLength));
    }
    return result;
 }

 function makeObjectId() {
    let result           = '';
    let characters       = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-.~()*:@,;';
    let charactersLength = characters.length;
    for ( let i = 0; i < 16; i++ ) {
       result += characters.charAt(Math.floor(Math.random() * charactersLength));
    }
    let datetime = new Date();
    let date = datetime.toISOString()
    result += date;
    let SaveToken = createHash('sha256').update(result).digest('base64');
    //Making it URLSafe
    SaveToken = SaveToken.replace(/\+/g, '-'); 
    SaveToken = SaveToken.replace(/\//g, '_');
    SaveToken = SaveToken.replace(/=+$/, '');
    return SaveToken;
 }


 function RemoveBadProperties(obj){
    let replace = /[\!\@\#\$\%\^\&\*\(\)\+\=\\\|\]\[\"\?\>\<\.\,\;\:\- ]/g;
    Object.keys(obj).forEach(function (k) {
        if (isObject(obj[k])) {
            obj[k] = RemoveBadProperties(obj[k]);
            return;
        }
        if (isArray(obj[k])){
            obj[k].forEach(j =>{
                if (isObject(j)){
                    j = RemoveBadProperties(j);
                }
            });
        }
        let K = k.replace(replace, "_");
        if (K !== k){
            obj[K] = obj[k];
            delete obj[k];
        }
    })
    return obj;
}


function versionCompare(v1, v2, options) {
    var lexicographical = options && options.lexicographical,
        zeroExtend = options && options.zeroExtend,
        v1parts = v1.split('.'),
        v2parts = v2.split('.');

    function isValidPart(x) {
        return (lexicographical ? /^\d+[A-Za-z]*$/ : /^\d+$/).test(x);
    }

    if (!v1parts.every(isValidPart) || !v2parts.every(isValidPart)) {
        return NaN;
    }

    if (zeroExtend) {
        while (v1parts.length < v2parts.length) v1parts.push("0");
        while (v2parts.length < v1parts.length) v2parts.push("0");
    }

    if (!lexicographical) {
        v1parts = v1parts.map(Number);
        v2parts = v2parts.map(Number);
    }

    for (var i = 0; i < v1parts.length; ++i) {
        if (v2parts.length == i) {
            return 1;
        }

        if (v1parts[i] == v2parts[i]) {
            continue;
        }
        else if (v1parts[i] > v2parts[i]) {
            return 1;
        }
        else {
            return -1;
        }
    }

    if (v1parts.length != v2parts.length) {
        return -1;
    }

    return 0;
}


async function InstallIndexes(){
  
    const client = new MongoClient(global.config.DBServer, { useUnifiedTopology: true });
    let returnvalue = false;
    try{
      await client.connect(); 
      const db = client.db(global.config.DB);
      let pcollection = db.collection("Players");
      const resultGUID = await pcollection.createIndex({ GUID: 1 });
      //console.log(resultGUID);
      const resultAUTH = await pcollection.createIndex({ GUID: 1, AUTH: 1 });
      //console.log(resultAUTH);
      let ocollection = db.collection("Objects");
      const oresult = await ocollection.createIndex({ ObjectId: 1, Mod: 1});
      //console.log(oresult);
      let gcollection = db.collection("Globals");
      const gresult = await gcollection.createIndex({ Mod: 1 });
      //console.log(gresult);
      log("Successfully Created Indexes")
      returnvalue= true;
    } catch(e){
      log(e, "warn");
      returnvalue= false;
    }finally{
      await client.close();
      return returnvalue;
    }
  }
  async function CheckIndexes(){
      if (global.config.CreateIndexes === undefined || global.config.CreateIndexes === null || global.config.CreateIndexes === true){
        if ((await InstallIndexes())){
          global.config.CreateIndexes = false;
          try {
            writeFileSync(global.SAVEPATH + ConfigPath, JSON.stringify(global.config, undefined, 4))
          } catch(e) {
            log(e, "warn")
          }
        } else {
          log("Failed to create indexes", "warn")
        }
    }
  }

  
async function CheckRecentVersion(){
    try {
      const data = await fetch("https://api.github.com/repos/daemonforge/DayZ-UniveralApi/releases").then( response => response.json()).catch(e => log(`${e}`, "warn"));
      if (data[0] !== undefined && data[0].tag_name !== undefined ){
        global.STABLEVERSION = data[0].tag_name;
        global.NEWVERSIONDOWNLOAD = data[0].html_url;
      }
      let vc = versionCompare(global.APIVERSION, global.STABLEVERSION);
      if (global.STABLEVERSION === "0.0.0"){
        log(`WARNING!!! Could check for the current stable version`, "warn");
      } else if (vc > 0){
        log(`WARNING!!! You are running a unpublished version, note it may not work as expected`, "warn")
        log(`Installed Version: ${global.APIVERSION} Stable Version: ${global.STABLEVERSION} `);
      } else if (vc < 0){
        log(`!!!WARNING!!! You're API is currently out of date `, "warn")
        log(`Installed Version: ${global.APIVERSION} Stable Version: ${global.STABLEVERSION}`);
        log(`WARNING!!! Download Link - ${global.NEWVERSIONDOWNLOAD}`, "warn");
      } else {
        log(`API Is currently running the most recent Stable Version: ${global.APIVERSION}`);
      }
    } catch (err){
      log(`WARNING!!! Couldn't check for the current stable version`, "warn");
      console.log(err);
    }
  }


  function NormalizeToGUID(idorguid){
    if (idorguid.match(/[1-9][0-9]{16}/g)){
        idorguid = createHash('sha256').update(idorguid).digest('base64');
        idorguid = idorguid.replace(/\+/g, '-'); 
        idorguid = idorguid.replace(/\//g, '_');
    }
    return idorguid;
  }


  function ExtractAuthKey (req, res, next) {
    let contentType = req.headers['content-type'] || "application/json";
    if ( `${contentType}`.match(/^(text\/|application\/|multipart\/|audio\/|image\/|video\/)/gi)){ 
      req.headers['auth-key'] = req.headers['auth-key'] || '';
    } else {
      req.headers['auth-key'] = req.headers['auth-key'] || req.headers['content-type'] || '';
      req.headers['content-type'] = 'application/json';
    }
    if (global.config.debug !== undefined && global.config.debug === 1) {
      console.log("Request: " + req.url);
    }
    if (global.config.debug !== undefined && global.config.debug === 2) {
      console.log("Request: " + req.url + " " + req.headers['auth-key'] + req.body);
    }
    next();
  }
  

  function CleanRegEx(value){
    return value.replace(/[-[\]{}()*+!<=:?.\/\\^$|#\s,]/g, '\\$&');
  }

  function GenerateLimiter(limitRate, seconds){
    let Seconds = seconds || 10;
    let LimitRate = limitRate || 300
    return RateLimit({
      windowMs: Seconds * 1000, // 40 req/sec
      max: LimitRate,
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
  }