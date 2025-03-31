/**
 * File: UFMsgEndpoint.c
 * Description: The class for calling against the Messages Queuing Endpoing
 */

class UFMsgEndpoint extends UFBaseEndpoint {
    
	override protected string EndpointBaseUrl(){
		return UFConfig().GetBaseURL() + "Messages/";
	}
		
	
	int Read(string mod, string queue, UFCallbackBase cb){
		return this.Read(mod, queue, -1, cb);
	}
	
	int Read(string mod, string queue, int limit, UFCallbackBase cb){
		if (mod == "" || queue == ""){
			Error2("[UF] Message Queue Read","mod, and queue must be valid strings");
			return -1;
		}
		if (!cb){
			Error2("[UF]Message Queue Read","Callback NULL");
			return -1;
		}
		int cid = -1;	
		string endpoint = "Read/" + mod + "/" + queue;
		autoptr UMsgReadObj obj = new UMsgReadObj(limit);
		Post(endpoint, obj.ToJson(), U().RegisterCall(new UNestedCallBack(cb), cid));
		
		if (cid == -1){
			Error2("[UF] Message Queue Read","Error Registering Callback");
		}
		return cid;
		
	}
	
	int Write(string mod, string queue, string jsonString, UFCallbackBase cb = NULL){
		if (mod == "" || queue == "" || jsonString == ""){
			Error2("[UF] Message Queue Write","mod, queue & jsonString must be valid string");
			return -1;
		}
		string endpoint = "Write/" + mod + "/" + queue;
		int cid = -1;
		if (cb){
			Post(endpoint, jsonString, U().RegisterCall(new UNestedCallBack(cb), cid));
		} else {
			Post(endpoint, jsonString, U().RegisterCall(new USilentCallBack(), cid));
		}
		
		if (cid == -1){
			Error2("[UF] Message Queue Write","Error Registering Callback");
		}
		return cid;
	}
	
	int Reset(string mod, string queue, UFCallbackBase cb = NULL){
		if (mod == "" || queue == ""){
			Error2("[UF] Message Queue SetMeta","mod & queue must be valid string");
			return -1;
		}
		string endpoint = "Reset/" + mod + "/" + queue;
		int cid = -1;
		if (cb){
			Post(endpoint, "{}", U().RegisterCall(new UNestedCallBack(cb), cid));
		} else {
			Post(endpoint, "{}", U().RegisterCall(new USilentCallBack(), cid));
		}
		
		if (cid == -1){
			Error2("[UF] Message Queue SetMeta","Error Registering Callback");
		}
		return cid;
		
	}
	
	int SetMeta(string mod, string queue, UQueueMeta obj, UFCallbackBase cb = NULL){
		if (mod == "" || queue == ""){
			Error2("[UF] Message Queue SetMeta","mod & queue must be valid string");
			return -1;
		}
		if (!obj){
			Error2("[UF]Message Queue SetMeta","UQueueMeta NULL");
			return -1;
		}
		string endpoint = "Meta/" + mod + "/" + queue;
		int cid = -1;
		if (cb){
			Post(endpoint, obj.ToJson(), U().RegisterCall(new UNestedCallBack(cb), cid));
		} else {
			Post(endpoint, obj.ToJson(), U().RegisterCall(new USilentCallBack(), cid));
		}
		
		if (cid == -1){
			Error2("[UF] Message Queue SetMeta","Error Registering Callback");
		}
		return cid;
	}
 
}
