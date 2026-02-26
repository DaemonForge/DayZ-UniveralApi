/**
 * UFModSettingsPayload
 * --------------------
 * Data transfer object for the ModSettings Register endpoint.
 *
 * Fields:
 *   - modName: Human-readable mod name
 *   - author: Author name
 *   - template: Full HTML template string (single page, inline CSS/JS)
 *   - globals: Array of Global names (informational only)
 *
 * Methods:
 *   - ToJson(): Serializes to JSON using UJSONHandler.
 */
class UFModSettingsPayload extends UFObject_Base {
	string modName = "";
	string author = "";
	string template = "";
	autoptr array<string> globals = new array<string>;
	
	override string ToJson(){
		return UJSONHandler<UFModSettingsPayload>.ToString(this);
	}
}
