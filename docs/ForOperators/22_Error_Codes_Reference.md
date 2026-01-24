# Error Codes & Messages Reference

This document provides a comprehensive reference for error messages you may encounter in the UF Server Service logs and DayZ server logs (RPT).

---

## ðŸ–¥ï¸ UF Server Service Errors (Node.js)

These errors appear in the service logs (`%APPDATA%\ufserverservice\logs\` or `/var/lib/ufserverservice/logs/`) or the console.

### WebServer & Connectivity

| Log Message | Meaning | Solution |
|-------------|---------|----------|
| `[WebServer] RateLimit reached - possible DDoS attack or need to increase request limit` | A client (server or player) is making too many API requests in a short time. | Check your mod's loops. Increase `RequestLimit*` values in `config.json` if legitimate traffic. |
| `[WebServer] Bad Request` | The API received malformed JSON or invalid data. | Check the client code sending the request. Ensure `content-type` is `application/json`. |
| `[WebServer] HTTPS server error` | Failed to start the SSL/HTTPS server. | Check certificate paths in `config.json`. Ensure port is not in use. |
| `[WebServer] Greenlock accounts directory missing` | Let's Encrypt account data is missing. | Normal on first run. If persistent, check directory permissions. |
| `[App] Worker died, restarting` | A clustered worker process crashed. | Review previous error logs for the cause (e.g., Out of Memory). |

### Database (MongoDB)

| Log Message | Meaning | Solution |
|-------------|---------|----------|
| `MongoDB connection error` | Service cannot connect to MongoDB. | Ensure MongoDB is running. Check `DBServer` connection string. Verify firewall allows port 27017. |
| `MongoDB connection lost, reconnecting...` | Connection dropped unexpectedly. | Usually temporary. If frequent, check network stability or database load. |
| `Failed to create indexes` | could not enforce unique keys/indexes. | Data in DB might conflict with unique constraints. Check `config.json` `CreateIndexes` setting. |

### AI & Knowledge Base

| Log Message | Meaning | Solution |
|-------------|---------|----------|
| `[KB] Could not create embedding index` | Vector search index creation failed. | Check MongoDB version (needs to support Atlas Search or Vector Search if using Atlas). |
| `[KB] Failed to generate embeddings` | OpenAI API failed to return vector data. | Check `OpenAIApi.ApiKey`. Check OpenAI quotas/credits. |
| `generateEmbeddings: Some texts were empty` | Attempted to index a document with no content. | Check the source file you uploaded to the KB. |

### authentication

| Log Message | Meaning | Solution |
|-------------|---------|----------|
| `AUTH ERROR` | Failed to generate or validate a token. | Check if MongoDB is writable. Ensure Server ID is correct. |

---

## ðŸŽ® Mod / SDK Errors (DayZ Server RPT)

These errors appear in your DayZ server's `.RPT` log file or script log, prefixed with `[UF]`.

### Core System

| Error Message | Meaning | Solution |
|---------------|---------|----------|
| `[UF] Webservice is outdated and should be updated right away` | The backend service version is older than the mod version. | Update the UF Server Service (run installer or script). |
| `[UF] [Api] UniversalRest.Post called with invalid token` | The server authentication key is missing or incorrect. | Check `_UFramework/UFramework.json`. Ensure `ServerAuth` matches a key in service `config.json`. |
| `[UF] Failed to create Query Results` | Database query returned invalid structure. | Check if service is online. Verify DB operation (Save/Load) parameters. |

### Discord Integration

| Error Message | Meaning | Solution |
|---------------|---------|----------|
| `[UF] [UDiscordUser] Error: Cannot add role, user ID is invalid` | Attempted to role a player who hasn't linked Discord. | Check `CanAddRole()` or `HasDiscord()` before calling `AddRole`. |
| `[UF] [UDiscordUser] Failed to add role for user: ... Error: ...` | Discord API refused the request. | Bot role might be too low in hierarchy. Bot might be missing "Manage Roles" permission. |

### AI Chat

| Error Message | Meaning | Solution |
|---------------|---------|----------|
| `[UF] [UAIChatHandler] JSON schema is required for typed responses` | You used `UAIChatAgent<T>` but didn't provide a schema. | Override `GetJsonSchema()` in your agent class. |
| `[UF] [UAIChatHandler] Cannot send message, no chat ID set` | Attempted to chat before session was created. | Ensure `StartSession()` or `Chat()` (which calls Create) completes before sending options. |
| `[UF] [UAIChatHandlerBase] SendMessage not implemented in base class` | Using the abstract base class directly. | Use `UFAIChatAgent` or `UAIChatAgent<T>` instead. |

### Weapons & Items (Informational)

| Message | Meaning | Solution |
|---------|---------|----------|
| `[UF] [INFO] Validating and Repairing the Weapon...` | The mod detected a weapon in an invalid state (glitched) and fixed it. | **Not an error.** This prevents server crashes. Ignore it unless it spams. |
| `[UF] Pushing Round to Chamber` | Weapon logic fixing chamber state. | Normal operation of the fix system. |

---

## ðŸ”Œ HTTP Status Codes

When using the REST API directly:

| Code | Meaning | Common Cause |
|------|---------|--------------|
| `200` | OK | Success. |
| `201` | Created | Resource created successfully. |
| `202` | Accepted | Request queued (often for async operations). |
| `203` | Non-Authoritative | Error logic handled inside JSON body (check `Status` field). |
| `400` | Bad Request | Invalid JSON body or missing parameters. |
| `401` | Unauthorized | Missing or invalid `Auth-Key` header. |
| `404` | Not Found | Endpoint or ID does not exist. |
| `429` | Too Many Requests | Rate limit exceeded. |
| `500` | Server Error | Internal crash or unhandled exception. |

## Tags
`operators`, `errors`, `status-codes`, `reference`, `troubleshooting`, `doc-usage`
