/**
 * ufctl – Direct MongoDB access for mod data management.
 *
 * Mirrors the service model (models/modData.js) but is self-contained
 * so ufctl never requires service code at runtime.
 *
 * Factory usage:
 *   const data = require('./directData')(dbUri, dbName);
 *   const mods = await data.scanInstalledMods();
 */

'use strict';

const { MongoClient } = require('mongodb');

// System/backend collections to exclude from mod management
const SYSTEM_MODS = new Set([
    'AUTH', 'auth', 'Auth',
    'System', 'system', 'SYSTEM',
    'UniversalApiStatus', 'universalapistatus',
]);

module.exports = function createDirectData(dbUri, dbName) {

    // ────────────────────────────────────────────────
    // Helpers
    // ────────────────────────────────────────────────

    async function connect() {
        const client = new MongoClient(dbUri);
        await client.connect();
        const db = client.db(dbName);
        return { client, db };
    }

    // ────────────────────────────────────────────────
    // Public API
    // ────────────────────────────────────────────────

    /**
     * Scans all collections and returns a list of installed mods with data counts.
     * @returns {Promise<Array>} [{ modName, collections: { name: count }, totalDocuments }]
     */
    async function scanInstalledMods() {
        const { client, db } = await connect();
        try {
            const modMap = new Map();

            const addModData = (modName, collection, count) => {
                if (SYSTEM_MODS.has(modName)) return;
                if (!modMap.has(modName)) {
                    modMap.set(modName, {
                        modName,
                        collections: {},
                        totalDocuments: 0,
                    });
                }
                const entry = modMap.get(modName);
                entry.collections[collection] = count;
                entry.totalDocuments += count;
            };

            // 1. Objects
            try {
                const coll = db.collection('Objects');
                const mods = await coll.aggregate([
                    { $group: { _id: '$Mod', count: { $sum: 1 } } }
                ]).toArray();
                for (const m of mods) { if (m._id) addModData(m._id, 'Objects', m.count); }
            } catch { /* skip */ }

            // 2. Players — mod subdocuments
            try {
                const coll = db.collection('Players');
                const players = await coll.find({}, { projection: {} }).toArray();
                for (const player of players) {
                    for (const key of Object.keys(player)) {
                        if (key === '_id' || key === 'GUID' || key === 'LastUpdate') continue;
                        if (!modMap.has(key)) addModData(key, 'Players (subdocuments)', 0);
                        const entry = modMap.get(key);
                        if (entry) {
                            entry.collections['Players (subdocuments)'] =
                                (entry.collections['Players (subdocuments)'] || 0) + 1;
                            entry.totalDocuments += 1;
                        }
                    }
                }
            } catch { /* skip */ }

            // 3. Messages
            try {
                const coll = db.collection('Messages');
                const mods = await coll.aggregate([
                    { $group: { _id: '$Mod', count: { $sum: 1 } } }
                ]).toArray();
                for (const m of mods) { if (m._id) addModData(m._id, 'Messages', m.count); }
            } catch { /* skip */ }

            // 4. MessagesMeta
            try {
                const coll = db.collection('MessagesMeta');
                const mods = await coll.aggregate([
                    { $group: { _id: '$Mod', count: { $sum: 1 } } }
                ]).toArray();
                for (const m of mods) { if (m._id) addModData(m._id, 'MessagesMeta', m.count); }
            } catch { /* skip */ }

            // 5. PlayerMessagesStatus
            try {
                const coll = db.collection('PlayerMessagesStatus');
                const mods = await coll.aggregate([
                    { $group: { _id: '$Mod', count: { $sum: 1 } } }
                ]).toArray();
                for (const m of mods) { if (m._id) addModData(m._id, 'PlayerMessagesStatus', m.count); }
            } catch { /* skip */ }

            // 6. AIChats
            try {
                const coll = db.collection('AIChats');
                const mods = await coll.aggregate([
                    { $group: { _id: '$mod', count: { $sum: 1 } } }
                ]).toArray();
                for (const m of mods) { if (m._id) addModData(m._id, 'AIChats', m.count); }
            } catch { /* skip */ }

            // 7. AIMessages
            try {
                const coll = db.collection('AIMessages');
                const mods = await coll.aggregate([
                    { $group: { _id: '$mod', count: { $sum: 1 } } }
                ]).toArray();
                for (const m of mods) { if (m._id) addModData(m._id, 'AIMessages', m.count); }
            } catch { /* skip */ }

            // 8. AIAssistantThreads
            try {
                const coll = db.collection('AIAssistantThreads');
                const mods = await coll.aggregate([
                    { $group: { _id: '$mod', count: { $sum: 1 } } }
                ]).toArray();
                for (const m of mods) { if (m._id) addModData(m._id, 'AIAssistantThreads', m.count); }
            } catch { /* skip */ }

            return Array.from(modMap.values()).sort((a, b) =>
                a.modName.localeCompare(b.modName)
            );
        } finally {
            await client.close();
        }
    }

    /**
     * Deletes all data for a specific mod across all collections.
     * @param {string} modName
     * @returns {Promise<Object>} { modName, collections: { name: deletedCount }, totalDeleted, errors }
     */
    async function deleteModData(modName) {
        if (SYSTEM_MODS.has(modName)) {
            throw new Error('Cannot delete system/backend data. AUTH and System collections are protected.');
        }

        const { client, db } = await connect();
        try {
            const summary = {
                modName,
                collections: {},
                totalDeleted: 0,
                errors: [],
            };

            // 1. Objects
            try {
                const result = await db.collection('Objects').deleteMany({ Mod: modName });
                summary.collections['Objects'] = result.deletedCount;
                summary.totalDeleted += result.deletedCount;
            } catch (err) {
                summary.errors.push({ collection: 'Objects', error: err.message });
            }

            // 2. Players — only remove the mod subdocument, NOT the player
            try {
                const result = await db.collection('Players').updateMany(
                    {},
                    { $unset: { [modName]: '' } }
                );
                summary.collections['Players (subdocuments)'] = result.modifiedCount;
                summary.totalDeleted += result.modifiedCount;
            } catch (err) {
                summary.errors.push({ collection: 'Players (subdocuments)', error: err.message });
            }

            // 3. Messages
            try {
                const result = await db.collection('Messages').deleteMany({ Mod: modName });
                summary.collections['Messages'] = result.deletedCount;
                summary.totalDeleted += result.deletedCount;
            } catch (err) {
                summary.errors.push({ collection: 'Messages', error: err.message });
            }

            // 4. MessagesMeta
            try {
                const result = await db.collection('MessagesMeta').deleteMany({ Mod: modName });
                summary.collections['MessagesMeta'] = result.deletedCount;
                summary.totalDeleted += result.deletedCount;
            } catch (err) {
                summary.errors.push({ collection: 'MessagesMeta', error: err.message });
            }

            // 5. PlayerMessagesStatus
            try {
                const result = await db.collection('PlayerMessagesStatus').deleteMany({ Mod: modName });
                summary.collections['PlayerMessagesStatus'] = result.deletedCount;
                summary.totalDeleted += result.deletedCount;
            } catch (err) {
                summary.errors.push({ collection: 'PlayerMessagesStatus', error: err.message });
            }

            // 6. AIChats
            try {
                const result = await db.collection('AIChats').deleteMany({ mod: modName });
                summary.collections['AIChats'] = result.deletedCount;
                summary.totalDeleted += result.deletedCount;
            } catch (err) {
                summary.errors.push({ collection: 'AIChats', error: err.message });
            }

            // 7. AIMessages
            try {
                const result = await db.collection('AIMessages').deleteMany({ mod: modName });
                summary.collections['AIMessages'] = result.deletedCount;
                summary.totalDeleted += result.deletedCount;
            } catch (err) {
                summary.errors.push({ collection: 'AIMessages', error: err.message });
            }

            // 8. AIAssistantThreads
            try {
                const result = await db.collection('AIAssistantThreads').deleteMany({ mod: modName });
                summary.collections['AIAssistantThreads'] = result.deletedCount;
                summary.totalDeleted += result.deletedCount;
            } catch (err) {
                summary.errors.push({ collection: 'AIAssistantThreads', error: err.message });
            }

            return summary;
        } finally {
            await client.close();
        }
    }

    return {
        scanInstalledMods,
        deleteModData,
    };
};
