class UFBaseEndpoint extends Managed {
	
	protected RestContext m_Context;
	
	protected string EndpointBaseUrl(){
		UFrameworkConfig cfg = UFConfig();
		if (!cfg){
			UFLog.Err("[UFBaseEndpoint] EndpointBaseUrl called but UFConfig() is null!");
			return "";
		}
		return cfg.GetBaseURL();
	}

	protected string AuthToken(){
		UFramework uf = U();
		if (!uf){
			UFLog.Err("[UFBaseEndpoint] AuthToken called but U() is null!");
			return "";
		}
		return uf.GetAuthToken();
	}
	
	
	protected RestContext Api()
	{
		RestApi clCore = GetRestApi();
		if (!clCore)
		{
			clCore = CreateRestApi();
			if (!clCore){
				Error2("[UF] UFBaseEndpoint::Api()", "CRITICAL: Failed to create RestApi!");
				return null;
			}
			clCore.SetOption(ERestOption.ERESTOPTION_READOPERATION, 30);
			clCore.EnableDebug(false);
		}
		
		string baseUrl = EndpointBaseUrl();
		if (baseUrl == "" || baseUrl == "null"){
			Error2("[UF] UFBaseEndpoint::Api()", "BaseURL is empty or null - config not loaded?");
			return null;
		}
		
		m_Context = clCore.GetRestContext(baseUrl);
		if (!m_Context){
			Error2("[UF] UFBaseEndpoint::Api()", "GetRestContext returned null for URL: " + baseUrl);
			return null;
		}
		
		// Get fresh token and set header
		string token = AuthToken();
		if (token == "" || token == "null"){
			UFLog.Info("[WARN] Api() called with invalid token: '" + token + "' - request will likely fail");
		}
		m_Context.SetHeader(token);
		return m_Context;
	}
	
	protected void Post(string route, string jsonString, RestCallback UCBX)
	{
		RestContext ctx = Api();
		if (!ctx){
			Error2("[UF] UFBaseEndpoint::Post()", "Api() returned null - cannot make request to: " + route);
			return;
		}
		ctx.POST(UCBX, route, jsonString);
	}
	
	void UpdateAuthToken(){
		if (m_Context){
			m_Context.SetHeader(AuthToken());
		}
	}
	
}
