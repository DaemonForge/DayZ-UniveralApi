/**
 * ufctl – Direct MongoDB access for index management.
 *
 * Mirrors the service model (models/indexManager.js) but is self-contained
 * so ufctl never requires service code at runtime.
 *
 * Factory usage:
 *   const idx = require('./directIndex')(dbUri, dbName);
 *   const collections = await idx.getCollections();
 */

'use strict';

const { MongoClient } = require('mongodb');

module.exports = function createDirectIndex(dbUri, dbName) {

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
     * Lists all collections in the database.
     * @returns {Promise<string[]>}
     */
    async function getCollections() {
        const { client, db } = await connect();
        try {
            const collections = await db.listCollections().toArray();
            return collections.map(c => c.name).sort();
        } finally {
            await client.close();
        }
    }

    /**
     * Gets all indexes for a specific collection.
     * @param {string} collectionName
     * @returns {Promise<Array>}
     */
    async function getIndexes(collectionName) {
        const { client, db } = await connect();
        try {
            const collection = db.collection(collectionName);
            const indexes = await collection.indexes();

            return indexes.map(idx => ({
                name: idx.name,
                key: idx.key,
                unique: idx.unique || false,
                sparse: idx.sparse || false,
                expireAfterSeconds: idx.expireAfterSeconds,
                fields: Object.keys(idx.key),
            }));
        } finally {
            await client.close();
        }
    }

    /**
     * Gets all indexes for all collections.
     * @returns {Promise<Object>} { collectionName: [indexes] }
     */
    async function getAllIndexes() {
        const { client, db } = await connect();
        try {
            const collections = await db.listCollections().toArray();
            const result = {};

            for (const col of collections) {
                const collection = db.collection(col.name);
                const indexes = await collection.indexes();

                result[col.name] = indexes.map(idx => ({
                    name: idx.name,
                    key: idx.key,
                    unique: idx.unique || false,
                    sparse: idx.sparse || false,
                    expireAfterSeconds: idx.expireAfterSeconds,
                    fields: Object.keys(idx.key),
                }));
            }

            return result;
        } finally {
            await client.close();
        }
    }

    /**
     * Gets index usage statistics via $indexStats aggregation.
     * @param {string} collectionName
     * @returns {Promise<Array>}
     */
    async function getIndexStats(collectionName) {
        const { client, db } = await connect();
        try {
            const collection = db.collection(collectionName);
            const stats = await collection.aggregate([{ $indexStats: {} }]).toArray();
            return stats;
        } catch (err) {
            // $indexStats may not be supported in all environments
            return [];
        } finally {
            await client.close();
        }
    }

    /**
     * Analyzes a collection — returns indexes, usage stats, and collection stats.
     * @param {string} collectionName
     * @returns {Promise<Object>}
     */
    async function analyzeCollection(collectionName) {
        const { client, db } = await connect();
        try {
            const collection = db.collection(collectionName);
            const indexes = await collection.indexes();
            let stats = [];
            try {
                stats = await collection.aggregate([{ $indexStats: {} }]).toArray();
            } catch { /* ignore */ }

            let collStats = {};
            try {
                collStats = await db.command({ collStats: collectionName });
            } catch { /* ignore */ }

            return {
                collection: collectionName,
                documentCount: collStats.count || 0,
                avgDocumentSize: collStats.avgObjSize || 0,
                totalSize: collStats.size || 0,
                indexCount: indexes.length,
                indexes: indexes.map(idx => {
                    const usage = stats.find(s => s.name === idx.name);
                    return {
                        name: idx.name,
                        key: idx.key,
                        unique: idx.unique || false,
                        sparse: idx.sparse || false,
                        expireAfterSeconds: idx.expireAfterSeconds,
                        fields: Object.keys(idx.key),
                        usageStats: usage ? {
                            ops: usage.accesses.ops,
                            since: usage.accesses.since,
                        } : null,
                    };
                }),
            };
        } finally {
            await client.close();
        }
    }

    /**
     * Creates an index on a collection.
     * @param {string} collectionName
     * @param {Object} indexSpec - e.g. { Mod: 1, OID: 1 }
     * @param {Object} options - e.g. { unique: true, background: true }
     * @returns {Promise<Object>} { success, indexName }
     */
    async function createIndex(collectionName, indexSpec, options = {}) {
        const { client, db } = await connect();
        try {
            // Validate collection exists
            const collections = await db.listCollections().toArray();
            const validNames = collections.map(c => c.name);
            if (!validNames.includes(collectionName)) {
                throw new Error(`Collection "${collectionName}" does not exist.`);
            }

            if (!indexSpec || typeof indexSpec !== 'object') {
                throw new Error('Invalid index specification.');
            }

            const fieldCount = Object.keys(indexSpec).length;
            if (fieldCount === 0) {
                throw new Error('Index must have at least one field.');
            }
            if (fieldCount > 10) {
                throw new Error('Index cannot have more than 10 fields (performance concern).');
            }

            const collection = db.collection(collectionName);
            const result = await collection.createIndex(indexSpec, options);
            return { success: true, indexName: result };
        } finally {
            await client.close();
        }
    }

    /**
     * Drops an index from a collection.
     * @param {string} collectionName
     * @param {string} indexName
     * @returns {Promise<Object>} { success }
     */
    async function dropIndex(collectionName, indexName) {
        const { client, db } = await connect();
        try {
            if (indexName === '_id_') {
                throw new Error('Cannot drop _id index.');
            }

            const collection = db.collection(collectionName);
            await collection.dropIndex(indexName);
            return { success: true };
        } finally {
            await client.close();
        }
    }

    return {
        getCollections,
        getIndexes,
        getAllIndexes,
        getIndexStats,
        analyzeCollection,
        createIndex,
        dropIndex,
    };
};
