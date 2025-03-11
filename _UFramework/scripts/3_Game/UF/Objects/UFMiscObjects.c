class UTextObject extends UFObject_Base{
	
	string Text = "";
	
	void UTextObject(string text){
		Text = text;
	}
	
	override string ToJson(){
		string jsonString = JsonFileLoader<UTextObject>.JsonMakeData(this);
		return jsonString;
	}
}

class URandomNumberResponse extends StatusObject {
	
	autoptr TIntArray Numbers;
	
}

class URandomNumberRequest extends UFObject_Base{
	int Count;
	
	void URandomNumberRequest(int count){
		Count = count;
	}
	
	override string ToJson(){
		string jsonString = JsonFileLoader<URandomNumberRequest>.JsonMakeData(this);
		return jsonString;
	}
}