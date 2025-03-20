const { Router } = require("express");
const { MongoClient } = require("mongodb");

const { isArray, isObject, CleanRegEx, GenerateLimiter, createLogger } = require('../utils');
const logger = createLogger(global.logger, 'DB.query');

const { CheckAuth, CheckServerAuth } = require("../auth/utils");

const router = Router();

// apply rate limiter to all requests
router.use(GenerateLimiter(global.config.RequestLimitQuery || 400, 10));

/**
 * Post: /[Collection]/[Mod]
 * 
 */
router.post('/:mod', (req, res) => {
    logger.info(`Received query request for mod: ${req.params.mod}`, { params: req.params });
    runQuery(req, res, req.params.mod, req.headers['auth-key'], GetCollection(req.baseUrl));
});

router.post('/Update/:mod', (req, res) => {
    logger.info(`Received update request for mod: ${req.params.mod}`, { params: req.params });
    runUpdateFromQuery(req, res, req.params.mod, req.headers['auth-key'], GetCollection(req.baseUrl));
});

function GetCollection(URL) {
    if (URL.includes("/Player/")) {
        return "Players";
    }
    if (URL.includes("/Object/")) {
        return "Objects";
    }
}

async function runQuery(req, res, mod, auth, COLL) {
    if (CheckServerAuth(auth) || ((await CheckAuth(auth)) && COLL === "Objects")) {
        var RawData = req.body;
        const client = new MongoClient(global.config.DBServer);
        try {

            // Connect the client to the server
            await client.connect();

            const db = client.db(global.config.DB);
            let collection = db.collection(COLL);
            let query, orderBy;
            try {
                query = JSON.parse(RawData.Query);
            } catch (e) {
                logger.error(`Invalid JSON in RawData.Query: ${e.message}`, { error: e });
                res.status(400).json({ Status: "Error", Error: "Invalid JSON in Query", Count: 0, Results: [] });
                return;
            }
            try {
                orderBy = JSON.parse(RawData.OrderBy);
            } catch (e) {
                logger.error(`Invalid JSON in RawData.OrderBy: ${e.message}`, { error: e });
                res.status(400).json({ Status: "Error", Error: "Invalid JSON in OrderBy", Count: 0, Results: [] });
                return;
            }
            let fixQuery = RawData.FixQuery || 0;
            let ReturnCol = "Data";
            if (COLL == "Players") {
                ReturnCol = mod;
            }
            if (fixQuery === 1) {
                query = FixQuery(query, ReturnCol);
                orderBy = FixQuery(orderBy, ReturnCol);
            }
            if (COLL == "Players") {
                if (query && Object.keys(query).length === 0 && query.constructor === Object) {
                    query[mod] = { "$exists": true };
                }
            }
            if (COLL == "Objects" && (query.Mod === undefined || query.Mod === null)) {
                query.Mod = mod;
            }
            let results = collection.find(query).sort(orderBy);
            if (RawData.MaxResults >= 1) {
                results.limit(RawData.MaxResults);
            }
            let theData = await results.toArray();
            let ReturnData = [];
            let count = 0;
            for (let result of theData) {
                for (const [key, value] of Object.entries(result)) {
                    if (key === ReturnCol) {
                        if (RawData.ReturnObject != "" && RawData.ReturnObject != null) {
                            count++;
                            ReturnData.push(value[RawData.ReturnObject]);
                        } else {
                            count++;
                            ReturnData.push(value);
                        }
                    }
                }
            }
            if (ReturnData) {
                if (count == 0) {
                    logger.info(`Query executed but got no results. Query: ${JSON.stringify(query)}`, {
                        collection: COLL,
                        returnColumn: ReturnCol
                    });
                    res.json({ Status: "Empty", Count: 0, Results: [] });
                } else {
                    logger.info(`Query executed successfully. ${count} results returned. Query: ${JSON.stringify(query)}`, {
                        collection: COLL,
                        returnColumn: ReturnCol,
                        resultCount: count
                    });
                    res.json({ Status: "Success", Count: count, Results: ReturnData });
                }
            }
        } catch (err) {
            logger.error(`Error in Query: ${err.message}`, {
                query: JSON.stringify(query),
                collection: COLL,
                mod: mod,
                error: err,
            });
            res.status(203).json({ Status: "Error", Count: 0, Results: [] });
        } finally {
            await client.close();
        }
    } else {
        logger.warn("Authentication failed during query", { mod, auth });
        res.status(401).json({ Status: "Error", Error: "Invalid Auth", Count: 0, Results: [] });
    }
};

async function runUpdateFromQuery(req, res, mod, auth, COLL) {
    if (CheckServerAuth(auth) || ((await CheckAuth(auth)) && global.config.AllowClientWrite)) {
        let RawData = req.body;
        const client = new MongoClient(global.config.DBServer);
        try {
            await client.connect();
            let query, orderBy;
            try {
                query = JSON.parse(RawData.Query.Query);
            } catch (e) {
                logger.error(`Invalid JSON in RawData.Query.Query: ${e.message}`, { error: e });
                res.status(400).json({ Status: "Error", Error: "Invalid JSON in Query", Count: 0, Results: [] });
                return;
            }
            try {
                orderBy = JSON.parse(RawData.Query.OrderBy);
            } catch (e) {
                logger.error(`Invalid JSON in RawData.Query.OrderBy: ${e.message}`, { error: e });
                res.status(400).json({ Status: "Error", Error: "Invalid JSON in OrderBy", Count: 0, Results: [] });
                return;
            }
            let fixQuery = RawData.Query.FixQuery || 0;
            let ReturnCol = "data";
            if (COLL == "Players") {
                ReturnCol = mod;
            }
            if (fixQuery === 1) {
                orderBy = FixQuery(orderBy, ReturnCol);
                query = FixQuery(query, ReturnCol);
            }
            if (COLL == "Players") {
                if (query && Object.keys(query).length === 0 && query.constructor === Object) {
                    query[mod] = { "$exists": true };
                }
            }
            if (COLL == "Objects" && (query.Mod === undefined || query.Mod === null)) {
                query.Mod = mod;
            }
            let element = RawData.Element;
            let operation = RawData.Operation || "set";
            let StringData;
            if (isObject(RawData.Value)) {
                StringData = JSON.stringify(RawData.Value);
            } else if (isArray(RawData.Value)) {
                StringData = JSON.stringify(RawData.Value);
            } else if (`${RawData.Value}`.match(/^-?(0|[1-9]\d*)(\.\d+)?$/g)) {
                StringData = RawData.Value * 1;
            } else {
                StringData = RawData.Value;
            }
            const db = client.db(global.config.DB);
            let collection = db.collection(COLL);
            const options = { upsert: false };

            let jsonString = `{ "data.${element}": ${StringData} }`;
            let updateDocValue;
            try { 
                updateDocValue = JSON.parse(jsonString);
            } catch (e) {
                jsonString = `{ "data.${element}": "${StringData}" }`;
                updateDocValue = JSON.parse(jsonString);
            }
            let updateDoc = { $set: updateDocValue };

            if (operation === "pull") {
                updateDoc = { $pull: updateDocValue };
            } else if (operation === "push") {
                updateDoc = { $push: updateDocValue };
            } else if (operation === "unset") {
                updateDoc = { $unset: updateDocValue };
            } else if (operation === "mul") {
                updateDoc = { $mul: updateDocValue };
            } else if (operation === "rename") {
                updateDoc = { $rename: updateDocValue };
            } else if (operation === "pullAll") {
                updateDoc = { $pullAll: updateDocValue };
            }

            const result = await collection.updateOne(query, updateDoc, options);
            if (result.matchedCount >= 1 || result.upsertedCount >= 1) {
                logger.info(`Update operation succeeded. ${result.matchedCount} document(s) affected. Query: ${JSON.stringify(query)}`, {
                    element,
                    mod,
                });
                res.status(200).json({ Status: "Success", Element: element, Mod: mod, Count: result.matchedCount });
            } else {
                logger.info(`Update operation found no matches. Query: ${JSON.stringify(query)}`, {
                    element,
                    mod,
                });
                res.status(203).json({ Status: "Empty", Element: element, Mod: mod, Count: 0 });
            }
        } catch (err) {
            logger.error(`Error during update operation: ${err.message}`, { error: err });
            res.status(203).json({ Status: "Error", Element: RawData.Element, Mod: mod, Count: 0 });
        } finally {
            await client.close();
        }
    } else {
        logger.warn("Authentication failed during update", { mod, auth, url: req.url });
        res.status(401).json({ Status: "Error", Error: "Invalid Auth", Element: "", Mod: mod });
    }
};

/**
 * Recursively processes the provided query object or array by prefixing keys with a given prefix.
 * 
 * This function traverses the query structure and for each property that does not start with the
 * "$" character or the given prefix (followed by a dot), it renames the property by prepending the 
 * prefix and a dot. For array values, each element is recursively processed.
 *
 * @param {(Object|Array|*)} query - The query structure to be processed, which can be an object, an array, or any value.
 * @param {string} prefix - The string prefix to add to keys that do not already match a specific pattern.
 * @returns {(Object|Array|*)} - The processed query with keys correctly prefixed.
 */
// Recursively fixes a query by prefixing non-operator keys with the given prefix.
// This is especially useful when the query needs to target specific fields within a nested structure.
function FixQuery(query, prefix) {
    // If the query is an object, process each key-value pair.
    if (isObject(query)) {
        for (const [key, value] of Object.entries(query)) {
            // Check if the key does not start with '$' and is not already prefixed with 'prefix.'
            if (!key.match(/^\$/i) && !key.match(new RegExp(`^${CleanRegEx(prefix)}\\.`, "g"))) {
                // Create a new key with the prefix and recursively fix the value.
                query[`${prefix}.${key}`] = FixQuery(value, prefix);
                // Remove the original unprefixed key.
                delete query[key];
            } else {
                // If the key is an operator or already properly prefixed,
                // recursively process its value.
                query[key] = FixQuery(value, prefix);
            }
        }
        return query;
    } else if (isArray(query)) {
        // If the query is an array, process each element recursively.
        return query.map(e => FixQuery(e, prefix));
    }
    // Return the value if it is neither an object nor an array.
    return query;
}

module.exports = router;
