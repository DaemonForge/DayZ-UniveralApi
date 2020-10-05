class UApiTransaction
{
	string Element;
	float Value;
	
	void UApiTransaction(string element, float value){
		Element = element;
		Value = value;
	}
	
	string ToJson(){
		string jsonString = JsonFileLoader<UApiTransaction>.JsonMakeData(this);;
		return jsonString;
	}
	
};

class UApiUpdateData
{
	string Element;
	string Value;
	
	void UApiUpdateData(string element, string value){
		Element = element;
		Value = value;
	}
	
	string ToJson(){
		string jsonString = JsonFileLoader<UApiUpdateData>.JsonMakeData(this);;
		return jsonString;
	}
	
};