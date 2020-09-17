class UniversalRestApi
{	
	static RestApi Api()
	{
		RestApi clCore = GetRestApi();
		if (!clCore)
		{
			clCore = CreateRestApi();
		}
		return clCore;
	}
	
	static void PlayerSave(string mod, string guid, string auth, void data_out, RestCallback UCBX = new UApiSilentCallBack)
	{		
		string url = UApiConfig().ServerURL + "/Player/Save/" + guid + "/" + auth + "/" + mod;
		
		string jsonString = "{}";
		bool ok = false;
		if (data_out){
			JsonSerializer js = new JsonSerializer();
			ok = js.WriteToString(data_out, false, jsonString);
		}
		if (ok){
			RestContext ctx = Api().GetRestContext(url);
			ctx.SetHeader("application/json");
			ctx.POST(UCBX, "", json);
		} else {
			Print("[GameApi] Error Saving Player Data for " + mod);
		}
	}
	
	static void PlayerLoad(string mod, string guid, string auth, RestCallback UCBX, void data_out = NULL)
	{
		string url = UApiConfig().ServerURL + "/Player/Load/" + guid + "/" + auth + "/" + mod;
		
		string jsonString = "{}";
		bool ok = true;
		if (data_out){
			JsonSerializer js = new JsonSerializer();
			ok = js.WriteToString(data_out, false, jsonString);
		}
		
		if (ok){
			RestContext ctx = Api().GetRestContext(url);
			ctx.SetHeader("application/json");
			ctx.POST(UCBX, "", jsonString);
		} else {
			Print("[GameApi] Error Loading Player Data for " + mod);
		}
	}
	
	static void GetAuth( string auth, string guid )
	{
		UApiAuthCallBack UCBX = new UApiAuthCallBack
		string url = UApiConfig().ServerURL + "/Player/GetAuth/" + guid + "/" + auth;
		RestContext ctx = Api().GetRestContext(url);
		ctx.SetHeader("application/json");
		ctx.POST(UCBX, "", "{}");
	}


	static void GlobalsSave(string mod, string auth, void data_out, RestCallback UCBX = new UApiSilentCallBack)
	{
		string url = UApiConfig().ServerURL + "/Gobals/Save/" + auth + "/" + mod;
		
		string jsonString = "{}";
		bool ok = false;
		if (data_out){
			JsonSerializer js = new JsonSerializer();
			ok = js.WriteToString(data_out, false, jsonString);
		}
		if (ok){
			RestContext ctx = Api().GetRestContext(url);
			ctx.SetHeader("application/json");
			ctx.POST(UCBX, "", json);
		} else {
			Print("[GameApi] Error Saving Globals Data for " + mod);
		}
	}
	
	static void GlobalsLoad(string mod, string auth, void data_out = NULL, RestCallback UCBX)
	{
		string url = UApiConfig().ServerURL + "/Gobals/Load/" + auth + "/" + mod;
		
		string jsonString = "{}";
		bool ok = true;
		if (data_out){
			JsonSerializer js = new JsonSerializer();
			ok = js.WriteToString(data_out, false, jsonString);
		}
		
		if (ok){
			RestContext ctx = Api().GetRestContext(url);
			ctx.SetHeader("application/json");
			ctx.POST(UCBX, "", jsonString);
		} else {
			Print("[GameApi] Error Loading Globals Data for " + mod);
		}
	}
	
	static void ServerSave(string mod, string serverId, string auth, void data_out, RestCallback UCBX = new UApiSilentCallBack)
	{
		string url = UApiConfig().ServerURL + "/Server/Save/" + serverId + "/" + auth + "/" + mod;
		
		string jsonString = "{}";
		bool ok = false;
		if (data_out){
			JsonSerializer js = new JsonSerializer();
			ok = js.WriteToString(data_out, false, jsonString);
		}
		if (ok){
			RestContext ctx = Api().GetRestContext(url);
			ctx.SetHeader("application/json");
			ctx.POST(UCBX, "", json);
		} else {
			Print("[GameApi] Error Saving Globals Data for " + mod);
		}
	}
	
	static void ServerLoad(string mod, string serverId, string auth, RestCallback UCBX, void data_out = NULL)
	{
		string url = UApiConfig().ServerURL + "/Server/Load/" + serverId + "/" + auth + "/" + mod;
		
		string jsonString = "{}";
		bool ok = true;
		if (data_out){
			JsonSerializer js = new JsonSerializer();
			ok = js.WriteToString(data_out, false, jsonString);
		}
		
		if (ok){
			RestContext ctx = Api().GetRestContext(url);
			ctx.SetHeader("application/json");
			ctx.POST(UCBX, "", jsonString);
		} else {
			Print("[GameApi] Error Loading Globals Data for " + mod);
		}
	}
};