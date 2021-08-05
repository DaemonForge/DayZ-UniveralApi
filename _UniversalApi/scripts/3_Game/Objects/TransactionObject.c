class UApiTransaction extends UApiObject_Base {
	string Element;
	float Value;
	
	void UApiTransaction(string element, float value){
		Element = element;
		Value = value;
	}
	
	override string ToJson(){
		string jsonString = JsonFileLoader<UApiTransaction>.JsonMakeData(this);
		return jsonString;
	}
	
};

class UApiValidatedTransaction extends UApiObject_Base {
	string Element;
	float Value;
	float Min;
	float Max;
	
	void UApiValidatedTransaction(string element, float value, float min, float max){
		Element = element;
		Value = value;
		Min = min;
		Max = max;
	}
	
	override string ToJson(){
		string jsonString = JsonFileLoader<UApiValidatedTransaction>.JsonMakeData(this);
		return jsonString;
	}
	
};
