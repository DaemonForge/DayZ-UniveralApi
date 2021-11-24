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
		string jsonString = UApiJSONHandler<UApiUpdateData>.ToString(this);
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