# UFServerServiceV2 — Remaining Work

Scope of what's left to complete on branch `v2`. All current work is
**uncommitted** in the working tree. Dependency-upgrade details live in
[UPGRADE_PLAN.md](UPGRADE_PLAN.md).

---

## 1. Commit the session's work

1. Review the diff on `v2` (24 modified files + 7 new) and commit.
2. Commit `package-lock.json` — it's now un-ignored; committing it enables
   reproducible builds via `npm ci`.

---

## 2. Decisions needed

1. **Dead `Functions` feature** — [controllers/functions.js](controllers/functions.js)
   and [models/functions.js](models/functions.js) are not mounted and not
   called by the mod. Decide: remove both files (plus the Functions section in
   the settings UI), or leave dead. Recommendation: remove.
2. **Operator-cert field mapping** — [app.js](app.js) `loadCertificates` reads
   `Certificate` → key and `CertificateKey` → cert, which looks swapped by
   name. Confirm intended semantics; fix if wrong.
3. **`TrustProxyHeaders`** — if the service runs behind the Cloudflare tunnel
   or a reverse proxy, set it `true` in config so per-client rate limiting
   works correctly.

---

## 3. Remaining dependency upgrades

Ordered low→high risk. Each on its own branch; `npm test` + smoke test before
merge. Do **not** run `npm audit fix --force`.

| Item | Risk | Notes |
|------|------|-------|
| ejs 3→6 | low-mod | Templates use escaped `<%=` only |
| mongodb 6→7 | moderate | Re-verify `findOneAndUpdate` return shape |
| openai 4→6 | high effort | Port `aiClient`/`kb`/`aiAssistant`/`tts`; live-test |
| express 4→5 | high | Empty-path routes already pre-fixed; audit wildcard patterns |
| electron 35→43 | high | 8 majors; requires desktop build + smoke test |

`selfsigned` stays at **v2** intentionally (v5 is async-only and would force an
async refactor of the boot path). `express-rate-limit` is already on v8.

---

## 4. Staging smoke test

Not verifiable in this environment — needs a real deployment:

- Live **MongoDB**: transactions, message queue end-to-end.
- Live **OpenAI / compat provider** (Vultr/Cloudflare/Ollama): chat,
  embeddings, TTS, assistants via the new `OpenAIApi.BaseURL` config.
- **Electron desktop build** and packaged (`pkg`) Linux build.
- Full server boot binding real ports / Let's Encrypt / tunnel.

`npm test` runs the 4 offline self-check suites (adapter translation,
sandbox input guard, message-cursor pagination, query guard) — keep it green
through the upgrades above.

---

## 5. Suggested order

1. Commit everything on `v2` (incl. lockfile).
2. Settle the three decisions in §2.
3. Staging smoke test (§4).
4. Electron upgrade as its own tested task.
5. Remaining majors one at a time (ejs → mongodb → openai → express).
