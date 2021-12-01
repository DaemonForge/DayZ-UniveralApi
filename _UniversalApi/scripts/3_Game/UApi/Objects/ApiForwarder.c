class UApiForwarder extends Managed{

	string URL = "";
    autoptr array<autoptr UApiHeaders> Headers = new array<autoptr UApiHeaders>;
    string Method = "post";
    string Body = "";
    string ReturnValue = "";
    int ReturnValueArrayIndex = -1;
	
	void UApiForwarder( string url, string body = "{}", autoptr array<autoptr UApiHeaders> headers = NULL ){
		URL = url;
		if (headers == NULL){
			Headers.Insert(new UApiHeaders("Content-Type", "application/json"));
		}
		Body = body;
	}
	
	void AddHeader(string key, string value){
		Headers.Insert(new UApiHeaders(key, value));
	} 
	
	string ToJson(){
		return UApiJSONHandler<UApiForwarder>.ToString(this);;
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