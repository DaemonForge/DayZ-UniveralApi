const { Router } = require('express');
const https = require('https');
const { GenerateLimiter, createLogger } = require('../utils');
const logger = createLogger(global.logger, 'random');
const { requirePlayerOrServerAuth } = require("../auth/utils");
const cluster = require('cluster');

// HTTPS agent that ignores TLS errors (ANU's cert is expired)
const insecureAgent = new https.Agent({ rejectUnauthorized: false });

// Quantum source parameters and fallbacks.
const FETCH_TIMEOUT_MS = 10_000; // Avoid hanging fetches
const FAILURE_COOLDOWN_MS = 10 * 60 * 1000; // Pause quantum fetches after repeated failures
const MAX_POOL_SIZE = 100_000; // Safety cap for the shared pool
const FALLBACK_BATCH_COUNT = 1024; // How many JS numbers to add when quantum fetch fails

const router = Router();

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
            res.status(203);
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
            res.status(203);
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
        res.status(203);
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
            logger.info(`Fallback: Generated ${remainingCount} numbers using Math.random`, { requested: count });
        }
        
        logger.debug("Request completed for random numbers", { requested: count });
        return res.status(200).json({ Status: "Success", Error: "", Numbers: numbers });
    } catch (e) {
        logger.error(`Error in getRandom: ${e.message}`, { error: e, stack: e.stack });
        res.status(203);
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
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), FETCH_TIMEOUT_MS);
    try {
        const res = await fetch(`https://qrng.anu.edu.au/API/jsonI.php?length=${length}&type=hex16&size=${bitsize}`, {
            signal: controller.signal,
            agent: insecureAgent // ANU's SSL cert is expired
        });
        if (!res.ok) {
            throw new Error(`HTTP ${res.status}`);
        }
        return await res.json();
    } finally {
        clearTimeout(timeout);
    }
}

async function FillRandomNumbers(bitsize) {
    const now = Date.now();
    if (now < circuitOpenUntil) {
        logger.warn('Quantum source in cooldown; skipping fetch', { nextRetryInMs: circuitOpenUntil - now });
        return;
    }

    if (randomNumbers.length > MAX_POOL_SIZE) {
        return;
    }

    logger.info("Starting to fill the quantum random number pool", { currentPoolSize: randomNumbers.length, bitsize });
    let data = {};
    data.success = false;
    
    try {
        data = await fetchQuantum(1024, bitsize);
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
        logger.info('Filled pool with JS fallback numbers after quantum fetch failure', { newPoolSize: randomNumbers.length });
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
