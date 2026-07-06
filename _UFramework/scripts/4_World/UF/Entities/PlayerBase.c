/**
 * Modded PlayerBase providing comprehensive currency management system.
 * 
 * Extends vanilla PlayerBase with:
 * - Multi-currency support with denomination handling
 * - Automatic money counting across inventory
 * - Adding/removing money with change-making logic
 * - Item removal utilities
 * - Integration with UCurrency framework
 * 
 * @see UCurrency for currency configuration
 */
modded class PlayerBase extends ManBase{

	/**
	 * Checks if a currency item can be accepted based on ruined state and currency config.
	 * 
	 * @param key Currency key (from UCurrency configuration)
	 * @param item ItemBase to check
	 * @return True if item can be used as currency, false if item is ruined and currency doesn't allow ruined items
	 * 
	 * @usage
	 * ItemBase ruble = ItemBase.Cast(player.GetItemInHands());
	 * if (player.UCanAcceptCurrency("RUB", ruble)) {
	 *     Print("This ruble note is valid");
	 * }
	 */
	bool UCanAcceptCurrency(string key, ItemBase item){
		UCurrency currency = UCurrency.GetCurrency(key);
		if (!item || !currency){
			return false;
		}
		return !item.IsRuined() || currency.CanUseRuined();
	}
	
	
	/**
	 * Calculates the total currency balance in player's inventory.
	 * Enumerates all items and sums up currency values based on denomination config.
	 * 
	 * @param key Currency key (from UCurrency.GetCurrency configuration)
	 * @return Total currency value (e.g., 1250 for 12 x 100-ruble notes + 1 x 50-ruble note)
	 * 
	 * @usage
	 * int rubles = player.UGetPlayerBalance("RUB");
	 * Print("Player has " + rubles + " rubles");
	 * 
	 * if (player.UGetPlayerBalance("USD") >= 500) {
	 *     Print("Player can afford $500 purchase");
	 * }
	 * 
	 * @note Respects UCanAcceptCurrency rules (ruined items may not count)
	 * @note Returns 0 if currency key is invalid or not configured
	 */
	int UGetPlayerBalance(string key){
		int PlayerBalance = 0;
		UCurrency currency = UCurrency.GetConfigured(key);
		if (!currency){
			UCurrency.UDebug();
			UFLog.Err("Currency key: " + key + " is not configured");
			return 0;
		}
		//Precompute lowered denomination names once (case-insensitive to match URemoveMoneyInventory)
		array<string> denomTypes = new array<string>;
		for (int j = 0; j < currency.Count(); j++){
			if (!currency.Get(j)){
				UFLog.Err("Currency key: " + key + " idx " + j + " is NULL");
				UCurrency.UDebug();
				denomTypes.Insert("");
				continue;
			}
			string denomType = currency.Get(j).TypeClass();
			denomType.ToLower();
			denomTypes.Insert(denomType);
		}
		array<EntityAI> inventory = new array<EntityAI>;
		this.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, inventory);

		ItemBase item;
		for (int i = 0; i < inventory.Count(); i++){
			if (Class.CastTo(item, inventory.Get(i))){
				string itemType = item.GetType();
				itemType.ToLower();
				for (j = 0; j < denomTypes.Count(); j++){
					if (denomTypes.Get(j) != "" && itemType == denomTypes.Get(j) && UCanAcceptCurrency(key, item)){
						PlayerBalance += UCurrentQuantity(item) * currency.Get(j).Value();
						break; //an item can only be one denomination
					}
				}
			}
		}
		return PlayerBalance;
	}
	
	
	/**
	 * Adds money to player inventory using optimal denomination breakdown.
	 * Automatically calculates the best mix of bills/coins to minimize item count.
	 * Puts money in inventory first, then spawns overflow on ground.
	 * 
	 * @param key Currency key (from UCurrency configuration)
	 * @param Amount Total currency value to add
	 * @return 0 = all added to inventory, 1 = some spawned on ground, 2 = invalid amount or unconfigured currency
	 * 
	 * @usage Award Quest Reward:
	 * int result = player.UAddMoney("RUB", 1250);
	 * if (result == 0) {
	 *     Print("Added 1250 rubles to inventory");
	 * } else if (result == 1) {
	 *     Print("Inventory full, some money dropped on ground");
	 * }
	 * 
	 * @usage Pay Salary:
	 * player.UAddMoney("USD", 5000);  // Automatically uses mix of $100, $50, $20, $10, etc.
	 * 
	 * @note Uses UCreateItemInInventory and UCreateItemGround from _UFBase
	 * @note Denomination breakdown uses UCurrency.GetHighestDenomination logic
	 * @note Has infinite loop protection (MaxLoop = 3000)
	 * @note Value below the lowest denomination is dropped (logged); use the overload with out NotAdded to detect it
	 */
	int UAddMoney(string key, int Amount){
		int notAdded;
		return UAddMoney(key, Amount, notAdded);
	}

	/**
	 * Overload reporting the value that could not be represented by any denomination.
	 *
	 * @param NotAdded Out - value that was dropped (0 when the full Amount was added)
	 */
	int UAddMoney(string key, int Amount, out int NotAdded){
		NotAdded = 0;
		if (Amount <= 0){
			return 2;
		}
		UCurrency currency = UCurrency.GetConfigured(key);
		if (!currency){
			UFLog.Err("UAddMoney: Currency key: " + key + " is not configured");
			NotAdded = Amount;
			return 2;
		}
		int Return = 0;
		int AmountToAdd = Amount;
		bool NoError = true;
		int LowestValue = currency.LowestDenominationValue();

		UCurrencyValue MoneyValue = currency.GetHighestDenomination(AmountToAdd);
		int MaxLoop = 3000;
		while (MoneyValue && AmountToAdd >= LowestValue && NoError && MaxLoop > 0){
			MaxLoop--;
			int AmountToSpawn = UCurrency.GetAmount(MoneyValue,AmountToAdd);
			if (AmountToSpawn == 0){
				NoError = false;
			}

			int AmountLeft = UCreateItemInInventory(MoneyValue.TypeClass(), AmountToSpawn);
			if (AmountLeft > 0){
				Return = 1;
				UCreateItemGround(MoneyValue.TypeClass(), AmountLeft);
			}

			int AmmountAdded = MoneyValue.Value() * AmountToSpawn;

			AmountToAdd = AmountToAdd - AmmountAdded;

			UCurrencyValue NewMoneyValue = currency.GetHighestDenomination(AmountToAdd);
			if (NewMoneyValue && NewMoneyValue != MoneyValue){
				MoneyValue = NewMoneyValue;
			} else {
				NoError = false;
			}
		}
		if (AmountToAdd > 0){
			NotAdded = AmountToAdd;
			UFLog.Debug("UAddMoney: " + AmountToAdd + " " + key + " is below the lowest denomination and was not added");
		}
		return Return;
	}
	
	
	/**
	 * Removes money from player inventory with automatic change-making.
	 * First removes exact denominations, then breaks larger bills if needed.
	 * 
	 * @param key Currency key (from UCurrency configuration)
	 * @param Amount Total currency value to remove
	 * @return 0 = success, 1 = success but change spawned on ground, 2 = invalid amount or unconfigured currency, 3 = insufficient funds (partial removal may have occurred - check balance first)
	 * 
	 * @usage Deduct Purchase Cost:
	 * int cost = 750;
	 * if (player.UGetPlayerBalance("RUB") >= cost) {
	 *     player.URemoveMoney("RUB", cost);
	 *     // Money removed, give player the item
	 * }
	 * 
	 * @usage Example Change-Making:
	 * // Player has: 1x 1000-ruble, 1x 500-ruble
	 * player.URemoveMoney("RUB", 750);
	 * // Result: Breaks 1000-ruble bill, removes 750, adds back 250 in change
	 * 
	 * @note Uses URemoveMoneyInventory to handle denomination removal
	 * @note Automatically calls UAddMoney to give change when breaking large bills
	 * @note Change logic iterates from lowest to highest denomination and breaks a single bill
	 */
	int URemoveMoney(string key, int Amount){
		if (Amount <= 0){
			return 2;
		}
		UCurrency currency = UCurrency.GetConfigured(key);
		if (!currency){
			UFLog.Err("URemoveMoney: Currency key: " + key + " is not configured");
			return 2;
		}
		int Return = 0;
		int AmountToRemove = Amount;
		//Enumerate the inventory once and share it across all removal passes
		array<EntityAI> itemsArray = new array<EntityAI>;
		this.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, itemsArray);
		for (int i = 0; i < currency.Count(); i++){
			if (AmountToRemove < 1){
				break;
			}
			AmountToRemove = URemoveMoneyInventory(key, currency.Get(i), AmountToRemove, itemsArray);
		}
		if (AmountToRemove >= currency.LowestDenominationValue()){ // Now to delete a larger bill and make change
			bool ChangeMade = false;
			for (int j = currency.LastIndex(); j >= 0; j--){
				int NewAmountToRemove = URemoveMoneyInventory(key, currency.Get(j), currency.Get(j).Value(), itemsArray);
				if (NewAmountToRemove == 0){
					int ChangeNotGiven;
					Return = UAddMoney(key, currency.Get(j).Value() - AmountToRemove, ChangeNotGiven);
					if (ChangeNotGiven > 0){
						UFLog.Info("URemoveMoney: " + ChangeNotGiven + " " + key + " of change could not be matched to a denomination and was lost");
					}
					ChangeMade = true;
					break; //change made - without this, every larger denomination also loses a bill
				}
			}
			if (!ChangeMade){
				//Nothing left to break - insufficient funds. Whatever the first pass removed stays removed.
				UFLog.Debug("URemoveMoney: insufficient " + key + " to cover remaining " + AmountToRemove + " of " + Amount);
				this.UpdateInventoryMenu();
				return 3;
			}
		} else if (AmountToRemove > 0){
			UFLog.Debug("URemoveMoney: remaining " + AmountToRemove + " " + key + " is below the lowest denomination and was not removed");
		}
		this.UpdateInventoryMenu();
		return Return;
	}
	
	/**
	 * Removes a specified amount of an item type from player inventory.
	 * Handles both stackable (quantified) and non-stackable items.
	 * 
	 * @param removeItemType Class name of item to remove (case-insensitive)
	 * @param Amount Number/quantity to remove (1.0 for single items, quantity for stackables)
	 * @return Amount remaining that could NOT be removed (0 = all removed successfully)
	 * 
	 * @usage Remove Crafting Materials:
	 * float leftover = player.URemoveItemFromInventory("Nail", 10);
	 * if (leftover == 0) {
	 *     Print("Removed 10 nails for crafting");
	 * } else {
	 *     Print("Only had " + (10 - leftover) + " nails available");
	 * }
	 * 
	 * @usage Remove Food:
	 * player.URemoveItemFromInventory("Rice", 50);  // Remove 50g of rice
	 * 
	 * @note Case-insensitive comparison
	 * @note Deletes items with ObjectDelete when quantity reaches 0
	 * @note Calls UpdateInventoryMenu() when items are modified
	 */
	int URemoveItemFromInventory(string removeItemType, float Amount = 1 ){
		int AmountToRemove = Amount;
		if (AmountToRemove > 0){
			array<EntityAI> itemsArray = new array<EntityAI>;
			this.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, itemsArray);
			string RemoveItemType = removeItemType;
			RemoveItemType.ToLower();
			for (int i = 0; i < itemsArray.Count(); i++){
				ItemBase item = ItemBase.Cast(itemsArray.Get(i));
				if ( item ){
					string ItemType = item.GetType();
					ItemType.ToLower();
					if (ItemType == RemoveItemType){
						//UCurrentQuantity is magazine-aware (ammo piles count rounds) to match the USetQuantity write below
						int CurQuantity = UCurrentQuantity(item);
						int AmountRemoved = 0;
						if (AmountToRemove < CurQuantity){
							AmountRemoved = AmountToRemove;
							item.USetQuantity(CurQuantity - AmountToRemove);
							this.UpdateInventoryMenu(); // RPC-Call needed?
							return Amount - AmountRemoved;
						} else if (AmountToRemove == CurQuantity){
							AmountRemoved = AmountToRemove;
							g_Game.ObjectDelete(item);
							this.UpdateInventoryMenu(); // RPC-Call needed?
							return Amount - AmountRemoved;
						} else {
							AmountRemoved = CurQuantity;
							AmountToRemove = AmountToRemove - CurQuantity;
							g_Game.ObjectDelete(item);
							Amount = Amount - AmountRemoved;
						}
						if (AmountToRemove <= 0){
							this.UpdateInventoryMenu(); // RPC-Call needed?
							return Amount;
						}
					}
				}
			}
		}
		this.UpdateInventoryMenu(); // RPC-Call needed?
		return Amount;
	}
	
	/**
	 * Counts total quantity of a specific item type in player inventory.
	 * 
	 * @param itemType Class name of item to count
	 * @param CountRuined Whether to include ruined items in count (default: true)
	 * @return Total quantity (1 per non-stackable item, quantity sum for stackables)
	 * 
	 * @usage Check Quest Requirements:
	 * int apples = player.UGetItemCount("Apple", false);  // Count only non-ruined apples
	 * if (apples >= 5) {
	 *     Print("Player has 5+ good apples for quest");
	 * }
	 * 
	 * @usage Count Ammo:
	 * int ammoCount = player.UGetItemCount("Ammo_762x39", true);  // Total ammo including damaged
	 * 
	 * @note Uses UCurrentQuantity to handle stackables correctly
	 * @note Case-insensitive comparison (matches URemoveItemFromInventory)
	 */
	int UGetItemCount(string itemType, bool CountRuined = true){
		int PlayerBalance = 0;
		array<EntityAI> inventory = new array<EntityAI>;
		this.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, inventory);
		
		string SearchType = itemType;
		SearchType.ToLower(); //case-insensitive to match URemoveItemFromInventory
		ItemBase item;
		for (int i = 0; i < inventory.Count(); i++){
			if (Class.CastTo(item, inventory.Get(i))){
				string InvType = item.GetType();
				InvType.ToLower();
				if (InvType == SearchType && ( !item.IsRuined() || CountRuined)){
					PlayerBalance += UCurrentQuantity(item);
				}
			}
		}
		return PlayerBalance;
	}
	
	/**
	 * Internal helper - removes a specific currency denomination from inventory.
	 * Used by URemoveMoney to handle denomination-specific removal.
	 * 
	 * @param key Currency key
	 * @param MoneyValue UCurrencyValue denomination to remove
	 * @param Amount Total value to remove in this denomination
	 * @return Remaining value that could not be removed
	 * 
	 * @note This is an internal method - use URemoveMoney instead for normal currency removal
	 * @note Respects UCanAcceptCurrency rules (skips ruined items if configured)
	 */
	float URemoveMoneyInventory(string key, UCurrencyValue MoneyValue, float Amount ){
		array<EntityAI> itemsArray = new array<EntityAI>;
		this.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, itemsArray);
		float Remaining = URemoveMoneyInventory(key, MoneyValue, Amount, itemsArray);
		this.UpdateInventoryMenu(); // RPC-Call needed?
		return Remaining;
	}

	/**
	 * Overload taking a pre-enumerated inventory so multi-pass callers (URemoveMoney)
	 * traverse the inventory once. Does NOT call UpdateInventoryMenu - the caller must.
	 */
	float URemoveMoneyInventory(string key, UCurrencyValue MoneyValue, float Amount, array<EntityAI> itemsArray){
		int AmountToRemove = UCurrency.GetAmount(MoneyValue, Amount);
		if (AmountToRemove > 0){
			string MoneyType = MoneyValue.TypeClass();
			MoneyType.ToLower();
			for (int i = 0; i < itemsArray.Count(); i++){
				ItemBase item = ItemBase.Cast(itemsArray.Get(i));
				//IsSetForDeletion skips items an earlier pass already removed (deletion is deferred)
				if (item && !item.IsSetForDeletion()){
					string ItemType = item.GetType();
					ItemType.ToLower();
					//cheap type compare first, currency lookup only for matches
					if (ItemType == MoneyType && UCanAcceptCurrency(key, item)){
						//UCurrentQuantity is magazine-aware (ammo piles count rounds) to match the USetQuantity write below
						int CurQuantity = UCurrentQuantity(item);
						int AmountRemoved = 0;
						if (AmountToRemove < CurQuantity){
							AmountRemoved = MoneyValue.Value() * AmountToRemove;
							item.USetQuantity(CurQuantity - AmountToRemove);
							return Amount - AmountRemoved;
						} else if (AmountToRemove == CurQuantity){
							AmountRemoved = MoneyValue.Value() * AmountToRemove;
							g_Game.ObjectDelete(item);
							return Amount - AmountRemoved;
						} else {
							AmountRemoved = MoneyValue.Value() * CurQuantity;
							AmountToRemove = AmountToRemove - CurQuantity;
							g_Game.ObjectDelete(item);
							Amount = Amount - AmountRemoved;
						}
						if (AmountToRemove <= 0){
							return Amount;
						}
					}
				}
			}
		}
		return Amount;
	}
	
}