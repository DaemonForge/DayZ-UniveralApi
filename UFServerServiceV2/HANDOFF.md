# UFServerServiceV2 — Remaining Work

All items from the previous handoff are now **done and committed on `v2`**:
the session's work (incl. `package-lock.json`), the dead-Functions removal,
the cert field-mapping fix (either orientation now works), operator docs for
`TrustProxyHeaders`/`MaxBodySize`, and all five major upgrades
(ejs 6, mongodb 7, openai 6, express 5, electron 43). `npm audit` is clean
and `npm test` is green.

What's left needs a real deployment or a human at the keyboard:

## 1. Staging smoke test

- Live **MongoDB**: transactions, message queue end-to-end (mongodb driver 7).
- Live **OpenAI / compat provider** (Vultr/Cloudflare/Ollama): chat,
  embeddings, TTS, and an Assistants thread round-trip — the openai v6 port
  changed `submitToolOutputsAndPoll` and the client factory; live-test both.
- **Desktop app**: run the built installer; check tray menu, settings, logs,
  KB, and mod-manager windows under electron 43.
- **`pkg` Linux build** (`npm run build:linux`) on/for a Linux host.
- Full server boot binding real ports / Let's Encrypt / tunnel under express 5.

`npm test` runs the 3 offline self-check suites (adapter translation,
message-cursor pagination, query guard) — keep it green.

## 2. Per-deployment config

- Set `TrustProxyHeaders: true` in the live config if the service sits behind
  the Cloudflare tunnel or a reverse proxy (see
  [02_ServiceConfiguration.md](../docs/ForOperators/02_ServiceConfiguration.md)).
