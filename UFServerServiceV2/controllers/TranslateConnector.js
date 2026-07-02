const { Router } = require('express');
// fetch is a Node global (>=18) - no node-fetch import needed.
const { requirePlayerOrServerAuth } = require('../auth/utils');
const { GenerateLimiter, createLogger } = require('../utils');
const logger = createLogger(global.logger, 'translate'); // Winston based logger
const querystring = require('querystring');

const router = Router();

router.use(GenerateLimiter(global.config.RequestLimitTranslate || 200, 10));

router.post('/', requirePlayerOrServerAuth, (req, res) => {
    if (global.config.Translate !== undefined &&
        global.config.Translate.Type === "Microsoft" &&
        global.config.Translate.SubscriptionKey !== "") {
        runTranslate(req, res, req.headers['auth-key']);
    } else {
        const errorMessage = "No Translation Configuration Found";
        logger.error(`Translation Error: ${errorMessage}`, { error: errorMessage });
        res.json({ Status: "Error" });
    }
});

async function runTranslate(req, res, auth) {
    try {
        let Tconfig = global.config.Translate;
        let text = req.body.Text;
        let lang = req.body.From;
        let queryobj = {
            "api-version": '3.0',
            to: req.body.To
        };

        if (lang !== undefined && lang !== "" && lang.toLowerCase() !== "auto") {
            queryobj.from = lang;
        }
        let querystr = querystring.stringify(queryobj);
        let json = await fetch(`${Tconfig.Endpoint}?${querystr}`, {
            method: "post",
            headers: {
                "Ocp-Apim-Subscription-Key": Tconfig.SubscriptionKey,
                "Ocp-Apim-Subscription-Region": Tconfig.SubscriptionRegion,
                "Content-Type": "application/json"
            },
            body: JSON.stringify([{ text }])
        }).then(response => response.json());

        let responseData;
        if (json[0] !== undefined && json[0].translations !== undefined) {
            responseData = {
                Status: "Success",
                Error: "",
                Translations: json[0].translations,
                // detectedLanguage is only returned when the source language is
                // auto-detected (no "From" supplied); fall back to the requested
                // source language otherwise so a successful translation isn't
                // turned into an error.
                Detected: json[0].detectedLanguage?.language || lang || ""
            }
        } else {
            let errorDetail = "Not a valid response from the API";
            if (json.error !== undefined) {
                errorDetail = json.error.message;
            }
            logger.error(`Translation Error: ${errorDetail}`, { error: errorDetail });
            responseData = {
                Status: "Error",
                Error: errorDetail,
                Translations: [{ text: "NA", to: "NA" }],
                Detected: "NA"
            }
        }
        logger.info(`Translation Request processed`, {
            from: lang,
            to: req.body.To,
            text,
            response: responseData
        });
        res.status(200).json(responseData);

    } catch (e) {
        logger.error(`Translation Error: ${e.message}`, { error: e, stack: e.stack });
        return res.status(200).json({
            Status: "Error",
            Error: `${e}`,
            Translations: [{ text: "NA", to: "NA" }],
            Detected: "NA"
        });
    }
}

module.exports = router;
