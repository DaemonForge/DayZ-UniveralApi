class SimpleValueStore extends Managed {
	string Value = "";
	void SimpleValueStore(string value){
		Value = value;
	}
	string ToJson(){
		string jsonString = UApiJSONHandler<SimpleValueStore>.ToString(this);
		return jsonString;
	}
	
	static string StoreValue(string value){
		autoptr SimpleValueStore obj = new SimpleValueStore(value);
		return obj.ToJson();
	}
	
	static string GetValue(string json){
		autoptr SimpleValueStore obj;
		if (UApiJSONHandler<SimpleValueStore>.FromString(json, obj)){
			return obj.Value;
		}
		return "";
	}
	
	static string GenerateJson(string data){
		return StoreValue(data);
	}
	
	static string UseJson(string jsonData){
		return GetValue(jsonData);
	}
}