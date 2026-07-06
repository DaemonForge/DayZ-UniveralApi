/**
 * Modded ItemBase class providing custom persistence and quantity management utilities.
 * 
 * Extends vanilla ItemBase with:
 * - UEntityStore integration hooks for custom item persistence
 * - Quantity management helpers that prevent overflow and handle remainder amounts
 */
modded class ItemBase {

	/**
	 * Called when the entity is being saved to a UEntityStore for persistence.
	 * Override this method in your modded item classes to save custom properties.
	 * 
	 * @param data UEntityStore instance for writing custom metadata
	 * 
	 * @usage
	 * override void OnUFSave(UEntityStore data) {
	 *     super.OnUFSave(data);
	 *     data.Write("customProperty", m_CustomValue);
	 * }
	 */
	void OnUFSave(UEntityStore data){
		
	}
	
	/**
	 * Called when the entity is being loaded from a UEntityStore after persistence.
	 * Override this method in your modded item classes to restore custom properties.
	 * 
	 * @param data UEntityStore instance for reading custom metadata
	 * 
	 * @usage
	 * override void OnUFLoad(UEntityStore data) {
	 *     super.OnUFLoad(data);
	 *     data.Read("customProperty", m_CustomValue);
	 * }
	 */
	void OnUFLoad(UEntityStore data){
		
	}
	
	/**
	 * Adds quantity to an item without exceeding maximum capacity.
	 * Returns the amount that could not be added (overflow).
	 * 
	 * @param amount Quantity to add
	 * @return Remaining amount that couldn't fit (0 if all was added)
	 * 
	 * @usage
	 * ItemBase water = ItemBase.Cast(GetGame().CreateObject("Canteen", playerPos));
	 * int overflow = water.UAddQuantity(150); // Add 150ml
	 * if (overflow > 0) {
	 *     Print("Couldn't add " + overflow + " ml - canteen full");
	 * }
	 */
    int UAddQuantity(float amount) {
        Magazine mag;
        if (IsMagazine() && Class.CastTo(mag, this)) {
            // Magazines and ammo piles track quantity via ammo count
            int remainingAmmo = mag.GetAmmoMax() - mag.GetAmmoCount();
            if (remainingAmmo <= 0){
                return amount;
            }
            if ( amount >= remainingAmmo ) {
                mag.ServerSetAmmoCount(mag.GetAmmoMax());
                return amount - remainingAmmo;
            }
            mag.ServerSetAmmoCount(mag.GetAmmoCount() + amount);
            return 0;
        }
        int remainingQty = GetQuantityMax() - GetQuantity();
        if (remainingQty == 0){
            return amount;
        }
        if ( amount >= remainingQty ) {
            AddQuantity(remainingQty);
            return amount - remainingQty;
        } else {
            AddQuantity(amount);
            return 0;
        }
	}

	/**
	 * Sets item quantity without exceeding maximum capacity.
	 * Returns the amount that exceeded the maximum.
	 * 
	 * @param amount Quantity to set
	 * @return Amount that exceeded maximum capacity (0 if within limits)
	 * 
	 * @usage
	 * ItemBase rice = ItemBase.Cast(GetGame().CreateObject("Rice", playerPos));
	 * int excess = rice.USetQuantity(200); // Try to set 200g
	 * if (excess > 0) {
	 *     Print("Item can only hold " + rice.GetQuantity() + "g, excess: " + excess + "g");
	 * }
	 */
    int USetQuantity(float amount) {
        Magazine mag;
        if (IsMagazine() && Class.CastTo(mag, this)) {
            // Magazines and ammo piles track quantity via ammo count
            int maxAmmo = mag.GetAmmoMax();
            if (maxAmmo <= 0){
                return amount;
            }
            if ( amount >= maxAmmo ) {
                mag.ServerSetAmmoCount(maxAmmo);
                return amount - maxAmmo;
            }
            mag.ServerSetAmmoCount(amount);
            return 0;
        }
        int maxQty = GetQuantityMax();
        if (maxQty <= 0){
            return amount; //no quantity capacity (e.g. quantityBar with no varQuantityMax) - nothing can be stored
        }
        if ( amount >= maxQty ) {
            SetQuantity(maxQty);
            return amount - maxQty;
        } else {
            SetQuantity(amount);
            return 0;
        }
	}
    
	/**
	 * Checks if this item type has a quantity bar in config.
	 * 
	 * @return True if item has quantityBar config property, false otherwise
	 * 
	 * @usage
	 * if (item.UHasQuantityBar()) {
	 *     Print("This item shows a quantity bar");
	 * }
	 */
    bool UHasQuantityBar() {
        return this.ConfigGetBool("quantityBar");
    }
}