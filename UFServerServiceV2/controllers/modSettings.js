// controllers/modSettings.js
// REST API controller for Mod Settings - custom HTML settings pages uploaded by mods

const { Router } = require("express");
const router = Router();
const { requireServerAuth } = require("../auth/utils");
const {
  listModSettings,
  getModSettings,
  upsertModSettings,
  deleteModSettings,
} = require("../models/modSettings");
const { createLogger } = require("../utils");

const logger = createLogger(global.logger, "modSettings");

// Validate modId format: lowercase alphanumeric, hyphens, underscores, 1-64 chars
const MOD_ID_REGEX = /^[a-zA-Z0-9_-]{1,64}$/;

/**
 * GET /ModSettings/List
 * Returns all registered mod settings pages (metadata only, no template HTML).
 * Requires server auth.
 */
router.get("/List", requireServerAuth, async (req, res) => {
  try {
    const data = await listModSettings();
    res.json({ Status: "Success", Data: data });
  } catch (err) {
    logger.error("Failed to list mod settings", { error: err.message });
    res.status(500).json({ Status: "Error", Error: err.message });
  }
});

/**
 * GET /ModSettings/Get/:modId
 * Returns the full mod settings document including the HTML template.
 * Requires server auth.
 */
router.get("/Get/:modId", requireServerAuth, async (req, res) => {
  try {
    const { modId } = req.params;
    if (!MOD_ID_REGEX.test(modId)) {
      return res.status(400).json({ Status: "Error", Error: "Invalid modId format" });
    }
    const doc = await getModSettings(modId);
    if (!doc) {
      return res.status(404).json({ Status: "Error", Error: "Mod settings not found" });
    }
    res.json({ Status: "Success", Data: doc });
  } catch (err) {
    logger.error("Failed to get mod settings", { modId: req.params.modId, error: err.message });
    res.status(500).json({ Status: "Error", Error: err.message });
  }
});

/**
 * GET /ModSettings/Template/:modId
 * Returns ONLY the raw HTML template for rendering in an iframe / sandbox.
 * Requires server auth.
 */
router.get("/Template/:modId", requireServerAuth, async (req, res) => {
  try {
    const { modId } = req.params;
    if (!MOD_ID_REGEX.test(modId)) {
      return res.status(400).json({ Status: "Error", Error: "Invalid modId format" });
    }
    const doc = await getModSettings(modId);
    if (!doc || !doc.template) {
      return res.status(404).json({ Status: "Error", Error: "Template not found" });
    }
    res.setHeader("Content-Type", "text/html; charset=utf-8");
    res.send(doc.template);
  } catch (err) {
    logger.error("Failed to get template", { modId: req.params.modId, error: err.message });
    res.status(500).json({ Status: "Error", Error: err.message });
  }
});

/**
 * POST /ModSettings/Register/:modId
 * Registers or updates a mod settings page. Body is JSON:
 * {
 *   "modName": "My Factions Mod",
 *   "author": "AuthorName",
 *   "template": "<html>...</html>",
 *   "globals": ["MyMod_Factions", "MyMod_Config"]
 * }
 * Requires server auth.
 */
router.post("/Register/:modId", requireServerAuth, async (req, res) => {
  try {
    const { modId } = req.params;
    if (!MOD_ID_REGEX.test(modId)) {
      return res.status(400).json({ Status: "Error", Error: "Invalid modId format. Use alphanumeric, hyphens, and underscores (1-64 chars)." });
    }
    const payload = req.body;
    if (!payload || typeof payload !== "object") {
      return res.status(400).json({ Status: "Error", Error: "Request body must be a JSON object" });
    }
    if (!payload.template || typeof payload.template !== "string") {
      return res.status(400).json({ Status: "Error", Error: "template field is required and must be a string" });
    }
    if (payload.template.length > 2 * 1024 * 1024) {
      return res.status(400).json({ Status: "Error", Error: "Template exceeds maximum size of 2MB" });
    }

    const doc = await upsertModSettings(modId, payload);
    logger.info("Mod settings registered", { modId, modName: doc.modName });
    res.json({ Status: "Success", Data: { modId: doc.modId, modName: doc.modName } });
  } catch (err) {
    logger.error("Failed to register mod settings", { modId: req.params.modId, error: err.message });
    res.status(500).json({ Status: "Error", Error: err.message });
  }
});

/**
 * DELETE /ModSettings/Delete/:modId
 * Removes a mod settings page.
 * Requires server auth.
 */
router.delete("/Delete/:modId", requireServerAuth, async (req, res) => {
  try {
    const { modId } = req.params;
    if (!MOD_ID_REGEX.test(modId)) {
      return res.status(400).json({ Status: "Error", Error: "Invalid modId format" });
    }
    const removed = await deleteModSettings(modId);
    if (!removed) {
      return res.status(404).json({ Status: "Error", Error: "Mod settings not found" });
    }
    logger.info("Mod settings deleted", { modId });
    res.json({ Status: "Success" });
  } catch (err) {
    logger.error("Failed to delete mod settings", { modId: req.params.modId, error: err.message });
    res.status(500).json({ Status: "Error", Error: err.message });
  }
});

module.exports = router;
