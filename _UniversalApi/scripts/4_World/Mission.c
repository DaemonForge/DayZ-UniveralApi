modded class MissionBaseWorld
{
	protected bool m_UApi_Initialized = false;
	
	void UniversalApiReadyTokenReceived(){
		Print("[UAPI] MissionBaseWorld - UniversalApiReadyTokenReceived");
		if (!m_UApi_Initialized){
			m_UApi_Initialized = true;
			UniversalApiReady();
		}
	}
	
	void UniversalApiReady(){
		Print("[UAPI] MissionBaseWorld - UniversalApiReady");
	
	}

}