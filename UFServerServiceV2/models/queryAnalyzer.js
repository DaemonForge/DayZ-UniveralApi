// models/queryAnalyzer.js
// Tracks MongoDB query patterns and recommends indexes for performance optimization

const { createLogger } = require('../utils');
const logger = createLogger(global.logger, 'QueryAnalyzer');

class QueryAnalyzer {
  constructor() {
    // Track query patterns: { collection: { querySignature: { count, lastUsed, examples } } }
    this.queryPatterns = new Map();
    this.startTime = Date.now();
    this.maxPatternsPerCollection = 100; // Prevent memory leak
  }

  /**
   * Records a query for analysis
   * @param {string} collection - Collection name
   * @param {Object} query - MongoDB query object
   * @param {Object} sort - MongoDB sort object (optional)
   * @param {number} executionTime - Query execution time in ms (optional)
   */
  recordQuery(collection, query, sort = {}, executionTime = 0) {
    try {
      const signature = this._generateQuerySignature(query, sort);
      
      if (!this.queryPatterns.has(collection)) {
        this.queryPatterns.set(collection, new Map());
      }
      
      const collectionPatterns = this.queryPatterns.get(collection);
      
      if (!collectionPatterns.has(signature)) {
        // Evict least recently used pattern if over limit
        if (collectionPatterns.size >= this.maxPatternsPerCollection) {
          let oldestKey = null;
          let oldestTime = Date.now();
          
          for (const [sig, pattern] of collectionPatterns.entries()) {
            if (pattern.lastUsed < oldestTime) {
              oldestTime = pattern.lastUsed;
              oldestKey = sig;
            }
          }
          
          if (oldestKey) {
            collectionPatterns.delete(oldestKey);
            logger.debug(`Evicted old query pattern from ${collection}`);
          }
        }
        
        collectionPatterns.set(signature, {
          count: 0,
          fields: this._extractFields(query, sort),
          totalExecutionTime: 0,
          examples: [],
          firstSeen: Date.now(),
          lastUsed: Date.now()
        });
      }
      
      const pattern = collectionPatterns.get(signature);
      pattern.count++;
      pattern.lastUsed = Date.now();
      pattern.totalExecutionTime += executionTime;
      
      // Keep up to 3 example queries
      if (pattern.examples.length < 3) {
        pattern.examples.push({ query, sort, executionTime });
      }
      
      logger.debug(`Recorded query pattern for ${collection}`, { 
        signature, 
        count: pattern.count 
      });
    } catch (err) {
      logger.error('Failed to record query pattern', { error: err.message });
    }
  }

  /**
   * Generates a signature for a query to identify similar patterns
   * @private
   */
  _generateQuerySignature(query, sort) {
    const fields = this._extractFields(query, sort);
    return fields.sort().join('|');
  }

  /**
   * Extracts fields being queried and sorted
   * @private
   */
  _extractFields(query, sort) {
    const fields = new Set();
    
    // Extract query fields
    this._extractFieldsRecursive(query, fields);
    
    // Extract sort fields
    if (sort && typeof sort === 'object') {
      Object.keys(sort).forEach(key => fields.add(key));
    }
    
    return Array.from(fields);
  }

  /**
   * Recursively extracts field names from query object
   * @private
   */
  _extractFieldsRecursive(obj, fields, path = '') {
    if (!obj || typeof obj !== 'object') return;
    
    for (const [key, value] of Object.entries(obj)) {
      // Skip MongoDB operators
      if (key.startsWith('$')) {
        if (Array.isArray(value)) {
          value.forEach(item => this._extractFieldsRecursive(item, fields, path));
        } else if (typeof value === 'object') {
          this._extractFieldsRecursive(value, fields, path);
        }
        continue;
      }
      
      const fieldPath = path ? `${path}.${key}` : key;
      fields.add(fieldPath);
      
      if (typeof value === 'object' && !Array.isArray(value)) {
        this._extractFieldsRecursive(value, fields, fieldPath);
      }
    }
  }

  /**
   * Generates index recommendations based on query patterns
   * @param {number} minQueryCount - Minimum query count to recommend index (default: 5)
   * @returns {Object} Recommendations by collection
   */
  generateRecommendations(minQueryCount = 5) {
    const recommendations = {};
    const uptimeHours = (Date.now() - this.startTime) / (1000 * 60 * 60);
    
    for (const [collection, patterns] of this.queryPatterns.entries()) {
      recommendations[collection] = [];
      
      for (const [signature, pattern] of patterns.entries()) {
        // Only recommend if query is used frequently enough
        if (pattern.count < minQueryCount) continue;
        
        const avgExecutionTime = pattern.totalExecutionTime / pattern.count;
        const queriesPerHour = pattern.count / Math.max(uptimeHours, 0.1);
        
        // Generate index spec
        const indexSpec = {};
        pattern.fields.forEach(field => {
          indexSpec[field] = 1; // Ascending by default
        });
        
        // Calculate priority score (higher = more important)
        const priorityScore = this._calculatePriority(
          pattern.count,
          avgExecutionTime,
          queriesPerHour,
          pattern.fields.length
        );
        
        recommendations[collection].push({
          indexSpec,
          fields: pattern.fields,
          queryCount: pattern.count,
          avgExecutionTime: Math.round(avgExecutionTime),
          queriesPerHour: queriesPerHour.toFixed(2),
          priorityScore,
          firstSeen: new Date(pattern.firstSeen).toISOString(),
          lastUsed: new Date(pattern.lastUsed).toISOString(),
          example: pattern.examples[0],
          reason: this._generateReason(pattern, avgExecutionTime, queriesPerHour)
        });
      }
      
      // Sort by priority score (descending)
      recommendations[collection].sort((a, b) => b.priorityScore - a.priorityScore);
    }
    
    return recommendations;
  }

  /**
   * Calculates priority score for index recommendation
   * @private
   */
  _calculatePriority(queryCount, avgExecutionTime, queriesPerHour, fieldCount) {
    // Weighted scoring:
    // - Query frequency: 40%
    // - Execution time: 30%
    // - Queries per hour: 20%
    // - Field count penalty: 10% (multi-field indexes are more complex)
    
    const frequencyScore = Math.min(queryCount / 100, 1) * 40;
    const executionScore = Math.min(avgExecutionTime / 1000, 1) * 30;
    const rateScore = Math.min(queriesPerHour / 60, 1) * 20;
    const complexityPenalty = Math.max(0, (5 - fieldCount) / 5) * 10;
    
    return Math.round(frequencyScore + executionScore + rateScore + complexityPenalty);
  }

  /**
   * Generates human-readable reason for recommendation
   * @private
   */
  _generateReason(pattern, avgExecutionTime, queriesPerHour) {
    const reasons = [];
    
    if (pattern.count > 100) {
      reasons.push(`Very frequently used (${pattern.count} times)`);
    } else if (pattern.count > 50) {
      reasons.push(`Frequently used (${pattern.count} times)`);
    } else {
      reasons.push(`Used ${pattern.count} times`);
    }
    
    if (avgExecutionTime > 500) {
      reasons.push('slow execution time');
    } else if (avgExecutionTime > 100) {
      reasons.push('moderate execution time');
    }
    
    if (queriesPerHour > 60) {
      reasons.push('high query rate');
    } else if (queriesPerHour > 10) {
      reasons.push('regular usage');
    }
    
    return reasons.join(', ');
  }

  /**
   * Clears recorded query patterns
   */
  reset() {
    this.queryPatterns.clear();
    this.startTime = Date.now();
    logger.info('Query analyzer reset');
  }

  /**
   * Gets statistics about tracked queries
   */
  getStats() {
    const stats = {
      collections: this.queryPatterns.size,
      totalPatterns: 0,
      totalQueries: 0,
      uptimeHours: ((Date.now() - this.startTime) / (1000 * 60 * 60)).toFixed(2)
    };
    
    for (const patterns of this.queryPatterns.values()) {
      stats.totalPatterns += patterns.size;
      for (const pattern of patterns.values()) {
        stats.totalQueries += pattern.count;
      }
    }
    
    return stats;
  }
}

// Global singleton instance
let analyzerInstance = null;

function getAnalyzer() {
  if (!analyzerInstance) {
    analyzerInstance = new QueryAnalyzer();
    logger.info('Query analyzer initialized');
  }
  return analyzerInstance;
}

module.exports = {
  getAnalyzer,
  QueryAnalyzer
};
