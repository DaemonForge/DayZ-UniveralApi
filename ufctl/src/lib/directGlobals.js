/**
 * ufctl – Direct MongoDB access for the Globals collection.
 *
 * Mirrors the service model (models/global.js) but is self-contained
 * so ufctl never requires service code at runtime.
 *
 * Factory usage:
 *   const globals = require('./directGlobals')(dbUri, dbName);
 *   const list = await globals.listGlobals();
 */

'use strict';

const { MongoClient } = require('mongodb');

module.exports = function createDirectGlobals(dbUri, dbName) {

    // ────────────────────────────────────────────────
    // Helpers
    // ────────────────────────────────────────────────

    async function connect() {
        const client = new MongoClient(dbUri);
        await client.connect();
        const db = client.db(dbName);
        const collection = db.collection('Globals');
        return { client, db, collection };
    }

    function normalizeId(id) {
        if (id && typeof id === 'object' && typeof id.toHexString === 'function') {
            return id.toHexString();
        }
        if (id === undefined || id === null) return '';
        return String(id);
    }

    // ────────────────────────────────────────────────
    // Public API
    // ────────────────────────────────────────────────

    /**
     * List all global documents – returns [ { id, mod } ].
     */
    async function listGlobals() {
        const { client, collection } = await connect();
        try {
            const docs = await collection
                .find({}, { projection: { Mod: 1 } })
                .sort({ Mod: 1 })
                .toArray();
            return docs.map(doc => ({
                id:  normalizeId(doc._id),
                mod: doc.Mod || '',
            }));
        } finally {
            await client.close();
        }
    }

    /**
     * Get the full global document for a module.
     * Returns { id, mod, data } or null.
     */
    async function getGlobal(mod) {
        const { client, collection } = await connect();
        try {
            const doc = await collection.findOne({ Mod: mod });
            if (!doc) return null;
            return {
                id:   normalizeId(doc._id),
                mod:  doc.Mod,
                data: doc.Data ?? {},
            };
        } finally {
            await client.close();
        }
    }

    /**
     * Set / replace the Data payload for a module (upsert).
     * Returns true on success.
     */
    async function setGlobal(mod, data) {
        const { client, collection } = await connect();
        try {
            const result = await collection.updateOne(
                { Mod: mod },
                { $set: { Mod: mod, Data: data } },
                { upsert: true },
            );
            return (result.matchedCount === 1 || result.upsertedCount === 1);
        } finally {
            await client.close();
        }
    }

    /**
     * Set a single parameter inside Data.
     * Supports dotted paths (e.g. "Stats.Kills").
     * Returns the new value of the field, or null on failure.
     */
    async function setParam(mod, element, value) {
        const { client, collection } = await connect();
        try {
            const field = 'Data.' + element;
            const result = await collection.updateOne(
                { Mod: mod },
                { $set: { [field]: value } },
                { upsert: false },
            );
            if (result.matchedCount === 0) return null;
            // Read back the updated value
            const doc = await collection.findOne(
                { Mod: mod },
                { projection: { [field]: 1 } },
            );
            // Navigate the dotted path through doc.Data
            const parts = element.split('.');
            let current = doc?.Data;
            for (const p of parts) {
                if (current == null) return null;
                current = current[p];
            }
            return current;
        } finally {
            await client.close();
        }
    }

    /**
     * Atomic increment on a numeric field inside Data.
     * Returns the new value, or null on failure.
     */
    async function transactionGlobal(mod, element, amount) {
        const { client, collection } = await connect();
        try {
            const field = 'Data.' + element;
            await collection.updateOne(
                { Mod: mod },
                { $inc: { [field]: amount } },
                { upsert: false },
            );
            const doc = await collection.findOne(
                { Mod: mod },
                { projection: { [field]: 1 } },
            );
            const parts = element.split('.');
            let current = doc?.Data;
            for (const p of parts) {
                if (current == null) return null;
                current = current[p];
            }
            return current;
        } finally {
            await client.close();
        }
    }

    /**
     * Delete the global document for a module.
     * Returns true if a document was removed.
     */
    async function deleteGlobal(mod) {
        const { client, collection } = await connect();
        try {
            const result = await collection.deleteOne({ Mod: mod });
            return result.deletedCount === 1;
        } finally {
            await client.close();
        }
    }

    return {
        listGlobals,
        getGlobal,
        setGlobal,
        setParam,
        transactionGlobal,
        deleteGlobal,
    };
};
