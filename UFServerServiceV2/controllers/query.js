const { Router } = require("express");
const { MongoClient } = require("mongodb");

const { isArray, isObject, CleanRegEx, GenerateLimiter, createLogger, findDangerousQueryOperator, accessGrants } = require('../utils');
const logger = createLogger(global.logger, 'DB.query');
const { getAnalyzer } = require('../models/queryAnalyzer');

const { CheckAuth, CheckServerAuth, AuthPlayerGuid } = require("../auth/utils");
const { getPlayer } = require("../models/player");

const router = Router();

// apply rate limiter to all requests
router.use(GenerateLimiter(global.config.RequestLimitQuery || 400, 10));

/**
 * Post: /[Collection]/[Mod]
 * 
 */
router.post('/:mod', (req, res) => {
    logger.info(`Query request received`, { mod: req.params.mod });
    runQuery(req, res, req.params.mod, req.headers['auth-key'], GetCollection(req.baseUrl));
});

router.post('/Update/:mod', (req, res) => {
    logger.info(`Query update request received`, { mod: req.params.mod });
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
    const isServer = CheckServerAuth(auth);
    if (isServer || ((await CheckAuth(auth)) && COLL === "Objects")) {
        var RawData = req.body;
        
        // Enhanced logging for territory duplication debugging
        logger.info(`[QUERY] Query request started`, {
            mod,
            collection: COLL,
            queryPreview: RawData.Query ? RawData.Query.substring(0, 200) : 'empty',
            timestamp: new Date().toISOString()
        });
        
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
            // Reject server-side-JavaScript operators ($where etc.) before the
            // query reaches MongoDB.
            const badOp = findDangerousQueryOperator(query) || findDangerousQueryOperator(orderBy);
            if (badOp) {
                logger.warn(`Rejected query containing disallowed operator`, { mod, collection: COLL, operator: badOp });
                res.status(400).json({ Status: "Error", Error: `Query operator ${badOp} is not allowed`, Count: 0, Results: [] });
                return;
            }
            let fixQuery = RawData.FixQuery || 0;
            let ReturnCol = "data";
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

            // Secure Objects: player-auth queries only see objects they're permitted to.
            // Rule-gated objects pass the DB filter as candidates and are post-filtered below.
            let playerGuid = null;
            if (COLL === "Objects" && !isServer) {
                playerGuid = AuthPlayerGuid(auth);
                query = buildObjectPermissionClause(query, playerGuid, true);
            }

            // Log final query for territory debugging
            if (mod === 'FactionTerritories' || mod === 'Factions') {
                logger.info(`[QUERY][${mod}] Executing query`, {
                    mod,
                    finalQuery: JSON.stringify(query),
                    orderBy: JSON.stringify(orderBy),
                    maxResults: RawData.MaxResults
                });
            }
            
            let results = collection.find(query).sort(orderBy);
            if (RawData.MaxResults >= 1) {
                results.limit(RawData.MaxResults);
            }
            
            const queryStartTime = Date.now();
            let theData = await results.toArray();
            const queryExecutionTime = Date.now() - queryStartTime;

            // Secure Objects: evaluate rule-gated candidates against the requester's player data.
            // ponytail: MaxResults limits the DB fetch before this filter, so a capped page can
            // return fewer results than MaxResults when rule-gated objects get filtered out.
            if (playerGuid !== null) {
                let playerDoc = null;
                if (theData.some(d => Array.isArray(d.AccessRules) && d.AccessRules.length > 0)) {
                    playerDoc = await getPlayer(playerGuid);
                }
                theData = theData.filter(d => accessGrants(d, playerGuid, playerDoc));
            }

            // Track query for index recommendations
            try {
                const analyzer = getAnalyzer();
                analyzer.recordQuery(COLL, query, orderBy, queryExecutionTime);
            } catch (analyzerErr) {
                // Don't fail the query if analyzer fails
                logger.debug('Failed to record query pattern', { error: analyzerErr.message });
            }
            
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
                    logger.info(`[QUERY][${mod}] Query returned NO RESULTS`, {
                        mod,
                        collection: COLL,
                        query: JSON.stringify(query),
                        returnColumn: ReturnCol
                    });
                    res.json({ Status: "Empty", Count: 0, Results: [] });
                } else {
                    logger.info(`[QUERY][${mod}] Query returned ${count} results`, {
                        mod,
                        collection: COLL,
                        returnColumn: ReturnCol,
                        resultCount: count
                    });
                    
                    // For territories, log IDs to track duplicates
                    if (mod === 'FactionTerritories') {
                        const territoryIds = ReturnData.map(t => {
                            if (t && t.ObjectId) return t.ObjectId;
                            if (t && t.flagId1) return `${t.flagId1}-${t.flagId2}-${t.flagId3}`;
                            return 'unknown';
                        });
                        logger.debug(`[QUERY][FactionTerritories] Territory IDs returned: ${JSON.stringify(territoryIds)}`);
                        
                        // Check for duplicates
                        const uniqueIds = new Set(territoryIds);
                        if (uniqueIds.size !== territoryIds.length) {
                            logger.error(`[QUERY][FactionTerritories] DUPLICATE TERRITORIES DETECTED IN QUERY RESULTS!`, {
                                totalReturned: territoryIds.length,
                                uniqueCount: uniqueIds.size,
                                ids: territoryIds
                            });
                        }
                    }
                    
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
            res.status(200).json({ Status: "Error", Count: 0, Results: [] });
        } finally {
            await client.close();
        }
    } else {
        logger.warn("Authentication failed during query", { mod });
        res.status(401).json({ Status: "Error", Error: "Invalid Auth", Count: 0, Results: [] });
    }
};

async function runUpdateFromQuery(req, res, mod, auth, COLL) {
    const isServer = CheckServerAuth(auth);
    if (isServer || ((await CheckAuth(auth)) && global.config.AllowClientWrite)) {
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
            // Reject server-side-JavaScript operators ($where etc.) before the
            // query reaches MongoDB.
            const badOp = findDangerousQueryOperator(query) || findDangerousQueryOperator(orderBy);
            if (badOp) {
                logger.warn(`Rejected update query containing disallowed operator`, { mod, operator: badOp });
                res.status(400).json({ Status: "Error", Error: `Query operator ${badOp} is not allowed`, Count: 0, Results: [] });
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

            // Secure Objects: player-auth query-updates can only touch objects the player
            // is allowlisted for (or public ones). Rule-gated objects are deliberately
            // excluded from player query-updates - rules can't be evaluated per-doc in updateMany.
            if (COLL === "Objects" && !isServer) {
                query = buildObjectPermissionClause(query, AuthPlayerGuid(auth), false);
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

            // Query-based update: apply to ALL documents matching the query,
            // not just the first one.
            const result = await collection.updateMany(query, updateDoc, options);
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
                res.status(200).json({ Status: "Empty", Element: element, Mod: mod, Count: 0 });
            }
        } catch (err) {
            logger.error(`Error during update operation: ${err.message}`, { error: err });
            res.status(200).json({ Status: "Error", Element: RawData.Element, Mod: mod, Count: 0 });
        } finally {
            await client.close();
        }
    } else {
        logger.warn("Authentication failed during update", { mod, url: req.url });
        res.status(401).json({ Status: "Error", Error: "Invalid Auth", Element: "", Mod: mod });
    }
};

/**
 * Wraps a player's query with the Secure Objects visibility clause so they only
 * match objects they are permitted to see. Server-auth queries never call this.
 *
 * @param {object} query - The (already Mod-scoped) user query.
 * @param {string} guid - The requesting player's normalized GUID.
 * @param {boolean} allowRuleCandidates - For reads (true): rule-gated objects pass
 *   as candidates and are post-filtered against the player's data. For bulk writes
 *   (false): rule-gated objects are excluded, since rules can't be evaluated per-doc
 *   in updateMany.
 * @returns {object} The query AND-ed with the permission clause.
 *
 * Perf note: the visibility $or ($exists:false / $size:0) is not index-selective on
 * its own, so it acts as a residual filter. This is bounded because the caller has
 * already added `Mod: <mod>` to the query, scoping the scan to a single mod's objects
 * (fine at game scale). If one mod's Objects collection ever grows large enough to
 * matter, add a partial index (e.g. on { Mod: 1, AllowedPlayers: 1 }) rather than
 * complicating this clause.
 */
function buildObjectPermissionClause(query, guid, allowRuleCandidates) {
    const visibility = [
        { AllowedPlayers: { "$exists": false } }, // legacy / never secured
        { AllowedPlayers: { "$size": 0 } },       // explicitly public
        { AllowedPlayers: guid }                  // on the allowlist
    ];
    if (allowRuleCandidates) {
        visibility.push({ AccessRules: { "$exists": true, "$ne": [] } });
        return { "$and": [query, { "$or": visibility }] };
    }
    return { "$and": [
        query,
        { "$or": visibility },
        { "$or": [{ AccessRules: { "$exists": false } }, { AccessRules: { "$size": 0 } }] }
    ] };
}

/**
 * Recursively processes the provided query object or array by prefixing keys with a given prefix.
 *
 * This function traverses the query structure and for each property that does not start with the
 * "$" character or the given prefix (followed by a dot), it renames the property by prepending the
 * prefix and a dot. For array values, each element is recursively processed.
 * Especially useful when the query needs to target specific fields within a nested structure.
 *
 * @param {(Object|Array|*)} query - The query structure to be processed, which can be an object, an array, or any value.
 * @param {string} prefix - The string prefix to add to keys that do not already match a specific pattern.
 * @returns {(Object|Array|*)} - The processed query with keys correctly prefixed.
 */
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
