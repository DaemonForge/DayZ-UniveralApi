/**
 * DirectKB – standalone MongoDB operations for KB management.
 *
 * This is a self-contained reimplementation of the subset of
 * UFServerServiceV2/models/kb.js that ufctl needs.  It avoids
 * require()'ing the service code directly so the CLI has zero
 * coupling to Express, Electron, global.logger, etc.
 *
 * Embedding generation uses OpenAI text-embedding-3-large  (same as
 * the service controller) so documents added via the CLI are fully
 * indexed and searchable immediately.
 *
 * Usage:
 *   const kb = require('./directKB')('mongodb://localhost:27017', 'DayZ', 'sk-...');
 *   const result = await kb.addDocument('myKb', 'readme.md', content);
 */

'use strict';

const { MongoClient, ObjectId } = require('mongodb');

const MAX_CHUNK_SIZE       = 22000;
const OVERLAP_SIZE         = 1200;
const EMBEDDING_DIMENSIONS = 3072;
const EMBEDDING_MODEL      = 'text-embedding-3-large';

module.exports = function createDirectKB(dbUri, dbName, openaiApiKey) {

    // ── OpenAI embeddings ────────────────────────────

    let _openai = null;
    function getOpenAI() {
        if (!openaiApiKey) return null;
        if (!_openai) {
            const OpenAI = require('openai');
            _openai = new OpenAI({ apiKey: openaiApiKey });
        }
        return _openai;
    }

    /**
     * Generate embeddings for an array of text chunks.
     * Retries up to 3 times on transient errors.
     * Returns null if OpenAI is not configured.
     *
     * IMPORTANT: Returns an array the same length as `texts`.
     * Empty/whitespace-only entries get a null embedding to keep
     * the indices aligned with the chunk array.
     */
    async function generateEmbeddings(texts, maxRetries = 3) {
        const ai = getOpenAI();
        if (!ai) return null;

        // Build a mapping so we can reassemble results in the original order
        const indexMap = [];  // positions in `texts` that have real content
        const valid = [];
        for (let i = 0; i < texts.length; i++) {
            if (texts[i] && texts[i].trim().length > 0) {
                indexMap.push(i);
                valid.push(texts[i]);
            }
        }
        if (valid.length === 0) return null;

        let lastError;
        for (let attempt = 1; attempt <= maxRetries; attempt++) {
            try {
                const response = await ai.embeddings.create({
                    model: EMBEDDING_MODEL,
                    input: valid,
                    dimensions: EMBEDDING_DIMENSIONS,
                });
                // Rebuild full-length array with nulls for skipped entries
                const result = new Array(texts.length).fill(null);
                for (let j = 0; j < response.data.length; j++) {
                    result[indexMap[j]] = response.data[j].embedding;
                }
                return result;
            } catch (err) {
                lastError = err;
                const retryable = err.status === 429 || err.status >= 500
                    || err.code === 'ETIMEDOUT' || err.code === 'ECONNRESET';
                if (retryable && attempt < maxRetries) {
                    await new Promise(r => setTimeout(r, 1000 * Math.pow(2, attempt - 1)));
                } else {
                    throw err;
                }
            }
        }
        throw lastError;
    }

    // ── helpers ──────────────────────────────────────

    async function connect() {
        const client = new MongoClient(dbUri);
        await client.connect();
        const db = client.db(dbName);
        return { client, db };
    }

    async function metaCollection() {
        const { client, db } = await connect();
        return { client, collection: db.collection('KBMetadata') };
    }

    async function kbCollection(kbId) {
        const { client, db } = await connect();
        const name = `KB_${kbId}`;
        return { client, collection: db.collection(name), name };
    }

    /**
     * Update the documentCount in KBMetadata.
     * Accepts an optional existing db handle to avoid opening a second connection.
     */
    async function updateDocumentCount(kbId, existingDb) {
        if (existingDb) {
            const col = existingDb.collection(`KB_${kbId}`);
            const meta = existingDb.collection('KBMetadata');
            const distinct = await col.distinct('documentId');
            await meta.updateOne(
                { kbId },
                { $set: { documentCount: distinct.length, updatedAt: new Date() } }
            );
            return;
        }
        const { client, db } = await connect();
        try {
            const col = db.collection(`KB_${kbId}`);
            const meta = db.collection('KBMetadata');
            const distinct = await col.distinct('documentId');
            await meta.updateOne(
                { kbId },
                { $set: { documentCount: distinct.length, updatedAt: new Date() } }
            );
        } finally {
            await client.close();
        }
    }

    function splitTextIntoChunks(text, maxSize = MAX_CHUNK_SIZE, overlap = OVERLAP_SIZE) {
        if (text.length <= maxSize) return [text];
        const chunks = [];
        let start = 0;
        while (start < text.length) {
            let end = Math.min(start + maxSize, text.length);
            if (end < text.length) {
                let bp = text.lastIndexOf('\n\n', end);
                if (bp > start + maxSize / 2) { end = bp + 2; }
                else {
                    bp = text.lastIndexOf('. ', end);
                    if (bp > start + maxSize / 2) { end = bp + 2; }
                    else {
                        bp = text.lastIndexOf(' ', end);
                        if (bp > start + maxSize / 2) { end = bp + 1; }
                    }
                }
            }
            chunks.push(text.substring(start, end).trim());
            start = end - overlap;
            if (start < 0) start = 0;
            if (start >= text.length || end >= text.length) break;
        }
        return chunks;
    }

    // ── Indexes (same as service) ────────────────────

    /**
     * Ensure indexes on a KB collection.
     * Accepts an optional existing collection handle to avoid a new connection.
     */
    async function ensureKBIndexes(kbId, existingCollection) {
        let client = null;
        let col = existingCollection;
        if (!col) {
            const r = await kbCollection(kbId);
            client = r.client;
            col = r.collection;
        }
        try {
            try {
                await col.createIndex(
                    { embedding: 1 },
                    { name: 'embedding_exists_idx', partialFilterExpression: { embedding: { $exists: true } } }
                );
            } catch (err) { if (err.code !== 85) { /* ignore index-already-exists */ } }

            try {
                await col.createIndex({ documentId: 1 }, { name: 'documentId_idx' });
            } catch (err) { if (err.code !== 85) { /* ignore */ } }

            try {
                await col.createIndex(
                    { content: 'text', name: 'text', contextHint: 'text' },
                    { name: 'text_search_idx' }
                );
            } catch (err) { if (err.code !== 85) { /* ignore */ } }
        } finally {
            if (client) await client.close();
        }
    }

    // ── KB CRUD ──────────────────────────────────────

    async function listKBs() {
        const { client, collection } = await metaCollection();
        try {
            return await collection.find({}).toArray();
        } finally {
            await client.close();
        }
    }

    async function getKB(kbId) {
        const { client, collection } = await metaCollection();
        try {
            return await collection.findOne({ kbId });
        } finally {
            await client.close();
        }
    }

    async function createKB(kbId, name, description = '') {
        if (!/^[a-zA-Z0-9_]+$/.test(kbId)) {
            throw new Error('KB ID must contain only alphanumeric characters and underscores');
        }
        const { client, collection } = await metaCollection();
        try {
            const existing = await collection.findOne({ kbId });
            if (existing) throw new Error(`KB '${kbId}' already exists`);
            const doc = {
                kbId,
                name,
                description,
                shorterAnswers: false,
                extractModel: 'gpt-4o-mini',
                documentCount: 0,
                createdAt: new Date(),
                updatedAt: new Date(),
            };
            await collection.insertOne(doc);
            // Create indexes for the new KB collection
            await ensureKBIndexes(kbId);
            return doc;
        } finally {
            await client.close();
        }
    }

    async function deleteKB(kbId) {
        const { client, db } = await connect();
        try {
            try { await db.collection(`KB_${kbId}`).drop(); } catch { /* may not exist */ }
            const r = await db.collection('KBMetadata').deleteOne({ kbId });
            return r.deletedCount > 0;
        } finally {
            await client.close();
        }
    }

    // ── Document CRUD ────────────────────────────────

    async function listDocuments(kbId) {
        const { client, collection } = await kbCollection(kbId);
        try {
            return await collection.aggregate([
                {
                    $group: {
                        _id: '$documentId',
                        name:           { $first: '$name' },
                        contextHint:    { $first: '$contextHint' },
                        fileType:       { $first: '$fileType' },
                        totalChunks:    { $max: '$totalChunks' },
                        totalCharCount: { $sum: '$charCount' },
                        hasEmbedding:   { $min: { $cond: [{ $ne: ['$embedding', null] }, 1, 0] } },
                        createdAt:      { $first: '$createdAt' },
                        updatedAt:      { $max: '$updatedAt' },
                    },
                },
                {
                    $project: {
                        _id: 0,
                        documentId:  '$_id',
                        name:        1,
                        contextHint: 1,
                        fileType:    1,
                        chunks:      '$totalChunks',
                        charCount:   '$totalCharCount',
                        hasEmbedding: { $eq: ['$hasEmbedding', 1] },
                        createdAt: 1,
                        updatedAt: 1,
                    },
                },
                { $sort: { createdAt: -1 } },
            ]).toArray();
        } finally {
            await client.close();
        }
    }

    async function addDocument(kbId, name, content, contextHint = '', fileType = 'txt') {
        const { client, db } = await connect();
        try {
            const collection = db.collection(`KB_${kbId}`);
            const documentId = new ObjectId().toString();
            const chunks = content.length > MAX_CHUNK_SIZE
                ? splitTextIntoChunks(content)
                : [content];

            // Generate embeddings (returns null if no API key)
            let embeddings = null;
            try {
                embeddings = await generateEmbeddings(chunks);
            } catch { /* will be null — can be backfilled later */ }

            const docs = chunks.map((chunk, i) => ({
                documentId,
                chunkIndex: i,
                totalChunks: chunks.length,
                name,
                content: chunk,
                contextHint,
                fileType,
                charCount: chunk.length,
                embedding: embeddings ? embeddings[i] : null,
                createdAt: new Date(),
                updatedAt: new Date(),
            }));

            await collection.insertMany(docs);

            // Reuse the same connection for count update & index creation
            await updateDocumentCount(kbId, db);
            try { await ensureKBIndexes(kbId, collection); } catch { /* best effort */ }

            return {
                documentId,
                name,
                chunks: chunks.length,
                charCount: content.length,
                hasEmbedding: embeddings !== null,
            };
        } finally {
            await client.close();
        }
    }

    async function deleteDocument(kbId, documentId) {
        const { client, db } = await connect();
        try {
            const r = await db.collection(`KB_${kbId}`).deleteMany({ documentId });
            await updateDocumentCount(kbId, db);
            return r.deletedCount > 0;
        } finally {
            await client.close();
        }
    }

    async function deleteAllDocuments(kbId) {
        const { client, db } = await connect();
        try {
            const r = await db.collection(`KB_${kbId}`).deleteMany({});
            await updateDocumentCount(kbId, db);
            return r.deletedCount;
        } finally {
            await client.close();
        }
    }

    /**
     * Update KB metadata fields.
     * Allowed: name, description, shorterAnswers, extractModel
     */
    async function updateKB(kbId, updates) {
        const { client, collection } = await metaCollection();
        try {
            const allowedFields = ['name', 'description', 'shorterAnswers', 'extractModel'];
            const sanitized = {};
            for (const field of allowedFields) {
                if (updates[field] !== undefined) {
                    sanitized[field] = updates[field];
                }
            }
            sanitized.updatedAt = new Date();
            const r = await collection.updateOne({ kbId }, { $set: sanitized });
            return r.modifiedCount > 0;
        } finally {
            await client.close();
        }
    }

    // ── public API ───────────────────────────────────

    return {
        listKBs,
        getKB,
        createKB,
        deleteKB,
        updateKB,
        listDocuments,
        addDocument,
        deleteDocument,
        deleteAllDocuments,
        ensureKBIndexes,
    };
};
