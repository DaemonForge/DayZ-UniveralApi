modded class ItemBase {

	void OnUApiSave(UApiEntityStore data){
		
	}
	
	void OnUApiLoad(UApiEntityStore data){
		
	}
	
	//returns remaining
    int UAddQuantity(float amount) {	
        if (!IsMagazine()) {
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
        return amount;
	}

	//returns remaining
    int USetQuantity(float amount) {	
        if (!IsMagazine()) {
            int maxQty = GetQuantityMax();			
            if ( amount >= maxQty ) {
                SetQuantity(maxQty);
                return amount - maxQty;
            } else {
                SetQuantity(amount);
                return 0;
            }
		}        
        return amount;
	}
    
    bool UHasQuantityBar() {
        return this.ConfigGetBool("quantityBar");
    }
}