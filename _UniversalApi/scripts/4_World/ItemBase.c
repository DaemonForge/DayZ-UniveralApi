modded class ItemBase {

	void OnUApiSave(EntityStore data){
		
	}
	
	void OnUApiLoad(EntityStore data){
		
	}
}

class EntityStore extends UApiObject_Base {
	
	string m_Type = "";
	float m_Health = -1;
	float m_Quantity;
	float m_Wet;
	float m_Tempature;
	float m_Energy;
	int m_Slot;
	int m_Idx;
	int m_Row;
	int m_Col;
	bool m_Flip;
	bool m_IsInHands;
	bool m_IsOn;
	int m_QuickBarSlot;
	
	autoptr array<autoptr EntityStore> m_Cargo;
	
	bool m_IsMagazine;
	autoptr array<autoptr UApiAmmoData> m_MagAmmo;
	bool m_IsWeapon;
	autoptr array<int> m_FireModes;
	autoptr UApiAmmoData m_ChamberedRound;
	
	autoptr array<autoptr UApiMetaData> m_MetaData;
	
	void EntityStore(EntityAI item = NULL){
		if (!item) return;
		SaveEntity(item, true);
	}
	
	void ~EntityStore(){
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
					if (!m_Cargo){m_Cargo = new array<autoptr EntityStore>;}
					if (child_item && ( item.GetInventory().HasEntityInCargo(child_item) || item.GetInventory().HasAttachment(child_item) ) ){
						EntityStore crg_itemstore = new EntityStore(child_item);
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
			WriteInt("Vanilla", "m_IsJammed",  weap.IsJammed());
			WriteInt("Vanilla", "m_CurrentMuzzle", m_CurrentMuzzle);
			WriteInt("Vanilla", "m_Zeroing", weap.GetStepZeroing(weap.GetCurrentMuzzle()));
			WriteFloat("Vanilla", "m_Zoom", weap.GetZoom());
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
		int i;
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
		item.SetHealth("", "", m_Health);
		ItemBase itemB;
		if (Class.CastTo(itemB, item)){
			if (itemB.HasQuantity() && !itemB.IsMagazine()){
				itemB.SetQuantity(m_Quantity);
			}
			itemB.SetWet(m_Wet);
			itemB.SetTemperature(m_Tempature);
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
		Weapon_Base weap;
		if (m_IsWeapon && Class.CastTo(weap, item)){
			weap.SetJammed(GetInt("Vanilla", "m_IsJammed"));
			int m_CurrentMuzzle = GetInt("Vanilla", "m_CurrentMuzzle");
			if (m_CurrentMuzzle >= weap.GetMuzzleCount() || m_CurrentMuzzle < 0){
				weap.SetCurrentMuzzle(m_CurrentMuzzle);
			}
			for (int mi = 0; mi < weap.GetMuzzleCount(); ++mi)
			{
				if (m_ChamberedRound){
					Print("[UAPI] Pushing Round to Chamber");
					weap.PushCartridgeToChamber(mi, m_ChamberedRound.dmg(), m_ChamberedRound.cartTypeName());
				}
				for (i = 0; i < m_MagAmmo.Count(); i++){
					if (i > m_Quantity) {break;}
					weap.PushCartridgeToInternalMagazine( mi, m_MagAmmo.Get(i).dmg(), m_MagAmmo.Get(i).cartTypeName());
				}
			}
			weap.SetStepZeroing(m_CurrentMuzzle, GetInt("Vanilla", "m_Zeroing"));
			weap.SetZoom(GetFloat("Vanilla", "m_Zoom"));
			if (m_FireModes){
				for (i = 0; i < m_FireModes.Count(); ++i)
				{
					weap.SetCurrentMode(i, m_FireModes.Get(i));
				}
			}
		}
		if (m_Cargo && m_Cargo.Count() > 0){
			for(i = 0; i < m_Cargo.Count(); i++){
				if (m_Cargo.Get(i) && m_Cargo.Get(i).m_IsMagazine && m_IsWeapon && weap){ //Is a mag in a weapon
					Magazine_Base child_mag = Magazine_Base.Cast(m_Cargo.Get(i).Create(item));
					if (weap && child_mag){
						weap.AttachMagazine(weap.GetCurrentMuzzle(), child_mag);
						GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(weap.SelectionMagazineShow,600,false);
						weap.SetJammed(GetInt("Vanilla", "m_IsJammed"));
					}
				} else {
					m_Cargo.Get(i).Create(item);				
				}
			}
		}
		if (m_IsWeapon && weap){
			Print("===========================================================================================================");
			Print("===========================================================================================================");
			Print("[UAPI] [INFO] Validating and Repairing the Weapon Unless this is just before a crash this was not the cause");
			Print("-----------------------------------------------------------------------------------------------------------");
			weap.ValidateAndRepair();
			Print("===========================================================================================================");
			Print("===========================================================================================================");
			/*int dummy_version = int.MAX;
			PlayerBase parentPlayer = PlayerBase.Cast(weap.GetHierarchyRootPlayer());
			if (!parentPlayer)
				dummy_version -= 1;
			ScriptReadWriteContext ctx = new ScriptReadWriteContext;
			weap.OnStoreSave(ctx.GetWriteContext());
			weap.OnStoreLoad(ctx.GetReadContext(), dummy_version);
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(weap.ValidateAndRepair,100,false);*/
		}
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Call(item.AfterStoreLoad);
		return item;
	}
	
	EntityAI CreateAtPos(vector Pos, vector Ori = "0 0 0"){
		int i;
		EntityAI item;
		item = EntityAI.Cast(GetGame().CreateObject(m_Type, Pos));
		if (!item){
			Print("[UAPI] [UAPI] [ERROR] Couldn't create item " + m_Type);
			return NULL;
		} 
		item.SetPosition(Pos);
		item.SetOrientation(Ori);
		item.SetHealth("", "", m_Health);
		ItemBase itemB;
		if (Class.CastTo(itemB, item)){
			if (itemB.HasQuantity() && !itemB.IsMagazine()){
				itemB.SetQuantity(m_Quantity);
			}
			itemB.SetWet(m_Wet);
			itemB.SetTemperature(m_Tempature);
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
			HoldingPlayer.SetQuickBarEntityShortcut(item, m_QuickBarSlot, true);
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
		Weapon_Base weap;
		if (m_IsWeapon && Class.CastTo(weap, item)){
			weap.SetJammed(GetInt("Vanilla", "m_IsJammed"));
			int m_CurrentMuzzle = GetInt("Vanilla", "m_CurrentMuzzle");
			if (m_CurrentMuzzle >= weap.GetMuzzleCount() || m_CurrentMuzzle < 0){
				weap.SetCurrentMuzzle(m_CurrentMuzzle);
			}
			for (int mi = 0; mi < weap.GetMuzzleCount(); ++mi)
			{
				if (m_ChamberedRound){
					Print("[UAPI] Pushing Round to Chamber");
					weap.PushCartridgeToChamber(mi, m_ChamberedRound.dmg(), m_ChamberedRound.cartTypeName());
				}
				for (i = 0; i < m_MagAmmo.Count(); i++){
					if (i > m_Quantity) {break;}
					weap.PushCartridgeToInternalMagazine( mi, m_MagAmmo.Get(i).dmg(), m_MagAmmo.Get(i).cartTypeName());
				}
			}
			weap.SetStepZeroing(m_CurrentMuzzle, GetInt("Vanilla", "m_Zeroing"));
			weap.SetZoom(GetFloat("Vanilla", "m_Zoom"));
			if (m_FireModes){
				for (i = 0; i < m_FireModes.Count(); ++i)
				{
					weap.SetCurrentMode(i, m_FireModes.Get(i));
				}
			}
		}
		if (m_Cargo && m_Cargo.Count() > 0){
			for(i = 0; i < m_Cargo.Count(); i++){
				if (m_Cargo.Get(i) && m_Cargo.Get(i).m_IsMagazine && m_IsWeapon && weap){ //Is a mag in a weapon
					Magazine_Base child_mag = Magazine_Base.Cast(m_Cargo.Get(i).Create(item));
					if (weap && child_mag){
						weap.AttachMagazine(weap.GetCurrentMuzzle(), child_mag);
						GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(weap.SelectionMagazineShow,600,false);
						weap.SetJammed(GetInt("Vanilla", "m_IsJammed"));
					}
				} else {
					m_Cargo.Get(i).Create(item);				
				}
			}
		}
		if (m_IsWeapon && weap){
			Print("===========================================================================================================");
			Print("===========================================================================================================");
			Print("[UAPI] [INFO] Validating and Repairing the Weapon Unless this is just before a crash this was not the cause");
			Print("-----------------------------------------------------------------------------------------------------------");
			weap.ValidateAndRepair();
			Print("===========================================================================================================");
			Print("===========================================================================================================");
			/*int dummy_version = int.MAX;
			PlayerBase parentPlayer = PlayerBase.Cast(weap.GetHierarchyRootPlayer());
			if (!parentPlayer)
				dummy_version -= 1;
			ScriptReadWriteContext ctx = new ScriptReadWriteContext;
			weap.OnStoreSave(ctx.GetWriteContext());
			weap.OnStoreLoad(ctx.GetReadContext(), dummy_version);
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(weap.ValidateAndRepair,100,false);*/
		}
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Call(item.AfterStoreLoad);
		return item;
	}
	
	override string ToJson(){
		string jsonString = JsonFileLoader<EntityStore>.JsonMakeData(this);
		return jsonString;
	}
	
	bool IsValid(){
		return m_Type != "" && m_Health >= 0;
	}
	
	bool Write(string mod, string var, string data){
		if (!m_MetaData) { m_MetaData = new array<autoptr UApiMetaData>;}
		m_MetaData.Insert(new UApiMetaData(mod, var, data));
		return true;
	}
	bool WriteInt(string mod, string var, int data){
		if (!m_MetaData) { m_MetaData = new array<autoptr UApiMetaData>;}
		m_MetaData.Insert(new UApiMetaData(mod, var, data.ToString()));
		return true;
	}
	bool WriteFloat(string mod, string var, float data){
		if (!m_MetaData) { m_MetaData = new array<autoptr UApiMetaData>;}
		m_MetaData.Insert(new UApiMetaData(mod, var, data.ToString()));
		return true;
	}
	
	
	bool Read(string mod, string var, out string data){
		for(int i = 0; i < m_MetaData.Count(); i++){
			if (m_MetaData.Get(i) && m_MetaData.Get(i).Is(mod, var)){
				data = m_MetaData.Get(i).ReadString();
				return true;
			}
		}
		data = "";
		return false;
	}
	
	int GetInt(string mod, string var){
		for(int i = 0; i < m_MetaData.Count(); i++){
			if (m_MetaData.Get(i) && m_MetaData.Get(i).Is(mod, var)){return m_MetaData.Get(i).ReadInt();}
		}
		return 0;
	}
	float GetFloat(string mod, string var){
		for(int i = 0; i < m_MetaData.Count(); i++){
			if (m_MetaData.Get(i) && m_MetaData.Get(i).Is(mod, var)){ return m_MetaData.Get(i).ReadFloat(); }
		}
		return 0;
	}
	vector GetVector(string mod, string var){
		for(int i = 0; i < m_MetaData.Count(); i++){
			if (m_MetaData.Get(i) && m_MetaData.Get(i).Is(mod, var)){ return m_MetaData.Get(i).ReadVector(); }
		}
		return Vector(0,0,0);
	}
	string GetString(string mod, string var){
		for (int i = 0; i < m_MetaData.Count(); i++){
			if (m_MetaData.Get(i) && m_MetaData.Get(i).Is(mod, var)){ return m_MetaData.Get(i).ReadString(); }
		}
		return "";
	}
}
