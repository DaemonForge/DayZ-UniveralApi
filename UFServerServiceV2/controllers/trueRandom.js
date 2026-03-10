const { Router } = require('express');
const { Agent, setGlobalDispatcher } = require('undici');
const { GenerateLimiter, createLogger } = require('../utils');
const logger = createLogger(global.logger, 'random');
const { requirePlayerOrServerAuth } = require("../auth/utils");
const cluster = require('cluster');
const fs = require('fs').promises;
const path = require('path');

// Agent for fetch() that ignores SSL errors (ANU's cert is expired)
const insecureAgent = new Agent({
    connect: {
        rejectUnauthorized: false
    }
});

// Set this as the global dispatcher for all fetch calls in this module
// This makes all fetch() calls use the insecure agent
setGlobalDispatcher(insecureAgent);

// Quantum source parameters and fallbacks.
const FETCH_TIMEOUT_MS = 60 * 60 * 1000; // 60 minutes
const FAILURE_COOLDOWN_MS = 15 * 60 * 1000; // Pause quantum fetches after repeated failures
const MAX_POOL_SIZE = 128_000; // Safety cap for the shared pool
const FALLBACK_BATCH_COUNT = 1024; // How many JS numbers to add when quantum fetch fails
const QUANTUM_BATCH_SIZE = 512; // Reduced batch size for quantum fetches

// Persistence configuration
const POOL_FILE = path.join(global.SAVEPATH || '.', 'data', 'randomPool.json');
const POOL_SAVE_INTERVAL = 15 * 60 * 1000; // Save every 15 minutes
const POOL_MAX_AGE = 7 * 24 * 60 * 60 * 1000; // Pool data expires after 7 days

const router = Router();

// Persistence tracking
let lastSaveTimestamp = 0;
let lastSaveCount = 0;
let saveInProgress = false;

/**
 * Load random number pool from disk on startup (async, non-blocking)
 * Only loads if the saved pool is recent and valid
 */
async function loadRandomPool() {
    try {
        const data = await fs.readFile(POOL_FILE, 'utf8');
        const { numbers, savedAt, consumedCount } = JSON.parse(data);
        
        const age = Date.now() - savedAt;
        
        // Validate data
        if (!Array.isArray(numbers)) {
            logger.warn('Saved random pool has invalid format, starting fresh');
            return;
        }
        
        if (age > POOL_MAX_AGE) {
            logger.info('Saved random pool is too old, starting fresh', { ageHours: Math.round(age / (60 * 60 * 1000)) });
            return;
        }
        
        // Load the pool
        randomNumbers = numbers;
        lastSaveTimestamp = savedAt;
        lastSaveCount = consumedCount || 0;
        
        logger.info('Loaded random number pool from disk', { 
            count: numbers.length, 
            ageMinutes: Math.round(age / (60 * 1000)),
            totalConsumed: lastSaveCount
        });
    } catch (err) {
        if (err.code === 'ENOENT') {
            logger.debug('No saved random pool found, starting fresh');
        } else {
            logger.warn('Failed to load random pool, starting fresh', { error: err.message });
        }
    }
}

/**
 * Save random number pool to disk (async, non-blocking)
 * Tracks consumed count to prevent any possibility of reusing numbers
 */
async function saveRandomPool() {
    if (saveInProgress) {
        logger.debug('Save already in progress, skipping');
        return;
    }
    
    // Don't save if pool is empty
    if (randomNumbers.length === 0) {
        logger.debug('Pool is empty, skipping save');
        return;
    }
    
    saveInProgress = true;
    
    try {
        // Ensure data directory exists
        const dataDir = path.dirname(POOL_FILE);
        await fs.mkdir(dataDir, { recursive: true });
        
        // Track how many numbers have been consumed since last save
        // This prevents any possibility of reusing numbers from an old save
        const currentCount = randomNumbers.length;
        const consumedSinceLastSave = lastSaveCount - currentCount;
        const totalConsumed = lastSaveCount + Math.max(0, consumedSinceLastSave);
        
        const saveData = {
            numbers: randomNumbers,
            savedAt: Date.now(),
            consumedCount: totalConsumed,
            poolSize: currentCount
        };
        
        await fs.writeFile(POOL_FILE, JSON.stringify(saveData), 'utf8');
        
        lastSaveTimestamp = saveData.savedAt;
        lastSaveCount = currentCount;
        
        logger.debug('Saved random number pool to disk', { 
            count: currentCount,
            totalConsumed
        });
    } catch (err) {
        logger.warn('Failed to save random pool', { error: err.message });
    } finally {
        saveInProgress = false;
    }
}

let randomNumbers = [];
if (cluster.isMaster) {
    // Master process holds the shared random number pool.
    randomNumbers = [];

    // Listen for requests from workers.
    cluster.on('message', (worker, msg) => {
        if (msg.type === 'getNumbers') {
            const count = msg.count;
            // Take only as many numbers as are available.
            const availableCount = Math.min(randomNumbers.length, count);
            const numbers = randomNumbers.splice(0, availableCount);
            logger.debug(`Master allocated ${availableCount} random numbers to worker ${worker.id}`, { requested: count, remaining: randomNumbers.length });
            worker.send({ type: 'randomNumbersResponse', id: msg.id, numbers });
        }
    });
} else {
    // Worker process: do not keep an independent copy of the pool.
    // Instead, workers will request numbers from the master.
    const pendingRequests = new Map();

    process.on('message', msg => {
        if (msg.type === 'randomNumbersResponse') {
            const resolve = pendingRequests.get(msg.id);
            if (resolve) {
                logger.debug(`Worker received ${msg.numbers.length} numbers for request id ${msg.id}`);
                resolve(msg.numbers);
                pendingRequests.delete(msg.id);
            }
        }
    });

    // Helper that asks the master for numbers.
    function requestNumbers(count) {
        return new Promise(resolve => {
            const id = Date.now() + Math.random();
            pendingRequests.set(id, resolve);
            process.send({ type: 'getNumbers', id, count });
        });
    }

    // Override getRandom to request numbers from the master so that each number is used only once.
    const originalGetRandom = getRandom;
    getRandom = async function(req, res) {
        let count = req.body.Count || 4096;
        if (count > 4096 || count < 1) {
            logger.warn(`Request rejected due to invalid count: ${count}`, { count });
            res.status(200);
            return res.json({ Status: "Error", Error: "Invalid Array Request Size" });
        }
        try {
            // Ask master for quantum numbers. This ensures no duplicate use.
            let qNumbers = await requestNumbers(count);
            let numbers = qNumbers;
            let usedJsRandom = false;
            // If not enough numbers were returned, fill in the remainder with fallback JS numbers.
            if (qNumbers.length < count) {
                const remainingCount = count - qNumbers.length;
                usedJsRandom = true;
                for (let i = 0; i < remainingCount; i++) {
                    const randomInt = Math.floor(Math.random() * 4294967295) - 2147483647;
                    numbers.push(randomInt);
                }
            }
            if (usedJsRandom) {
                logger.info(`Fallback: Generated ${count - qNumbers.length} numbers using JavaScript's Math.random`, {
                    requested: count,
                    quantumProvided: qNumbers.length
                });
            }
            logger.debug("Worker processed random number request", { requested: count });
            return res.status(200).json({ Status: "Success", Error: "", Numbers: numbers });
        } catch (e) {
            logger.error(`Error generating random numbers: ${e.message}`, { error: e, stack: e.stack });
            res.status(200);
            return res.json({ Status: "Error", Error: `${e}` });
        }
    };
}
router.use(GenerateLimiter(200, 10));

/**
 *  Quantum Random Number Generator -2147483647 to 2147483647
 *  Post: /Random
 *
 *  Description: This endpoint generates the specified amount of random numbers from 
 *    ANU's Quantum Random number API within the range of -2147483647 to 2147483647
 *
 *  Accepts: `{ "Count": |NumberToGenerate| }`
 *
 *  Returns: `{ 
 *                 "Status": "|STATUSOFREQUEST|", 
 *                 "Error": "|ANYERRORMESSAGE|",
 *                 "Numbers": [|ARRAYOFINTEGERS|] 
 *            }`
 *
 */
router.post('/', requirePlayerOrServerAuth, getRandom);

let errorCount = 0;
let errorLimit = 3;
let consecutiveFailures = 0;
let circuitOpenUntil = 0;

async function getRandom(req, res) {
    let count = req.body.Count || 4096;
    if (count > 4096 || count < 1) {
        logger.warn(`Request rejected due to invalid count: ${count}`, { count });
        res.status(200);
        return res.json({ Status: "Error", Error: `Invalid Array Request Size` });
    }
    
    try {
        let numbers = [];
        let usedJsRandom = false;
        
        // Retrieve available quantum random numbers.
        if (randomNumbers.length > 0) {
            const availableCount = Math.min(randomNumbers.length, count);
            numbers = randomNumbers.splice(0, availableCount);
            logger.debug(`Master provided ${availableCount} quantum random numbers`, { requested: count, remaining: randomNumbers.length });
        }
        
        // Use JS's Math.random if more numbers are needed.
        if (numbers.length < count) {
            const remainingCount = count - numbers.length;
            usedJsRandom = true;
            
            for (let i = 0; i < remainingCount; i++) {
                const randomInt = Math.floor(Math.random() * 4294967295) - 2147483647;
                numbers.push(randomInt);
            }
            logger.debug(`Fallback: Generated ${remainingCount} numbers using Math.random`, { requested: count });
        }
        
        logger.debug("Request completed for random numbers", { requested: count });
        return res.status(200).json({ Status: "Success", Error: "", Numbers: numbers });
    } catch (e) {
        logger.error(`Error in getRandom: ${e.message}`, { error: e, stack: e.stack });
        res.status(200);
        return res.json({ Status: "Error", Error: `${e}` });
    }
}

function AddToInts(ints, hex) {
    const buf = Buffer.from(hex, "hex");
    const intCount = Math.floor(buf.length / 4);
    
    for (let i = 0; i < intCount; i++) {
        ints.push(buf.readInt32LE(i * 4));
    }
    
    return ints;
}

function fillWithJsRandom(targetArray, count) {
    const capacity = Math.max(0, MAX_POOL_SIZE - targetArray.length);
    const toGenerate = Math.min(count, capacity);
    for (let i = 0; i < toGenerate; i++) {
        const randomInt = Math.floor(Math.random() * 4294967295) - 2147483647;
        targetArray.push(randomInt);
    }
    return targetArray;
}

async function fetchQuantum(length, bitsize) {
    const url = `https://qrng.anu.edu.au/API/jsonI.php?length=${length}&type=hex16&size=${bitsize}`;
    logger.debug(`Starting quantum fetch`, { length, bitsize, url, timeoutMinutes: FETCH_TIMEOUT_MS / 60000 });
    
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), FETCH_TIMEOUT_MS);
    
    try {
        const response = await fetch(url, {
            signal: controller.signal
        });
        
        clearTimeout(timeout);
        
        if (!response.ok) {
            // Read the response body to get the actual error message
            let errorMsg = `HTTP ${response.status}`;
            try {
                const errorText = await response.text();
                if (errorText) {
                    errorMsg += `: ${errorText}`;
                }
            } catch (e) {
                // Ignore if we can't read the body
            }
            throw new Error(errorMsg);
        }
        
        const data = await response.json();
        logger.debug('Quantum fetch successful', { dataSize: data.data ? data.data.length : 0 });
        return data;
        
    } catch (error) {
        clearTimeout(timeout);
        
        if (error.name === 'AbortError') {
            logger.error(`Quantum fetch timeout after ${FETCH_TIMEOUT_MS}ms`, { url });
            throw new Error(`Request timeout after ${FETCH_TIMEOUT_MS}ms`);
        }
        
        logger.error(`Quantum fetch error: ${error.message}`, { error: error.code || error.name, url });
        throw error;
    }
}

async function FillRandomNumbers(bitsize) {
    const now = Date.now();
    if (now < circuitOpenUntil) {
        logger.debug('Quantum source in cooldown; skipping fetch', { nextRetryInMs: circuitOpenUntil - now });
        return;
    }

    if (randomNumbers.length >= MAX_POOL_SIZE) {
        logger.debug('Random number pool is full; skipping fetch', { poolSize: randomNumbers.length, maxSize: MAX_POOL_SIZE });
        return;
    }

    logger.debug("Starting to fill the quantum random number pool", { currentPoolSize: randomNumbers.length, bitsize });
    let data = {};
    data.success = false;
    
    try {
        data = await fetchQuantum(QUANTUM_BATCH_SIZE, bitsize);
        if (!data || !data.data || !Array.isArray(data.data)) {
            throw new Error('Quantum source returned invalid payload');
        }
        data.success = true;
        logger.debug('Successfully fetched random numbers from quantum source', {
            dataSize: data.data ? data.data.length : 0
        });
        errorCount = 0;
        consecutiveFailures = 0;
    } catch (error) {
        errorCount++;
        consecutiveFailures++;

        const shouldOpenCircuit = consecutiveFailures >= errorLimit;
        if (shouldOpenCircuit) {
            circuitOpenUntil = Date.now() + FAILURE_COOLDOWN_MS;
        }

        logger.warn(`Failed to fetch random numbers from quantum source: ${error.message}`, {
            consecutiveFailures,
            willCooldown: shouldOpenCircuit,
            nextRetryInMs: shouldOpenCircuit ? FAILURE_COOLDOWN_MS : 0,
            stack: error.stack
        });

        // Ensure we still have some entropy available even if quantum source is down.
        fillWithJsRandom(randomNumbers, FALLBACK_BATCH_COUNT);
        logger.debug('Filled pool with JS fallback numbers after quantum fetch failure', { newPoolSize: randomNumbers.length });
        return;
    }
    if (data.success) {
        const capacity = Math.max(0, MAX_POOL_SIZE - randomNumbers.length);
        let added = 0;
        for (const e of data.data) {
            if (randomNumbers.length >= MAX_POOL_SIZE) {
                break;
            }
            const before = randomNumbers.length;
            randomNumbers = AddToInts(randomNumbers, e);
            if (randomNumbers.length > MAX_POOL_SIZE) {
                randomNumbers = randomNumbers.slice(0, MAX_POOL_SIZE);
            }
            added += randomNumbers.length - before;
            if (added >= capacity) {
                break;
            }
        }
        logger.debug('Added fetched random numbers to the pool', { added, newPoolSize: randomNumbers.length, capacity });
    }
}

if (cluster.isMaster) {
    // Load persisted pool asynchronously (non-blocking startup)
    loadRandomPool().catch(err => {
        logger.error('Failed to load random pool on startup', { error: err.message });
    });
    
    // Save pool periodically
    setInterval(() => {
        saveRandomPool().catch(err => {
            logger.error('Failed to save random pool', { error: err.message });
        });
    }, POOL_SAVE_INTERVAL);
    
    // Save on graceful shutdown
    process.on('SIGTERM', async () => {
        logger.info('SIGTERM received, saving random pool...');
        await saveRandomPool();
        process.exit(0);
    });
    
    process.on('SIGINT', async () => {
        logger.info('SIGINT received, saving random pool...');
        await saveRandomPool();
        process.exit(0);
    });
    
    // Schedule first run in 90 seconds.
    setTimeout(() => {
        FillRandomNumbers(96);
        
        function scheduleNext() {
            // Random interval: between 2 and 3.5 minutes.
            const nextInterval = Math.floor(Math.random() * 90000) + 120000;
            setTimeout(() => {
                FillRandomNumbers(192);
                scheduleNext();
            }, nextInterval);
        }
        
        scheduleNext();
    }, 90000);
}

module.exports = router;
