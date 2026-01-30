/**
 * Modded PlayerBase providing currency management and inventory utilities.
 * 
 * Extends vanilla PlayerBase with:
 * - Item creation helpers that handle quantity/stacking automatically
 * - Ground spawn utilities
 * - Quantity inspection and manipulation
 * - Item type checking and removal
 */
modded class PlayerBase extends ManBase {
		
	/**
	 * Gets the quickbar slot index for a specific entity.
	 * 
	 * @param entity Entity to find in quickbar
	 * @return Quickbar slot index (0-9), or -1 if not in quickbar
	 */
	int GetQuickBarEntityIndex(EntityAI entity){
		return m_QuickBarBase.FindEntityIndex(entity);
	}
	
	/**
	 * Creates items in player inventory with intelligent stacking and overflow handling.
	 * Automatically fills existing partial stacks before creating new items.
	 * If inventory is full, does NOT spawn items on ground.
	 * 
	 * @param itemType Class name of item to create (e.g., "Apple", "Ammo_308Win")
	 * @param amount Number of items to create (for stackables, total quantity)
	 * @return Number of items that could NOT be created (0 = all created successfully)
	 * 
	 * @usage Smart Stacking:
	 * // Player has 1 apple with 50/100 quantity
	 * int leftover = player.UCreateItemInInventory("Apple", 75);
	 * // Result: Fills first apple to 100, creates new apple with 25
	 * // leftover = 0 (all 75 were added)
	 * 
	 * @usage Overflow Handling:
	 * int failed = player.UCreateItemInInventory("AK74", 5);
	 * if (failed > 0) {
	 *     Print(failed + " rifles couldn't fit in inventory");
	 *     // Consider using UCreateItemGround() for the remainder
	 * }
	 * 
	 * @note Automatically handles:
	 * - Magazines (fills ammo count)
	 * - Ammunition piles (stacks properly)
	 * - Quantified items (rice, water, etc.)
	 * - Non-stackable items (guns, tools)
	 * - Nested inventory (tries to put items in bags/containers)
	 */
	int UCreateItemInInventory(string itemType, int amount = 1)
	{
		array<EntityAI> itemsArray = new array<EntityAI>;
		this.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, itemsArray);
		string itemTypeLower = itemType;
		itemTypeLower.ToLower();
		ItemBase item;
		Ammunition_Base ammoItem;
		int currentAmount = amount;
		bool hasQuantity = ((UMaxQuantity(itemType) > 0) || UHasQuantity(itemType));
		if (hasQuantity){
			for (int i = 0; i < itemsArray.Count(); i++){
				if (currentAmount <= 0){
					this.UpdateInventoryMenu(); // RPC-Call needed?
					return 0;
				}
				Class.CastTo(item, itemsArray.Get(i));
				string itemPlayerType = "";
				if (item){
					if (item.IsRuined()){
						continue;
					}
					itemPlayerType = item.GetType();
					itemPlayerType.ToLower();
					if (itemTypeLower == itemPlayerType && !item.IsFullQuantity() && !item.IsMagazine()){
						currentAmount = item.UAddQuantity(currentAmount);
					}
				}

				Class.CastTo(ammoItem, itemsArray.Get(i));
				if (ammoItem){
					if (ammoItem.IsRuined()){	
						continue;
					}
					itemPlayerType = ammoItem.GetType();
					itemPlayerType.ToLower();
					if (itemTypeLower == itemPlayerType && ammoItem.IsAmmoPile()){
						currentAmount = ammoItem.UAddQuantity(currentAmount);
					}
				}
			}
		}
		bool stoploop = false;
		int MaxLoop = 5000;
		//any leftover or new stacks
		while (currentAmount > 0 && !stoploop && MaxLoop > 0){
			MaxLoop--;
			ItemBase newItem = ItemBase.Cast(this.GetInventory().CreateInInventory(itemType));
			if (!newItem){
				stoploop = true; //To stop the loop from running away since it couldn't create an item
				for (int j = 0; j < itemsArray.Count(); j++){
					Class.CastTo(item, itemsArray.Get(j));
					if (item){ 
						newItem = ItemBase.Cast(item.GetInventory().CreateInInventory(itemType)); //CreateEntityInCargo	
						if (newItem){
							//MLLog.Debug("NewItem Created " + newItem.GetType() + " in " + item.GetType());
							stoploop = false; //Item was created so we don't need to stop the loop anymore
							break;
						}
					}
				}
			}
			
			Magazine newMagItem = Magazine.Cast(newItem);
			Ammunition_Base newammoItem = Ammunition_Base.Cast(newItem);
			if (newMagItem && !newammoItem)	{	
				int SetAmount = currentAmount;
				if (newMagItem.GetQuantityMax() <= currentAmount){
					SetAmount = currentAmount;
					currentAmount = 0;
				} else {
					SetAmount = newMagItem.GetQuantityMax();
					currentAmount = currentAmount - SetAmount;
				}
				newMagItem.ServerSetAmmoCount(SetAmount);
			} else if (hasQuantity){
				if (newammoItem){
					currentAmount = newammoItem.USetQuantity(currentAmount);
	
				}	
				ItemBase newItemBase;
				if (Class.CastTo(newItemBase, newItem)){
					currentAmount = newItemBase.USetQuantity(currentAmount);
				}
			} else { //It created just one of the item
				currentAmount--;
			}
		}
		return currentAmount;
	}
	
	/**
	 * Spawns items on the ground near the player with proper quantity handling.
	 * Automatically calculates required stacks for quantified items.
	 * 
	 * @param Type Class name of item to spawn
	 * @param Amount Number/quantity of items to spawn
	 * 
	 * @usage Spawn Currency:
	 * player.UCreateItemGround("MoneyRuble100", 5);  // Spawns 5x 100-ruble notes
	 * 
	 * @usage Spawn Ammo:
	 * player.UCreateItemGround("Ammo_308Win", 120);  // Spawns stacks totaling 120 rounds
	 * 
	 * @note Items spawn at ECE_PLACE_ON_SURFACE (on ground at player position)
	 */
	void UCreateItemGround(string Type, int Amount = 1){
		int AmountToSpawn = Amount;
		bool HasQuantity = ((UMaxQuantity(Type) > 0) || UHasQuantity(Type));
		int MaxQuanity = UMaxQuantity(Type);
		int StacksRequired = AmountToSpawn;
		if (MaxQuanity != 0){
			StacksRequired = Math.Ceil( AmountToSpawn /  MaxQuanity);
		}
		for (int i = 0; i <= StacksRequired; i++){
			if (AmountToSpawn > 0){
				ItemBase newItem = ItemBase.Cast(g_Game.CreateObjectEx(Type, GetPosition(), ECE_PLACE_ON_SURFACE));
				if (newItem && HasQuantity){
					AmountToSpawn = newItem.USetQuantity(AmountToSpawn);
				}
			}
		}
	}
	
	/**
	 * Gets the current quantity of an item, handling both magazines and quantified items.
	 * 
	 * @param money ItemBase to check (name is misleading - works for any item)
	 * @return Current quantity (1 for non-quantified items, ammo count for magazines, quantity for stackables)
	 * 
	 * @usage
	 * ItemBase ammo = ItemBase.Cast(player.GetItemInHands());
	 * int currentAmmo = player.UCurrentQuantity(ammo);  // e.g., 27 rounds
	 */
	int UCurrentQuantity(ItemBase money){
		ItemBase moneyItem = ItemBase.Cast(money);
		if (!moneyItem){
			return false;
		}	
		if (UMaxQuantity(moneyItem.GetType()) == 0){
			return 1;
		}
		if ( moneyItem.IsMagazine() ){
			Magazine mag = Magazine.Cast(moneyItem);
			if (mag){
				return mag.GetAmmoCount();
			}
		}
		return moneyItem.GetQuantity();
	}

	/**
	 * Gets the maximum quantity an item type can hold from config.
	 * 
	 * @param Type Item class name to check
	 * @return Maximum quantity (0 if item has no quantity system)
	 * 
	 * @usage
	 * int maxAmmo = player.UMaxQuantity("Ammo_308Win");  // Returns max ammo pile size from config
	 * int maxWater = player.UMaxQuantity("Canteen");  // Returns canteen capacity in ml
	 * int maxGun = player.UMaxQuantity("AK74");  // Returns 0 (no quantity)
	 * 
	 * @note Checks both CFG_MAGAZINESPATH (for ammo) and CFG_VEHICLESPATH varQuantityMax (for items)
	 */
	int UMaxQuantity(string Type)
	{
		if ( g_Game.ConfigIsExisting(  CFG_MAGAZINESPATH  + " " + Type + " count" ) ){
			return g_Game.ConfigGetInt(  CFG_MAGAZINESPATH  + " " + Type + " count" );
		}
		if ( g_Game.ConfigIsExisting(  CFG_VEHICLESPATH + " " + Type + " varQuantityMax" ) ){
			return g_Game.ConfigGetInt( CFG_VEHICLESPATH + " " + Type + " varQuantityMax" );
		}
		return 0;
	}
	
	/**
	 * Sets the quantity/ammo count for an item.
	 * 
	 * @param item ItemBase to modify (name is misleading - works for any quantified item)
	 * @param amount Quantity to set
	 * @return True if quantity was set successfully, false otherwise
	 * 
	 * @usage
	 * ItemBase mag = Magazine.Cast(player.GetItemInHands());
	 * if (mag) {
	 *     player.USetMoneyAmount(mag, 30);  // Fill magazine to 30 rounds
	 * }
	 * 
	 * @note Handles both magazines (ServerSetAmmoCount) and quantified items (SetQuantity)
	 */
	bool USetMoneyAmount(ItemBase item, int amount)
	{
		ItemBase money = ItemBase.Cast(item);
		if (!money){
			return false;
		}
		if ( money.IsMagazine() ){
			Magazine mag = Magazine.Cast(money);
			if (mag){
				return true;
				mag.ServerSetAmmoCount(amount);
			}
		}
		else{
			money.SetQuantity(amount);
			return true;
		}
		return false;
	}
	
	/**
	 * Checks if an item type has a quantity system in config.
	 * 
	 * @param type Item class name to check
	 * @return True if item has quantity/ammo count, false otherwise
	 * 
	 * @usage
	 * if (player.UHasQuantity("Rice")) {
	 *     Print("Rice is quantified");  // True - has varQuantityMax
	 * }
	 * if (player.UHasQuantity("Ammo_9x19")) {
	 *     Print("Ammo is quantified");  // True - has magazine count
	 * }
	 * if (player.UHasQuantity("Axe")) {
	 *     Print("Will not print");  // False - no quantity
	 * }
	 * 
	 * @note Checks both CFG_MAGAZINESPATH count and CFG_VEHICLESPATH quantityBar
	 */
	bool UHasQuantity(string type)
	{   
		string path = CFG_MAGAZINESPATH  + " " + type + " count";
	    if (g_Game.ConfigIsExisting(path)){
	     	if (g_Game.ConfigGetInt(path) > 0){
				return true;
			}
		}
	    path = CFG_VEHICLESPATH  + " " + type + " quantityBar";
	    if (g_Game.ConfigIsExisting(path))   {
	        return g_Game.ConfigGetInt(path) == 1;
		}
	
	    return false;
	}
}