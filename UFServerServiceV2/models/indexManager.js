// models/indexManager.js
// Manages MongoDB indexes - retrieving current indexes, creating new ones, and analyzing coverage

const { createLogger } = require('../utils');
const { getDb } = require('./db');
const { getAnalyzer } = require('./queryAnalyzer');
const logger = createLogger(global.logger, 'IndexManager');

/**
 * Gets all collections in the database
 */
async function getCollections() {
  try {
    const db = await getDb();
    const collections = await db.listCollections().toArray();
    return collections.map(c => c.name).sort();
  } catch (err) {
    logger.error('Failed to list collections', { error: err.message });
    throw err;
  }
}

/**
 * Gets all indexes for a specific collection
 */
async function getIndexes(collectionName) {
  try {
    const db = await getDb();
    const collection = db.collection(collectionName);
    const indexes = await collection.indexes();
    
    // Enhance index info
    return indexes.map(idx => ({
      name: idx.name,
      key: idx.key,
      unique: idx.unique || false,
      sparse: idx.sparse || false,
      expireAfterSeconds: idx.expireAfterSeconds,
      fields: Object.keys(idx.key),
      size: null, // Will be populated by stats if needed
      usageStats: null // Can be populated from $indexStats
    }));
  } catch (err) {
    logger.error('Failed to get indexes', { collection: collectionName, error: err.message });
    throw err;
  }
}

/**
 * Gets all indexes for all collections
 */
async function getAllIndexes() {
  try {
    const collections = await getCollections();
    const allIndexes = {};
    
    for (const collectionName of collections) {
      allIndexes[collectionName] = await getIndexes(collectionName);
    }
    
    return allIndexes;
  } catch (err) {
    logger.error('Failed to get all indexes', { error: err.message });
    throw err;
  }
}

/**
 * Gets index recommendations based on query patterns
 */
async function getRecommendations(minQueryCount = 5) {
  try {
    const analyzer = getAnalyzer();
    const recommendations = analyzer.generateRecommendations(minQueryCount);
    const existingIndexes = await getAllIndexes();
    
    // Filter out recommendations for indexes that already exist
    const filteredRecommendations = {};
    
    for (const [collection, recs] of Object.entries(recommendations)) {
      filteredRecommendations[collection] = [];
      const collectionIndexes = existingIndexes[collection] || [];
      
      for (const rec of recs) {
        // Check if index already exists
        const alreadyExists = collectionIndexes.some(idx => {
          return _indexMatchesSpec(idx.key, rec.indexSpec);
        });
        
        if (!alreadyExists) {
          rec.status = 'recommended';
          filteredRecommendations[collection].push(rec);
        } else {
          // Mark as existing for informational purposes
          rec.status = 'exists';
          rec.existingIndexName = collectionIndexes.find(idx => 
            _indexMatchesSpec(idx.key, rec.indexSpec)
          )?.name;
        }
      }
    }
    
    return {
      recommendations: filteredRecommendations,
      stats: analyzer.getStats(),
      existingIndexes
    };
  } catch (err) {
    logger.error('Failed to generate recommendations', { error: err.message });
    throw err;
  }
}

/**
 * Checks if an existing index matches a proposed spec
 * @private
 */
function _indexMatchesSpec(existingKey, proposedSpec) {
  const existingFields = Object.keys(existingKey);
  const proposedFields = Object.keys(proposedSpec);
  
  // Must have same number of fields
  if (existingFields.length !== proposedFields.length) return false;
  
  // Must match all fields in order
  for (let i = 0; i < existingFields.length; i++) {
    if (existingFields[i] !== proposedFields[i]) return false;
  }
  
  return true;
}

/**
 * Creates an index on a collection
 */
async function createIndex(collectionName, indexSpec, options = {}) {
  try {
    // Validate collection exists
    const validCollections = await getCollections();
    if (!validCollections.includes(collectionName)) {
      throw new Error(`Invalid collection: ${collectionName}`);
    }
    
    // Validate index spec
    if (!indexSpec || typeof indexSpec !== 'object') {
      throw new Error('Invalid index specification');
    }
    
    const fieldCount = Object.keys(indexSpec).length;
    if (fieldCount === 0) {
      throw new Error('Index must have at least one field');
    }
    
    if (fieldCount > 10) {
      throw new Error('Index cannot have more than 10 fields (performance concern)');
    }
    
    const db = await getDb();
    const collection = db.collection(collectionName);
    
    logger.info(`Creating index on ${collectionName}`, { indexSpec, options });
    
    const result = await collection.createIndex(indexSpec, options);
    
    logger.info(`Index created successfully`, { 
      collection: collectionName, 
      indexName: result 
    });
    
    return { success: true, indexName: result };
  } catch (err) {
    logger.error('Failed to create index', { 
      collection: collectionName, 
      indexSpec, 
      error: err.message 
    });
    throw err;
  }
}

/**
 * Creates multiple indexes in batch
 */
async function createIndexes(indexRequests) {
  const results = [];
  
  for (const req of indexRequests) {
    try {
      const result = await createIndex(req.collection, req.indexSpec, req.options || {});
      results.push({
        collection: req.collection,
        indexSpec: req.indexSpec,
        success: true,
        indexName: result.indexName
      });
    } catch (err) {
      results.push({
        collection: req.collection,
        indexSpec: req.indexSpec,
        success: false,
        error: err.message
      });
    }
  }
  
  return results;
}

/**
 * Drops an index from a collection
 */
async function dropIndex(collectionName, indexName) {
  try {
    const db = await getDb();
    const collection = db.collection(collectionName);
    
    // Don't allow dropping _id index
    if (indexName === '_id_') {
      throw new Error('Cannot drop _id index');
    }
    
    logger.info(`Dropping index ${indexName} from ${collectionName}`);
    
    await collection.dropIndex(indexName);
    
    logger.info(`Index dropped successfully`, { 
      collection: collectionName, 
      indexName 
    });
    
    return { success: true };
  } catch (err) {
    logger.error('Failed to drop index', { 
      collection: collectionName, 
      indexName, 
      error: err.message 
    });
    throw err;
  }
}

/**
 * Gets index usage statistics (requires MongoDB 3.2+)
 */
async function getIndexStats(collectionName) {
  try {
    const db = await getDb();
    const collection = db.collection(collectionName);
    
    const stats = await collection.aggregate([
      { $indexStats: {} }
    ]).toArray();
    
    return stats;
  } catch (err) {
    logger.warn('Failed to get index stats (may not be supported)', { 
      collection: collectionName, 
      error: err.message 
    });
    return [];
  }
}

/**
 * Analyzes a collection's indexes and provides insights
 */
async function analyzeCollection(collectionName) {
  try {
    const indexes = await getIndexes(collectionName);
    const stats = await getIndexStats(collectionName);
    const db = await getDb();
    const collection = db.collection(collectionName);
    
    // Get collection stats
    const collStats = await db.command({ collStats: collectionName });
    
    return {
      collection: collectionName,
      documentCount: collStats.count,
      avgDocumentSize: collStats.avgObjSize,
      totalSize: collStats.size,
      indexes: indexes.map(idx => {
        const usage = stats.find(s => s.name === idx.name);
        return {
          ...idx,
          usageStats: usage ? {
            ops: usage.accesses.ops,
            since: usage.accesses.since
          } : null
        };
      })
    };
  } catch (err) {
    logger.error('Failed to analyze collection', { 
      collection: collectionName, 
      error: err.message 
    });
    throw err;
  }
}

module.exports = {
  getCollections,
  getIndexes,
  getAllIndexes,
  getRecommendations,
  createIndex,
  createIndexes,
  dropIndex,
  getIndexStats,
  analyzeCollection
};
