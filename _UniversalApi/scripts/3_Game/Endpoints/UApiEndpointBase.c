class UApiBaseEndpoint extends Managed {
	
	protected RestContext m_Context;
	
	protected string EndpointBaseUrl(){
		return UApiConfig().GetBaseURL();
	}

	protected string AuthToken(){
		return UApi().GetAuthToken();
	}
	
	
	protected RestContext Api()
	{
		if (!m_Context){
			RestApi clCore = GetRestApi();
			if (!clCore)
			{
				clCore = CreateRestApi();
				clCore.SetOption(ERestOption.ERESTOPTION_READOPERATION, 15);
			}
			m_Context = clCore.GetRestContext(EndpointBaseUrl());
			m_Context.SetHeader(AuthToken());
		}
		return m_Context;
	}
	
	protected void Post(string route, string jsonString, RestCallback UCBX)
	{
		Api().POST(UCBX, route, jsonString);
	}
	
	void UpdateAuthToken(){
		if (m_Context){
			m_Context.SetHeader(AuthToken());
		}
	}
	
}
