class UApiEntityStore extends UApiObject_Base {
	
	string m_Type = "";
	float m_Health = -1;
	float m_Quantity;
	float m_Wet;
	float m_Tempature;
	float m_Energy;
	int m_LiquidType;
	int m_Slot;
	int m_Idx;
	int m_Row;
	int m_Col;
	bool m_Flip;
	bool m_IsInHands;
	bool m_IsOn;
	int m_QuickBarSlot;
	
	protected autoptr array<autoptr UApiEntityStore> m_Cargo;
	
	bool m_IsMagazine;
	autoptr array<autoptr UApiAmmoData> m_MagAmmo;
	bool m_IsWeapon;
	autoptr array<int> m_FireModes;
	autoptr UApiAmmoData m_ChamberedRound;
	
	protected autoptr array<autoptr UApiMetaData> m_MetaData;
	
	void UApiEntityStore(EntityAI item = NULL){
		if (!item) return;
		SaveEntity(item, true);
	}
	
	void ~UApiEntityStore(){
		delete m_Cargo;
		delete m_MagAmmo;
		delete m_ChamberedRound;
		delete m_FireModes;
		delete m_MetaData;
	}
	
	void SaveEntity(notnull EntityAI item, bool recursive = true ){
		m_Type = item.GetType();
		m_Health = item.GetHealth("", "");
		array<EntityAI> items = new array<EntityAI>;
		int i = 0;
		InventoryLocation il = new InventoryLocation;
		if (item.GetInventory().GetCurrentInventoryLocation(il)){
			m_Slot = il.GetSlot();
			m_Idx = il.GetIdx();
			m_Row = il.GetRow();
			m_Col = il.GetCol();
			m_Flip = il.GetFlip();
		}
		if (recursive){
			item.GetInventory().EnumerateInventory(InventoryTraversalType.LEVELORDER, items);
			if (items && items.Count() > 0){
				//items.Debug();
				for (i = 0; i < items.Count(); i++){
					EntityAI child_item = EntityAI.Cast(items.Get(i));
					if (!m_Cargo){m_Cargo = new array<autoptr UApiEntityStore>;}
					if (child_item && ( item.GetInventory().HasEntityInCargo(child_item) || item.GetInventory().HasAttachment(child_item) ) ){
						UApiEntityStore crg_itemstore = new UApiEntityStore(child_item);
						m_Cargo.Insert(crg_itemstore);
					} else {
						break;
					}
				}
			}
		}
		PlayerBase HoldingPlayer;
		if (Class.CastTo(HoldingPlayer, item.GetHierarchyRootPlayer())){
			m_IsInHands = (HoldingPlayer.GetHumanInventory().GetEntityInHands() == item);
			m_QuickBarSlot = HoldingPlayer.GetQuickBarEntityIndex(item);
		}
		m_IsMagazine = item.IsMagazine() && !item.IsAmmoPile();
		m_IsWeapon = item.IsWeapon();
		
		ItemBase itemB;
		if (Class.CastTo(itemB, item)){
			if (itemB.HasQuantity()){
				m_Quantity = itemB.GetQuantity();
			}
			m_Wet = itemB.GetWet();
			m_Tempature = itemB.GetTemperature();
			m_Energy = itemB.GetEnergy();
			if (itemB.GetCompEM()){
				m_IsOn = itemB.GetCompEM().IsSwitchedOn();
			}
			m_LiquidType = itemB.GetLiquidType();
			itemB.OnUApiSave(this);
		}
		Magazine_Base mag;
		float dmg;
		string cartType;
		if (m_IsMagazine && Class.CastTo(mag, item)){
			m_Quantity = mag.GetAmmoCount();
			for (i = 0; i < mag.GetAmmoCount(); i++){
				dmg = -1;
				cartType = "";
				if (mag.GetCartridgeAtIndex(i, dmg, cartType) && cartType != "" && dmg >= 0){
					if (!m_MagAmmo){ m_MagAmmo = new array<autoptr UApiAmmoData>}
					m_MagAmmo.Insert(new UApiAmmoData(i, dmg, cartType));
				}
			}
		} else if (item.IsAmmoPile() && Class.CastTo(mag, item)){
			m_Quantity = mag.GetAmmoCount();
		}
		Weapon_Base weap;
		if (m_IsWeapon && Class.CastTo(weap, item)){
			int m_CurrentMuzzle = weap.GetCurrentMuzzle();
			m_Quantity = weap.GetTotalCartridgeCount(m_CurrentMuzzle);
			Write("m_IsJammed",  weap.IsJammed());
			Write("m_CurrentMuzzle", m_CurrentMuzzle);
			Write("m_Zeroing", weap.GetStepZeroing(weap.GetCurrentMuzzle()));
			Write("m_Zoom", weap.GetZoom());
			for (i = 0; i < weap.GetTotalCartridgeCount(m_CurrentMuzzle); i++){
				dmg = -1;
				cartType = "";
				if (weap.GetInternalMagazineCartridgeInfo(m_CurrentMuzzle, i, dmg, cartType) && cartType != "" && dmg >= 0){
					if (!m_MagAmmo){ m_MagAmmo = new array<autoptr UApiAmmoData>}
					m_MagAmmo.Insert(new UApiAmmoData(i, dmg, cartType));
				}
			}
			if (!weap.IsChamberEmpty(m_CurrentMuzzle)){
				dmg = -1;
				cartType = "";
				if (weap.GetCartridgeInfo(m_CurrentMuzzle, dmg, cartType) && cartType != "" && dmg >= 0 ){
					m_ChamberedRound = new UApiAmmoData(-1, dmg,cartType);
				}
			}
			if (!m_FireModes){m_FireModes = new array<int>;}
			for (i = 0; i < weap.GetMuzzleCount(); ++i){
				m_FireModes.Insert(weap.GetCurrentMode(i));
			}
		}
	}
	
	EntityAI Create(EntityAI parent = NULL, bool RestoreOrginalLocation = true){
		EntityAI item;
		if (parent == NULL){
			item = EntityAI.Cast(GetGame().CreateObject(m_Type, "0 0 0"));
		} 
		if (m_Slot == -1) {
			item = EntityAI.Cast(parent.GetInventory().CreateEntityInCargoEx(m_Type, m_Idx, m_Row, m_Col, m_Flip));
			
		} else if (m_IsInHands){
			PlayerBase player = PlayerBase.Cast(parent.GetHierarchyRootPlayer());
			if ( player ) {
			 	item = EntityAI.Cast(player.GetHumanInventory().CreateInHands(m_Type));
			}		
		} else {
			item = EntityAI.Cast(parent.GetInventory().CreateAttachmentEx(m_Type, m_Slot));
		}
		if (!item && parent){
			item = EntityAI.Cast(GetGame().CreateObject(m_Type, parent.GetPosition()));
		} 
		if (!item){
			Print("[UAPI] [ERROR] Couldn't create item " + m_Type);
			return NULL;
		} 
		LoadEntity(item);
		return item;
	}
	
	EntityAI CreateAtPos(vector Pos, vector Ori = "0 0 0"){
		EntityAI item;
		item = EntityAI.Cast(GetGame().CreateObject(m_Type, Pos));
		if (!item){
			Print("[UAPI] [UAPI] [ERROR] Couldn't create item " + m_Type);
			return NULL;
		} 
		item.SetPosition(Pos);
		item.SetOrientation(Ori);
		LoadEntity(item);
		return item;
	}
	
	void LoadEntity(EntityAI item){
		int i;
		item.SetHealth("", "", m_Health);
		ItemBase itemB;
		
		Weapon_Base weap;
		if (m_IsWeapon && Class.CastTo(weap, item)){
			int m_CurrentMuzzle = GetInt("m_CurrentMuzzle");
			if (m_CurrentMuzzle >= weap.GetMuzzleCount() || m_CurrentMuzzle < 0){
				weap.SetCurrentMuzzle(m_CurrentMuzzle);
			}
		}
		if (m_Cargo && m_Cargo.Count() > 0){
			for(i = 0; i < m_Cargo.Count(); i++){
				if (m_Cargo.Get(i) && m_Cargo.Get(i).m_IsMagazine && m_IsWeapon && weap){ //Is a mag in a weapon
					Magazine_Base child_mag = Magazine_Base.Cast(m_Cargo.Get(i).Create(item));
					if (weap && child_mag){
						weap.AttachMagazine(weap.GetCurrentMuzzle(), child_mag);
					}
				} else {
					m_Cargo.Get(i).Create(item);				
				}
			}
		}
		if (Class.CastTo(itemB, item)){
			if (itemB.HasQuantity() && !itemB.IsMagazine()){
				itemB.SetQuantity(m_Quantity);
			}
			itemB.SetWet(m_Wet);
			itemB.SetTemperature(m_Tempature);
			itemB.SetLiquidType(m_LiquidType);
			if (itemB.GetCompEM()){
				itemB.GetCompEM().SetEnergy(m_Energy);
				if (m_IsOn){
					itemB.GetCompEM().SwitchOn();
				}
			}
			itemB.OnUApiLoad(this);
		}
		PlayerBase HoldingPlayer;
		if (Class.CastTo(HoldingPlayer, item.GetHierarchyRootPlayer())){
			if (m_QuickBarSlot >= 0){
				Print("[UAPI] SetQuickBarEntityShortcut " + m_Type + " to " + m_QuickBarSlot);
				HoldingPlayer.SetQuickBarEntityShortcut(item, m_QuickBarSlot);
			}
		}
		Magazine_Base mag;
		float dmg;
		string cartType;
		int count;
		if (m_IsMagazine && Class.CastTo(mag, item)){
			count = m_Quantity;
			mag.ServerSetAmmoCount(count);
			for (i = 0; i < mag.GetAmmoCount(); i++){
				if (i > m_MagAmmo.Count()){break;}
				if (m_MagAmmo.Get(i) && m_MagAmmo.Get(i).dmg() >= 0 && m_MagAmmo.Get(i).cartTypeName() != "" && m_MagAmmo.Get(i).cartIndex() == i){
					mag.SetCartridgeAtIndex(m_MagAmmo.Get(i).cartIndex(), m_MagAmmo.Get(i).dmg(), m_MagAmmo.Get(i).cartTypeName());
				}
			}
		} else if (item.IsAmmoPile() && Class.CastTo(mag, item)){
			count = m_Quantity;
			mag.ServerSetAmmoCount(count);
		}
		item.SetSynchDirty();
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Call(item.AfterStoreLoad);
	}
	
	
	override string ToJson(){
		string jsonString = JsonFileLoader<UApiEntityStore>.JsonMakeData(this);
		return jsonString;
	}
	
	bool IsValid(){
		return m_Type != "" && m_Health >= 0;
	}
	
	bool Write(string var, bool data){
		if (!m_MetaData) { m_MetaData = new array<autoptr UApiMetaData>;}
		m_MetaData.Insert(new UApiMetaData(var, data.ToString()));
		return true;
	}
	bool Write(string var, int data){
		if (!m_MetaData) { m_MetaData = new array<autoptr UApiMetaData>;}
		m_MetaData.Insert(new UApiMetaData(var, data.ToString()));
		return true;
	}
	bool Write(string var, float data){
		if (!m_MetaData) { m_MetaData = new array<autoptr UApiMetaData>;}
		m_MetaData.Insert(new UApiMetaData(var, data.ToString()));
		return true;
	}
	bool Write(string var, vector data){
		if (!m_MetaData) { m_MetaData = new array<autoptr UApiMetaData>;}
		m_MetaData.Insert(new UApiMetaData(var, data.ToString()));
		return true;
	}
	bool Write(string var, string data){
		if (!m_MetaData) { m_MetaData = new array<autoptr UApiMetaData>;}
		m_MetaData.Insert(new UApiMetaData(var, data));
		return true;
	}
	bool Write(string var, Class data){
		Error("[UAPI] Trying to save undefined data class to " + var + " for " + m_Type + " try converting to a string before saving");
		return false;
	}
	
	
	bool Read(string var, out bool data){
		for(int i = 0; i < m_MetaData.Count(); i++){
			if (m_MetaData.Get(i) && m_MetaData.Get(i).Is(var)){
				data = m_MetaData.Get(i).ReadInt();
				return true;
			}
		}
		return false;
	}
	bool Read(string var, out int data){
		for(int i = 0; i < m_MetaData.Count(); i++){
			if (m_MetaData.Get(i) && m_MetaData.Get(i).Is(var)){
				data = m_MetaData.Get(i).ReadInt();
				return true;
			}
		}
		return false;
	}
	bool Read(string var, out string data){
		for(int i = 0; i < m_MetaData.Count(); i++){
			if (m_MetaData.Get(i) && m_MetaData.Get(i).Is(var)){
				data = m_MetaData.Get(i).ReadString();
				return true;
			}
		}
		return false;
	}
	bool Read(string var, out float data){
		for(int i = 0; i < m_MetaData.Count(); i++){
			if (m_MetaData.Get(i) && m_MetaData.Get(i).Is(var)){
				data = m_MetaData.Get(i).ReadFloat();
				return true;
			}
		}
		return false;
	}
	bool Read(string var, out vector data){
		for(int i = 0; i < m_MetaData.Count(); i++){
			if (m_MetaData.Get(i) && m_MetaData.Get(i).Is(var)){
				data = m_MetaData.Get(i).ReadVector();
				return true;
			}
		}
		return false;
	}
	bool Read(string var, out Class data){
		Error("[UAPI] Trying to read undefined data class to " + var + " for " + m_Type + " try converting to a string before saving");
		return false;
	}
	
	int GetInt(string var){
		for(int i = 0; i < m_MetaData.Count(); i++){
			if (m_MetaData.Get(i) && m_MetaData.Get(i).Is(var)){return m_MetaData.Get(i).ReadInt();}
		}
		return 0;
	}
	float GetFloat(string var){
		for(int i = 0; i < m_MetaData.Count(); i++){
			if (m_MetaData.Get(i) && m_MetaData.Get(i).Is(var)){ return m_MetaData.Get(i).ReadFloat(); }
		}
		return 0;
	}
	vector GetVector(string var){
		for(int i = 0; i < m_MetaData.Count(); i++){
			if (m_MetaData.Get(i) && m_MetaData.Get(i).Is(var)){ return m_MetaData.Get(i).ReadVector(); }
		}
		return Vector(0,0,0);
	}
	string GetString(string var){
		for (int i = 0; i < m_MetaData.Count(); i++){
			if (m_MetaData.Get(i) && m_MetaData.Get(i).Is(var)){ return m_MetaData.Get(i).ReadString(); }
		}
		return "";
	}
}
