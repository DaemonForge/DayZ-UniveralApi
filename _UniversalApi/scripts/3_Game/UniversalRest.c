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
			UCBX = new ref UApiSilentCallBack
		}
		RestContext ctx =  Api().GetRestContext(url);
		ctx.SetHeader("application/json");
		ctx.POST(UCBX , "", jsonString);
	}
	
	
	static string BaseUrl(){
		if (m_BaseUrl != ""){
			return m_BaseUrl;
		}
		return UApiConfig().ServerURL;
	}
	
	static void PlayerSave(string mod, string guid, string jsonString, ref RestCallback UCBX = NULL, string auth = "")
	{		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		string url = BaseUrl() + "Player/Save/" + guid + "/" + mod  + "/" + auth;
		if (!UCBX){
			UCBX = new ref UApiSilentCallBack;
		}
		
		if (jsonString){
			Post(url,jsonString,UCBX);
		} else {
			Print("[UPAI] [Api] Error Saving Player Data for " + mod);
		}
	}
	
	static void PlayerLoad(string mod, string guid,  ref RestCallback UCBX, string jsonString = "{}",string auth = "")
	{
		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		string url = BaseUrl() + "Player/Load/" + guid + "/" + mod  + "/" + auth;
		
		if (UCBX){
			Post(url,jsonString,UCBX);
		} else {
			Print("[UPAI] [Api] Error Loading Player Data for " + mod);
		}
	}
	
	static void GetAuth( string guid, string auth  = "")
	{
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		string url = BaseUrl() + "GetAuth/" + guid + "/" + auth;
		
		Post(url, "{}", new ref UApiAuthCallBack(guid));
	}


	static void GlobalsSave(string mod, string jsonString, ref RestCallback UCBX = NULL, string auth = "")
	{
		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		string url = BaseUrl() + "Gobals/Save/" + mod  + "/" + auth;
		
		if (!UCBX){
			UCBX = new ref UApiSilentCallBack;
		}
		
		if (jsonString){
			Post(url,jsonString,UCBX);
		} else {
			Print("[UPAI] [Api] Error Saving Globals Data for " + mod);
		}
	}
	
	static void GlobalsLoad(string mod, ref RestCallback UCBX, string jsonString = "{}", string auth = ""){
		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		string url = BaseUrl() + "Gobals/Load/" + mod  + "/" + auth;

		if (UCBX){
			Post(url,jsonString,UCBX);
		} else {
			Print("[UPAI] [Api] Error Loading Globals Data for " + mod);
		}
	}
	
	static void ItemSave(string mod, string itemId, string jsonString, ref RestCallback UCBX = NULL, string auth = ""){
		
		if (!UCBX){
			UCBX = new ref UApiSilentCallBack;
		}
		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		string url = BaseUrl() + "Item/Save/" + itemId + "/" +  mod + "/" + auth;
		
		if (jsonString){
			Post(url,jsonString,UCBX);
		} else {
			Print("[UPAI] [Api] Error Saving Globals Data for " + mod);
		}
	}
	
	static void ItemLoad(string mod, string itemId, ref RestCallback UCBX, string jsonString = "{}", string auth = ""){
		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		string url = BaseUrl() + "Item/Load/" +  itemId + "/" + mod  + "/" + auth;
		
		if (UCBX){
			Post(url,jsonString,UCBX);
		} else {
			Print("[UPAI] [Api] Error Loading Globals Data for " + mod);
		}
	}
	
	
	//This is just something I'm playing with maybe helpfull
	static void QnA(string question, bool alwaysAnswer = true, ref RestCallback UCBX = NULL, string jsonString = "{}", string auth = ""){
		
		if (!UCBX && alwaysAnswer){
			ref UApiQnACallBack QnACBX = new ref UApiQnACallBack;
			QnACBX.SetAlwaysAnswer();
			UCBX = QnACBX;
		} else if (!UCBX) {
			UCBX = new ref UApiQnACallBack;
		}
		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		if (jsonString == "{}" && question != "" ){
			QnAQuestion QuestionObj = new QnAQuestion(question);
			jsonString = QuestionObj.ToJson();
		}
		string url = BaseUrl() + "QnAMaker/" + auth;
		
		if (jsonString != "{}" ){
			Post(url,jsonString,UCBX);
		} else {
			Print("[UPAI] [Api] Error Asking Question ");
		}
	}
};