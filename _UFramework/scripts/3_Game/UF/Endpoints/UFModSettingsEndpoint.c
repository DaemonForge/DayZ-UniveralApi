/**
 * UFModSettingsEndpoint - Registers custom HTML settings pages with the UF Service.
 *
 * This endpoint allows mods to upload a single-page HTML template that server operators
 * can use to visually configure the mod's Globals (settings stored in MongoDB).
 *
 * The template is stored in the service's ModSettings collection and displayed in the
 * Electron UI under Options > Mod Settings.
 *
 * @usage
 *   // Minimal — just modId + template
 *   UF().Settings().Register("my-mod", htmlTemplate);
 *
 *   // With a display name
 *   UF().Settings().Register("my-mod", "My Mod", htmlTemplate);
 *
 *   // With callback
 *   UF().Settings().Register("my-mod", "My Mod", htmlTemplate, this, "OnRegistered");
 *
 * @note Templates should be self-contained single-page HTML files with inline CSS & JS.
 *       The service injects a bridge script providing window.UF API for loading/saving globals.
 *       Templates can load/save ANY global — no need to declare them upfront.
 *       See the SDK documentation for the full template API reference.
 *
 * @note Register is idempotent (upsert). Calling Register() with the same modId
 *       overwrites the previous template and metadata. Mods should call Register()
 *       on every server start to ensure the latest template version is deployed.
 */
class UFModSettingsEndpoint extends UFBaseEndpoint {
	
	/**
	 * Returns the base URL for the Mod Settings endpoint.
	 */
	override protected string EndpointBaseUrl(){
		UFrameworkConfig ucfg = UFrameworkConfig.Cast(UFConfig());
		if (!ucfg){
			UFLog.Err("[UFModSettingsEndpoint] EndpointBaseUrl called but UFConfig() is null - RPC not received yet?");
			return "";
		}
		return ucfg.GetBaseURL() + "ModSettings/";
	}
	
	/**
	 * Build a UFModSettingsPayload from the given parameters.
	 */
	protected UFModSettingsPayload CreatePayload(string modName, string author, string tmpl, TStringArray globals) {
		autoptr UFModSettingsPayload payload = new UFModSettingsPayload();
		payload.modName = modName;
		payload.author = author;
		payload.template = tmpl;
		if (globals) {
			for (int i = 0; i < globals.Count(); i++) {
				payload.globals.Insert(globals[i]);
			}
		}
		return payload;
	}
	
	// ── Minimal overloads (modId + template only) ──
	
	/**
	 * Register a mod settings page (fire-and-forget, minimal).
	 * Only modId and template are required. The service defaults modName to modId.
	 *
	 * @param modId  Unique identifier (alphanumeric, hyphens, underscores, 1-64 chars)
	 * @param tmpl   Full HTML template string
	 * @return Call ID or -1 on error
	 */
	int Register(string modId, string tmpl) {
		return Register(modId, "", "", tmpl, NULL);
	}
	
	/**
	 * Register with a display name (fire-and-forget).
	 *
	 * @param modId    Unique identifier
	 * @param modName  Human-readable mod name shown in the sidebar
	 * @param tmpl     Full HTML template string
	 * @return Call ID or -1 on error
	 */
	int Register(string modId, string modName, string tmpl) {
		return Register(modId, modName, "", tmpl, NULL);
	}
	
	// ── Callback overloads (modId + modName + template + callback) ──
	
	/**
	 * Register with instance+function callback.
	 *
	 * @param modId       Unique identifier
	 * @param modName     Human-readable mod name (pass "" to default to modId)
	 * @param tmpl        Full HTML template string
	 * @param cbInstance  Callback instance
	 * @param cbFunction  Callback function name
	 * @return Call ID or -1 on error
	 *
	 * @note Callback signature: void OnRegistered(int cid, int status, string oid, string data)
	 */
	int Register(string modId, string modName, string tmpl, Class cbInstance, string cbFunction) {
		return Register(modId, modName, "", tmpl, NULL, cbInstance, cbFunction);
	}
	
	/**
	 * Register with UFCallbackBase callback.
	 *
	 * @param modId    Unique identifier
	 * @param modName  Human-readable mod name (pass "" to default to modId)
	 * @param tmpl     Full HTML template string
	 * @param cb       UFCallbackBase-derived callback
	 * @return Call ID or -1 on error
	 */
	int Register(string modId, string modName, string tmpl, UFCallbackBase cb) {
		return Register(modId, modName, "", tmpl, NULL, cb);
	}
	
	// ── Full overloads (with author + globals) ──
	
	/**
	 * Register a mod settings page with full metadata (fire-and-forget).
	 *
	 * @param modId     Unique identifier (alphanumeric, hyphens, underscores, 1-64 chars)
	 * @param modName   Human-readable mod name (pass "" to default to modId)
	 * @param author    Author name (optional, pass "" to omit)
	 * @param tmpl      Full HTML template string
	 * @param globals   Array of Global names (informational only, pass NULL to skip)
	 * @return Call ID or -1 on error
	 */
	int Register(string modId, string modName, string author, string tmpl, TStringArray globals) {
		int cid = -1;
		
		if (!modId || modId == "" || !tmpl || tmpl == ""){
			UFLog.Err("[UFModSettingsEndpoint] Register: modId and template are required");
			return -1;
		}
		
		string displayName = modName;
		if (displayName == "") displayName = modId;
		UFLog.Info("[UF] Registering mod settings page '" + displayName + "' (modId: " + modId + ", template: " + tmpl.Length().ToString() + " chars)");
		
		autoptr UFModSettingsPayload payload = CreatePayload(modName, author, tmpl, globals);
		Post("Register/" + modId, payload.ToJson(), UF().RegisterCall(new USilentCallBack(), cid));
		return cid;
	}
	
	/**
	 * Register with full metadata and instance+function callback.
	 *
	 * @param modId       Unique identifier
	 * @param modName     Human-readable mod name (pass "" to default to modId)
	 * @param author      Author name (optional)
	 * @param tmpl        Full HTML template string
	 * @param globals     Array of Global names (informational only, pass NULL to skip)
	 * @param cbInstance  Callback instance
	 * @param cbFunction  Callback function name
	 * @return Call ID or -1 on error
	 *
	 * @note Callback signature: void OnRegistered(int cid, int status, string oid, string data)
	 */
	int Register(string modId, string modName, string author, string tmpl, TStringArray globals, Class cbInstance, string cbFunction) {
		int cid = UF().CallId();
		
		if (!modId || modId == "" || !tmpl || tmpl == ""){
			UFLog.Err("[UFModSettingsEndpoint] Register: modId and template are required");
			return -1;
		}
		
		string displayName = modName;
		if (displayName == "") displayName = modId;
		UFLog.Info("[UF] Registering mod settings page '" + displayName + "' (modId: " + modId + ", template: " + tmpl.Length().ToString() + " chars)");
		
		autoptr UFModSettingsPayload payload = CreatePayload(modName, author, tmpl, globals);
		Post("Register/" + modId, payload.ToJson(), new UDBCallBack(cbInstance, cbFunction, cid, modId));
		return cid;
	}
	
	/**
	 * Register with full metadata and UFCallbackBase callback.
	 *
	 * @param modId     Unique identifier
	 * @param modName   Human-readable mod name (pass "" to default to modId)
	 * @param author    Author name (optional)
	 * @param tmpl      Full HTML template string
	 * @param globals   Array of Global names (informational only, pass NULL to skip)
	 * @param cb        UFCallbackBase-derived callback
	 * @return Call ID or -1 on error
	 */
	int Register(string modId, string modName, string author, string tmpl, TStringArray globals, UFCallbackBase cb) {
		int cid = -1;
		
		if (!modId || modId == "" || !tmpl || tmpl == "" || !cb){
			UFLog.Err("[UFModSettingsEndpoint] Register: modId, template, and callback are required");
			return -1;
		}
		
		string displayName = modName;
		if (displayName == "") displayName = modId;
		UFLog.Info("[UF] Registering mod settings page '" + displayName + "' (modId: " + modId + ", template: " + tmpl.Length().ToString() + " chars)");
		
		autoptr UFModSettingsPayload payload = CreatePayload(modName, author, tmpl, globals);
		cb.SetOID(modId);
		Post("Register/" + modId, payload.ToJson(), UF().RegisterCall(new UNestedCallBack(cb), cid));
		return cid;
	}
}
