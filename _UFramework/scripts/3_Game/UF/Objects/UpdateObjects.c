class UUpdateData extends UFObject_Base {
	string Element;
	string Operation = UpdateOpts.SET; // set | push | pull | unset | mul | rename | pullAll
	string Value;
	
	void UUpdateData(string element, string value, string operation = UpdateOpts.SET){
		Element = element;
		Value = value;
		Operation = operation;
	}
	
	override string ToJson(){
		string jsonString = UJSONHandler<UUpdateData>.ToString(this);
		return jsonString;
	}
	
};
class UDBQueryUpdateResponse extends StatusObject {
	string Element;
	string Mod;
	int Count;
}

class UDBUpdateResponse extends UDBTransactionResponse {
	float Value;
}

class UDBTransactionResponse extends StatusObject {
	string ID;
	string Element;
	string Mod;
}