/**
 * Knowledge Base Controller
 * Handles KB management, document uploads, and search operations
 */

const express = require('express');
const router = express.Router();
const multer = require('multer');
const { OpenAI } = require('openai').default;
const mammoth = require('mammoth');
const pdf = require('pdf-parse');
const { createLogger } = require('../utils');
const logger = createLogger(global.logger, 'kb');

const { requireServerAuth } = require('../auth/utils');

const {
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
    RECOMMENDED_CHUNK_SIZE
} = require('../models/kb');

// Configure multer for file uploads (memory storage)
const upload = multer({
    storage: multer.memoryStorage(),
    limits: {
        fileSize: 50 * 1024 * 1024, // 50MB max
    },
    fileFilter: (req, file, cb) => {
        const allowedTypes = [
            'text/plain',
            'text/markdown',
            'text/xml',
            'application/xml',
            'application/json',
            'application/pdf',
            'application/msword',
            'application/vnd.openxmlformats-officedocument.wordprocessingml.document'
        ];
        const allowedExtensions = ['.txt', '.md', '.xml', '.json', '.pdf', '.doc', '.docx'];
        
        const ext = file.originalname.toLowerCase().substring(file.originalname.lastIndexOf('.'));
        if (allowedTypes.includes(file.mimetype) || allowedExtensions.includes(ext)) {
            cb(null, true);
        } else {
            cb(new Error(`File type not allowed: ${file.mimetype} (${ext})`));
        }
    }
});

// Initialize OpenAI client
let openai = null;
function getOpenAI() {
    if (!openai && global.config.OpenAIApi?.ApiKey) {
        openai = new OpenAI({ apiKey: global.config.OpenAIApi.ApiKey });
    }
    return openai;
}

/**
 * Generate embeddings for text using text-embedding-3-large
 * Includes retry logic for transient failures
 * @param {string|string[]} texts - Text(s) to embed
 * @param {number} maxRetries - Maximum retry attempts (default: 3)
 * @returns {Promise<number[][]>} Array of embedding vectors
 */
async function generateEmbeddings(texts, maxRetries = 3) {
    const ai = getOpenAI();
    if (!ai) {
        logger.debug('generateEmbeddings: OpenAI API not configured');
        throw new Error('OpenAI API not configured');
    }

    const textsArray = Array.isArray(texts) ? texts : [texts];
    
    // Validate input
    if (textsArray.length === 0) {
        throw new Error('No texts provided for embedding');
    }
    
    // Check for empty texts
    const validTexts = textsArray.filter(t => t && t.trim().length > 0);
    if (validTexts.length !== textsArray.length) {
        logger.warn('generateEmbeddings: Some texts were empty', { 
            provided: textsArray.length, 
            valid: validTexts.length 
        });
    }
    
    if (validTexts.length === 0) {
        throw new Error('All provided texts were empty');
    }
    
    logger.info('generateEmbeddings: Starting embedding generation', { 
        textCount: validTexts.length, 
        totalChars: validTexts.reduce((sum, t) => sum + t.length, 0),
        dimensions: EMBEDDING_DIMENSIONS
    });
    
    let lastError;
    for (let attempt = 1; attempt <= maxRetries; attempt++) {
        try {
            const response = await ai.embeddings.create({
                model: 'text-embedding-3-large',
                input: validTexts,
                dimensions: EMBEDDING_DIMENSIONS
            });

            logger.info('generateEmbeddings: Embeddings generated successfully', { 
                embeddingCount: response.data.length,
                usage: response.usage,
                attempt
            });
            return response.data.map(d => d.embedding);
        } catch (err) {
            lastError = err;
            
            // Check if it's a retryable error (rate limit, server error, timeout)
            const isRetryable = err.status === 429 || err.status >= 500 || 
                                err.code === 'ETIMEDOUT' || err.code === 'ECONNRESET';
            
            if (isRetryable && attempt < maxRetries) {
                const delay = Math.min(1000 * Math.pow(2, attempt - 1), 10000); // Exponential backoff, max 10s
                logger.warn('generateEmbeddings: Retrying after error', { 
                    error: err.message, 
                    attempt, 
                    maxRetries,
                    retryDelay: delay 
                });
                await new Promise(r => setTimeout(r, delay));
            } else {
                logger.error('generateEmbeddings: Failed to generate embeddings', { 
                    error: err.message, 
                    stack: err.stack,
                    attempt,
                    isRetryable
                });
                throw err;
            }
        }
    }
    
    throw lastError;
}

/**
 * Extract text content from uploaded file
 * @param {Buffer} buffer - File buffer
 * @param {string} filename - Original filename
 * @param {string} mimetype - File MIME type
 */
async function extractTextFromFile(buffer, filename, mimetype) {
    const ext = filename.toLowerCase().substring(filename.lastIndexOf('.'));
    
    try {
        switch (ext) {
            case '.txt':
            case '.md':
            case '.xml':
            case '.json':
                return buffer.toString('utf-8');
            
            case '.pdf':
                const pdfData = await pdf(buffer);
                return pdfData.text;
            
            case '.doc':
            case '.docx':
                const result = await mammoth.extractRawText({ buffer });
                return result.value;
            
            default:
                // Try to read as text
                return buffer.toString('utf-8');
        }
    } catch (err) {
        logger.error('Failed to extract text from file', { filename, error: err.message });
        throw new Error(`Failed to extract text from ${filename}: ${err.message}`);
    }
}

/**
 * Use AI to extract relevant excerpts from search results (Shorter Answers feature)
 * Uses OpenAI Responses API
 * @param {string} query - The search query
 * @param {array} results - Search results with content
 * @param {string} model - Model to use for extraction
 */
async function extractRelevantExcerpts(query, results, model = 'gpt-5-mini') {
    const ai = getOpenAI();
    if (!ai) {
        throw new Error('OpenAI API not configured');
    }

    const combinedContent = results.map((r, i) => 
        `[Document ${i + 1}: ${r.name}]\n${r.contextHint ? `Context: ${r.contextHint}\n` : ''}${r.content}`
    ).join('\n\n---\n\n');

    logger.debug('extractRelevantExcerpts: Starting AI extraction with Responses API', { 
        query, 
        model, 
        documentCount: results.length,
        combinedLength: combinedContent.length 
    });
    
    try {
        const response = await ai.responses.create({
            model,
            instructions: `You are a precise information extractor. Given a query and multiple document excerpts, extract ONLY the most relevant sentences or paragraphs that directly answer or relate to the query. Keep your response concise and focused. Include document names for attribution. If no relevant information is found, say "No relevant information found."`,
            input: `Query: ${query}\n\nDocuments:\n${combinedContent}`,
            temperature: 0.3,
            max_output_tokens: 2000
        });

        const extracted = response.output_text || '';
        logger.debug('extractRelevantExcerpts: Extraction complete', { 
            extractedLength: extracted?.length,
            usage: response.usage
        });
        return extracted;
    } catch (err) {
        logger.error('Failed to extract excerpts', { error: err.message, stack: err.stack });
        // Return original content on failure
        logger.debug('extractRelevantExcerpts: Returning fallback content');
        return results.map(r => `[${r.name}]: ${r.content}`).join('\n\n');
    }
}

// ============ KB Management Routes ============

/**
 * List all KBs
 * GET /KB
 */
router.get('/', requireServerAuth, async (req, res) => {
    try {
        const kbs = await listKBs();
        res.json({ Status: 'Success', data: kbs });
    } catch (err) {
        logger.error('Failed to list KBs', { error: err.message });
        res.status(500).json({ Status: 'Error', Error: err.message });
    }
});

/**
 * Create a new KB
 * POST /KB
 * Body: { kbId, name, description?, shorterAnswers?, extractModel? }
 */
router.post('/', requireServerAuth, async (req, res) => {
    try {
        const { kbId, name, description, shorterAnswers, extractModel } = req.body;
        
        if (!kbId || !name) {
            return res.status(400).json({ Status: 'Error', Error: 'kbId and name are required' });
        }

        const kb = await createKB(kbId, name, description, { shorterAnswers, extractModel });
        res.status(201).json({ Status: 'Success', data: kb });
    } catch (err) {
        logger.error('Failed to create KB', { error: err.message });
        res.status(400).json({ Status: 'Error', Error: err.message });
    }
});

/**
 * Get KB by ID
 * GET /KB/:kbId
 */
router.get('/:kbId', requireServerAuth, async (req, res) => {
    try {
        const { kbId } = req.params;
        const kb = await getKB(kbId);
        
        if (!kb) {
            return res.status(404).json({ Status: 'Error', Error: 'KB not found' });
        }

        res.json({ Status: 'Success', data: kb });
    } catch (err) {
        logger.error('Failed to get KB', { kbId: req.params.kbId, error: err.message });
        res.status(500).json({ Status: 'Error', Error: err.message });
    }
});

/**
 * Update KB settings
 * PUT /KB/:kbId
 * Body: { name?, description?, shorterAnswers?, extractModel? }
 */
router.put('/:kbId', requireServerAuth, async (req, res) => {
    try {
        const { kbId } = req.params;
        const updates = req.body;

        const success = await updateKB(kbId, updates);
        if (!success) {
            return res.status(404).json({ Status: 'Error', Error: 'KB not found' });
        }

        res.json({ Status: 'Success' });
    } catch (err) {
        logger.error('Failed to update KB', { kbId: req.params.kbId, error: err.message });
        res.status(500).json({ Status: 'Error', Error: err.message });
    }
});

/**
 * Delete KB
 * DELETE /KB/:kbId
 */
router.delete('/:kbId', requireServerAuth, async (req, res) => {
    try {
        const { kbId } = req.params;
        const success = await deleteKB(kbId);
        
        if (!success) {
            return res.status(404).json({ Status: 'Error', Error: 'KB not found' });
        }

        res.json({ Status: 'Success' });
    } catch (err) {
        logger.error('Failed to delete KB', { kbId: req.params.kbId, error: err.message });
        res.status(500).json({ Status: 'Error', Error: err.message });
    }
});

// ============ Document Management Routes ============

/**
 * List documents in a KB
 * GET /KB/:kbId/documents
 */
router.get('/:kbId/documents', requireServerAuth, async (req, res) => {
    try {
        const { kbId } = req.params;
        
        const kb = await getKB(kbId);
        if (!kb) {
            return res.status(404).json({ Status: 'Error', Error: 'KB not found' });
        }

        const documents = await listDocuments(kbId);
        res.json({ Status: 'Success', data: documents });
    } catch (err) {
        logger.error('Failed to list documents', { kbId: req.params.kbId, error: err.message });
        res.status(500).json({ Status: 'Error', Error: err.message });
    }
});

/**
 * Upload a document to KB
 * POST /KB/:kbId/documents
 * Multipart form: file, name?, contextHint?
 */
router.post('/:kbId/documents', requireServerAuth, upload.single('file'), async (req, res) => {
    try {
        const { kbId } = req.params;
        const { name, contextHint } = req.body;
        
        const kb = await getKB(kbId);
        if (!kb) {
            return res.status(404).json({ Status: 'Error', Error: 'KB not found' });
        }

        if (!req.file) {
            return res.status(400).json({ Status: 'Error', Error: 'No file uploaded' });
        }

        // Extract text from file
        const content = await extractTextFromFile(
            req.file.buffer,
            req.file.originalname,
            req.file.mimetype
        );

        if (!content || content.trim().length === 0) {
            return res.status(400).json({ Status: 'Error', Error: 'No text content could be extracted from file' });
        }

        const docName = name || req.file.originalname;
        const ext = req.file.originalname.toLowerCase().substring(req.file.originalname.lastIndexOf('.'));
        const fileType = ext.replace('.', '');

        // Check size recommendations
        const charCount = content.length;
        const splitInfo = {
            charCount,
            needsSplit: charCount > MAX_CHUNK_SIZE,
            recommendSplit: charCount > RECOMMENDED_CHUNK_SIZE && charCount <= MAX_CHUNK_SIZE
        };

        // Add document (will be auto-split if needed)
        const result = await addDocument(kbId, docName, content, contextHint || '', fileType);

        // Generate embeddings for the chunks
        try {
            const chunks = splitTextIntoChunks(content);
            const embeddings = await generateEmbeddings(chunks);
            await updateDocumentEmbeddings(kbId, result.documentId, embeddings);
            result.hasEmbedding = true;
        } catch (embErr) {
            logger.warn('Failed to generate embeddings for document', { 
                documentId: result.documentId, 
                error: embErr.message 
            });
            result.hasEmbedding = false;
            result.embeddingError = embErr.message;
        }

        res.status(201).json({ 
            Status: 'Success', 
            data: result,
            splitInfo
        });
    } catch (err) {
        logger.error('Failed to upload document', { kbId: req.params.kbId, error: err.message });
        res.status(500).json({ Status: 'Error', Error: err.message });
    }
});

/**
 * Add document from text content (for editing)
 * POST /KB/:kbId/documents/text
 * Body: { name, content, contextHint? }
 */
router.post('/:kbId/documents/text', requireServerAuth, async (req, res) => {
    try {
        const { kbId } = req.params;
        const { name, content, contextHint, fileType } = req.body;
        
        const kb = await getKB(kbId);
        if (!kb) {
            return res.status(404).json({ Status: 'Error', Error: 'KB not found' });
        }

        if (!name || !content) {
            return res.status(400).json({ Status: 'Error', Error: 'name and content are required' });
        }

        // Add document
        const result = await addDocument(kbId, name, content, contextHint || '', fileType || 'txt');

        // Generate embeddings
        try {
            const chunks = splitTextIntoChunks(content);
            const embeddings = await generateEmbeddings(chunks);
            await updateDocumentEmbeddings(kbId, result.documentId, embeddings);
            result.hasEmbedding = true;
        } catch (embErr) {
            logger.warn('Failed to generate embeddings', { error: embErr.message });
            result.hasEmbedding = false;
        }

        res.status(201).json({ Status: 'Success', data: result });
    } catch (err) {
        logger.error('Failed to add text document', { error: err.message });
        res.status(500).json({ Status: 'Error', Error: err.message });
    }
});

/**
 * Get document content
 * GET /KB/:kbId/documents/:documentId
 */
router.get('/:kbId/documents/:documentId', requireServerAuth, async (req, res) => {
    try {
        const { kbId, documentId } = req.params;
        
        const doc = await getDocument(kbId, documentId);
        if (!doc) {
            return res.status(404).json({ Status: 'Error', Error: 'Document not found' });
        }

        res.json({ Status: 'Success', data: doc });
    } catch (err) {
        logger.error('Failed to get document', { error: err.message });
        res.status(500).json({ Status: 'Error', Error: err.message });
    }
});

/**
 * Update document
 * PUT /KB/:kbId/documents/:documentId
 * Body: { name?, content?, contextHint? }
 */
router.put('/:kbId/documents/:documentId', requireServerAuth, async (req, res) => {
    try {
        const { kbId, documentId } = req.params;
        const { name, content, contextHint } = req.body;

        const result = await updateDocument(kbId, documentId, name, content, contextHint);

        // Regenerate embeddings if content changed
        if (result.needsEmbedding && content) {
            try {
                const chunks = splitTextIntoChunks(content);
                const embeddings = await generateEmbeddings(chunks);
                await updateDocumentEmbeddings(kbId, documentId, embeddings);
                result.hasEmbedding = true;
            } catch (embErr) {
                logger.warn('Failed to regenerate embeddings', { error: embErr.message });
                result.hasEmbedding = false;
            }
        }

        res.json({ Status: 'Success', data: result });
    } catch (err) {
        logger.error('Failed to update document', { error: err.message });
        res.status(500).json({ Status: 'Error', Error: err.message });
    }
});

/**
 * Delete document
 * DELETE /KB/:kbId/documents/:documentId
 */
router.delete('/:kbId/documents/:documentId', requireServerAuth, async (req, res) => {
    try {
        const { kbId, documentId } = req.params;
        
        const success = await deleteDocument(kbId, documentId);
        if (!success) {
            return res.status(404).json({ Status: 'Error', Error: 'Document not found' });
        }

        res.json({ Status: 'Success' });
    } catch (err) {
        logger.error('Failed to delete document', { error: err.message });
        res.status(500).json({ Status: 'Error', Error: err.message });
    }
});

/**
 * Regenerate embeddings for documents missing them
 * POST /KB/:kbId/regenerate-embeddings
 */
router.post('/:kbId/regenerate-embeddings', requireServerAuth, async (req, res) => {
    try {
        const { kbId } = req.params;
        
        const docsMissing = await getDocumentsMissingEmbeddings(kbId);
        if (docsMissing.length === 0) {
            return res.json({ Status: 'Success', message: 'All documents have embeddings', processed: 0 });
        }

        // Group by documentId
        const byDoc = {};
        for (const doc of docsMissing) {
            if (!byDoc[doc.documentId]) {
                byDoc[doc.documentId] = [];
            }
            byDoc[doc.documentId].push(doc);
        }

        let processed = 0;
        let failed = 0;

        for (const [documentId, chunks] of Object.entries(byDoc)) {
            try {
                // Sort chunks by index
                chunks.sort((a, b) => a.chunkIndex - b.chunkIndex);
                const contents = chunks.map(c => c.content);
                
                const embeddings = await generateEmbeddings(contents);
                await updateDocumentEmbeddings(kbId, documentId, embeddings);
                processed++;
            } catch (err) {
                logger.warn('Failed to regenerate embeddings for document', { documentId, error: err.message });
                failed++;
            }
        }

        res.json({ 
            Status: 'Success', 
            processed, 
            failed,
            total: Object.keys(byDoc).length 
        });
    } catch (err) {
        logger.error('Failed to regenerate embeddings', { error: err.message });
        res.status(500).json({ Status: 'Error', Error: err.message });
    }
});

// ============ Search Routes ============

/**
 * Search KB for relevant documents
 * POST /KB/:kbId/search
 * Body: { query, limit?, useShorterAnswers? }
 */
router.post('/:kbId/search', requireServerAuth, async (req, res) => {
    try {
        const { kbId } = req.params;
        const { query, limit = 5, useShorterAnswers } = req.body;
        
        logger.info('KB search request received', { kbId, query: query?.substring(0, 100), limit });

        if (!query) {
            logger.debug('KB search rejected: no query provided');
            return res.status(400).json({ Status: 'Error', Error: 'query is required' });
        }

        const kb = await getKB(kbId);
        if (!kb) {
            logger.debug('KB search rejected: KB not found', { kbId });
            return res.status(404).json({ Status: 'Error', Error: 'KB not found' });
        }
        
        logger.debug('KB found for search', { kbId, kbName: kb.name, shorterAnswers: kb.shorterAnswers });

        // Generate embedding for query
        let results;
        try {
            logger.debug('Generating embedding for search query');
            const [queryEmbedding] = await generateEmbeddings(query);
            logger.debug('Performing vector search', { kbId, embeddingLength: queryEmbedding?.length });
            results = await vectorSearch(kbId, queryEmbedding, limit);
            logger.info('Vector search completed', { kbId, resultCount: results.length });
        } catch (embErr) {
            // Fallback to text search
            logger.warn('Vector search failed, using text search', { error: embErr.message });
            results = await textSearch(kbId, query, limit);
            logger.info('Text search fallback completed', { kbId, resultCount: results.length });
        }

        // Apply shorter answers if enabled
        const shouldUseShorterAnswers = useShorterAnswers !== undefined ? useShorterAnswers : kb.shorterAnswers;
        logger.debug('Shorter answers setting', { shouldUseShorterAnswers, hasResults: results.length > 0 });
        
        if (shouldUseShorterAnswers && results.length > 0) {
            logger.debug('Extracting shorter answers from results');
            const extractedContent = await extractRelevantExcerpts(query, results, kb.extractModel);
            logger.debug('Shorter answers extraction complete', { extractedLength: extractedContent?.length });
            return res.json({
                Status: 'Success',
                data: {
                    results,
                    extractedContent,
                    shorterAnswers: true
                }
            });
        }

        res.json({ 
            Status: 'Success', 
            data: {
                results,
                shorterAnswers: false
            }
        });
    } catch (err) {
        logger.error('Failed to search KB', { kbId: req.params.kbId, error: err.message });
        res.status(500).json({ Status: 'Error', Error: err.message });
    }
});

/**
 * Internal search endpoint for AI Chat tool integration
 * This bypasses auth for internal use
 */
async function internalKBSearch(kbId, query, limit = 5) {
    logger.info('internalKBSearch called', { kbId, query: query?.substring(0, 100), limit });
    
    const kb = await getKB(kbId);
    if (!kb) {
        logger.debug('internalKBSearch: KB not found', { kbId });
        return { error: 'KB not found' };
    }
    logger.debug('internalKBSearch: KB found', { kbId, kbName: kb.name, shorterAnswers: kb.shorterAnswers });

    let results;
    try {
        logger.debug('internalKBSearch: Generating query embedding', { kbId, queryLength: query.length });
        const [queryEmbedding] = await generateEmbeddings(query);
        logger.debug('internalKBSearch: Embedding generated, performing vector search', { kbId, embeddingDimensions: queryEmbedding?.length });
        results = await vectorSearch(kbId, queryEmbedding, limit);
        logger.info('internalKBSearch: Vector search completed', { kbId, resultCount: results.length });
    } catch (embErr) {
        logger.warn('Vector search failed, using text search', { kbId, error: embErr.message });
        results = await textSearch(kbId, query, limit);
        logger.info('internalKBSearch: Text search fallback completed', { kbId, resultCount: results.length });
    }

    if (kb.shorterAnswers && results.length > 0) {
        logger.debug('internalKBSearch: Extracting shorter answers', { kbId, extractModel: kb.extractModel });
        const extractedContent = await extractRelevantExcerpts(query, results, kb.extractModel);
        logger.debug('internalKBSearch: Shorter answers extraction complete', { kbId, extractedLength: extractedContent?.length });
        return {
            results,
            extractedContent,
            shorterAnswers: true
        };
    }

    logger.info('internalKBSearch: Returning results', { kbId, resultCount: results.length, shorterAnswers: false });
    return {
        results,
        shorterAnswers: false
    };
}

/**
 * Get list of available OpenAI chat models
 * @returns {Promise<Array<{id: string, name: string}>>} List of models
 */
async function listOpenAIModels() {
    const ai = getOpenAI();
    if (!ai) {
        logger.debug('listOpenAIModels: OpenAI API not configured');
        throw new Error('OpenAI API not configured');
    }

    try {
        logger.debug('listOpenAIModels: Fetching models from OpenAI');
        const response = await ai.models.list();
        
        // Filter for chat-compatible models and sort
        const chatModels = [];
        for await (const model of response) {
            // Include gpt and o1/o3/o4 models, exclude embedding/whisper/tts/dall-e models
            if ((model.id.includes('gpt') || model.id.match(/^o[0-9]/)) && 
                !model.id.includes('embedding') && 
                !model.id.includes('whisper') && 
                !model.id.includes('tts') && 
                !model.id.includes('dall-e') &&
                !model.id.includes('realtime') &&
                !model.id.includes('audio') &&
                !model.id.includes('transcribe') &&
                !model.id.includes('search') &&
                !model.id.includes('instruct')) {
                chatModels.push({
                    id: model.id,
                    name: model.id
                });
            }
        }

        // Sort: prioritize mini models, then by name
        chatModels.sort((a, b) => {
            const aIsMini = a.id.includes('mini');
            const bIsMini = b.id.includes('mini');
            if (aIsMini && !bIsMini) return -1;
            if (!aIsMini && bIsMini) return 1;
            return a.id.localeCompare(b.id);
        });

        logger.debug('listOpenAIModels: Found models', { count: chatModels.length });
        return chatModels;
    } catch (err) {
        logger.error('listOpenAIModels: Failed to fetch models', { error: err.message });
        throw err;
    }
}

module.exports = router;
module.exports.internalKBSearch = internalKBSearch;
module.exports.generateEmbeddings = generateEmbeddings;
module.exports.listOpenAIModels = listOpenAIModels;
