class ApiQueryBase : RestCallback{
	int ResultLimit;
	string Mod;
	string Query;
	
	/*
	Replace with your Results
	ref array<ref UApiConfigBase> Results;
	*/
	int Count;
	
	
	
	string ToJson(){
		// Override and Replace with your class Name
		string jsonString = JsonFileLoader<ApiQueryBase>.JsonMakeData(this);
		Print("[UAPI] Error You didn't override ToJson: " + jsonString); 
		return jsonString;
	}
	
	override void OnSuccess(string data, int dataSize) {
		//Change to your Class Name
		JsonFileLoader<ApiQueryBase>.JsonLoadData(data, this);
		if (this){
			
		} else {
			Print("[UAPI] CallBack Failed errorCode: Invalid Data");
		}
	};
	
}


class APIQueryResult<Class T> : StatusObject {
	
	autoptr array<ref T> Results;
	int Count;
	
	
	bool FromJson(string stringData) {
		return UApiJSONHandler<T>.FromString( stringData, this);
	}

}
