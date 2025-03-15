const { Router } = require('express');
const { GenerateLimiter, createLogger } = require('../utils');
const logger = createLogger(global.logger, 'random');
const { requirePlayerOrServerAuth } = require("../auth/utils");
const cluster = require('cluster');

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
            logger.info(`Master allocated ${availableCount} random numbers to worker ${worker.id}`, { requested: count, remaining: randomNumbers.length });
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
                logger.info(`Worker received ${msg.numbers.length} numbers for request id ${msg.id}`);
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
            logger.info("Worker processed random number request", { requested: count });
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
            logger.info(`Master provided ${availableCount} quantum random numbers`, { requested: count, remaining: randomNumbers.length });
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
        
        logger.info("Request completed for random numbers", { requested: count });
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

async function FillRandomNumbers(bitsize) {
    if (randomNumbers.length > 1024 * 600) {
        return;
    }

    logger.info("Starting to fill the quantum random number pool", { currentPoolSize: randomNumbers.length, bitsize });
    let data = {};
    data.success = false;
    
    try {
        const res = await fetch(`https://qrng.anu.edu.au/API/jsonI.php?length=1024&type=hex16&size=${bitsize}`);
        data = await res.json();
        data.success = true;
        logger.info('Successfully fetched random numbers from quantum source', {
            dataSize: data.data ? data.data.length : 0,
            responseStatus: res.status
        });
        errorCount = 0;
    } catch (error) {
        errorCount++;
        if (errorCount >= errorLimit) {
            logger.error(`Failed to fetch random numbers from quantum source after ${errorCount} attempts: ${error.message}`, { error, stack: error.stack });
        } else {
            logger.warn(`Attempt ${errorCount} - Failed to fetch random numbers: ${error.message}`, { error: error.message });
        }
    }
    if (data.success) {
        data.data.forEach(e => {
            randomNumbers = AddToInts(randomNumbers, e);
        });
        logger.info('Added fetched random numbers to the pool', { newPoolSize: randomNumbers.length });
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
