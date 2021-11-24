class UApiCryptoRequest extends UApiObject_Base {
	
	autoptr TStringArray From;
	
	void UApiCryptoRequest(TStringArray from){
		From.Copy(from);
	}
	
	override string ToJson(){
		string jsonString = UApiJSONHandler<UApiCryptoRequest>.ToString(this);
		return jsonString;
	}
}

class UApiCryptoConvertRequest extends UApiObject_Base {
	
	float Value = 0;
	
	void UApiCryptoConvertRequest(float value){
		Value = value;
	}
	
	override string ToJson(){
		string jsonString = UApiJSONHandler<UApiCryptoConvertRequest>.ToString(this);
		return jsonString;
	}
}

class UApiCryptoConvertResult extends StatusObject{
	float Value;
	
	float Get(){
		return Value;
	}
}

class UApiCryptoResults extends StatusObject{
	map<string,float> Values;
	float Get(string key){
		return Values.Get(key);
	}
}