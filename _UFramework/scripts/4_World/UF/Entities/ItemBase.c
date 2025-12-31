modded class ItemBase {
	protected int m_SkinPaintIdx = -1;
	protected int m_SkinPaintIdxRemote = -1;
	protected autoptr array<autoptr TStringArray> m_AvaibleTextures = new array<autoptr TStringArray>;
	protected autoptr array<autoptr TStringArray> m_AvaibleMaterials = new array<autoptr TStringArray>;
	protected autoptr TStringArray m_AvaibleSkinNames = {};
	protected autoptr array<autoptr TStringArray> m_AllowedIds = new array<autoptr TStringArray>;
	
	protected int m_SkinNeedRefresh = 1;
	protected int m_SkinNeedRefreshRemote = -1;


	override void OnUFSave(UEntityStore data){
		super.OnUFSave(data);
		
		data.Write("m_SkinPaintIdx", m_SkinPaintIdx);
	}
	override void OnUFLoad(UEntityStore data){
		super.OnUFLoad(data);
		if (!data.Read("m_SkinPaintIdx", m_SkinPaintIdx )) {
			m_SkinPaintIdx = -1;
		}
	}
	
	override void EEOnCECreate() {
		super.EEOnCECreate();
		if(m_AvaibleSkinNames && m_AvaibleSkinNames.Count() > 0 && m_SkinPaintIdx == -1){
			m_SkinPaintIdx = m_AvaibleSkinNames.GetRandomIndex();
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater( this.RefreshTextures, 100, false );
		}
	}
	
	override void EEInit() {
		super.EEInit();
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Call( this.RefreshTextures );
	}
	
	int GetCurrentSkinIdx(){
		return m_SkinPaintIdx;
	}	
	
	
	void ItemBase() {
		RegisterNetSyncVariableInt("m_SkinNeedRefresh", -1, 1);
		RegisterNetSyncVariableInt("m_SkinPaintIdx", -1, 9999);
		InitSkins();
	}
	
	
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
	
	override bool NameOverride(out string output) {
        if (g_Game.IsClient() && GetCurrentSkinIdx() != -1 && m_AvaibleSkinNames && m_AvaibleSkinNames.Count() > GetCurrentSkinIdx()) {
            output = ConfigGetString("displayName") + " (" + GetTextureName(GetCurrentSkinIdx()) + ")";
            return true;
        }
        return super.NameOverride(output);
    }
	
	
	void SkinMgr_debug() {
		m_AvaibleTextures.Debug();
		m_AvaibleMaterials.Debug();
		m_AvaibleSkinNames.Debug();
		m_AllowedIds.Debug();
	}
	
	void RegisterTextureAndMaterial(string name, string Tpath = "", string Mpath = "", TStringArray ids = NULL) {
		int idx = m_AvaibleTextures.Insert({Tpath});
		m_AvaibleMaterials.Insert({Mpath});
		m_AvaibleSkinNames.Insert(name);
		m_AllowedIds.Insert(new TStringArray);
		if (ids){
			m_AllowedIds.Get(idx).Copy(ids);
		}
	}
	
	int GetIndexByName(string name){
		return m_AvaibleSkinNames.Find(name);
	}
	
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
	
	bool CanPaint() {
		return (m_AvaibleTextures && m_AvaibleTextures.Count() > 1);
	}
	
	int GetTextureCount() {
		return m_AvaibleTextures.Count();
	}
	
	TStringArray GetTexture(int textureID) {
		return m_AvaibleTextures.Get(textureID);
	}
	
	TStringArray GetMaterial(int textureID) {
		return m_AvaibleMaterials.Get(textureID);
	}
	
	TStringArray GetAllowedIds(int textureID) {
		return m_AllowedIds.Get(textureID);
	}
	
	TStringArray GetNames(){
			return m_AvaibleSkinNames;
	}	
	
	string GetTextureName(int textureID) {
		return m_AvaibleSkinNames.Get(textureID);
	}
	
	void InitSkins(){}
	
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
	
	void SetTexture(int index) {
		if (index < 0 || index >= GetTextureCount()){return;}// Cancel if the texture is not valid
		m_SkinPaintIdx = index;
		int i = 0;
		if (g_Game.IsServer()){
			SetSynchDirty();
		}
		RefreshTextures();
	}

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
	
	override void OnStoreSave(ParamsWriteContext ctx) {
		super.OnStoreSave(ctx);
		ctx.Write(m_SkinPaintIdx );
		ctx.Write( -1 );
	}
	
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

	override void OnWasAttached( EntityAI parent, int slot_id ) {
		super.OnWasAttached(parent, slot_id);
		if(m_AvaibleSkinNames && m_AvaibleSkinNames.Count() > 0 && m_SkinPaintIdx == -1){
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Call( this.RefreshTextures );
			if (g_Game.IsServer()){
				g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater( this.MarkSkinRefreshNeeded, 100, false );
			}
		}
	}
	
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