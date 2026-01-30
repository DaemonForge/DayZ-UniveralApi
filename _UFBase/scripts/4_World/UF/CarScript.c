/**
 * Modded CarScript for Universal Framework entity persistence.
 * 
 * @note Override OnUFSave/OnUFLoad to customize vehicle save/load behavior
 */
modded class CarScript extends Car
{
	/**
	 * Called when vehicle is being saved to database via UEntityStore.
	 * 
	 * @param data UEntityStore instance containing vehicle data
	 * 
	 * @usage Override to save custom vehicle data:
	 * void OnUFSave(UEntityStore data) {
	 *     super.OnUFSave(data);
	 *     data.SetString("myCustomData", myValue);
	 * }
	 */
	void OnUFSave(UEntityStore data){
		
	}
	
	/**
	 * Called when vehicle is being loaded from database via UEntityStore.
	 * 
	 * @param data UEntityStore instance containing saved vehicle data
	 * 
	 * @usage Override to restore custom vehicle data:
	 * void OnUFLoad(UEntityStore data) {
	 *     super.OnUFLoad(data);
	 *     myValue = data.GetString("myCustomData");
	 * }
	 */
	void OnUFLoad(UEntityStore data){
		
	}
}