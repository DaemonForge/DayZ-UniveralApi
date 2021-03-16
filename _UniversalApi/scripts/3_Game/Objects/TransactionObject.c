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
	string Operation = "set"; // set | push | pull | unset | mul | rename | pullAll
	string Value;
	
	void UApiUpdateData(string element, string value, string operation = "set"){
		Element = element;
		Value = value;
		Operation = operation;
	}
	
	string ToJson(){
		string jsonString = JsonFileLoader<UApiUpdateData>.JsonMakeData(this);;
		return jsonString;
	}
	
};