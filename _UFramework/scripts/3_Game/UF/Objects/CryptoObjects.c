class UCryptoRequest extends UFObject_Base {
	
	autoptr TStringArray From = new TStringArray;
	
	void UCryptoRequest(TStringArray from){
		From.Copy(from);
	}
	
	override string ToJson(){
		string jsonString = UJSONHandler<UCryptoRequest>.ToString(this);
		return jsonString;
	}
}

class UCryptoConvertRequest extends UFObject_Base {
	
	float Value = 0;
	
	void UCryptoConvertRequest(float value){
		Value = value;
	}
	
	override string ToJson(){
		string jsonString = UJSONHandler<UCryptoConvertRequest>.ToString(this);
		return jsonString;
	}
}

class UCryptoConvertResult extends StatusObject{
	float Value;
	
	float Get(){
		return Value;
	}
} 

class UCryptoResults extends StatusObject{
	autoptr map<string,float> Values;
	map<string,float> Get(){
		autoptr map<string,float> rValue = new  map<string,float>;
		rValue.Copy(Values);
		return rValue;
	}
}