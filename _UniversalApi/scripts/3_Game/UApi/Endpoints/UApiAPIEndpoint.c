class UApiAPIEndpoint extends UApiBaseEndpoint {
	
	
	//Uses the QnA Endpoint to send requests returns QnAAnswer
	int QnA(string Question,  string Key, Class instance, string function, string oid = "", bool ReturnString = false){
		int cid = UApi().CallId();
		string endpoint = "QnA/" + Key;
				
		autoptr UApiQuestionRequest questionObj = new UApiQuestionRequest(Question);
		string jsonString = questionObj.ToJson();
		
		if (Question && jsonString && ReturnString){
			Post(endpoint, jsonString, new UApiDBCallBack(instance, function, cid, oid));
		} else if (Question && jsonString){
			Post(endpoint, jsonString, new UApiDBNestedCallBack(new UApiCallback<QnAAnswer>(instance, function, oid), cid));
		} else {
			Print("[UAPI] [Api] Error QnA K:" +  Key + " Q: " + Question );
			cid = -1;
		}
		return cid;	
	}
		
	//Helper function for returning the question to chat
	int ChatQnA(string Question, bool Slient){
		if (Slient){
			return QnA(Question, "", GetDayZGame(), "CBQnAChatMessageSilent");	
		}
		return QnA(Question, "", GetDayZGame(), "CBQnAChatMessage");	
	}
	
	
	//Sends request to get text translated returns a `UApiTranslationResponse` object
	int Translate(string Text, TStringArray To, Class instance, string function, string oid = "", bool ReturnString = false){
		int cid = UApi().CallId();
		string endpoint = "Translate";
		
		autoptr RestCallback DBCBX;
		if (instance && function != "" && ReturnString){
			DBCBX = new UApiDBCallBack(instance, function, cid, oid);
		} else if (instance && function != ""){
			DBCBX = new UApiDBNestedCallBack(new UApiCallback<UApiTranslationResponse>(instance, function, oid), cid);
		}
		
		autoptr UApiTranslationRequest translationReq = new UApiTranslationRequest(Text, To);
		string jsonString = translationReq.ToJson();
		
		if ( jsonString && Text && To && DBCBX){
			Post(endpoint,jsonString,DBCBX);
		} else {
			Print("[UAPI] [Api] Error Translate " +  Text);
			cid = -1;
		}
		return cid;
		
	}
		
	
	//Runs a wit query to the key specificed must be configured server side
	int Wit(string Text, string Key, Class instance, string function, string oid = ""){
		int cid = UApi().CallId();
		string endpoint = "Wit/" + Key;
		
		autoptr RestCallback DBCBX;
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
	
	//Sends a query to Microsoft's LUIS
	int LUIS(string Text, string Key, Class instance, string function, string oid = ""){
		int cid = UApi().CallId();
		string endpoint = "LUIS/" + Key;
		
		autoptr RestCallback DBCBX;
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
	
	
	
	//Replacing ServerQuery Runs a Steam Query for a server returning a `UApiServerStatus` object
	int SteamQuery(string ip, string queryPort, Class instance, string function, string oid = "", bool ReturnString = false){
		int cid = UApi().CallId();
		string endpoint = "ServerQuery/Status/" + ip + "/" + queryPort;
		
		autoptr RestCallback DBCBX;
		if (instance && function != "" && ReturnString){
			DBCBX = new UApiDBCallBack(instance, function, cid, oid);
		} else if (instance && function != ""){
			DBCBX = new UApiDBNestedCallBack(new UApiCallback<UApiServerStatus>(instance, function, oid), cid);
		}
		
		if (  ip && ip != "" && queryPort && queryPort != "" && DBCBX){
			Post(endpoint,"{}",DBCBX);
		} else {
			Print("[UAPI] [Api] Error ServerQuery IP:" +  ip + " Port:" + queryPort);
			cid = -1;
		}
		return cid;
	}
	
	//To Be removed
	//Runs a Steam Query for a server returning a `UApiServerStatus` object
	int ServerQuery(string ip, string queryPort, Class instance, string function, string oid = ""){
		int cid = UApi().CallId();
		string endpoint = "ServerQuery/Status/" + ip + "/" + queryPort;
		
		autoptr RestCallback DBCBX;
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
	
	//Runs a Steam Query for a server returning a `UApiServerStatus` object
	int ServerQueryObj(string ip, string queryPort, Class instance, string function, string oid = ""){
		int cid = UApi().CallId();
		string endpoint = "ServerQuery/Status/" + ip + "/" + queryPort;
		
		if (  ip && ip != "" && queryPort && queryPort != "" ){
			Post(endpoint,"{}",new UApiDBNestedCallBack(new UApiCallback<UApiServerStatus>(instance, function, oid), cid));
		} else {
			Print("[UAPI] [Api] Error ServerQuery IP:" +  ip + " Port:" + queryPort);
			cid = -1;
		}
		return cid;
	}
	
	//Sends text for Toxicity Check returns a `UApiToxicityResponse` object
	int Toxicity(string text, Class instance, string function, string oid = "", bool ReturnString = false){
		int cid = UApi().CallId();
		string endpoint = "Toxicity";
		
		autoptr RestCallback DBCBX;
		if (instance && function != "" && ReturnString){
			DBCBX = new UApiDBCallBack(instance, function, cid, oid);
		} else if (instance && function != ""){
			DBCBX = new UApiDBNestedCallBack(new UApiCallback<UApiToxicityResponse>(instance, function, oid), cid);
		}
		
		autoptr UApiQuestionRequest questionreq = new UApiQuestionRequest(text);
		
		if (  text && text != "" && questionreq && DBCBX){
			Post(endpoint, questionreq.ToJson(), DBCBX);
		} else {
			Print("[UAPI] [Api] Error Toxicity Text:" +  text + " CID:" + cid);
			cid = -1;
		}
		return cid;
	}
	
	//Get a array of random numbers from 0 - 65535 returns `UApiRandomNumberResponse`
	int RandomNumbers(int count, Class instance, string function, string oid = "", bool ReturnString = false){
		int cid = UApi().CallId();
		string endpoint = "Random";
		if (count == -1){
			count = 2048;
		}
		autoptr RestCallback DBCBX;
		if (instance && function != "" && ReturnString){
			DBCBX = new UApiDBCallBack(instance, function, cid, oid);
		} else if (instance && function != ""){
			DBCBX = new UApiDBNestedCallBack(new UApiCallback<UApiRandomNumberResponse>(instance, function, oid), cid);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		autoptr UApiRandomNumberRequest randomreq = new UApiRandomNumberRequest(count);
		
		if (  count > 0 && count <= 2048 && randomreq && DBCBX){
			Post(endpoint, randomreq.ToJson(), DBCBX);
		} else {
			Print("[UAPI] [Api] Error Random " +  count + " CID:" + cid);
			cid = -1;
		}
		return cid;
	}
	
	//Gets an array of random number from  -2147483647 to 2147483647 returns `UApiRandomNumberResponse`
	int RandomNumbersFull(int count, Class instance, string function, bool ReturnString = false, string oid = ""){
		int cid = UApi().CallId();
		string endpoint = "Random/Full";
		if (count == -1){
			count = 4096;
		}
		autoptr RestCallback DBCBX;
		if (instance && function != "" && ReturnString){
			DBCBX = new UApiDBCallBack(instance, function, cid, oid);
		} else if (instance && function != "") {
			DBCBX = new UApiDBNestedCallBack(new UApiCallback<UApiRandomNumberResponse>(instance, function, oid), cid);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		autoptr UApiRandomNumberRequest randomreq = new UApiRandomNumberRequest(count);
		
		if (  count > 0 && count <= 4096 && randomreq && DBCBX){
			Post(endpoint, randomreq.ToJson(), DBCBX);
		} else {
			Print("[UAPI] [Api] Error Random " +  count + " CID:" + cid);
			cid = -1;
		}
		return cid;
	}
	
	//Request a status check from the api so you can get version number and such returns a `UApiStatus` object
	int Status(Class instance, string function, string oid = "", bool ReturnString = false){
		int cid = UApi().CallId();
		if (ReturnString){	
			Post("Status", "{}", new UApiDBCallBack(instance, function, cid, oid));
		} else {
			Post("Status", "{}",  new UApiDBNestedCallBack(new UApiCallback<UApiStatus>(instance, function, oid), cid));
		}
		return cid;
	}
}