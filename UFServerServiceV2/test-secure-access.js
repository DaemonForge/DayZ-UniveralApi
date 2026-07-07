/**
 * Self-check for Secure Objects / AI chat access control helpers.
 * No DB needed. Run with: node test-secure-access.js
 */
const assert = require('assert');
const { createHash } = require('crypto');
const { NormalizeToGUID, normalizeGuidList, validAccessRule, normalizeRuleOp, ruleMatches, accessRulesPass, accessGrants } = require('./utils');

// ---- NormalizeToGUID: SteamID64 -> DayZ GUID (sha256 base64url), GUIDs pass through ----
const steamId = '76561198012345678';
const expectedGuid = createHash('sha256').update(steamId).digest('base64').replace(/\+/g, '-').replace(/\//g, '_');
assert.strictEqual(NormalizeToGUID(steamId), expectedGuid, 'SteamID64 should hash to DayZ GUID');
assert.strictEqual(NormalizeToGUID(expectedGuid), expectedGuid, 'GUIDs should pass through unchanged');

const GUID_A = NormalizeToGUID('76561198000000001');
const GUID_B = NormalizeToGUID('76561198000000002');

// ---- normalizeGuidList: filters junk, normalizes SteamIDs, tolerates non-arrays ----
assert.deepStrictEqual(normalizeGuidList([steamId, expectedGuid]), [expectedGuid, expectedGuid], 'mixed SteamID + GUID both normalize');
assert.deepStrictEqual(normalizeGuidList([steamId, '', null, 42, undefined]), [expectedGuid], 'non-string/empty entries dropped');
assert.deepStrictEqual(normalizeGuidList(undefined), [], 'undefined -> empty list');
assert.deepStrictEqual(normalizeGuidList('not-an-array'), [], 'non-array -> empty list');
assert.deepStrictEqual(normalizeGuidList([]), [], 'empty stays empty');

// ---- validAccessRule ----
assert.ok(validAccessRule({ Mod: 'MyMod', Field: 'Level', Op: '>=', Value: '10' }));
assert.ok(validAccessRule({ Mod: 'MyMod', Field: 'Tags', Op: 'contains', Value: 'vip' }));
assert.ok(!validAccessRule(null), 'null rule invalid');
assert.ok(!validAccessRule({ Mod: '', Field: 'Level', Op: '=', Value: '1' }), 'empty Mod invalid');
assert.ok(!validAccessRule({ Mod: 'M', Field: '', Op: '=', Value: '1' }), 'empty Field invalid');
assert.ok(!validAccessRule({ Mod: 'M', Field: 'F', Op: '$where', Value: '1' }), 'unknown Op invalid');

// ---- ruleMatches: evaluated against a Players document ----
const playerDoc = {
  GUID: GUID_A,
  MyRPGMod: {
    Level: 25,
    Faction: 'Traders',
    IsVIP: 1, // DayZ serializes booleans as 0/1
    Stats: { Reputation: 80.5 },
    Achievements: ['first_blood', 'veteran']
  }
};
const rule = (Mod, Field, Op, Value) => ({ Mod, Field, Op, Value });

// numeric comparisons (values arrive as strings from Enforce)
assert.ok(ruleMatches(rule('MyRPGMod', 'Level', '>=', '25'), playerDoc));
assert.ok(ruleMatches(rule('MyRPGMod', 'Level', '>', '24'), playerDoc));
assert.ok(!ruleMatches(rule('MyRPGMod', 'Level', '<', '25'), playerDoc));
assert.ok(ruleMatches(rule('MyRPGMod', 'Level', '<=', '25'), playerDoc));
assert.ok(ruleMatches(rule('MyRPGMod', 'Level', '=', '25'), playerDoc));
assert.ok(ruleMatches(rule('MyRPGMod', 'Level', '!=', '26'), playerDoc));
// booleans as "1"/"0"
assert.ok(ruleMatches(rule('MyRPGMod', 'IsVIP', '=', '1'), playerDoc));
assert.ok(!ruleMatches(rule('MyRPGMod', 'IsVIP', '=', '0'), playerDoc));
// string comparison
assert.ok(ruleMatches(rule('MyRPGMod', 'Faction', '=', 'Traders'), playerDoc));
assert.ok(!ruleMatches(rule('MyRPGMod', 'Faction', '=', 'Bandits'), playerDoc));
// dot path into nested data
assert.ok(ruleMatches(rule('MyRPGMod', 'Stats.Reputation', '>=', '80'), playerDoc));
assert.ok(!ruleMatches(rule('MyRPGMod', 'Stats.Reputation', '>', '90'), playerDoc));
// contains on arrays (string and numeric values)
assert.ok(ruleMatches(rule('MyRPGMod', 'Achievements', 'contains', 'veteran'), playerDoc));
assert.ok(!ruleMatches(rule('MyRPGMod', 'Achievements', 'contains', 'noob'), playerDoc));
assert.ok(!ruleMatches(rule('MyRPGMod', 'Faction', 'contains', 'Traders'), playerDoc), 'contains on non-array fails');
// fail closed: missing mod, missing field, missing player doc
assert.ok(!ruleMatches(rule('OtherMod', 'Level', '>=', '1'), playerDoc));
assert.ok(!ruleMatches(rule('MyRPGMod', 'Missing', '>=', '1'), playerDoc));
assert.ok(!ruleMatches(rule('MyRPGMod', 'Level', '>=', '1'), null));

// ---- op aliases: case-insensitive word forms normalize to canonical ops ----
assert.strictEqual(normalizeRuleOp('EQUAL'), '=');
assert.strictEqual(normalizeRuleOp('NotEqual'), '!=');
assert.strictEqual(normalizeRuleOp('IN'), 'in');
assert.strictEqual(normalizeRuleOp('NOTIN'), 'notin');
assert.strictEqual(normalizeRuleOp('nin'), 'notin');
assert.strictEqual(normalizeRuleOp('GTE'), '>=');
assert.strictEqual(normalizeRuleOp('>='), '>=');
assert.strictEqual(normalizeRuleOp('$where'), '', 'unknown op rejected');
assert.ok(ruleMatches(rule('MyRPGMod', 'Level', 'EQUAL', '25'), playerDoc), 'alias works in ruleMatches');
assert.ok(validAccessRule({ Mod: 'M', Field: 'F', Op: 'NOTIN', Value: 'a,b' }), 'alias accepted by validation');

// ---- in / notin: comma-separated value lists, numeric or string ----
assert.ok(ruleMatches(rule('MyRPGMod', 'Faction', 'in', 'Traders,Medics'), playerDoc));
assert.ok(ruleMatches(rule('MyRPGMod', 'Faction', 'in', ' Medics , Traders '), playerDoc), 'list entries are trimmed');
assert.ok(!ruleMatches(rule('MyRPGMod', 'Faction', 'in', 'Bandits,Medics'), playerDoc));
assert.ok(ruleMatches(rule('MyRPGMod', 'Level', 'in', '10,25,50'), playerDoc), 'numeric in');
assert.ok(ruleMatches(rule('MyRPGMod', 'Faction', 'notin', 'Bandits,Raiders'), playerDoc));
assert.ok(!ruleMatches(rule('MyRPGMod', 'Faction', 'notin', 'Traders,Bandits'), playerDoc));
assert.ok(!ruleMatches(rule('MyRPGMod', 'Missing', 'in', 'a,b'), playerDoc), 'in on missing field fails');
assert.ok(ruleMatches(rule('MyRPGMod', 'Missing', 'notin', 'a,b'), playerDoc), 'notin on missing field passes (negative op)');

// ---- notcontains / exists ----
assert.ok(ruleMatches(rule('MyRPGMod', 'Achievements', 'notcontains', 'noob'), playerDoc));
assert.ok(!ruleMatches(rule('MyRPGMod', 'Achievements', 'notcontains', 'veteran'), playerDoc));
assert.ok(ruleMatches(rule('MyRPGMod', 'Missing', 'notcontains', 'x'), playerDoc), 'notcontains on missing field passes');
assert.ok(ruleMatches(rule('MyRPGMod', 'Level', 'exists', ''), playerDoc));
assert.ok(ruleMatches(rule('MyRPGMod', 'Stats.Reputation', 'exists', ''), playerDoc), 'exists with dot path');
assert.ok(!ruleMatches(rule('MyRPGMod', 'Missing', 'exists', ''), playerDoc));
assert.ok(!ruleMatches(rule('OtherMod', 'Level', 'exists', ''), playerDoc), 'exists fails without mod data');

// ---- accessRulesPass: stored shape is always grouped (OR of ANDs) ----
const passA = rule('MyRPGMod', 'Level', '>=', '10');
const failA = rule('MyRPGMod', 'Level', '>=', '99');
const passB = rule('MyRPGMod', 'IsVIP', '=', '1');
assert.ok(accessRulesPass([{ Rules: [passA, passB] }], playerDoc), 'single group: all rules pass (AND)');
assert.ok(!accessRulesPass([{ Rules: [passA, failA] }], playerDoc), 'single group: one failing rule denies');
assert.ok(!accessRulesPass([], playerDoc), 'empty rules never grant');
assert.ok(accessRulesPass([{ Rules: [failA] }, { Rules: [passA, passB] }], playerDoc), 'any full-passing group grants (OR)');
assert.ok(!accessRulesPass([{ Rules: [failA] }, { Rules: [passA, failA] }], playerDoc), 'no group fully passes');
assert.ok(!accessRulesPass([{ Rules: [] }], playerDoc), 'empty group never grants');
// Non-grouped entries (hand-edited DBs, pre-canonicalization data) fail closed:
assert.ok(!accessRulesPass([passA, passB], playerDoc), 'plain flat rules are not evaluated - denied');
assert.ok(accessRulesPass([{ Rules: [passA] }, passB], playerDoc), 'stray plain entry ignored, valid group still grants');
assert.ok(!accessRulesPass([{ Rules: [failA] }, passB], playerDoc), 'stray plain entry cannot grant');

// grouped rules via the full access decision
const groupedDoc = { AllowedPlayers: [], AccessRules: [
  { Rules: [rule('MyRPGMod', 'IsVIP', '=', '1')] },
  { Rules: [rule('MyRPGMod', 'Level', '>=', '50'), rule('MyRPGMod', 'Faction', 'in', 'Traders,Medics')] }
] };
assert.ok(accessGrants(groupedDoc, GUID_A, playerDoc), 'VIP group grants even though level group fails');
assert.ok(!accessGrants(groupedDoc, GUID_A, { GUID: GUID_A, MyRPGMod: { IsVIP: 0, Level: 60, Faction: 'Bandits' } }), 'no group passes -> denied');
assert.ok(accessGrants(groupedDoc, GUID_A, { GUID: GUID_A, MyRPGMod: { IsVIP: 0, Level: 60, Faction: 'Medics' } }), 'level+faction group grants');

// ---- accessGrants: allowlist OR all rules pass; both empty = public ----
const publicDoc = { data: {} };
const listedDoc = { AllowedPlayers: [GUID_A], AccessRules: [] };
const ruledDoc = { AllowedPlayers: [], AccessRules: [{ Rules: [rule('MyRPGMod', 'Level', '>=', '10'), rule('MyRPGMod', 'IsVIP', '=', '1')] }] };
const mixedDoc = { AllowedPlayers: [GUID_B], AccessRules: [{ Rules: [rule('MyRPGMod', 'Faction', '=', 'Traders')] }] };

assert.ok(accessGrants(publicDoc, GUID_A, null), 'no allowlist + no rules = public');
assert.ok(accessGrants({ AllowedPlayers: [], AccessRules: [] }, GUID_A, null), 'empty allowlist + empty rules = public');
assert.ok(accessGrants(listedDoc, GUID_A, null), 'allowlisted player granted');
assert.ok(!accessGrants(listedDoc, GUID_B, null), 'non-listed player denied');
assert.ok(accessGrants(ruledDoc, GUID_A, playerDoc), 'all rules pass -> granted');
assert.ok(!accessGrants(ruledDoc, GUID_A, { GUID: GUID_A, MyRPGMod: { Level: 5, IsVIP: 1 } }), 'one failing rule denies (rules AND together)');
assert.ok(!accessGrants(ruledDoc, GUID_A, null), 'rules with missing player doc deny (fail closed)');
assert.ok(accessGrants(mixedDoc, GUID_B, null), 'allowlist grants without evaluating rules');
assert.ok(accessGrants(mixedDoc, GUID_A, playerDoc), 'rules grant when not allowlisted');
assert.ok(!accessGrants(mixedDoc, GUID_A, { GUID: GUID_A, MyRPGMod: { Faction: 'Bandits' } }), 'neither allowlist nor rules -> denied');

// AI chat sessions use the same helper with allowlist only (see canAccessChat in
// controllers/aiChat.js) - covered by the allowlist cases above.

console.log('secure-access self-check passed');
