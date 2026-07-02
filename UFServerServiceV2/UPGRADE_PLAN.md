# Dependency Upgrade Plan

Status of the tree after the last maintenance pass:

- All safe **within-major** updates applied (`npm update`) and `npm audit fix` run.
- `node-fetch` removed — the service uses Node's built-in global `fetch` (requires Node >= 18, now enforced via `engines` in `package.json`).
- Remaining work is the **major-version** upgrades below, each of which has breaking changes and must be done deliberately, one at a time, on a branch, with `npm test` + a manual smoke test before merge.

General procedure for every item:
1. Branch: `git checkout -b upgrade/<pkg>`.
2. Read that package's CHANGELOG/migration guide for the target major.
3. Apply the code changes listed below.
4. `npm install <pkg>@<target>` (or edit `package.json` + `npm install`).
5. `npm test` + `node --check` all source + manual smoke test of the affected feature.
6. Roll back with `git checkout .` / reinstall the old version if anything fails.

Do **not** run `npm audit fix --force` — it will pull these majors in unreviewed.

---

## 0. Commit the lockfile (do this first — foundational)

**Risk: none. Value: high.** `package-lock.json` is currently gitignored, so builds aren't reproducible and these upgrades can't be reviewed as diffs.

Steps:
1. Remove the `package-lock.json` line from `UFServerServiceV2/.gitignore`.
2. `git add UFServerServiceV2/package-lock.json`.
3. Commit. From now on every dependency change is an auditable lockfile diff.

Verification: `npm ci` installs cleanly from the lockfile.

---

## 1. express-rate-limit 7 → 8  (LOW risk)

**What changes:** v8 finalizes the `max` → `limit` option rename and tightens the store interface. Our usage is the standard in-memory limiter.

**Affected files:** `app.js` (the top-level `limiter`), `utils.js` (`GenerateLimiter`). Both pass `{ windowMs, max, message, keyGenerator, handler, skip }`.

**Steps:**
1. Rename `max:` → `limit:` in both limiter definitions.
2. Confirm `keyGenerator`/`skip`/`handler` signatures are unchanged in the v8 docs (they are, as of 8.x).
3. `npm install express-rate-limit@8`.

**Verification:** hit any rate-limited route repeatedly and confirm a `429` after the limit; confirm whitelisted IPs still skip (`getClientIp` logic unchanged).

---

## 2. selfsigned 2 → 5  (LOW risk, tiny surface)

**What changes:** internal (node-forge bumps, options tweaks). Only one call site.

**Affected files:** `app.js` → `ensureSelfSignedCertificate()`.

**Steps:**
1. `npm install selfsigned@5`.
2. Confirm `selfsigned.generate(attrs, options)` still returns `{ private, cert }` and the `extensions`/`keySize`/`algorithm` option shape is unchanged (check CHANGELOG).

**Verification:** delete `<dataDir>/certs/`, start the service, confirm a fresh cert is generated and HTTPS comes up. (The existing validation one-liner: `tls.createSecureContext({ key, cert })` must not throw.)

---

## 3. ejs 3 → 6  (LOW–MODERATE risk)

**What changes:** several majors of accumulated changes; delimiter/escaping and `render()` option defaults are the areas to check. Our templates only use escaped `<%= %>` output, which is the safest case.

**Affected files:** `discord/login.js` (`render(...)` for login/error/success), `templates/defaultTemplates.json`, `templates/discordLogin.ejs`.

**Steps:**
1. `npm install ejs@6`.
2. Diff the rendered output of the login page, error page, and success page against the current output (same template + same data).
3. Watch for any change in HTML-escaping behavior of `<%= %>`.

**Verification:** load `/Discord/login/<validSteamId>` and the error paths; confirm the pages render identically and no template variable leaks unescaped.

---

## 4. mongodb 6 → 7  (MODERATE risk)

**What changes:** v7 drops older Node/BSON support and adjusts some defaults; the driver API is largely stable from v6. The specific thing to re-verify for this codebase is `findOneAndUpdate` return shape (v6 returns the document directly; confirm v7 keeps that and that `includeResultMetadata` default is unchanged).

**Affected files:** every `models/*.js` (all use the driver), especially `models/object.js` and `models/player.js` (`findOneAndUpdate` in the validated-transaction paths), and `models/messages.js` (the compound-cursor reads).

**Steps:**
1. `npm install mongodb@7`.
2. Re-verify `findOneAndUpdate` returns the doc (not `{ value }`) — the transaction code depends on this.
3. Check connection-option compatibility in the pooled `getConnection()` helpers (`maxPoolSize`, `serverSelectionTimeoutMS`, etc. are stable, but confirm).

**Verification:** run against a real Mongo: create a player/object, run a validated increment at the min/max boundary, read a message queue with duplicate timestamps (the compound-cursor test scenario) and confirm no message loss. Add a throwaway integration script if needed.

---

## 5. openai 4 → 6  (HIGH effort)

**What changes:** two majors of SDK changes (client construction, method signatures, response shapes, streaming). This codebase leans on the SDK heavily.

**Affected files:** `aiClient.js` (`responses.create`, `chat.completions.create`, client construction with `baseURL`), `controllers/kb.js` (`embeddings.create`, `responses.create`), `controllers/aiAssistant.js` (`beta.assistants.*`, `beta.threads.*`), `controllers/tts.js` (`audio.speech.create`).

**Steps:**
1. Read the openai-node v5 **and** v6 migration guides.
2. Update client construction in `aiClient.js` first; confirm `baseURL`/compat-mode still works.
3. Port each surface: Responses API, Chat Completions, embeddings, Assistants (beta namespace may have moved), audio speech.
4. Re-run the `test-ai-client.js` adapter self-check (it mocks the client, so it validates our translation logic independent of the SDK).

**Verification:** with a real key — a chat round-trip (both OpenAI and a compat `BaseURL`), a KB embedding + search, a TTS clip, and an Assistant thread. This one needs live API testing; budget real time.

---

## 6. express 4 → 5  (HIGH risk)

**What changes:** Express 5 uses path-to-regexp v8 (stricter route patterns), removes `req.param()`, `res.redirect('back')`, `app.del()`, changes wildcard `*` syntax, and changes the default query parser. Middleware error propagation for rejected promises improves.

**Known landmines in THIS codebase (must fix before/with the bump):**
- **Empty-path routes:** `controllers/status.js` (`router.post('')`) and `controllers/TranslateConnector.js` (`router.post('')`) — path-to-regexp v8 rejects `''`. Change both to `router.post('/')`.
- Audit every route string for `*` wildcards and optional `:param?` patterns and convert to the v5 syntax.
- Verify the catch-all `app.use('/', ...)` 404 handler and the `/Object/Query`, `/Player/Query` sub-router mounts still match.

**Affected files:** `app.js` (mounts, catch-all), all `controllers/*.js` route definitions, `discord/router.js`.

**Steps:**
1. `npm install express@5`.
2. Fix the empty-path routes and any wildcard patterns.
3. Grep for `req.param(` / `res.redirect('back')` / `app.del(` (none expected, but confirm).
4. Start the server and watch for path-to-regexp throw-on-load errors.

**Verification:** exercise a route from every controller (object, player, query, messages, AI, TTS, images, discord, status) and confirm 2xx/expected responses. Highest regression surface of all the upgrades — test broadly.

---

## 7. electron 35 → 43  (HIGH risk — desktop build)

**What changes:** 8 major versions of Chromium + Node ABI. Resolves the one remaining `npm audit` high finding. Native modules must be rebuilt for the new ABI; several Electron APIs may be deprecated/removed.

**Affected files:** `main.js` (tray, BrowserWindow, ipcMain, dialog, nativeImage, app lifecycle), `preload/*`, `package.json` build config, `installer.nsi`. `electron-builder` is already updated and supports newer Electron.

**Steps:**
1. Bump in steps if possible (e.g. 35 → 39 → 43) rather than one leap, to isolate breakage.
2. `npm install electron@<target> --save-dev`.
3. Rebuild native deps for the new ABI (`electron-rebuild` if any native modules are used in-process).
4. Review the Electron breaking-changes docs for each major crossed (BrowserWindow options, `nativeImage`, IPC, `app` events).

**Verification:** build the Windows installer (`npm run build:unsigned`), install it, and smoke-test: tray menu, settings window, logs window, KB/mod-settings windows, and that the HTTPS service still starts inside the packaged app. Cannot be validated without an actual build + desktop run.

---

## Suggested order

0 (lockfile) → 1 (rate-limit) → 2 (selfsigned) → 3 (ejs) → 4 (mongodb) → 5 (openai) → 6 (express) → 7 (electron).

Do the cheap, low-risk ones first to build confidence; save express and electron (the two broadest-blast-radius upgrades) for last, each on its own branch with a full smoke test.
