modded class MissionBaseWorld
{
	protected bool m_UF_Initialized = false;
	
	bool UFIsInitialized(){
		return m_UF_Initialized;
	}
	
	void UFrameworkReadyTokenReceived(){
		//Print("[UF] MissionBaseWorld - UFrameworkReadyTokenReceived");
		if (!UFIsInitialized()){
			m_UF_Initialized = true;
			this.UFrameworkReady();
		}
	}
	
	void UFrameworkReady(){
		//Print("[UF] MissionBaseWorld - UFrameworkReady");
	
	}

}