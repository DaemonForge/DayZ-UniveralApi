class UApiForwarder{

	string URL = "";
    ref array<ref UApiHeaders> Headers = new ref array<ref UApiHeaders>;
    string Method = "post";
    string Body = "";
    string ReturnValue = "";
    int ReturnValueArrayIndex = -1;
	
	void UApiForwarder( string url, string body = "{}", ref array<ref UApiHeaders> headers = NULL ){
		URL = url;
		if (headers == NULL){
			Headers.Insert(new ref UApiHeaders("Content-Type", "application/json"));
		}
		Body = body;
	}
	
	void AddHeader(string key, string value){
		Headers.Insert(new ref UApiHeaders(key, value));
	} 
	
	string ToJson(){
		return JsonFileLoader<UApiForwarder>.JsonMakeData(this);;
	}
}

class UApiHeaders{
	string Key;
	string Value;
	
	void UApiHeaders(string key, string value){
		Key = key;
		Value = value;
	}
	
}