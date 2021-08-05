modded class MissionBaseWorld
{
	protected bool m_UApi_Initialized = false;
	
	bool UApiIsInitialized(){
		return m_UApi_Initialized;
	}
	
	void UniversalApiReadyTokenReceived(){
		//Print("[UAPI] MissionBaseWorld - UniversalApiReadyTokenReceived");
		if (!UApiIsInitialized()){
			m_UApi_Initialized = true;
			this.UniversalApiReady();
		}
	}
	
	void UniversalApiReady(){
		//Print("[UAPI] MissionBaseWorld - UniversalApiReady");
	
	}

}