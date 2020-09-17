class DayZGame extends CGame
{

	protected ref ApiAuthToken m_authToken;
	
	protected string m_ReadAuthToken;

	string GetAuthToken(){
		if (m_authToken && !IsServer()){
			return m_authToken.GetAuthToken();
		} else if (IsServer() && UApiConfig().ServerAuth != ""){
			return UApiConfig().ServerAuth;
		}
		return "null";
	}
	
	string GetReadAuth(){
		return m_ReadAuthToken;
	}
	
	void SetReadAuth(string readAuthToken){
		m_ReadAuthToken  = readAuthToken;
	}
	
	override void DeferredInit()
	{
		super.DeferredInit();
		
		GetRPCManager().AddRPC( "UAPI", "RPCUniversalApiConfig", this, SingeplayerExecutionType.Both );
	}
	
	void RPCUniversalApiConfig( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		Param2<ApiAuthToken, DayZGameApiConfig> data;  //Player ID, Icon
		if ( !ctx.Read( data ) ) return;
		m_authToken = params.param1;
		m_UniversalApiConfig = params.param2;
	}
}