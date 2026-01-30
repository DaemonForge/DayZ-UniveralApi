/**
 * @class MissionBaseWorld
 * @brief Extended mission base class for Universal Framework initialization.
 * 
 * Handles UFramework initialization lifecycle in the mission context.
 * Provides hooks for mods to execute code after UFramework is ready.
 * 
 * @note This is a modded vanilla class - extends DayZ's MissionBaseWorld.
 * @note Override UFrameworkReady() in your mod to execute code after UF initializes.
 */
modded class MissionBaseWorld
{
	protected bool m_UF_Initialized = false;
	
	/**
	 * Checks if Universal Framework has completed initialization.
	 *
	 * @return True if UFrameworkReady() has been called, false otherwise.
	 * 
	 * @note Use this to check if UF is ready before accessing U() singleton.
	 * 
	 * @usage
	 * if (GetMission().UFIsInitialized()) {
	 *     U().db().Load("MyMod", "data123", callback);
	 * }
	 */
	bool UFIsInitialized(){
		return m_UF_Initialized;
	}
	
	/**
	 * Internal callback when UFramework initialization token is received.
	 *
	 * @note Called by UFramework core - DO NOT call manually.
	 * @note Ensures UFrameworkReady() is only called once.
	 * @note Triggers the UFrameworkReady() hook for mod developers.
	 */
	void UFrameworkReadyTokenReceived(){
		UFLog.Debug("MissionBaseWorld - UFrameworkReadyTokenReceived");
		if (!UFIsInitialized()){
			m_UF_Initialized = true;
			this.UFrameworkReady();
		}
	}
	
	/**
	 * Hook called when Universal Framework is fully initialized and ready.
	 *
	 * @note Override this in your mod to execute initialization code.
	 * @note Called once per mission start, after all UF systems are ready.
	 * @note At this point, U() singleton is safe to use.
	 * 
	 * @usage
	 * modded class MissionBaseWorld {
	 *     override void UFrameworkReady() {
	 *         super.UFrameworkReady();
	 *         // Your mod initialization code here
	 *         U().db().Load("MyMod", "config", this, "OnConfigLoaded");
	 *     }
	 * }
	 */
	void UFrameworkReady(){
		UFLog.Debug("MissionBaseWorld - UFrameworkReady");
	
	}

}