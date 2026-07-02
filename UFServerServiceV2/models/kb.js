/**
 * Knowledge Base (KB) Model
 * Handles all MongoDB operations for KB documents including:
 * - Vector embeddings using text-embedding-3-large
 * - Vector search using MongoDB Atlas Search
 * - Document management (CRUD)
 */

const { MongoClient, ObjectId } = require('mongodb');
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
    const client = new MongoClient(global.config.DBServer);
    await client.connect();
    return client;
}

/**
 * Get the KB metadata collection
 */
async function getKBMetadataCollection() {
    const client = await getClient();
    const db = client.db(global.config.DB);
    return { client, collection: db.collection("KBMetadata") };
}

/**
 * Get a specific KB collection by ID
 * @param {string} kbId - The KB identifier
 */
async function getKBCollection(kbId) {
    const client = await getClient();
    const db = client.db(global.config.DB);
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
 * Create indexes for a KB collection
 * For self-managed MongoDB, we create standard indexes to support efficient queries
 * The actual vector similarity is computed in-application using cosine similarity
 * @param {string} kbId - The KB identifier
 */
async function createKBVectorIndex(kbId) {
    const { client, collection, collectionName } = await getKBCollection(kbId);
    try {
        // Create index on embedding field existence for efficient filtering
        // This helps quickly find documents that have embeddings
        try {
            await collection.createIndex(
                { embedding: 1 },
                { 
                    name: "embedding_exists_idx",
                    partialFilterExpression: { embedding: { $exists: true } }
                }
            );
            logger.debug('[KB] Embedding existence index created', { collectionName });
        } catch (err) {
            if (err.code !== 85) { // Ignore "index already exists" errors
                logger.warn('[KB] Could not create embedding index', { collectionName, error: err.message });
            }
        }

        // Create index on documentId for efficient lookups and deduplication
        try {
            await collection.createIndex(
                { documentId: 1 },
                { name: "documentId_idx" }
            );
            logger.debug('[KB] DocumentId index created', { collectionName });
        } catch (err) {
            if (err.code !== 85) {
                logger.warn('[KB] Could not create documentId index', { collectionName, error: err.message });
            }
        }

        // Create text index for fallback text search
        try {
            await collection.createIndex(
                { content: "text", name: "text", contextHint: "text" },
                { name: "text_search_idx" }
            );
            logger.info('[KB] Text search index created', { collectionName });
        } catch (err) {
            if (err.code !== 85) {
                logger.warn('[KB] Could not create text index', { collectionName, error: err.message });
            }
        }

    } finally {
        await client.close();
    }
}

/**
 * Ensure indexes exist for all KB collections
 * Call this on startup to ensure indexes are created for any KBs that existed before the index update
 */
async function ensureAllKBIndexes() {
    try {
        const kbs = await listKBs();
        logger.debug('[KB] Ensuring indexes for all KBs', { kbCount: kbs.length });
        
        for (const kb of kbs) {
            try {
                await createKBVectorIndex(kb.kbId);
            } catch (err) {
                logger.warn('[KB] Failed to ensure indexes for KB', { kbId: kb.kbId, error: err.message });
            }
        }
        
        logger.info('[KB] Index check complete for all KBs');
    } catch (err) {
        logger.error('[KB] Failed to ensure indexes', { error: err.message });
    }
}

/**
 * Ensure all documents in all KBs have embeddings
 * Call this on startup to generate missing embeddings
 * @param {function} generateEmbeddingsFn - Function to generate embeddings (passed from controller)
 */
async function ensureAllEmbeddings(generateEmbeddingsFn) {
    try {
        const kbs = await listKBs();
        logger.debug('[KB] Checking embeddings for all KBs', { kbCount: kbs.length });
        
        let totalMissing = 0;
        let totalFixed = 0;
        let totalFailed = 0;
        
        for (const kb of kbs) {
            try {
                const missingChunks = await getDocumentsMissingEmbeddings(kb.kbId);
                
                if (missingChunks.length === 0) {
                    continue;
                }
                
                // Group by documentId
                const byDoc = {};
                for (const chunk of missingChunks) {
                    if (!byDoc[chunk.documentId]) byDoc[chunk.documentId] = [];
                    byDoc[chunk.documentId].push(chunk);
                }
                
                const docCount = Object.keys(byDoc).length;
                totalMissing += docCount;
                logger.info('[KB] Found documents missing embeddings', { 
                    kbId: kb.kbId, 
                    documentCount: docCount,
                    chunkCount: missingChunks.length 
                });
                
                for (const [documentId, chunks] of Object.entries(byDoc)) {
                    try {
                        chunks.sort((a, b) => a.chunkIndex - b.chunkIndex);
                        const contents = chunks.map(c => c.content);
                        const embeddings = await generateEmbeddingsFn(contents);
                        await updateDocumentEmbeddings(kb.kbId, documentId, embeddings);
                        totalFixed++;
                        logger.info('[KB] Generated missing embeddings', { 
                            kbId: kb.kbId, 
                            documentId, 
                            name: chunks[0]?.name,
                            chunks: embeddings.length 
                        });
                    } catch (err) {
                        totalFailed++;
                        logger.error('[KB] Failed to generate embeddings for document', { 
                            kbId: kb.kbId, 
                            documentId, 
                            name: chunks[0]?.name,
                            error: err.message 
                        });
                    }
                }
            } catch (err) {
                logger.error('[KB] Failed to check embeddings for KB', { kbId: kb.kbId, error: err.message });
            }
        }
        
        if (totalMissing === 0) {
            logger.info('[KB] All documents have embeddings');
        } else {
            logger.info('[KB] Embedding check complete', { 
                totalMissing, 
                fixed: totalFixed, 
                failed: totalFailed 
            });
        }
        
        return { totalMissing, fixed: totalFixed, failed: totalFailed };
    } catch (err) {
        logger.error('[KB] Failed to ensure embeddings', { error: err.message });
        return { error: err.message };
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
        const db = metaClient.db(global.config.DB);
        
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
        const db = client.db(global.config.DB);
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
 * Compute cosine similarity between two vectors
 * @param {number[]} a - First vector
 * @param {number[]} b - Second vector
 * @returns {number} Cosine similarity (0 to 1)
 */
function cosineSimilarity(a, b) {
    if (!a || !b || a.length !== b.length) return 0;
    
    let dotProduct = 0;
    let normA = 0;
    let normB = 0;
    
    for (let i = 0; i < a.length; i++) {
        dotProduct += a[i] * b[i];
        normA += a[i] * a[i];
        normB += b[i] * b[i];
    }
    
    const magnitude = Math.sqrt(normA) * Math.sqrt(normB);
    return magnitude === 0 ? 0 : dotProduct / magnitude;
}

/**
 * Vector search for relevant documents using in-application cosine similarity
 * Works with self-managed MongoDB without requiring Atlas Search
 * @param {string} kbId - The KB identifier
 * @param {array} queryEmbedding - The query embedding vector
 * @param {number} limit - Maximum results to return
 */
async function vectorSearch(kbId, queryEmbedding, limit = 5) {
    logger.info('[KB] Vector search starting', { kbId, limit, embeddingLength: queryEmbedding?.length });
    const { client, collection } = await getKBCollection(kbId);
    try {
        // Fetch all documents with embeddings from this KB
        // For large KBs, consider adding pagination or caching
        const docs = await collection.find(
            { embedding: { $exists: true, $ne: null } },
            { 
                projection: {
                    _id: 0,
                    documentId: 1,
                    name: 1,
                    content: 1,
                    contextHint: 1,
                    chunkIndex: 1,
                    embedding: 1
                }
            }
        ).toArray();

        logger.info('[KB] Fetched documents for similarity', { kbId, docCount: docs.length });

        if (docs.length === 0) {
            logger.warn('[KB] No documents with embeddings found', { kbId });
            return [];
        }

        // Compute cosine similarity for each document
        let dimensionMismatches = 0;
        const scoredDocs = docs.map(doc => {
            if (Array.isArray(queryEmbedding) && Array.isArray(doc.embedding) && doc.embedding.length !== queryEmbedding.length) {
                dimensionMismatches++;
            }
            return {
                documentId: doc.documentId,
                name: doc.name,
                content: doc.content,
                contextHint: doc.contextHint,
                chunkIndex: doc.chunkIndex,
                score: cosineSimilarity(queryEmbedding, doc.embedding)
            };
        });

        // Mismatched documents score 0, so results are effectively random for
        // them. Happens when EmbeddingModel changed after documents were
        // embedded - the KB documents must be re-uploaded or re-embedded.
        if (dimensionMismatches > 0) {
            logger.warn('[KB] Embedding dimension mismatch: documents were embedded with a different model than the current EmbeddingModel. Re-upload or re-embed the KB documents.', {
                kbId,
                mismatchedChunks: dimensionMismatches,
                totalChunks: docs.length,
                queryDimensions: queryEmbedding?.length
            });
        }

        // Sort by score descending
        scoredDocs.sort((a, b) => b.score - a.score);

        // Deduplicate by documentId, keeping highest scoring chunk
        const seen = new Set();
        const deduped = [];
        for (const result of scoredDocs) {
            if (!seen.has(result.documentId)) {
                seen.add(result.documentId);
                deduped.push(result);
                if (deduped.length >= limit) break;
            }
        }

        logger.info('[KB] Vector search complete', { 
            kbId, 
            dedupedCount: deduped.length,
            topScores: deduped.slice(0, 3).map(r => ({ doc: r.name, score: r.score?.toFixed(4) }))
        });
        return deduped;
    } catch (err) {
        logger.error('[KB] Vector search failed', { kbId, error: err.message, stack: err.stack });
        throw err;
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
    logger.info('[KB] Text search starting', { kbId, query, limit });
    const { client, collection } = await getKBCollection(kbId);
    try {
        let results = [];
        
        // First try $text search (requires text index)
        try {
            results = await collection
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
            logger.debug('[KB] $text search succeeded', { kbId, rawCount: results.length });
        } catch (textErr) {
            // Fallback to regex search if no text index
            logger.warn('[KB] $text search failed, using regex fallback', { kbId, error: textErr.message });
            
            // Build regex pattern from query words
            const words = query.split(/\s+/).filter(w => w.length > 2);
            if (words.length > 0) {
                const regexPattern = words.map(w => `(?=.*${w.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')})`).join('');
                results = await collection
                    .find({ content: { $regex: regexPattern, $options: 'i' } })
                    .project({
                        documentId: 1,
                        name: 1,
                        content: 1,
                        contextHint: 1,
                        chunkIndex: 1
                    })
                    .limit(limit * 2)
                    .toArray();
                logger.info('[KB] Regex search completed', { kbId, rawCount: results.length });
            }
        }

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

        logger.info('[KB] Text search complete', { kbId, query, resultCount: deduped.length });
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
    ensureAllKBIndexes,
    ensureAllEmbeddings,
    splitTextIntoChunks,
    EMBEDDING_DIMENSIONS,
    MAX_CHUNK_SIZE,
    RECOMMENDED_CHUNK_SIZE,
    OVERLAP_SIZE
};
