class ApiQueryBase : RestCallback{
	int ResultLimit;
	string Mod;
	string Query;
	
	/*
	Replace with your Results
	autoptr array<autoptr UApiConfigBase> Results;
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
