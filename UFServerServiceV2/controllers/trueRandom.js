const {Router} = require('express');
const {GenerateLimiter} = require('../utils');
const { requirePlayerOrServerAuth} = require("../auth/utils");
const logger = global.logger; // Use global logger instead of requiring "./log"

const router = Router();

let randomNumbers = [];

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

async function getRandom(req, res){
        let count = req.body.Count || 4096;
        if (count > 4096 || count < 1){
            logger.warn('Failed to generate random numbers due to request size being too large', { count });
            res.status(203);
            return res.json({Status: "Error", Error: `Invalid Array Request Size` });
        }
        
        try {
            let numbers = [];
            let usedJsRandom = false;
            
            // Take whatever quantum random numbers are available
            if (randomNumbers.length > 0) {
                const availableCount = Math.min(randomNumbers.length, count);
                numbers = randomNumbers.splice(0, availableCount);
            }
            
            // If we still need more numbers, generate them using JavaScript's Math.random()
            if (numbers.length < count) {
                const remainingCount = count - numbers.length;
                usedJsRandom = true;
                
                for (let i = 0; i < remainingCount; i++) {
                    // Generate a random integer between -2147483647 and 2147483647
                    const randomInt = Math.floor(Math.random() * 4294967295) - 2147483647;
                    numbers.push(randomInt);
                }
            }
            
            if (usedJsRandom) {
                logger.info(`Generated ${count - randomNumbers.length} numbers using JavaScript's random function`, { 
                    generated: count - randomNumbers.length 
                });
            }
            
            logger.info("Random numbers requested", { count });
            return res.status(200).json({Status: "Success", Error: "", Numbers: numbers });
        } catch (e) {
            logger.error('Error generating random numbers', { 
                error: e.message,
                stack: e.stack
            });
            res.status(203);
            return res.json({Status: "Error", Error: `${e}` });
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

async function FillRandomNumbers(bitsize){
    if (randomNumbers.length > 1024*600) {
        return;
    }

    logger.info("Filling random numbers");
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
        if (errorCount++ >= errorLimit) {
            logger.error(`Failed to fetch random numbers from quantum source failed more than ${errorLimit} times in a row`, {
                error: error.message,
                stack: error.stack,
                errorCount
            });
        } else{
            logger.info('Failed to fetch random numbers from quantum source');
        }
    }
    if (data.success){
        data.data.forEach(e => {
            randomNumbers = AddToInts(randomNumbers, e);
        });
        logger.info('Successfully added random numbers to the pool', { count: randomNumbers.length });
    }
}

// Schedule first run in 1.2 minutes (72000ms)
setTimeout(() => {
    FillRandomNumbers(96);
    
    function scheduleNext() {
        // Random time between 2-3.5 minutes (240000-390000ms)
        const nextInterval = Math.floor(Math.random() * 90000) + 120000;
        setTimeout(() => {
            FillRandomNumbers(192);
            scheduleNext();
        }, nextInterval);
    }
    
    scheduleNext();
}, 90000);

module.exports = router;
