class UApiAPIEndpoint extends Managed {
	
	static protected string m_BaseUrl = "";
	
	protected RestContext m_Context;
	
	
	static void SetBaseUrl(string new_BaseUrl){
		m_BaseUrl = new_BaseUrl;
	}
	
	static protected string BaseUrl(){
		if (m_BaseUrl != ""){
			return m_BaseUrl;
		}
		return UApiConfig().ServerURL;
	}

	static protected string AuthToken(){
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
			string url = BaseUrl();
			m_Context =  clCore.GetRestContext(url);
			m_Context.SetHeader("application/json");
		}
		return m_Context;
	}
	
	protected void Post(string endpoint, string jsonString, RestCallback UCBX)
	{
		string route = endpoint + "/" + AuthToken();
		Api().POST(UCBX, route, jsonString);
	}
	
	
	int QnA(string Question,  string Key, Class instance, string function, string oid = ""){
		int cid = UApi().CallId();
		string endpoint = "QnA/" + Key;
		
		ref RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, oid);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		autoptr UApiQuestionRequest questionObj = new UApiQuestionRequest(Question);
		string jsonString = questionObj.ToJson();
		
		if ( jsonString && Key && DBCBX){
			Post(endpoint,jsonString,DBCBX);
		} else {
			Print("[UAPI] [Api] Error QnA K:" +  Key + " Q: " + Question );
			cid = -1;
		}
		return cid;
		
	}
	
	
	int Translate(string Text, TStringArray To, Class instance, string function, string oid = ""){
		int cid = UApi().CallId();
		string endpoint = "Translate";
		
		ref RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, oid);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		autoptr UApiTranslationRequest translationReq = new UApiTranslationRequest(Text, To);
		string jsonString = translationReq.ToJson();
		
		if ( jsonString && Text && DBCBX){
			Post(endpoint,jsonString,DBCBX);
		} else {
			Print("[UAPI] [Api] Error Translate " +  Text);
			cid = -1;
		}
		return cid;
		
	}
	
	int Wit(string Text, string Key, Class instance, string function, string oid = ""){
		int cid = UApi().CallId();
		string endpoint = "Wit/" + Key;
		
		ref RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, oid);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		autoptr UApiQuestionRequest questionreq = new UApiQuestionRequest(Text);
		string jsonString = questionreq.ToJson();
		
		if ( jsonString && Text && Text != "" && Key && Key != "" && DBCBX){
			Post(endpoint,jsonString,DBCBX);
		} else {
			Print("[UAPI] [Api] Error Wit K:" +  Key + " T:" + Text);
			cid = -1;
		}
		return cid;
	}
	
	int LUIS(string Text, string Key, Class instance, string function, string oid = ""){
		int cid = UApi().CallId();
		string endpoint = "LUIS/" + Key;
		
		ref RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, oid);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		autoptr UApiQuestionRequest questionreq = new UApiQuestionRequest(Text);
		string jsonString = questionreq.ToJson();
		
		if ( jsonString && Text && Text != "" && Key && Key != "" && DBCBX){
			Post(endpoint,jsonString,DBCBX);
		} else {
			Print("[UAPI] [Api] Error LUIS K:" +  Key + " T:" + Text);
			cid = -1;
		}
		return cid;
	}
	
	
	int ServerQuery(string ip, string queryPort, Class instance, string function, string oid = ""){
		int cid = UApi().CallId();
		string endpoint = "ServerQuery/Status/" + ip + "/" + queryPort;
		
		ref RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, oid);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		
		if (  ip && ip != "" && queryPort && queryPort != "" && DBCBX){
			Post(endpoint,"{}",DBCBX);
		} else {
			Print("[UAPI] [Api] Error ServerQuery IP:" +  ip + " Port:" + queryPort);
			cid = -1;
		}
		return cid;
	}
}