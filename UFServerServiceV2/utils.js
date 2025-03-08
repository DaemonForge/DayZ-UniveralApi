const { MongoClient } = require("mongodb");
const { writeFileSync } = require('fs');
const ConfigPath = "config.json";
const { createHash } = require('crypto');
const RateLimit = require('express-rate-limit');

/**
 * Resolves all promises in an object and maintains the key structure
 * @param {Object} object - Object containing promises as values
 * @returns {Promise<Object>} - Object with resolved promise values
 */
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

/**
 * Sorts an array of objects by multiple properties
 * @param {Array<string>} props - Array of property names to sort by
 * @returns {Function} - Comparison function for Array.sort()
 */
function dynamicSortMultiple(props) {
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

/**
 * Creates a sorting function for a single property
 * @param {string} property - Property name to sort by (prefix with - for descending)
 * @returns {Function} - Comparison function for Array.sort()
 */
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

/**
 * Checks if a value is an Object
 * @param {*} a - Value to check
 * @returns {boolean} - True if value is an Object
 */
function isObject(a) {
  return (!!a) && (a.constructor === Object);
};

/**
 * Checks if a value is an Array
 * @param {*} a - Value to check
 * @returns {boolean} - True if value is an Array
 */
function isArray(a) {
  return (!!a) && (a.constructor === Array);
};

/**
 * Checks if an object is empty
 * @param {Object} obj - Object to check
 * @returns {boolean} - True if object is empty
 */
function isEmpty(obj){
  return (obj && Object.keys(obj).length === 0 && obj.constructor === Object)
}

/**
 * Generates a random authentication token
 * @returns {string} - Random 48 character authentication token
 */
function makeAuthToken() {
  let result           = '';
  let characters       = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-.!~';
  let charactersLength = characters.length;
  for ( let i = 0; i < 48; i++ ) {
    result += characters.charAt(Math.floor(Math.random() * charactersLength));
  }
  return result;
}

/**
 * Generates a unique object ID based on random characters and current timestamp
 * @returns {string} - URL-safe SHA-256 hashed object ID
 */
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
  // Making it URLSafe
  SaveToken = SaveToken.replace(/\+/g, '-'); 
  SaveToken = SaveToken.replace(/\//g, '_');
  SaveToken = SaveToken.replace(/=+$/, '');
  return SaveToken;
}

/**
 * Recursively sanitizes object property names by replacing special characters
 * @param {Object} obj - Object to sanitize
 * @returns {Object} - Object with sanitized property names
 */
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

/**
 * Compares two version strings
 * @param {string} v1 - First version
 * @param {string} v2 - Second version
 * @param {Object} options - Comparison options
 * @returns {number} - 1 if v1 > v2, -1 if v1 < v2, 0 if equal, NaN if invalid
 */
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

/**
 * Creates database indexes for improved query performance
 * @returns {Promise<boolean>} - True if indexes were created successfully
 */
async function InstallIndexes() {
  const client = new MongoClient(global.config.DBServer);
  let returnvalue = false;
  try {
    await client.connect();
    const db = client.db(global.config.DB);
    let pcollection = db.collection("Players");
    const resultGUID = await pcollection.createIndex({ GUID: 1 });
    const resultAUTH = await pcollection.createIndex({ GUID: 1, AUTH: 1 });
    let ocollection = db.collection("Objects");
    const oresult = await ocollection.createIndex({ ObjectId: 1, Mod: 1 });
    let gcollection = db.collection("Globals");
    const gresult = await gcollection.createIndex({ Mod: 1 });
    let mcollection = db.collection("Messages");
    const mresult = await mcollection.createIndex({ Mod: 1, Queue: 1, createdAt: 1 });
    let pmscollection = db.collection("PlayerMessagesStatus");
    const pmsresult = await pmscollection.createIndex({ Mod: 1, Queue: 1, playerGuid: 1 });
    let mmcollection = db.collection("MessagesMeta");
    const mmsresult = await mmcollection.createIndex({ Mod: 1, Queue: 1 });
    global.logger.info("Successfully Created Indexes");
    returnvalue = true;
  } catch (e) {
    global.logger.warn('Failed to create indexes', { error: e });
    returnvalue = false;
  } finally {
    await client.close();
    return returnvalue;
  }
}

/**
 * Checks if indexes need to be created and creates them if needed
 */
async function CheckIndexes() {
  if (global.config.CreateIndexes === undefined || global.config.CreateIndexes === null || global.config.CreateIndexes === true) {
    if ((await InstallIndexes())) {
      global.config.CreateIndexes = false;
      try {
        writeFileSync(global.SAVEPATH + ConfigPath, JSON.stringify(global.config, undefined, 4));
      } catch (e) {
        global.logger.warn('Failed to write config file', { error: e });
      }
    } else {
      global.logger.warn('Failed to create indexes');
    }
  }
}

/**
 * Checks the latest version from GitHub and logs relevant version information
 */
async function CheckRecentVersion() {
  try {
    const data = await fetch("https://api.github.com/repos/daemonforge/DayZ-UniveralApi/releases").then(response => response.json()).catch(e => global.logger.warn('Failed to fetch releases', { error: e }));
    if (data[0] !== undefined && data[0].tag_name !== undefined) {
      global.STABLEVERSION = data[0].tag_name;
      global.NEWVERSIONDOWNLOAD = data[0].html_url;
    }
    let vc = versionCompare(global.APIVERSION, global.STABLEVERSION);
    if (global.STABLEVERSION === "0.0.0") {
      global.logger.warn('Could not check for the current stable version');
    } else if (vc > 0) {
      global.logger.warn('You are running an unpublished version, it may not work as expected', { installedVersion: global.APIVERSION, stableVersion: global.STABLEVERSION });
    } else if (vc < 0) {
      global.logger.warn('Your API is currently out of date', { installedVersion: global.APIVERSION, stableVersion: global.STABLEVERSION, downloadLink: global.NEWVERSIONDOWNLOAD });
    } else {
      global.logger.info('API is currently running the most recent stable version', { version: global.APIVERSION });
    }
  } catch (err) {
    global.logger.warn('Could not check for the current stable version', { error: err });
  }
}

/**
 * Normalizes a Steam ID to a GUID format
 * @param {string} idorguid - ID or GUID to normalize
 * @returns {string} - Normalized GUID
 */
function NormalizeToGUID(idorguid) {
  if (idorguid.match(/[1-9][0-9]{16}/g)) {
    idorguid = createHash('sha256').update(idorguid).digest('base64');
    idorguid = idorguid.replace(/\+/g, '-');
    idorguid = idorguid.replace(/\//g, '_');
  }
  return idorguid;
}

/**
 * Express middleware to extract and validate authentication keys from requests
 * @param {Object} req - Express request object
 * @param {Object} res - Express response object
 * @param {Function} next - Express next function
 */
function ExtractAuthKey(req, res, next) {
  let contentType = req.headers['content-type'] || "application/json";
  if (`${contentType}`.match(/^(text\/|application\/|multipart\/|audio\/|image\/|video\/)/gi)) {
    req.headers['auth-key'] = req.headers['auth-key'] || '';
  } else {
    req.headers['auth-key'] = req.headers['auth-key'] || req.headers['content-type'] || '';
    req.headers['content-type'] = 'application/json';
  }
  if (global.config.debug !== undefined && global.config.debug === 1) {
    global.logger.debug('Request received', { url: req.url });
  }
  if (global.config.debug !== undefined && global.config.debug === 2) {
    global.logger.debug('Request received with auth key', { url: req.url, authKey: req.headers['auth-key'], body: req.body });
  }
  next();
}

/**
 * Escapes special characters in a string for use in regular expressions
 * @param {string} value - String to clean
 * @returns {string} - Escaped string safe for RegEx
 */
function CleanRegEx(value) {
  return value.replace(/[-[\]{}()*+!<=:?.\/\\^$|#\s,]/g, '\\$&');
}

/**
 * Creates a rate limiter middleware for Express
 * @param {number} limitRate - Maximum number of requests allowed
 * @param {number} seconds - Time window in seconds
 * @returns {Function} - Express middleware for rate limiting
 */
function GenerateLimiter(limitRate, seconds) {
  let Seconds = seconds || 10;
  let LimitRate = limitRate || 300;
  return RateLimit({
    windowMs: Seconds * 1000,
    max: LimitRate,
    message: '{ "Status": "Error", "Error": "RateLimited" }',
    keyGenerator: (req) => {
      // Identify clients by IP, checking various header options
      return req.headers['CF-Connecting-IP'] || 
              req.headers['x-forwarded-for'] || 
              req.socket.remoteAddress || 
              req.ip;
    },
    handler: (request, response, next, options) => {
      if (request.rateLimit.used === request.rateLimit.limit + 1) {
        // Original onLimitReached code
        const ip = request.headers['CF-Connecting-IP'] || 
                    request.headers['x-forwarded-for'] || 
                    request.socket.remoteAddress || 
                    request.ip;
        logger.warn('RateLimit reached - possible DDoS attack or need to increase request limit', { ip });
      }
      response.status(options.statusCode).send(options.message);
    },
    skip: (req) => {
      // Skip rate limiting for whitelisted IPs
      const ip = req.headers['CF-Connecting-IP'] || 
                  req.headers['x-forwarded-for'] || 
                  req.socket.remoteAddress || 
                  req.ip;
      const whitelist = global.config.RateLimitWhiteList;
      return ip && whitelist && isArray(whitelist) && whitelist.includes(ip);
    }
  });
}

function buildUpdateDoc(mod, element, operation, value) {
  const fieldName = `${mod}.${element}`;
  const updateObj = { [fieldName]: value };
  switch (operation) {
      case "pull":
          return { $pull: updateObj };
      case "push":
          return { $push: updateObj };
      case "unset":
          return { $unset: updateObj };
      case "mul":
          return { $mul: updateObj };
      case "rename":
          return { $rename: updateObj };
      case "pullAll":
          return { $pullAll: updateObj };
      default:
          return { $set: updateObj };
  }
}
function processValue(value) {
  if (isObject(value) || isArray(value)) {
      return value;
  } else if (!isNaN(value) && value !== "") {
      return Number(value);
  } else {
      return value;
  }
}

module.exports = {
  processValue,
  buildUpdateDoc,
  promisedProperties,
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
  GenerateLimiter
};  
