/**
 * File: SecureAccessObjects.c
 * Description: Object classes for Secure Objects access control.
 *
 * A secure object carries an allowlist of players and/or access rules.
 * Access is granted when the player is on the allowlist OR the rules pass.
 * Empty allowlist and no rules = public object (default behavior).
 *
 * Rules added with AddRule() all have to pass (AND). For OR logic, use
 * AddGroup(): access is granted when ANY group has all of its rules pass.
 */

/**
 * One access rule evaluated against the player's saved data (PLAYER_DB).
 * Example: Mod "MyRPGMod", Field "Level", Op ">=", Value "10" grants access
 * to players whose MyRPGMod player data has Level >= 10.
 *
 * Ops: "=", "!=", ">", ">=", "<", "<=",
 *      "in" / "notin"             - Value is a comma separated list: "Traders,Medics"
 *      "contains" / "notcontains" - array field membership
 *      "exists"                   - field is present (Value ignored)
 * Word aliases work too: "EQUAL", "NOTEQUAL", "NOTIN", "GTE", "LTE", ...
 *
 * Values are strings: numbers as "10", booleans as "1"/"0" (DayZ JSON convention).
 * Field supports dot paths into nested data, e.g. "Stats.Reputation".
 * Missing player data fails positive ops; negative ops ("!=", "notin",
 * "notcontains") pass on missing data - add an "exists" rule to require it.
 */
class UAccessRule extends Managed {
	string Mod;
	string Field;
	string Op;
	string Value;
}

/**
 * One OR alternative: ALL rules inside a group must pass for the group to grant access.
 */
class UAccessRuleGroup extends Managed {
	autoptr array<autoptr UAccessRule> Rules = new array<autoptr UAccessRule>;

	/**
	 * Adds a rule to this group. All rules in a group must pass (AND).
	 */
	void AddRule(string mod, string field, string op, string value) {
		UAccessRule rule = new UAccessRule();
		rule.Mod = mod;
		rule.Field = field;
		rule.Op = op;
		rule.Value = value;
		Rules.Insert(rule);
	}
}

/**
 * Access definition for a secure object. Used with SaveSecure/SetAccess.
 *
 * @usage
 *   USecureAccess access = new USecureAccess();
 *   access.AllowPlayer(player.GetIdentity().GetId()); // GUID or SteamID64
 *   access.AddRule("MyRPGMod", "IsVIP", "=", "1");    // simple rules AND together
 *
 *   // OR logic: access granted if the simple rules pass OR any group passes
 *   UAccessRuleGroup vets = access.AddGroup();
 *   vets.AddRule("MyRPGMod", "Level", ">=", "50");
 *   vets.AddRule("MyRPGMod", "Faction", "in", "Traders,Medics");
 *
 *   handler.SaveSecure("Stash_042", stashData, access, this, "OnSaved");
 */
class USecureAccess extends UFObject_Base {
	autoptr array<string> AllowedPlayers = new array<string>;
	autoptr array<autoptr UAccessRuleGroup> AccessRules;

	// AccessRules is built in the constructor (not a field initializer) because we
	// use it here: AccessRules[0] is the default group that AddRule() fills, and
	// AddGroup() appends additional OR-alternatives after it. Pre-creating the
	// default group means AddRule() and AddGroup() never collide regardless of call
	// order. An unused (empty) default group is simply dropped by the service at write time.
	void USecureAccess() {
		AccessRules = new array<autoptr UAccessRuleGroup>;
		AccessRules.Insert(new UAccessRuleGroup());
	}

	/**
	 * Allows a single player. Accepts a DayZ GUID or a SteamID64 -
	 * the service normalizes SteamIDs to GUIDs.
	 */
	void AllowPlayer(string guidOrSteamId) {
		if (guidOrSteamId != "") {
			AllowedPlayers.Insert(guidOrSteamId);
		}
	}

	/**
	 * Allows multiple players (GUIDs or SteamID64s, mixed freely).
	 */
	void AllowPlayers(array<string> ids) {
		if (!ids) {
			return;
		}
		foreach (string id : ids) {
			AllowPlayer(id);
		}
	}

	/**
	 * Adds a rule to the default group - rules added this way all have
	 * to pass together (AND). Independent of AddGroup(): calling AddRule()
	 * before or after AddGroup() always targets the default group, never an
	 * OR-alternative. See UAccessRule for ops and value format.
	 * @param mod The mod namespace in the player's PLAYER_DB data
	 * @param field Field name (dot path allowed, e.g. "Stats.Reputation")
	 * @param op "=", "!=", ">", ">=", "<", "<=", "in", "notin", "contains", "notcontains", "exists"
	 * @param value Value as string (numbers "10", booleans "1"/"0", lists "a,b,c")
	 */
	void AddRule(string mod, string field, string op, string value) {
		AccessRules.Get(0).AddRule(mod, field, op, value);
	}

	/**
	 * Starts a new OR alternative. Access is granted when ANY group has all
	 * of its rules pass. Add rules to the returned group with grp.AddRule(...).
	 */
	UAccessRuleGroup AddGroup() {
		UAccessRuleGroup grp = new UAccessRuleGroup();
		AccessRules.Insert(grp);
		return grp;
	}

	override string ToJson() {
		return UJSONHandler<USecureAccess>.ToString(this);
	}
}
