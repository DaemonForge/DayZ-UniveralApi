/**
 * Modded UEntityStore implementation providing full entity persistence functionality.
 * Extends base UEntityStore with complete serialization and deserialization logic.
 * 
 * Handles persistence for:
 * - Item properties (health, quantity, temperature, wetness, energy, cleanliness)
 * - Inventory positions (slot, cargo coordinates, flip state, quickbar, in-hands)
 * - Cargo contents (recursive item serialization)
 * - Magazines (ammo count, cartridge damage/type per round)
 * - Weapons (chambered rounds, fire modes, muzzle state, zeroing, zoom)
 * - Vehicles (custom vehicle data via OnUFSave/OnUFLoad)
 * - Damage zones (per-zone health values)
 * - Liquids and contamination agents
 * 
 * @see UEntityStore base class for data structure and metadata methods
 */
modded class UEntityStore extends UFObject_Base {
	
	/**
	 * Serializes a complete EntityAI into this store, capturing all state.
	 * 
	 * @param item Entity to serialize (must not be NULL)
	 * @param recursive If true, serializes all cargo/attachments recursively
	 * 
	 * @usage Save Player's Full Inventory:
	 * PlayerBase player = GetGame().GetPlayer();
	 * UEntityStore playerStore = new UEntityStore();
	 * playerStore.SaveEntity(player, true);  // Saves player + all inventory
	 * string json = playerStore.ToJson();
	 * // Save json to database
	 * 
	 * @usage Save Single Item (No Cargo):
	 * ItemBase rifle = ItemBase.Cast(player.GetItemInHands());
	 * UEntityStore rifleStore = new UEntityStore();
	 * rifleStore.SaveEntity(rifle, false);  // Just the rifle, no attachments
	 * 
	 * @note Calls item.OnUFSave(this) to allow custom data storage
	 * @note Recursively saves cargo/attachments when recursive=true
	 * @note Saves damage zones, ammo, weapon state, vehicle state automatically
	 */
	override void SaveEntity(notnull EntityAI item, bool recursive = true ){
		m_Type = item.GetType();
		item.GetPersistentID(m_pid1, m_pid2, m_pid3, m_pid4); //Just for testing but maybe someone will find this usefull
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
					if (!m_Cargo){m_Cargo = new array<autoptr UEntityStore>;}
					if (child_item && ( item.GetInventory().HasEntityInCargo(child_item) || item.GetInventory().HasAttachment(child_item) ) ){
						UEntityStore crg_itemstore = new UEntityStore(child_item);
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
			m_Agents = itemB.GetAgents();
			m_Cleanness = itemB.m_Cleanness;
			itemB.OnUFSave(this);
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
					if (!m_MagAmmo){ m_MagAmmo = new array<autoptr UAmmoData>}
					m_MagAmmo.Insert(new UAmmoData(i, dmg, cartType));
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
					if (!m_MagAmmo){ m_MagAmmo = new array<autoptr UAmmoData>}
					m_MagAmmo.Insert(new UAmmoData(i, dmg, cartType));
				}
			}
			if (!weap.IsChamberEmpty(m_CurrentMuzzle)){
				dmg = -1;
				cartType = "";
				if (weap.GetCartridgeInfo(m_CurrentMuzzle, dmg, cartType) && cartType != "" && dmg >= 0 ){
					m_ChamberedRound = new UAmmoData(-1, dmg,cartType);
				}
			}
			if (!m_FireModes){m_FireModes = new array<int>;}
			for (i = 0; i < weap.GetMuzzleCount(); ++i){
				m_FireModes.Insert(weap.GetCurrentMode(i));
			}
		}
		
		// Damage System
		DamageZoneMap zones = new DamageZoneMap;
		DamageSystem.GetDamageZoneMap(item,zones);
		for( i = 0; i < zones.Count(); i++ ){
			string zone = zones.GetKey(i);
			SaveZoneHealth(zone, item.GetHealth(zone, ""));
		}
		
		CarScript vehicle;
		if (Class.CastTo(vehicle,item)){
			m_IsVehicle = true;
			vehicle.OnUFSave(this);
		}
	}
	
	/**
	 * Creates and restores an entity from this store.
	 * Attempts placement based on stored inventory location.
	 * 
	 * @param parent Parent entity to create within (NULL = spawn at world origin)
	 * @param RestoreOrginalLocation Currently unused parameter (for future expansion)
	 * @return Created EntityAI with all properties restored, or NULL on failure
	 * 
	 * @usage Restore Item to Player Inventory:
	 * UEntityStore itemStore;  // Loaded from database
	 * PlayerBase player = GetGame().GetPlayer();
	 * EntityAI restoredItem = itemStore.Create(player);
	 * if (restoredItem) {
	 *     Print("Item restored to inventory at original slot");
	 * }
	 * 
	 * @note Placement priority:
	 * 1. If m_IsInHands=true, creates in player hands
	 * 2. If m_Slot!=-1, creates in attachment slot
	 * 3. If m_Slot==-1, creates in cargo at stored coordinates
	 * 4. If all fail, spawns at parent position on ground
	 * @note Restores quickbar slot if m_QuickBarSlot >= 0
	 * @note Recursively creates all cargo items from m_Cargo array
	 */
	override EntityAI Create(EntityAI parent = NULL, bool RestoreOrginalLocation = true){
		EntityAI item;
		if (parent == NULL){
			item = EntityAI.Cast(g_Game.CreateObject(m_Type, "0 0 0"));
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
			item = EntityAI.Cast(g_Game.CreateObject(m_Type, parent.GetPosition()));
		} 
		if (!item){
			UFLog.Err("[ERROR] Couldn't create item " + m_Type);
			return NULL;
		} 
		LoadEntity(item);
		return item;
	}
	
	/**
	 * Creates and restores an entity at a specific world position.
	 * Useful for spawning stored items at custom locations.
	 * 
	 * @param Pos World position vector
	 * @param Ori Orientation vector (default: "0 0 0")
	 * @return Created EntityAI with all properties restored, or NULL on failure
	 * 
	 * @usage Spawn Stash at Map Coordinates:
	 * UEntityStore stashStore;  // Loaded from database
	 * vector stashPos = "1234.5 0 6789.2";
	 * EntityAI stash = stashStore.CreateAtPos(stashPos);
	 * if (stash) {
	 *     Print("Stash spawned at " + stashPos);
	 * }
	 * 
	 * @usage Spawn Vehicle with Rotation:
	 * UEntityStore carStore;
	 * vector carPos = "5000 0 5000";
	 * vector carOri = "0 45 0";  // Facing 45 degrees
	 * EntityAI car = carStore.CreateAtPos(carPos, carOri);
	 * 
	 * @note Sets position and orientation after creation
	 * @note Restores all properties via LoadEntity()
	 * @note Recursively creates all cargo items
	 */
	override EntityAI CreateAtPos(vector Pos, vector Ori = "0 0 0"){
		EntityAI item;
		item = EntityAI.Cast(g_Game.CreateObject(m_Type, Pos));
		if (!item){
			UFLog.Err("[ERROR] Couldn't create item " + m_Type);
			return NULL;
		} 
		item.SetPosition(Pos);
		item.SetOrientation(Ori);
		LoadEntity(item);
		return item;
	}
	
	/**
	 * Loads all stored properties into an existing entity.
	 * Called automatically by Create() and CreateAtPos().
	 * 
	 * @param item Entity to load properties into
	 * 
	 * @usage Manual Property Restoration:
	 * UEntityStore backupStore;  // Previously saved state
	 * ItemBase existingItem = ItemBase.Cast(GetGame().CreateObject("AK74", playerPos));
	 * backupStore.LoadEntity(existingItem);  // Restore saved state to new item
	 * 
	 * @note Restores:
	 * - Health (global and per-zone)
	 * - Quantity/ammo count
	 * - Physical properties (wet, temperature, energy, cleanness)
	 * - Liquid type and contamination agents
	 * - Magazine ammo (per-cartridge damage and type)
	 * - Weapon state (chambered round, fire modes, muzzle, zeroing, zoom)
	 * - Quickbar slot assignment
	 * - Recursively creates cargo items
	 * - Calls item.OnUFLoad(this) for custom restoration
	 * - Calls vehicle.OnUFLoad(this) for vehicles
	 */
	override void LoadEntity(EntityAI item){
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
			itemB.RemoveAllAgents();//Removes any default agents then add the needed ones.
			itemB.TransferAgents(m_Agents);
			itemB.SetCleanness(m_Cleanness);
			itemB.OnUFLoad(this);
		}
		PlayerBase HoldingPlayer;
		if (Class.CastTo(HoldingPlayer, item.GetHierarchyRootPlayer())){
			if (m_QuickBarSlot >= 0){
				UFLog.Debug("SetQuickBarEntityShortcut " + m_Type + " to " + m_QuickBarSlot);
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
					dmg = m_MagAmmo.Get(i).dmg();
					cartType = m_MagAmmo.Get(i).cartTypeName();
					mag.SetCartridgeAtIndex(m_MagAmmo.Get(i).cartIndex(), dmg, cartType);
					m_MagAmmo.Get(i).setDmg(dmg);
					m_MagAmmo.Get(i).setCartTypeName(cartType);
				}
			}
		} else if (item.IsAmmoPile() && Class.CastTo(mag, item)){
			count = m_Quantity;
			mag.ServerSetAmmoCount(count);
		}
		
		CarScript vehicle;
		if (m_IsVehicle && Class.CastTo(vehicle,item)){
			vehicle.OnUFLoad(this);
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Call(vehicle.Synchronize);
		}
		
		// Damage System
		DamageZoneMap zones = new DamageZoneMap;
		DamageSystem.GetDamageZoneMap(item, zones);
		for( i = 0; i < zones.Count(); i++ ){
			string zone = zones.GetKey(i);
			float health;
			if (ReadZoneHealth(zone, health)){
				item.SetHealth(zone, "", health);
			}
		}
		
		item.SetSynchDirty();
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Call(item.AfterStoreLoad);
	}
}
