modded class PlayerBase extends ManBase{

	bool UCanAcceptCurrency(string key, ItemBase item){
		return !item.IsRuined() || UCurrency.GetCurrency(key).CanUseRuined();
	}
	
	
	int UGetPlayerBalance(string key){
		int PlayerBalance = 0;
		if (!UCurrency.GetCurrency(key) || UCurrency.GetCurrency(key).Count() < 1){
			UCurrency.UDebug();
			UApiLog.Err("Currency key: " + key + " is not configured");
			return 0;
		}
		array<EntityAI> inventory = new array<EntityAI>;
		this.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, inventory);
		
		ItemBase item;
		for (int i = 0; i < inventory.Count(); i++){
			if (Class.CastTo(item, inventory.Get(i))){
				for (int j = 0; j < UCurrency.GetCurrency(key).Count(); j++){
					if (!UCurrency.GetCurrency(key).Get(j)){
						UApiLog.Err("Currency key: " + key + " idx " + j + " is NULL");
						UCurrency.UDebug();
						break;
					}
					if (item.GetType() == UCurrency.GetCurrency(key).Get(j).TypeClass() && UCanAcceptCurrency(key, item)){
						PlayerBalance += UCurrentQuantity(item) * UCurrency.GetCurrency(key).Get(j).Value();
					}
				}
			}
		}
		return PlayerBalance;
	}
	
	
	int UAddMoney(string key, int Amount){
		if (Amount <= 0){
			return 2;
		}
		int Return = 0;
		int AmountToAdd = Amount;
		bool NoError = true;
		int PlayerBalance = UGetPlayerBalance(key);
		int OptimalPlayerBalance = PlayerBalance + AmountToAdd;
		
		UCurrencyValue MoneyValue = UCurrency.GetCurrency(key).GetHighestDenomination(AmountToAdd);
		int MaxLoop = 3000;
		while (MoneyValue && AmountToAdd >= UCurrency.GetLowestDenominationValue(key) && NoError && MaxLoop > 0){
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
			
			UCurrencyValue NewMoneyValue = UCurrency.GetCurrency(key).GetHighestDenomination(AmountToAdd);
			if (NewMoneyValue && NewMoneyValue != MoneyValue){
				MoneyValue = NewMoneyValue;
			} else {
				NoError = false;
			}
		}
		return Return;
	}
	
	
	int URemoveMoney(string key, int Amount){
		if (Amount <= 0){
			return 2;
		}
		int Return = 0;
		int AmountToRemove = Amount;
		bool NoError = true;
		for (int i = 0; i < UCurrency.GetCurrency(key).Count(); i++){
			AmountToRemove =  URemoveMoneyInventory(key, UCurrency.GetCurrency(key).Get(i), AmountToRemove);
		}
		if (AmountToRemove >= UCurrency.GetLowestDenominationValue(key)){ // Now to delete a larger bill and make change
			for (int j = UCurrency.GetLastIndex(key); j >= 0; j--){
				//UApiLog.Debug("Trying to remove " + UCurrency.GetCurrency(key).Get(j).TypeClass());
				int NewAmountToRemove =  URemoveMoneyInventory(key, UCurrency.GetCurrency(key).Get(j), UCurrency.GetCurrency(key).Get(j).Value());
				if (NewAmountToRemove == 0){
					int AmountToAddBack = UCurrency.GetCurrency(key).Get(j).Value() - AmountToRemove;
					//UApiLog.Debug("A " + UCurrency.GetCurrency(key).Get(j).TypeClass + " removed trying to add back " + AmountToAddBack );
					Return = UAddMoney(key, AmountToAddBack);
				}
			}
		}
		return Return;
	}
	
	//Return how much left still to remove
	int URemoveItemFromInventory(string removeItemType, float Amount = 1 ){
		int AmountToRemove = Amount;
		if (AmountToRemove > 0){
			array<EntityAI> itemsArray = new array<EntityAI>;
			this.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, itemsArray);
			for (int i = 0; i < itemsArray.Count(); i++){
				ItemBase item = ItemBase.Cast(itemsArray.Get(i));
				if ( item ){
					string ItemType = item.GetType();
					ItemType.ToLower();
					string RemoveItemType = removeItemType;
					RemoveItemType.ToLower();
					if (ItemType == RemoveItemType){
						int CurQuantity = item.GetQuantity();
						int AmountRemoved = 0;
						if (!item.HasQuantity()){
							CurQuantity = 1;
						} 
						if (AmountToRemove < CurQuantity){
							AmountRemoved = AmountToRemove;
							item.USetQuantity(CurQuantity - AmountToRemove);
							this.UpdateInventoryMenu(); // RPC-Call needed?
							return Amount - AmountRemoved;
						} else if (AmountToRemove == CurQuantity){
							AmountRemoved = AmountToRemove;
							GetGame().ObjectDelete(item);
							this.UpdateInventoryMenu(); // RPC-Call needed?
							return Amount - AmountRemoved;
						} else {
							AmountRemoved = CurQuantity;
							AmountToRemove = AmountToRemove - CurQuantity;
							GetGame().ObjectDelete(item);
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
	
	int UGetItemCount(string itemType, bool CountRuined = true){
		int PlayerBalance = 0;
		array<EntityAI> inventory = new array<EntityAI>;
		this.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, inventory);
		
		ItemBase item;
		for (int i = 0; i < inventory.Count(); i++){
			if (Class.CastTo(item, inventory.Get(i))){
				if (item.GetType() == itemType && ( !item.IsRuined() || CountRuined)){
					PlayerBalance += UCurrentQuantity(item);;
				}
			}
		}
		return PlayerBalance;
	}
	
	//Return how much left still to remove
	float URemoveMoneyInventory(string key, UCurrencyValue MoneyValue, float Amount ){
		int AmountToRemove = UCurrency.GetAmount(MoneyValue, Amount);
		if (AmountToRemove > 0){
			array<EntityAI> itemsArray = new array<EntityAI>;
			this.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, itemsArray);
			for (int i = 0; i < itemsArray.Count(); i++){
				ItemBase item = ItemBase.Cast(itemsArray.Get(i));
				if (item && UCanAcceptCurrency(key, item)){
					string ItemType = item.GetType();
					ItemType.ToLower();
					string MoneyType = MoneyValue.TypeClass();
					MoneyType.ToLower();
					if (ItemType == MoneyType){
						int CurQuantity = item.GetQuantity();
						int AmountRemoved = 0;
						if (!item.HasQuantity()){
							CurQuantity = 1;
						} 
						if (AmountToRemove < CurQuantity){
							AmountRemoved = MoneyValue.Value() * AmountToRemove;
							item.USetQuantity(CurQuantity - AmountToRemove);
							this.UpdateInventoryMenu(); // RPC-Call needed?
							return Amount - AmountRemoved;
						} else if (AmountToRemove == CurQuantity){
							AmountRemoved = MoneyValue.Value() * AmountToRemove;
							GetGame().ObjectDelete(item);
							this.UpdateInventoryMenu(); // RPC-Call needed?
							return Amount - AmountRemoved;
						} else {
							AmountRemoved = MoneyValue.Value() * CurQuantity;
							AmountToRemove = AmountToRemove - CurQuantity;
							GetGame().ObjectDelete(item);
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
	
}