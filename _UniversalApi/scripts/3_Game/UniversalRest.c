class UniversalRest
{	
	static protected string m_BaseUrl = "";
	
	static void SetBaseUrl(string new_BaseUrl){
		m_BaseUrl = new_BaseUrl;
	}
	
	static RestApi Api()
	{
		RestApi clCore = GetRestApi();
		if (!clCore)
		{
			clCore = CreateRestApi();
			clCore.SetOption(ERestOption.ERESTOPTION_READOPERATION, 15);
		}
		return clCore;
	}
	
	static void Post(string url, string jsonString = "{}", ref RestCallback UCBX = NULL)
	{
		if (!UCBX){
			UCBX = new UApiSilentCallBack;
		}
		RestContext ctx =  Api().GetRestContext(url);
		ctx.SetHeader("application/json");
		ctx.POST(UCBX , "", jsonString);
	}
	
	static void Get(string url, ref RestCallback UCBX = NULL)
	{
		if (!UCBX){
			UCBX = new UApiSilentCallBack;
		}
		RestContext ctx =  Api().GetRestContext(url);
		ctx.GET(UCBX , "");
	}
	
	static string BaseUrl(){
		if (m_BaseUrl != ""){
			return m_BaseUrl;
		}
		return UApiConfig().ServerURL;
	}
	
	static void GetAuth( string guid, string auth  = ""){
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		string url = BaseUrl() + "GetAuth/" + guid + "/" + auth;
		
		Post(url, "{}", new UApiAuthCallBack(guid));
	}
	
	static void PlayerSave(string mod, string guid, string jsonString, ref RestCallback UCBX = NULL, string auth = "") {		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		string url = BaseUrl() + "Player/Save/" + guid + "/" + mod  + "/" + auth;
		if (!UCBX){
			UCBX = new UApiSilentCallBack;
		}
		
		if (jsonString){
			Post(url,jsonString,UCBX);
		} else {
			Print("[UAPI] [Api] Error Saving Player Data for " + mod);
		}
	}
	
	static void PlayerLoad(string mod, string guid,  ref RestCallback UCBX, string jsonString = "{}",string auth = "") {
		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		string url = BaseUrl() + "Player/Load/" + guid + "/" + mod  + "/" + auth;
		
		if (UCBX){
			Post(url,jsonString,UCBX);
		} else {
			Print("[UAPI] [Api] Error Loading Player Data for " + mod);
		}
	}
	
	static void PlayerQuery(string mod, UApiQueryObject query, ref RestCallback UCBX, string auth = "") {
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		string url = BaseUrl() + "Player/Query/" + mod  + "/" + auth;
		
		if ( query && UCBX){
			Post(url,query.ToJson(),UCBX);
		} else {
			Print("[UAPI] [Api] Error Querying " +  mod);
		}
	}
	
	static void PlayerIncrement(string mod, string guid, string element, float value = 1){
		PlayerTransaction(mod, guid, element, value, NULL, "");
	}
	
	static void PlayerTransaction(string mod, string guid, string element, float value = 1, ref RestCallback UCBX = NULL, string auth = "") {
		
		if (!UCBX){
			UCBX = new UApiTransactionCallBack;
		}
		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		
		string url = BaseUrl() + "Player/Transaction/" + guid   + "/"+ mod + "/" + auth;
		
		ref UApiTransaction transaction = new UApiTransaction(element, value);
		
		if ( element && transaction && UCBX){
			Post(url,transaction.ToJson(),UCBX);
		} else {
			Print("[UAPI] [Api] Error Transaction " +  mod);
		}
	}
	
	//String Values must be wrapped with Quotes example string newValue = "\"NewValue\""
	static void PlayerUpdate(string mod, string guid, string element, string value, ref RestCallback UCBX = NULL, string auth = "") {
		
		if (!UCBX){
			UCBX = new UApiSilentCallBack;
		}
		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		
		string url = BaseUrl() + "Player/Update/" + guid   + "/"+ mod + "/" + auth;
		
		ref UApiUpdateData updatedata = new UApiUpdateData(element, value);
		
		if ( element && updatedata && UCBX){
			Post(url,updatedata.ToJson(),UCBX);
		} else {
			Print("[UAPI] [Api] Error Transaction " +  mod);
		}
	}
	
	//String Values must be wrapped with Quotes example string newValue = "\"NewValue\""
	static void PlayerUpdateAdv(string mod, string guid, string element, string value, string operation, ref RestCallback UCBX = NULL, string auth = "") {
		
		if (!UCBX){
			UCBX = new UApiSilentCallBack;
		}
		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		
		string url = BaseUrl() + "Player/Update/" + guid   + "/"+ mod + "/" + auth;
		
		ref UApiUpdateData updatedata = new UApiUpdateData(element, value, operation);
		
		if ( element && updatedata && UCBX){
			Post(url,updatedata.ToJson(),UCBX);
		} else {
			Print("[UAPI] [Api] Error Transaction " +  mod);
		}
	}
	
	static void GlobalsSave(string mod, string jsonString, ref RestCallback UCBX = NULL, string auth = "") {
		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		string url = BaseUrl() + "Gobals/Save/" + mod  + "/" + auth;
		
		if (!UCBX){
			UCBX = new UApiSilentCallBack;
		}
		
		if (jsonString){
			Post(url,jsonString,UCBX);
		} else {
			Print("[UAPI] [Api] Error Saving Globals Data for " + mod);
		}
	}
	
	static void GlobalsLoad(string mod, ref RestCallback UCBX, string jsonString = "{}", string auth = "") {
		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		string url = BaseUrl() + "Gobals/Load/" + mod  + "/" + auth;

		if (UCBX){
			Post(url,jsonString,UCBX);
		} else {
			Print("[UAPI] [Api] Error Loading Globals Data for " + mod);
		}
	}
	
	
	static void GlobalsIncrement(string mod, string element, float value = 1){
		GlobalsTransaction(mod, element, value, NULL, "");
	}
	
	static void GlobalsTransaction(string mod, string element, float value = 1, ref RestCallback UCBX = NULL, string auth = "") {
		
		if (!UCBX){
			UCBX = new UApiTransactionCallBack;
		}
		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		
		string url = BaseUrl() + "Gobals/Transaction/" + mod  + "/" + auth;
		
		ref UApiTransaction transaction = new UApiTransaction(element, value);
		
		if ( element && transaction && UCBX){
			Post(url,transaction.ToJson(),UCBX);
		} else {
			Print("[UAPI] [Api] Error Transaction " +  mod);
		}
	}
	
	//String Values must be wrapped with Quotes example string newValue = "\"NewValue\""
	static void GlobalsUpdate(string mod, string element, string value, ref RestCallback UCBX = NULL, string auth = "") {
		
		if (!UCBX){
			UCBX = new UApiSilentCallBack;
		}
		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		
		string url = BaseUrl() + "Gobals/Update/" + mod + "/" + auth;
		
		ref UApiUpdateData updatedata = new UApiUpdateData(element, value);
		
		if ( element && updatedata && UCBX){
			Post(url,updatedata.ToJson(),UCBX);
		} else {
			Print("[UAPI] [Api] Error Transaction " +  mod);
		}
	}
	
	//String Values must be wrapped with Quotes example string newValue = "\"NewValue\""
	static void GlobalsUpdateAdv(string mod, string element, string value, string operation, ref RestCallback UCBX = NULL, string auth = "") {
		
		if (!UCBX){
			UCBX = new UApiSilentCallBack;
		}
		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		
		string url = BaseUrl() + "Gobals/Update/" + mod + "/" + auth;
		
		ref UApiUpdateData updatedata = new UApiUpdateData(element, value, operation);
		
		if ( element && updatedata && UCBX){
			Post(url,updatedata.ToJson(),UCBX);
		} else {
			Print("[UAPI] [Api] Error Transaction " +  mod);
		}
	}
	
	
	//Saving or loading an object with the ObjectId of "NewObject" will generate an Object ID for you, this Object ID will be returned
	//in the ObjectId var of the Class so make sure your Class has the varible ObjectId if you plan on using this feature
	static void ObjectSave(string mod, string objectId, string jsonString, ref RestCallback UCBX = NULL, string auth = "") {
		
		if (!UCBX){
			UCBX = new UApiSilentCallBack;
		}
		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		string url = BaseUrl() + "Object/Save/" + objectId + "/" +  mod + "/" + auth;
		
		if (jsonString){
			Post(url,jsonString,UCBX);
		} else {
			Print("[UAPI] [Api] Error Saving Object " + objectId + " Data for " + mod);
		}
	}
	
	static void ObjectLoad(string mod, string objectId, ref RestCallback UCBX, string jsonString = "{}", string auth = "") {
		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		string url = BaseUrl() + "Object/Load/" +  objectId + "/" + mod  + "/" + auth;
		
		if (UCBX){
			Post(url,jsonString,UCBX);
		} else {
			Print("[UAPI] [Api] Error Loading Object (" + objectId + ") Data for " + mod);
		}
	}
	
	static void ObjectQuery(string mod, UApiQueryObject query, ref RestCallback UCBX, string auth = "") {
		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		string url = BaseUrl() + "Object/Query/" + mod  + "/" + auth;
		
		if ( query && UCBX){
			Post(url,query.ToJson(),UCBX);
		} else {
			Print("[UAPI] [Api] Error Querying " +  mod);
		}
	}
	
	static void ObjectIncrement(string mod, string objectId, string element, float value = 1){
		ObjectTransaction(mod, objectId, element, value, NULL, "");
	}
	
	static void ObjectTransaction(string mod, string objectId, string element, float value = 1, ref RestCallback UCBX = NULL, string auth = "") {
		
		if (!UCBX){
			UCBX = new UApiTransactionCallBack;
		}
		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		
		string url = BaseUrl() + "Object/Transaction/" + objectId + "/"+ mod + "/" + auth;
		
		
		ref UApiTransaction transaction = new UApiTransaction(element, value);
		
		if ( element && transaction && UCBX){
			Post(url,transaction.ToJson(),UCBX);
		} else {
			Print("[UAPI] [Api] Error Transaction " +  mod);
		}
	}
	
	//String Values must be wrapped with Quotes example string newValue = "\"NewValue\""
	static void ObjectUpdate(string mod, string guid, string element, string value, ref RestCallback UCBX = NULL, string auth = "") {
		
		if (!UCBX){
			UCBX = new UApiSilentCallBack;
		}
		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		
		string url = BaseUrl() + "Object/Update/" + guid  + "/"+ mod + "/" + auth;
		
		ref UApiUpdateData updatedata = new UApiUpdateData(element, value);
		
		if ( element && updatedata && UCBX){
			Post(url,updatedata.ToJson(),UCBX);
		} else {
			Print("[UAPI] [Api] Error Transaction " +  mod);
		}
	}
	
	//String Values must be wrapped with Quotes example string newValue = "\"NewValue\""
	static void ObjectUpdateAdv(string mod, string guid, string element, string value, string operation = "set", ref RestCallback UCBX = NULL, string auth = "") {
		
		if (!UCBX){
			UCBX = new UApiSilentCallBack;
		}
		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		
		string url = BaseUrl() + "Object/Update/" + guid  + "/" + mod + "/" + auth;
		
		ref UApiUpdateData updatedata = new UApiUpdateData(element, value, operation);
		
		if ( element && updatedata && UCBX){
			Post(url,updatedata.ToJson(),UCBX);
		} else {
			Print("[UAPI] [Api] Error Transaction " +  mod);
		}
	}
	
	static void Request(ref UApiForwarder data, ref RestCallback UCBX = NULL, string auth = ""){
		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		if (!UCBX){
			UCBX = new UApiSilentCallBack;
		}
		
		string url = BaseUrl() + "Forward/" + auth;
		
		if ( data && UCBX){
			Post(url,data.ToJson(),UCBX);
		} else {
			Print("[UAPI] [Api] Error Fowarding ");
		}
	}

	static void Log(string jsonString, ref RestCallback UCBX = NULL, string auth = ""){
		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		if (!UCBX){
			UCBX = new UApiSilentCallBack;
		}
		
		string url = BaseUrl() + "Logger/One/" + UApiConfig().ServerID + "/"+ auth;
		
		if ( jsonString && UCBX){
			Post(url,jsonString,UCBX);
		} else {
			Print("[UAPI] [Api] Error Fowarding ");
		}
	}
	
	//JsonFileLoader<array<ref LogObject>>.JsonMakeData(AnArrayOfYourObjects);
	static void LogBulk(string jsonString, ref RestCallback UCBX = NULL, string auth = ""){
		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		if (!UCBX){
			UCBX = new UApiSilentCallBack;
		}
		
		string url = BaseUrl() + "Logger/Many/" + UApiConfig().ServerID + "/"+ auth;
		if (jsonString && UCBX){
			Post(url,jsonString,UCBX);
		} else {
			Print("[UAPI] [Api] Error Fowarding ");
		}
	}	
	
};