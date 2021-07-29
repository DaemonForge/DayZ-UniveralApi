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

class UApiUpdateData extends UApiObject_Base {
	string Element;
	string Operation = UpdateOpts.SET; // set | push | pull | unset | mul | rename | pullAll
	string Value;
	
	void UApiUpdateData(string element, string value, string operation = UpdateOpts.SET){
		Element = element;
		Value = value;
		Operation = operation;
	}
	
	override string ToJson(){
		string jsonString = JsonFileLoader<UApiUpdateData>.JsonMakeData(this);
		return jsonString;
	}
	
};

class UApiUpdateResponse extends UApiTransactionResponse {
	float Value;
}

class UApiTransactionResponse extends StatusObject {
	string ID;
	string Element;
	string Mod;
}