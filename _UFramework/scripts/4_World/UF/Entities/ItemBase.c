/**
 * Modded ItemBase providing dynamic texture/material skin system for items.
 * 
 * Allows items to have multiple visual variants (skins) that can be:
 * - Randomly assigned on spawn
 * - Changed at runtime programmatically
 * - Persisted across server restarts
 * - Synchronized between client and server
 * - Restricted by Steam IDs for exclusive skins
 * 
 * @usage
 * modded class MyCustomRifle extends ItemBase {
 *     override void InitSkins() {
 *         super.InitSkins();
 *         RegisterTextureAndMaterial("Default", "path/to/texture.paa", "path/to/material.rvmat");
 *         RegisterTextureAndMaterial("Camo", "path/to/camo_texture.paa", "path/to/camo_material.rvmat");
 *     }
 * }
 */
modded class ItemBase {
	protected int m_SkinPaintIdx = -1;  // Current selected skin index (-1 = no skin)
	protected int m_SkinPaintIdxRemote = -1;  // Remote client's last synced skin index
	protected autoptr array<autoptr TStringArray> m_AvaibleTextures = new array<autoptr TStringArray>;  // Texture paths per skin
	protected autoptr array<autoptr TStringArray> m_AvaibleMaterials = new array<autoptr TStringArray>;  // Material paths per skin
	protected autoptr TStringArray m_AvaibleSkinNames = {};  // Human-readable skin names
	protected autoptr array<autoptr TStringArray> m_AllowedIds = new array<autoptr TStringArray>;  // Steam IDs allowed per skin
	
	protected int m_SkinNeedRefresh = 1;  // Server flag to trigger client refresh
	protected int m_SkinNeedRefreshRemote = -1;  // Client's last synced refresh flag


	/**
	 * Saves the current skin index to entity store for persistence.
	 * 
	 * @param data UEntityStore instance for writing metadata
	 */
	override void OnUFSave(UEntityStore data){
		super.OnUFSave(data);
		
		data.Write("m_SkinPaintIdx", m_SkinPaintIdx);
	}
	
	/**
	 * Restores the skin index from entity store after persistence.
	 * 
	 * @param data UEntityStore instance for reading metadata
	 */
	override void OnUFLoad(UEntityStore data){
		super.OnUFLoad(data);
		if (!data.Read("m_SkinPaintIdx", m_SkinPaintIdx )) {
			m_SkinPaintIdx = -1;
		}
	}
	
	/**
	 * Called when entity is spawned by central economy.
	 * Randomly selects a skin if skins are available and no skin is set.
	 */
	override void EEOnCECreate() {
		super.EEOnCECreate();
		if(m_AvaibleSkinNames && m_AvaibleSkinNames.Count() > 0 && m_SkinPaintIdx == -1){
			m_SkinPaintIdx = m_AvaibleSkinNames.GetRandomIndex();
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater( this.RefreshTextures, 100, false );
		}
	}
	
	/**
	 * Called during entity initialization.
	 * Applies the selected skin textures/materials to the model.
	 */
	override void EEInit() {
		super.EEInit();
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Call( this.RefreshTextures );
	}
	
	/**
	 * Gets the currently selected skin index.
	 * 
	 * @return Current skin index, or -1 if no skin selected
	 */
	int GetCurrentSkinIdx(){
		return m_SkinPaintIdx;
	}	
	
	/**
	 * Constructor - registers network sync variables and calls InitSkins.
	 * Override InitSkins() to register custom skins in your modded item classes.
	 */
	void ItemBase() {
		RegisterNetSyncVariableInt("m_SkinNeedRefresh", -1, 1);
		RegisterNetSyncVariableInt("m_SkinPaintIdx", -1, 9999);
		InitSkins();
	}
	
	
	/**
	 * Overrides item description to show if it can be painted.
	 * 
	 * @param output Output parameter for modified description
	 * @return True if description was modified, false otherwise
	 */
	override bool DescriptionOverride(out string output) {
		string name = ConfigGetString("descriptionShort");
		bool rValue = false;
		if (super.NameOverride(output)){
			name = output;
			rValue = true;
		}
        if (g_Game.IsClient() && CanPaint() ) {
            output = name + " Can be painted.";
            return true;
        }
        return rValue;
    }
	
	/**
	 * Overrides item name to include the current skin name in parentheses.
	 * 
	 * @param output Output parameter for modified name
	 * @return True if name was modified, false otherwise
	 * 
	 * @example "Mosin 9130 (Camo)"
	 */
	override bool NameOverride(out string output) {
        if (g_Game.IsClient() && GetCurrentSkinIdx() != -1 && m_AvaibleSkinNames && m_AvaibleSkinNames.Count() > GetCurrentSkinIdx()) {
            output = ConfigGetString("displayName") + " (" + GetTextureName(GetCurrentSkinIdx()) + ")";
            return true;
        }
        return super.NameOverride(output);
    }
	
	
	/**
	 * Debug helper - prints all registered skin data to log.
	 */
	void SkinMgr_debug() {
		m_AvaibleTextures.Debug();
		m_AvaibleMaterials.Debug();
		m_AvaibleSkinNames.Debug();
		m_AllowedIds.Debug();
	}
	
	/**
	 * Registers a single texture and material for a skin.
	 * 
	 * @param name Human-readable skin name (shown in item name)
	 * @param Tpath Path to texture .paa file (or empty for default)
	 * @param Mpath Path to material .rvmat file (or empty for default)
	 * @param ids Optional array of Steam IDs allowed to use this skin (NULL = everyone)
	 * 
	 * @usage
	 * RegisterTextureAndMaterial("Default", "", "");  // Empty strings = vanilla textures
	 * RegisterTextureAndMaterial("Camo", "ModFolder\\Textures\\camo.paa", "ModFolder\\Materials\\camo.rvmat");
	 * RegisterTextureAndMaterial("VIP", "vip.paa", "vip.rvmat", {"76561198012345678"});  // Restricted to specific Steam ID
	 */
	void RegisterTextureAndMaterial(string name, string Tpath = "", string Mpath = "", TStringArray ids = NULL) {
		int idx = m_AvaibleTextures.Insert({Tpath});
		m_AvaibleMaterials.Insert({Mpath});
		m_AvaibleSkinNames.Insert(name);
		m_AllowedIds.Insert(new TStringArray);
		if (ids){
			m_AllowedIds.Get(idx).Copy(ids);
		}
	}
	
	/**
	 * Gets the skin index for a given name.
	 * 
	 * @param name Skin name to find
	 * @return Skin index, or -1 if not found
	 */
	int GetIndexByName(string name){
		return m_AvaibleSkinNames.Find(name);
	}
	
	/**
	 * Registers multiple textures and materials for a skin (multi-texture models).
	 * Use this for items with multiple texture slots (e.g., separate textures for barrel, stock, etc.).
	 * 
	 * @param name Human-readable skin name
	 * @param Tpath Array of texture paths (one per texture slot on the model)
	 * @param Mpath Array of material paths (NULL = no materials)
	 * @param ids Optional array of Steam IDs allowed to use this skin
	 * 
	 * @usage
	 * TStringArray textures = {"barrel.paa", "stock.paa", "grip.paa"};
	 * TStringArray materials = {"barrel.rvmat", "stock.rvmat", "grip.rvmat"};
	 * RegisterTextureAndMaterialArray("Woodland", textures, materials);
	 */
	void RegisterTextureAndMaterialArray(string name, TStringArray Tpath, TStringArray Mpath = NULL, TStringArray ids = NULL) {
		int idx = m_AvaibleTextures.Insert(new TStringArray());
		m_AvaibleTextures.Get(idx).Copy(Tpath);
		m_AvaibleMaterials.Insert(new TStringArray());
		
		if (Mpath){
			m_AvaibleMaterials.Get(idx).Copy(Mpath);	
		}
		m_AvaibleSkinNames.Insert(name);
		m_AllowedIds.Insert(new TStringArray);
		if(ids){
			m_AllowedIds.Get(idx).Copy(ids);
		}
	}
	
	/**
	 * Checks if this item can be painted (has more than one skin option).
	 * 
	 * @return True if item has 2 or more skins registered, false otherwise
	 */
	bool CanPaint() {
		return (m_AvaibleTextures && m_AvaibleTextures.Count() > 1);
	}
	
	/**
	 * Gets the total number of registered skins.
	 * 
	 * @return Number of available skins
	 */
	int GetTextureCount() {
		return m_AvaibleTextures.Count();
	}
	
	/**
	 * Gets the texture paths for a specific skin.
	 * 
	 * @param textureID Skin index
	 * @return Array of texture paths (empty array if multi-texture, single element if single texture)
	 */
	TStringArray GetTexture(int textureID) {
		return m_AvaibleTextures.Get(textureID);
	}
	
	/**
	 * Gets the material paths for a specific skin.
	 * 
	 * @param textureID Skin index
	 * @return Array of material paths
	 */
	TStringArray GetMaterial(int textureID) {
		return m_AvaibleMaterials.Get(textureID);
	}
	
	/**
	 * Gets the Steam IDs allowed to use a specific skin.
	 * 
	 * @param textureID Skin index
	 * @return Array of Steam IDs (empty = everyone allowed)
	 */
	TStringArray GetAllowedIds(int textureID) {
		return m_AllowedIds.Get(textureID);
	}
	
	/**
	 * Gets all registered skin names.
	 * 
	 * @return Array of skin names
	 */
	TStringArray GetNames(){
			return m_AvaibleSkinNames;
	}	
	
	/**
	 * Gets the human-readable name for a skin.
	 * 
	 * @param textureID Skin index
	 * @return Skin name (e.g., "Camo", "Default")
	 */
	string GetTextureName(int textureID) {
		return m_AvaibleSkinNames.Get(textureID);
	}
	
	/**
	 * Override this method to register skins for your custom item.
	 * Called automatically from constructor.
	 * 
	 * @usage
	 * override void InitSkins() {
	 *     super.InitSkins();
	 *     RegisterTextureAndMaterial("Default", "", "");
	 *     RegisterTextureAndMaterial("Black", "black.paa", "black.rvmat");
	 * }
	 */
	void InitSkins(){}
	
	/**
	 * Gets the next skin index based on current tendency (for UI cycling).
	 * 
	 * @param Tendancy Current skin selection tendency
	 * @return Next valid skin index (skips currently selected)
	 */
	int GetNextTendancy(int Tendancy){
		Tendancy++;
		if (Tendancy == GetCurrentSkinIdx()) {
			Tendancy++;
		}
		if (Tendancy >= GetTextureCount()) {
			if (GetCurrentSkinIdx() == 0){ return 1; }
			return 0;
		}
		return Tendancy;
	}
	
	/**
	 * Sets the active skin by index and applies it to the item model.
	 * Call this to programmatically change an item's skin.
	 * 
	 * @param index Skin index to apply (must be valid index from 0 to GetTextureCount()-1)
	 * 
	 * @usage
	 * ItemBase item = ItemBase.Cast(player.GetItemInHands());
	 * int camoIndex = item.GetIndexByName("Camo");
	 * if (camoIndex != -1) {
	 *     item.SetTexture(camoIndex);
	 * }
	 */
	void SetTexture(int index) {
		if (index < 0 || index >= GetTextureCount()){return;}// Cancel if the texture is not valid
		m_SkinPaintIdx = index;
		int i = 0;
		if (g_Game.IsServer()){
			SetSynchDirty();
		}
		RefreshTextures();
	}

	/**
	 * Applies the currently selected skin's textures and materials to the item model.
	 * Called automatically by SetTexture, but can be called manually if needed.
	 * Uses SetObjectTexture and SetObjectMaterial to update the model.
	 */
	void RefreshTextures() {
		if (GetCurrentSkinIdx() < 0 || GetCurrentSkinIdx() >= GetTextureCount()){return;}// Cancel if the texture is not valid
		if (GetTexture(GetCurrentSkinIdx()).Count() > 0) {
			for (int i = 0; i < GetTexture(GetCurrentSkinIdx()).Count(); i++){
				SetObjectTexture(i, GetTexture(GetCurrentSkinIdx()).Get(i));
			}
		}
		if (GetMaterial(GetCurrentSkinIdx()).Count() > 0) {
			for (int j = 0; j < GetMaterial(GetCurrentSkinIdx()).Count(); j++){
				SetObjectMaterial(j, GetMaterial(GetCurrentSkinIdx()).Get(j));
			}
		}
	}
	
	/**
	 * Vanilla persistence save - stores skin index.
	 * 
	 * @param ctx Context for writing persistence data
	 * @param version Version number
	 * @return True on success, false on failure
	 */
	override bool OnStoreLoad(ParamsReadContext ctx, int version) {
		if ( !super.OnStoreLoad( ctx, version ) ) {
			return false;
		}
		if (!ctx.Read(m_SkinPaintIdx ))  {
			return false;
		}
		int future;
		if (!ctx.Read( future ))  {
			return false;
		}
		return true;
	}
	
	/**
	 * Vanilla persistence load - restores skin index.
	 * 
	 * @param ctx Context for reading persistence data
	 */
	override void OnStoreSave(ParamsWriteContext ctx) {
		super.OnStoreSave(ctx);
		ctx.Write(m_SkinPaintIdx );
		ctx.Write( -1 );
	}
	
	/**
	 * Called after vanilla persistence load completes.
	 * Reapplies the loaded skin and triggers client refresh.
	 */
	override void AfterStoreLoad()
	{    
		super.AfterStoreLoad();
		SetTexture( GetCurrentSkinIdx() );
		if(g_Game.IsClient()){
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater( this.RefreshTextures, 100, false );
		}
		if (g_Game.IsServer()){
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater( this.MarkSkinRefreshNeeded, 100, false );
		}
	}
	
	/**
	 * Called when network sync variables change on client.
	 * Detects when skin index changes and refreshes textures accordingly.
	 */
	override void OnVariablesSynchronized() 
    {
        super.OnVariablesSynchronized();
        if (m_SkinPaintIdx != m_SkinPaintIdxRemote ) {
            m_SkinPaintIdxRemote = m_SkinPaintIdx;
            RefreshTextures();
        } 
		if (m_SkinNeedRefresh != m_SkinNeedRefreshRemote ) {
			m_SkinNeedRefreshRemote = m_SkinNeedRefresh;
			RefreshTextures();
		}
    }
	
	/**
	 * Server-side method to force client texture refresh by toggling sync flag.
	 * Useful after attachment events to ensure clients display correct textures.
	 */
	void MarkSkinRefreshNeeded() {
		if (!g_Game.IsServer()){
			return;
		}
		if (m_SkinNeedRefresh == 1){
			m_SkinNeedRefresh = 0;
		} else {
			m_SkinNeedRefresh = 1;
		}
		SetSynchDirty();
	}

	/**
	 * Called when item is attached to a parent entity.
	 * Refreshes textures to ensure skin is applied correctly after attachment.
	 * 
	 * @param parent Parent entity the item was attached to
	 * @param slot_id Attachment slot ID
	 */
	override void OnWasAttached( EntityAI parent, int slot_id ) {
		super.OnWasAttached(parent, slot_id);
		if(m_AvaibleSkinNames && m_AvaibleSkinNames.Count() > 0 && m_SkinPaintIdx == -1){
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Call( this.RefreshTextures );
			if (g_Game.IsServer()){
				g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater( this.MarkSkinRefreshNeeded, 100, false );
			}
		}
	}
	
	/**
	 * Called when item is detached from a parent entity.
	 * Refreshes textures to ensure skin persists correctly after detachment.
	 * 
	 * @param parent Parent entity the item was detached from
	 * @param slot_id Attachment slot ID
	 */
	override void OnWasDetached( EntityAI parent, int slot_id ) {
		super.OnWasDetached(parent, slot_id);
		if(m_AvaibleSkinNames && m_AvaibleSkinNames.Count() > 0 && m_SkinPaintIdx == -1){
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Call( this.RefreshTextures );
			if (g_Game.IsServer()){
				g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater( this.MarkSkinRefreshNeeded );
			}
		}
	}
}