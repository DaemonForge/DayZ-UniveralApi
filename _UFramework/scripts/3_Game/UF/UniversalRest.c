class UniversalRest extends Managed
{		
	protected static RestApi Api()
	{
		RestApi clCore = GetRestApi();
		if (!clCore)
		{
			clCore = CreateRestApi();
			clCore.SetOption(ERestOption.ERESTOPTION_READOPERATION, 15);
		}
		return clCore;
	}
	
	protected static void Post(string url, string jsonString = "{}", RestCallback UCBX = NULL)
	{
		autoptr RestCallback vUCBX = UCBX;
		if (!vUCBX){
			vUCBX = new USilentCallBack;
		}
		RestContext ctx =  Api().GetRestContext(url);
		ctx.SetHeader(U().GetAuthToken());
		ctx.POST(vUCBX , "", jsonString);
	}
	
	protected static void Get(string url, RestCallback UCBX = NULL)
	{
		autoptr RestCallback vUCBX = UCBX;
		if (!vUCBX){
			vUCBX = new USilentCallBack;
		}
		RestContext ctx =  Api().GetRestContext(url);
		ctx.GET(vUCBX , "");
	}
	
	protected static string BaseUrl(){
		return UFConfig().ServerURL;
	}
	
	static void GetAuth( string guid ){
		string url = BaseUrl() + "GetAuth/" + guid;
		
		Post(url, "{}", new UAuthCallBack(guid));
	}
	
	static void GlobalsSave(string mod, string jsonString, RestCallback UCBX = NULL) {

		string url = BaseUrl() + "Globals/Save/" + mod;
		autoptr RestCallback vUCBX = UCBX;
		if (!vUCBX){
			vUCBX = new USilentCallBack;
		}
				
		if (jsonString){
			Post(url,jsonString,vUCBX);
		} else {
			Print("[UF] [Api] Error Saving Globals Data for " + mod);
		}
	}
	
	static void GlobalsLoad(string mod, RestCallback UCBX, string jsonString = "{}") {

		string url = BaseUrl() + "Globals/Load/" + mod;
		autoptr RestCallback vUCBX = UCBX;
		if (!vUCBX){
			vUCBX = new USilentCallBack;
		}

		if (vUCBX){
			Post(url,jsonString,vUCBX);
		} else {
			Print("[UF] [Api] Error Loading Globals Data for " + mod);
		}
	}
	
	
	static void GlobalsIncrement(string mod, string element, float value = 1){
		GlobalsTransaction(mod, element, value, NULL);
	}
	
	static void GlobalsTransaction(string mod, string element, float value = 1, RestCallback UCBX = NULL) {
		
		autoptr RestCallback vUCBX = UCBX;
		if (!vUCBX){
			vUCBX = new UDBTransactionCallBack;
		}
		string url = BaseUrl() + "Globals/Transaction/" + mod;

		
		autoptr UDBTransaction transaction = new UDBTransaction(element, value);
		
		if ( element && transaction && vUCBX){
			Post(url,transaction.ToJson(),vUCBX);
		} else {
			Print("[UF] [Api] Error Transaction " +  mod);
		}
	}
	
	//String Values must be wrapped with Quotes example string newValue = "\"NewValue\""
	static void GlobalsUpdate(string mod, string element, string value, RestCallback UCBX = NULL) {
		
		autoptr RestCallback vUCBX = UCBX;
		if (!vUCBX){
			vUCBX = new USilentCallBack;
		}

		string url = BaseUrl() + "Globals/Update/" + mod;

		
		autoptr UUpdateData updatedata = new UUpdateData(element, value);
		
		if ( element && updatedata && vUCBX){
			Post(url,updatedata.ToJson(),vUCBX);
		} else {
			Print("[UF] [Api] Error Transaction " +  mod);
		}
	}
	
	//String Values must be wrapped with Quotes example string newValue = "\"NewValue\""
	static void GlobalsUpdateAdv(string mod, string element, string value, string operation, RestCallback UCBX = NULL) {
		
		autoptr RestCallback vUCBX = UCBX;
		if (!vUCBX){
			vUCBX = new USilentCallBack;
		}

		string url = BaseUrl() + "Globals/Update/" + mod;
		
		autoptr UUpdateData updatedata = new UUpdateData(element, value, operation);
		
		if ( element && updatedata && vUCBX){
			Post(url,updatedata.ToJson(),vUCBX);
		} else {
			Print("[UF] [Api] Error Transaction " +  mod);
		}
	}
	
	static void Request(UApiForwarder data, RestCallback UCBX = NULL){
				
		autoptr RestCallback vUCBX = UCBX;
		if (!vUCBX){
			vUCBX = new USilentCallBack;
		}
		
		
		string url = BaseUrl() + "Forward";
		
		if ( data && vUCBX){
			Post(url,data.ToJson(),vUCBX);
		} else {
			Print("[UF] [Api] Error Fowarding ");
		}
	}

	static void Log(string jsonString, RestCallback UCBX = NULL){
		
		autoptr RestCallback vUCBX = UCBX;
		if (!vUCBX){
			vUCBX = new USilentCallBack;
		}
		
		string url = BaseUrl() + "Logger/One/" + UFConfig().ServerID;
		
		if ( jsonString && vUCBX){
			Post(url,jsonString,vUCBX);
		} else {
			Print("[UF] [Api] Error Fowarding ");
		}
	}
	
	//JsonFileLoader<array<autoptr LogObject>>.JsonMakeData(AnArrayOfYourObjects);
	static void LogBulk(string jsonString, RestCallback UCBX = NULL){
		
		autoptr RestCallback vUCBX = UCBX;
		if (!vUCBX){
			vUCBX = new USilentCallBack;
		}
		
		string url = BaseUrl() + "Logger/Many/" + UFConfig().ServerID;
		if (jsonString && vUCBX){
			Post(url,jsonString,vUCBX);
		} else {
			Print("[UF] [Api] Error Fowarding ");
		}
	}	
	
};