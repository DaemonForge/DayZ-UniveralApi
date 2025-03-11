class UDBTransaction extends UFObject_Base {
	string Element;
	float Value;
	
	void UDBTransaction(string element, float value){
		Element = element;
		Value = value;
	}
	
	override string ToJson(){
		string jsonString = JsonFileLoader<UDBTransaction>.JsonMakeData(this);
		return jsonString;
	}
	
};

class UDBValidatedTransaction extends UFObject_Base {
	string Element;
	float Value;
	float Min;
	float Max;
	
	void UDBValidatedTransaction(string element, float value, float min, float max){
		Element = element;
		Value = value;
		Min = min;
		Max = max;
	}
	
	override string ToJson(){
		string jsonString = JsonFileLoader<UDBValidatedTransaction>.JsonMakeData(this);
		return jsonString;
	}
	
};
