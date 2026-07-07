// controllers/objectController.js

const { Router } = require('express');
const router = Router();

const { requireServerAuth, requirePlayerOrServerAuth } = require('../auth/utils');
const { updateObject, runObjectTransaction, runValidatedObjectTransaction, getObject, getObjectFull, setObjectAccess, newObject, updateObjectField, deleteObject } = require('../models/object');
const { getPlayer } = require('../models/player');
const { makeObjectId, isEmpty, createLogger, tryConvertToObject, normalizeGuidList, validAccessRule, normalizeRuleOp, accessGrants } = require('../utils');
const logger = createLogger(global.logger, 'c.object');

// Mount the legacy query handler if needed
const queryHandler = require('./query');
router.use('/Query', queryHandler);

/**
 * POST /Load/:ObjectId/:mod
 * Loads an object. If not found and request is from a server, 
 * a new object can be created.
 * Both players and servers can load, but only servers can create.
 */
router.post('/Load/:ObjectId/:mod', requirePlayerOrServerAuth, loadObject);

/**
 * POST /Save/:ObjectId/:mod
 * Upserts an object with the provided data.
 * Only servers are allowed to save.
 */
router.post('/Save/:ObjectId/:mod', requireServerAuth, saveObject);

/**
 * POST /SecureSave/:ObjectId/:mod
 * Upserts an object together with its access control (Secure Objects).
 * Body: { Access: { AllowedPlayers: [...], AccessRules: [...] }, Data: {...} }
 * Only servers are allowed to save.
 */
router.post('/SecureSave/:ObjectId/:mod', requireServerAuth, secureSaveObject);

/**
 * POST /SetAccess/:ObjectId/:mod
 * Replaces the access control fields of an existing object.
 * Body: { AllowedPlayers: [...], AccessRules: [...] }
 * Only servers are allowed to change access.
 */
router.post('/SetAccess/:ObjectId/:mod', requireServerAuth, runSetAccess);

/**
 * POST /Update/:ObjectId/:mod
 * Updates a specific field in an object document using a designated operation.
 * Only servers are allowed to update.
 */
router.post('/Update/:ObjectId/:mod', requireServerAuth, runUpdate);

/**
 * POST /Transaction/:ObjectId/:mod
 * This function processes incoming transaction requests for objects in the DayZ universe.
 * It determines whether to run a validated transaction (when Min/Max range is specified)
 * or a standard transaction based on the provided data.
 */
router.post('/Transaction/:ObjectId/:mod', requireServerAuth, runTransaction);

/**
 * POST /Delete/:ObjectId/:mod
 * Deletes an object from the database
 * Only servers are allowed to delete.
 */
router.post('/Delete/:ObjectId/:mod', requireServerAuth, deleteObjectHandler);

/**
 * Loads an object with the specified ID and mod, creating a new one if requested
 */
async function loadObject(req, res) {
    let { ObjectId, mod } = req.params;
    const data = req.body;
    
    logger.info(`[LOAD][${mod}] Load object request`, { 
        mod, 
        ObjectId, 
        isServer: req.isServer,
        hasBodyData: !isEmpty(data),
        timestamp: new Date().toISOString()
    });
    
    try {
        const doc = await getObjectFull(ObjectId, mod);
        const results = doc ? doc.data : undefined;

        if (results === null || typeof results === 'undefined') {
            logger.info(`[LOAD][${mod}] Object NOT FOUND in database`, { mod, ObjectId });
            
            if (req.isServer && !isEmpty(data)) {
                if (ObjectId === "NewObject") {
                    ObjectId = makeObjectId();
                    data.ObjectId = ObjectId;
                    logger.info(`[LOAD][${mod}] Creating NEW object with generated ID`, { mod, ObjectId });
                } else {
                    logger.warn(`[LOAD][${mod}] Creating NEW object with PROVIDED ID (potential duplicate risk!)`, { mod, ObjectId, data: JSON.stringify(data) });
                }
                await newObject(ObjectId, mod, data);
                logger.info(`[LOAD][${mod}] NEW OBJECT CREATED via Load endpoint`, { mod, ObjectId, timestamp: new Date().toISOString() });
                return res.status(200).json(data);
            } else {
                logger.debug(`[LOAD][${mod}] Object not found, no creation (not server or no data)`, { mod, ObjectId, isServer: req.isServer });
                return res.status(200).json(data);
            }
        } else {
            // Secure Objects: enforce per-player access for restricted objects (server bypasses)
            if (!req.isServer) {
                let playerDoc = null;
                const rules = doc.AccessRules || [];
                const list = doc.AllowedPlayers || [];
                if (rules.length > 0 && !list.includes(req.GUID)) {
                    playerDoc = await getPlayer(req.GUID);
                }
                if (!accessGrants(doc, req.GUID, playerDoc)) {
                    logger.warn(`[LOAD][${mod}] Player not permitted for object`, { mod, ObjectId, GUID: req.GUID });
                    return res.status(403).json({ Status: "NoPerms", Error: "Not permitted for this object" });
                }
            }
            logger.info(`[LOAD][${mod}] Object FOUND - returning existing data`, { mod, ObjectId, hasData: !!results });
            return res.status(200).json(results);
        }
    } catch (err) {
        logger.error(`Error in loadObject endpoint: ${err.message}`, { error: err, mod, ObjectId });
        return res.status(500).json({ error: err.message });
    }
}

/**
 * Saves an object with the provided data, creating a new one if needed
 */
async function saveObject(req, res) {
    let { ObjectId, mod } = req.params;
    const data = req.body;
    
    // Enhanced logging for territory duplication debugging
    logger.info(`[SAVE] Object save request started`, { 
        mod, 
        ObjectId, 
        dataKeys: Object.keys(data || {}),
        timestamp: new Date().toISOString()
    });
    
    // Log full data if mod is FactionTerritories to trace duplicates
    if (mod === 'FactionTerritories' || mod === 'Factions') {
        logger.info(`[SAVE][${mod}] Full save data:`, { 
            mod, 
            ObjectId, 
            fullData: JSON.stringify(data),
            stackPreview: new Error().stack.split('\n').slice(2, 5).join('\n')
        });
    }
    
    try {
        // Check if object already exists BEFORE saving
        const existingObject = await getObject(ObjectId, mod);
        if (existingObject) {
            logger.warn(`[SAVE][${mod}] DUPLICATE DETECTED - Object already exists!`, {
                mod,
                ObjectId,
                existingData: JSON.stringify(existingObject),
                newData: JSON.stringify(data)
            });
        }
        
        if (ObjectId === "NewObject") {
            ObjectId = makeObjectId();
            data.ObjectId = ObjectId;
            logger.info(`[SAVE] Generated new ObjectId: ${ObjectId} for new object`, { mod });
        }
        const options = { upsert: true };
        const updateDoc = { $set: { data: data, ObjectId, Mod: mod } };
        const result = await updateObject(ObjectId, mod, updateDoc, options);
        
        // Detailed result logging
        logger.info(`[SAVE][${mod}] Save operation completed`, {
            mod,
            ObjectId,
            matchedCount: result.matchedCount,
            upsertedCount: result.upsertedCount,
            modifiedCount: result.modifiedCount,
            wasUpdate: result.matchedCount === 1,
            wasInsert: result.upsertedCount === 1,
            timestamp: new Date().toISOString()
        });
        
        if (result.matchedCount === 1 || result.upsertedCount === 1) {
            if (result.upsertedCount === 1) {
                logger.info(`[SAVE][${mod}] NEW RECORD CREATED`, { mod, ObjectId });
            } else {
                logger.info(`[SAVE][${mod}] EXISTING RECORD UPDATED`, { mod, ObjectId });
            }
            res.status(200).json(data);
        } else {
            logger.error(`[SAVE][${mod}] SAVE FAILED - No match or upsert!`, { mod, ObjectId, result });
            res.status(200).json(data);
        }
    } catch (err) {
        logger.error(`Error in Save endpoint: ${err.message}`, { error: err, mod, ObjectId });
        res.status(500).json(data);
    }
}

/**
 * Normalizes and validates an access block from a request body.
 * Returns { AllowedPlayers, AccessRules } or { error } on invalid rules.
 * SteamID64s in AllowedPlayers are normalized to DayZ GUIDs; ops are
 * normalized to canonical form (EQUAL -> =, NOTIN -> notin, ...).
 * AccessRules may arrive flat [rule, ...] (AND shorthand) or grouped
 * [{ Rules: [...] }, ...] (OR of ANDs) - mixing the two shapes is rejected.
 * Flat input is canonicalized to a single group before storage, so persisted
 * documents always carry the grouped shape.
 */
const RULE_ERROR = "Invalid access rule - Mod/Field must be non-empty strings and Op one of =, !=, >, >=, <, <=, in, notin, contains, notcontains, exists (aliases like EQUAL/NOTIN/GTE accepted)";

function normalizeRule(rule) {
    if (!validAccessRule(rule)) return null;
    return { Mod: rule.Mod, Field: rule.Field, Op: normalizeRuleOp(rule.Op), Value: `${rule.Value ?? ""}` };
}

function parseAccessBody(access) {
    const rawRules = Array.isArray(access.AccessRules) ? access.AccessRules : [];
    let rules = [];
    if (rawRules.some(r => r && Array.isArray(r.Rules))) {
        // Grouped shape: every entry must be a group; empty groups are dropped
        if (!rawRules.every(r => r && Array.isArray(r.Rules))) {
            return { error: "AccessRules cannot mix plain rules and groups - wrap every rule in a { Rules: [...] } group" };
        }
        for (const group of rawRules) {
            const groupRules = group.Rules.map(normalizeRule);
            if (groupRules.includes(null)) return { error: RULE_ERROR };
            if (groupRules.length > 0) rules.push({ Rules: groupRules });
        }
    } else {
        const flat = rawRules.map(normalizeRule);
        if (flat.includes(null)) return { error: RULE_ERROR };
        // Flat rules are accepted as shorthand but stored canonically as a single
        // group, so persisted AccessRules always have one shape.
        if (flat.length > 0) rules = [{ Rules: flat }];
    }
    return {
        AllowedPlayers: normalizeGuidList(access.AllowedPlayers),
        AccessRules: rules
    };
}

/**
 * Upserts an object together with its access control fields (Secure Objects).
 * Body: { Access: { AllowedPlayers, AccessRules }, Data: {...} }
 */
async function secureSaveObject(req, res) {
    let { ObjectId, mod } = req.params;
    const { Access, Data } = req.body;

    logger.info(`[SECURESAVE][${mod}] Secure save request`, { mod, ObjectId });

    if (isEmpty(Data) || typeof Data !== 'object') {
        return res.status(400).json({ Status: "Error", Error: "Data object is required" });
    }
    const access = parseAccessBody(Access || {});
    if (access.error) {
        logger.warn(`[SECURESAVE][${mod}] Invalid access block`, { mod, ObjectId, error: access.error });
        return res.status(400).json({ Status: "Error", Error: access.error });
    }

    try {
        if (ObjectId === "NewObject") {
            ObjectId = makeObjectId();
            Data.ObjectId = ObjectId;
            logger.info(`[SECURESAVE] Generated new ObjectId: ${ObjectId}`, { mod });
        }
        const updateDoc = { $set: { data: Data, ObjectId, Mod: mod, AllowedPlayers: access.AllowedPlayers, AccessRules: access.AccessRules } };
        const result = await updateObject(ObjectId, mod, updateDoc, { upsert: true });

        logger.info(`[SECURESAVE][${mod}] Save completed`, {
            mod,
            ObjectId,
            allowedCount: access.AllowedPlayers.length,
            ruleCount: access.AccessRules.length,
            wasInsert: result.upsertedCount === 1
        });
        return res.status(200).json(Data);
    } catch (err) {
        logger.error(`Error in SecureSave endpoint: ${err.message}`, { error: err, mod, ObjectId });
        return res.status(500).json({ Status: "Error", Error: err.message });
    }
}

/**
 * Replaces the access control fields of an existing object.
 * Body: { AllowedPlayers, AccessRules }
 */
async function runSetAccess(req, res) {
    const { ObjectId, mod } = req.params;

    logger.info(`[SETACCESS][${mod}] Set access request`, { mod, ObjectId });

    const access = parseAccessBody(req.body || {});
    if (access.error) {
        logger.warn(`[SETACCESS][${mod}] Invalid access block`, { mod, ObjectId, error: access.error });
        return res.status(400).json({ Status: "Error", Error: access.error });
    }

    try {
        const found = await setObjectAccess(ObjectId, mod, access);
        if (!found) {
            logger.warn(`[SETACCESS][${mod}] Object not found`, { mod, ObjectId });
            return res.status(404).json({ Status: "NotFound", Error: "Object not found", ID: ObjectId, Mod: mod });
        }
        return res.status(200).json({ Status: "Success", ID: ObjectId, Mod: mod });
    } catch (err) {
        logger.error(`Error in SetAccess endpoint: ${err.message}`, { error: err, mod, ObjectId });
        return res.status(500).json({ Status: "Error", Error: err.message, ID: ObjectId, Mod: mod });
    }
}

/**
 * Executes a field update operation on a game object
 */
async function runUpdate(req, res) {
    let { ObjectId, mod } = req.params;
    const data = req.body;
    const element = data.Element;
    logger.info(`Object update request`, { mod, ObjectId, element });
    try {
        const operation = data.Operation || "set";
        const value = tryConvertToObject(data.Value);
        logger.debug(`Updating element: ${element} using operation: ${operation} with value: ${JSON.stringify(value)}`);
        const result = await updateObjectField(ObjectId, mod, element, operation, value);
        logger.debug(`updateObjectField result: ${JSON.stringify(result)}`);
        if (result.matchedCount >= 1 || result.upsertedCount >= 1) {
            logger.debug(`Updated ${element} for mod: ${mod}, ObjectId: ${ObjectId}`);
            res.status(200).json({ Status: "Success", Element: element, Mod: mod, ID: ObjectId });
        } else {
            logger.warn(`Error updating ${element} for mod: ${mod}, ObjectId: ${ObjectId}`);
            res.status(200).json({ Status: "NotFound", Element: element, Mod: mod, ID: ObjectId });
        }
    } catch (err) {
        logger.error(`Error in Update endpoint: ${err.message}`, { error: err, mod, ObjectId });
        res.status(200).json({ Status: "Error", Element: data.Element, Mod: mod, ID: ObjectId });
    }
}

/**
 * Executes a transaction on a game object based on request data
 */
async function runTransaction(req, res) {
    let { ObjectId, mod } = req.params;
    const data = req.body;
    logger.info(`Object transaction request`, { mod, ObjectId });
    try {
        let response;
        if (data.Min !== undefined && data.Max !== undefined) {
            logger.debug(`Running validated transaction with Min: ${data.Min} and Max: ${data.Max}`);
            response = await runValidatedObjectTransaction(data, ObjectId, mod);
        } else {
            logger.debug(`Running standard transaction`);
            response = await runObjectTransaction(data, ObjectId, mod);
        }
        logger.debug(`Transaction response: ${JSON.stringify(response)}`);
        res.json(response);
    } catch (err) {
        logger.error(`Transaction error: ${err.message}`, { error: err, mod, id: ObjectId });
        res.status(500).json({ Status: "Error", Error: err.message, ID: ObjectId, Mod: mod, Value: 0, Element: data.Element });
    }
}

/**
 * Deletes an object from the database
 */
async function deleteObjectHandler(req, res) {
    const { ObjectId, mod } = req.params;
    
    logger.info(`[DELETE][${mod}] Delete object request`, { 
        mod, 
        ObjectId,
        timestamp: new Date().toISOString()
    });
    
    try {
        const result = await deleteObject(ObjectId, mod);
        
        if (!result.success || !result.deleted) {
            logger.warn(`[DELETE][${mod}] Object not found or not deleted`, { mod, ObjectId, result });
            return res.status(404).json({ 
                error: 'Object not found',
                deleted: false,
                deletedCount: 0
            });
        }
        
        logger.info(`[DELETE][${mod}] Object deleted successfully`, { 
            mod, 
            ObjectId, 
            deletedCount: result.deletedCount,
            timestamp: new Date().toISOString()
        });
        
        return res.status(200).json({
            success: true,
            deleted: true,
            deletedCount: result.deletedCount
        });
    } catch (err) {
        logger.error(`Error in deleteObject endpoint: ${err.message}`, { error: err, mod, ObjectId });
        return res.status(500).json({ error: err.message });
    }
}

module.exports = router;
