/**
 * Knowledge Base (KB) Model
 * Handles all MongoDB operations for KB documents including:
 * - Vector embeddings using text-embedding-3-large
 * - Vector search using MongoDB Atlas Search
 * - Document management (CRUD)
 */

const { MongoClient, ObjectId } = require('mongodb');
const config = require('../config'); // Loads config.json - expects { DBServer, DB }
const { createLogger } = require('../utils');
const logger = createLogger(global.logger, 'db.kb');

// Embedding dimensions for text-embedding-3-large
const EMBEDDING_DIMENSIONS = 3072;
const MAX_CHUNK_SIZE = 22000; // Characters before we must split
const RECOMMENDED_CHUNK_SIZE = 16000; // Characters before we recommend splitting
const OVERLAP_SIZE = 1200; // Character overlap when splitting

/**
 * Get MongoDB client and KB metadata collection
 */
async function getClient() {
    const client = new MongoClient(config.DBServer);
    await client.connect();
    return client;
}

/**
 * Get the KB metadata collection
 */
async function getKBMetadataCollection() {
    const client = await getClient();
    const db = client.db(config.DB);
    return { client, collection: db.collection("KBMetadata") };
}

/**
 * Get a specific KB collection by ID
 * @param {string} kbId - The KB identifier
 */
async function getKBCollection(kbId) {
    const client = await getClient();
    const db = client.db(config.DB);
    const collectionName = `KB_${kbId}`;
    return { client, collection: db.collection(collectionName), collectionName };
}

/**
 * Create a new KB
 * @param {string} kbId - Unique identifier for the KB
 * @param {string} name - Human-readable name
 * @param {string} description - Description of the KB
 * @param {object} options - Additional options (shorterAnswers, extractModel)
 */
async function createKB(kbId, name, description = '', options = {}) {
    const { client, collection } = await getKBMetadataCollection();
    try {
        // Validate kbId format (alphanumeric and underscores only)
        if (!/^[a-zA-Z0-9_]+$/.test(kbId)) {
            throw new Error('KB ID must contain only alphanumeric characters and underscores');
        }

        // Check if KB already exists
        const existing = await collection.findOne({ kbId });
        if (existing) {
            throw new Error(`KB with ID '${kbId}' already exists`);
        }

        const kbMetadata = {
            kbId,
            name,
            description,
            shorterAnswers: options.shorterAnswers || false,
            extractModel: options.extractModel || 'gpt-5-mini',
            documentCount: 0,
            createdAt: new Date(),
            updatedAt: new Date()
        };

        await collection.insertOne(kbMetadata);
        logger.info('KB created', { kbId, name });

        // Create the KB collection and vector search index
        await createKBVectorIndex(kbId);

        return kbMetadata;
    } finally {
        await client.close();
    }
}

/**
 * Create vector search index for a KB collection
 * @param {string} kbId - The KB identifier
 */
async function createKBVectorIndex(kbId) {
    const { client, collection, collectionName } = await getKBCollection(kbId);
    try {
        // Create the vector search index
        const indexDefinition = {
            name: "vector_index",
            type: "vectorSearch",
            definition: {
                fields: [
                    {
                        type: "vector",
                        path: "embedding",
                        numDimensions: EMBEDDING_DIMENSIONS,
                        similarity: "cosine"
                    },
                    {
                        type: "filter",
                        path: "documentId"
                    }
                ]
            }
        };

        try {
            await collection.createSearchIndex(indexDefinition);
            logger.info('Vector search index created', { collectionName });
        } catch (indexErr) {
            // Index creation might fail if Atlas search isn't available
            // Log warning but don't fail - we can still use the KB without vector search
            logger.warn('Could not create vector search index - vector search may not be available', {
                collectionName,
                error: indexErr.message
            });
        }

        // Also create a standard text index as fallback
        await collection.createIndex({ content: "text", name: "text", contextHint: "text" });
        logger.info('Text search index created', { collectionName });

    } finally {
        await client.close();
    }
}

/**
 * List all KBs
 */
async function listKBs() {
    const { client, collection } = await getKBMetadataCollection();
    try {
        const kbs = await collection.find({}).toArray();
        return kbs;
    } finally {
        await client.close();
    }
}

/**
 * Get KB metadata by ID
 * @param {string} kbId - The KB identifier
 */
async function getKB(kbId) {
    logger.debug('getKB: Looking up KB', { kbId });
    const { client, collection } = await getKBMetadataCollection();
    try {
        const kb = await collection.findOne({ kbId });
        logger.debug('getKB: Result', { kbId, found: !!kb, name: kb?.name });
        return kb;
    } finally {
        await client.close();
    }
}

/**
 * Update KB settings
 * @param {string} kbId - The KB identifier
 * @param {object} updates - Fields to update
 */
async function updateKB(kbId, updates) {
    const { client, collection } = await getKBMetadataCollection();
    try {
        const allowedFields = ['name', 'description', 'shorterAnswers', 'extractModel'];
        const sanitizedUpdates = {};
        for (const field of allowedFields) {
            if (updates[field] !== undefined) {
                sanitizedUpdates[field] = updates[field];
            }
        }
        sanitizedUpdates.updatedAt = new Date();

        const result = await collection.updateOne(
            { kbId },
            { $set: sanitizedUpdates }
        );
        return result.modifiedCount > 0;
    } finally {
        await client.close();
    }
}

/**
 * Delete a KB and all its documents
 * @param {string} kbId - The KB identifier
 */
async function deleteKB(kbId) {
    const metaClient = await getClient();
    try {
        const db = metaClient.db(config.DB);
        
        // Delete the KB collection
        const collectionName = `KB_${kbId}`;
        try {
            await db.collection(collectionName).drop();
            logger.info('KB collection dropped', { collectionName });
        } catch (dropErr) {
            // Collection might not exist
            logger.warn('Could not drop KB collection', { collectionName, error: dropErr.message });
        }

        // Delete the metadata entry
        const result = await db.collection("KBMetadata").deleteOne({ kbId });
        return result.deletedCount > 0;
    } finally {
        await metaClient.close();
    }
}

/**
 * Split text into chunks with overlap
 * @param {string} text - The text to split
 * @param {number} maxSize - Maximum chunk size
 * @param {number} overlap - Overlap between chunks
 */
function splitTextIntoChunks(text, maxSize = MAX_CHUNK_SIZE, overlap = OVERLAP_SIZE) {
    if (text.length <= maxSize) {
        return [text];
    }

    const chunks = [];
    let start = 0;
    
    while (start < text.length) {
        let end = Math.min(start + maxSize, text.length);
        
        // Try to find a good break point (paragraph, sentence, or word boundary)
        if (end < text.length) {
            // Look for paragraph break
            let breakPoint = text.lastIndexOf('\n\n', end);
            if (breakPoint > start + maxSize / 2) {
                end = breakPoint + 2;
            } else {
                // Look for sentence break
                breakPoint = text.lastIndexOf('. ', end);
                if (breakPoint > start + maxSize / 2) {
                    end = breakPoint + 2;
                } else {
                    // Look for word break
                    breakPoint = text.lastIndexOf(' ', end);
                    if (breakPoint > start + maxSize / 2) {
                        end = breakPoint + 1;
                    }
                }
            }
        }

        chunks.push(text.substring(start, end).trim());
        
        // Move start position, accounting for overlap
        start = end - overlap;
        if (start < 0) start = 0;
        
        // Prevent infinite loop
        if (start >= text.length) break;
        if (end >= text.length) break;
    }

    return chunks;
}

/**
 * Add a document to a KB
 * @param {string} kbId - The KB identifier
 * @param {string} name - Document name
 * @param {string} content - Document text content
 * @param {string} contextHint - Optional context hint for retrieval
 * @param {string} fileType - Original file type (md, txt, pdf, etc.)
 * @param {array} embeddings - Pre-computed embeddings (optional, will be computed if not provided)
 */
async function addDocument(kbId, name, content, contextHint = '', fileType = 'txt', embeddings = null) {
    const { client, collection } = await getKBCollection(kbId);
    try {
        const documentId = new ObjectId().toString();
        const charCount = content.length;
        const needsSplit = charCount > MAX_CHUNK_SIZE;
        const recommendSplit = charCount > RECOMMENDED_CHUNK_SIZE && charCount <= MAX_CHUNK_SIZE;

        // Split content if needed
        const chunks = needsSplit ? splitTextIntoChunks(content) : [content];
        
        // If embeddings not provided, we'll need to generate them
        // This will be done by the controller which has access to OpenAI
        const documents = chunks.map((chunk, index) => ({
            documentId,
            chunkIndex: index,
            totalChunks: chunks.length,
            name,
            content: chunk,
            contextHint,
            fileType,
            charCount: chunk.length,
            embedding: embeddings ? embeddings[index] : null,
            createdAt: new Date(),
            updatedAt: new Date()
        }));

        await collection.insertMany(documents);

        // Update document count in metadata
        await updateDocumentCount(kbId);

        logger.info('Document added to KB', { 
            kbId, 
            documentId, 
            name, 
            chunks: chunks.length,
            charCount 
        });

        return {
            documentId,
            name,
            chunks: chunks.length,
            charCount,
            needsSplit,
            recommendSplit: recommendSplit && !needsSplit
        };
    } finally {
        await client.close();
    }
}

/**
 * Update document count in KB metadata
 */
async function updateDocumentCount(kbId) {
    const client = await getClient();
    try {
        const db = client.db(config.DB);
        const kbCollection = db.collection(`KB_${kbId}`);
        const metaCollection = db.collection("KBMetadata");
        
        // Count unique documentIds
        const distinctDocs = await kbCollection.distinct('documentId');
        const count = distinctDocs.length;
        
        await metaCollection.updateOne(
            { kbId },
            { $set: { documentCount: count, updatedAt: new Date() } }
        );
    } finally {
        await client.close();
    }
}

/**
 * Update embeddings for documents that don't have them
 * @param {string} kbId - The KB identifier
 * @param {string} documentId - The document ID
 * @param {array} embeddings - Array of embeddings for each chunk
 */
async function updateDocumentEmbeddings(kbId, documentId, embeddings) {
    const { client, collection } = await getKBCollection(kbId);
    try {
        const docs = await collection.find({ documentId }).sort({ chunkIndex: 1 }).toArray();
        
        for (let i = 0; i < docs.length && i < embeddings.length; i++) {
            await collection.updateOne(
                { _id: docs[i]._id },
                { $set: { embedding: embeddings[i], updatedAt: new Date() } }
            );
        }

        logger.info('Document embeddings updated', { kbId, documentId, chunks: embeddings.length });
        return true;
    } finally {
        await client.close();
    }
}

/**
 * List documents in a KB
 * @param {string} kbId - The KB identifier
 */
async function listDocuments(kbId) {
    const { client, collection } = await getKBCollection(kbId);
    try {
        // Get unique documents with their metadata
        const pipeline = [
            {
                $group: {
                    _id: "$documentId",
                    name: { $first: "$name" },
                    contextHint: { $first: "$contextHint" },
                    fileType: { $first: "$fileType" },
                    totalChunks: { $max: "$totalChunks" },
                    totalCharCount: { $sum: "$charCount" },
                    hasEmbedding: { $min: { $cond: [{ $ne: ["$embedding", null] }, 1, 0] } },
                    createdAt: { $first: "$createdAt" },
                    updatedAt: { $max: "$updatedAt" }
                }
            },
            {
                $project: {
                    _id: 0,
                    documentId: "$_id",
                    name: 1,
                    contextHint: 1,
                    fileType: 1,
                    chunks: "$totalChunks",
                    charCount: "$totalCharCount",
                    hasEmbedding: { $eq: ["$hasEmbedding", 1] },
                    createdAt: 1,
                    updatedAt: 1
                }
            },
            { $sort: { createdAt: -1 } }
        ];

        const documents = await collection.aggregate(pipeline).toArray();
        return documents;
    } finally {
        await client.close();
    }
}

/**
 * Get a document by ID (all chunks)
 * @param {string} kbId - The KB identifier
 * @param {string} documentId - The document ID
 */
async function getDocument(kbId, documentId) {
    const { client, collection } = await getKBCollection(kbId);
    try {
        const chunks = await collection.find({ documentId }).sort({ chunkIndex: 1 }).toArray();
        if (chunks.length === 0) return null;

        // Combine chunks for full content
        const fullContent = chunks.map(c => c.content).join('\n');

        return {
            documentId,
            name: chunks[0].name,
            content: fullContent,
            contextHint: chunks[0].contextHint,
            fileType: chunks[0].fileType,
            chunks: chunks.length,
            charCount: fullContent.length,
            hasEmbedding: chunks.every(c => c.embedding !== null),
            createdAt: chunks[0].createdAt,
            updatedAt: chunks[chunks.length - 1].updatedAt
        };
    } finally {
        await client.close();
    }
}

/**
 * Update a document
 * @param {string} kbId - The KB identifier
 * @param {string} documentId - The document ID
 * @param {string} name - New name (optional)
 * @param {string} content - New content (optional)
 * @param {string} contextHint - New context hint (optional)
 */
async function updateDocument(kbId, documentId, name = null, content = null, contextHint = null) {
    const { client, collection } = await getKBCollection(kbId);
    try {
        if (content !== null) {
            // Content changed - need to delete old chunks and re-add
            await collection.deleteMany({ documentId });
            
            // Get original document info
            const chunks = splitTextIntoChunks(content);
            
            const documents = chunks.map((chunk, index) => ({
                documentId,
                chunkIndex: index,
                totalChunks: chunks.length,
                name: name || 'Untitled',
                content: chunk,
                contextHint: contextHint || '',
                fileType: 'txt',
                charCount: chunk.length,
                embedding: null, // Will need to regenerate embeddings
                createdAt: new Date(),
                updatedAt: new Date()
            }));

            await collection.insertMany(documents);
            return { documentId, needsEmbedding: true, chunks: chunks.length };
        } else {
            // Only updating metadata
            const updateFields = { updatedAt: new Date() };
            if (name !== null) updateFields.name = name;
            if (contextHint !== null) updateFields.contextHint = contextHint;

            await collection.updateMany(
                { documentId },
                { $set: updateFields }
            );
            return { documentId, needsEmbedding: false };
        }
    } finally {
        await client.close();
    }
}

/**
 * Delete a document
 * @param {string} kbId - The KB identifier
 * @param {string} documentId - The document ID
 */
async function deleteDocument(kbId, documentId) {
    const { client, collection } = await getKBCollection(kbId);
    try {
        const result = await collection.deleteMany({ documentId });
        
        // Update document count
        await updateDocumentCount(kbId);

        return result.deletedCount > 0;
    } finally {
        await client.close();
    }
}

/**
 * Vector search for relevant documents
 * @param {string} kbId - The KB identifier
 * @param {array} queryEmbedding - The query embedding vector
 * @param {number} limit - Maximum results to return
 */
async function vectorSearch(kbId, queryEmbedding, limit = 5) {
    logger.debug('vectorSearch: Starting', { kbId, limit, embeddingLength: queryEmbedding?.length });
    const { client, collection } = await getKBCollection(kbId);
    try {
        const pipeline = [
            {
                $vectorSearch: {
                    index: "vector_index",
                    queryVector: queryEmbedding,
                    path: "embedding",
                    limit: limit * 2, // Get more to dedupe by document
                    numCandidates: limit * 10
                }
            },
            {
                $project: {
                    _id: 0,
                    documentId: 1,
                    name: 1,
                    content: 1,
                    contextHint: 1,
                    chunkIndex: 1,
                    score: { $meta: "vectorSearchScore" }
                }
            }
        ];

        const results = await collection.aggregate(pipeline).toArray();
        logger.debug('vectorSearch: Raw results received', { kbId, rawCount: results.length });
        
        // Deduplicate by documentId, keeping highest scoring chunk
        const seen = new Set();
        const deduped = [];
        for (const result of results) {
            if (!seen.has(result.documentId)) {
                seen.add(result.documentId);
                deduped.push(result);
                if (deduped.length >= limit) break;
            }
        }

        logger.debug('vectorSearch: Deduped results', { 
            kbId, 
            dedupedCount: deduped.length,
            scores: deduped.map(r => ({ doc: r.name, score: r.score?.toFixed(4) }))
        });
        return deduped;
    } catch (err) {
        // Fallback to text search if vector search not available
        logger.warn('Vector search failed, falling back to text search', { kbId, error: err.message, stack: err.stack });
        return await textSearch(kbId, '', limit);
    } finally {
        await client.close();
    }
}

/**
 * Text search fallback
 * @param {string} kbId - The KB identifier
 * @param {string} query - Search query
 * @param {number} limit - Maximum results
 */
async function textSearch(kbId, query, limit = 5) {
    logger.debug('textSearch: Starting', { kbId, query, limit });
    const { client, collection } = await getKBCollection(kbId);
    try {
        const results = await collection
            .find({ $text: { $search: query } })
            .project({
                documentId: 1,
                name: 1,
                content: 1,
                contextHint: 1,
                chunkIndex: 1,
                score: { $meta: "textScore" }
            })
            .sort({ score: { $meta: "textScore" } })
            .limit(limit * 2)
            .toArray();

        logger.debug('textSearch: Raw results received', { kbId, rawCount: results.length });

        // Deduplicate by documentId
        const seen = new Set();
        const deduped = [];
        for (const result of results) {
            if (!seen.has(result.documentId)) {
                seen.add(result.documentId);
                deduped.push(result);
                if (deduped.length >= limit) break;
            }
        }

        logger.debug('textSearch: Deduped results', { kbId, dedupedCount: deduped.length });
        return deduped;
    } finally {
        await client.close();
    }
}

/**
 * Get documents missing embeddings
 * @param {string} kbId - The KB identifier
 */
async function getDocumentsMissingEmbeddings(kbId) {
    const { client, collection } = await getKBCollection(kbId);
    try {
        const docs = await collection.find({ embedding: null }).toArray();
        return docs;
    } finally {
        await client.close();
    }
}

module.exports = {
    createKB,
    listKBs,
    getKB,
    updateKB,
    deleteKB,
    addDocument,
    updateDocumentEmbeddings,
    listDocuments,
    getDocument,
    updateDocument,
    deleteDocument,
    vectorSearch,
    textSearch,
    getDocumentsMissingEmbeddings,
    splitTextIntoChunks,
    EMBEDDING_DIMENSIONS,
    MAX_CHUNK_SIZE,
    RECOMMENDED_CHUNK_SIZE,
    OVERLAP_SIZE
};
