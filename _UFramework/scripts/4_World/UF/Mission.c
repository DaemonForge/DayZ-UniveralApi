modded class MissionBaseWorld
{
	protected bool m_UF_Initialized = false;
	
	bool UFIsInitialized(){
		return m_UF_Initialized;
	}
	
	void UFrameworkReadyTokenReceived(){
		UFLog.Debug("MissionBaseWorld - UFrameworkReadyTokenReceived");
		if (!UFIsInitialized()){
			m_UF_Initialized = true;
			this.UFrameworkReady();
		}
	}
	
	void UFrameworkReady(){
		UFLog.Debug("MissionBaseWorld - UFrameworkReady");
	
	}

}