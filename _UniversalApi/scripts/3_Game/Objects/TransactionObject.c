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