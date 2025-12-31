class ApiQueryBase : RestCallback{
	int ResultLimit;
	string Mod;
	string Query;
	
	/*
	Replace with your Results
	autoptr array<autoptr UFConfigBase> Results;
	*/
	int Count;
	
	
	
	string ToJson(){
		// Override and Replace with your class Name
		string jsonString = JsonFileLoader<ApiQueryBase>.JsonMakeData(this);
		UFLog.Err("Error You didn't override ToJson: " + jsonString); 
		return jsonString;
	}
	
	override void OnSuccess(string data, int dataSize) {
		//Change to your Class Name
		JsonFileLoader<ApiQueryBase>.JsonLoadData(data, this);
		if (this){
			
		} else {
			UFLog.Err("CallBack Failed errorCode: Invalid Data");
		}
	};
	
}
