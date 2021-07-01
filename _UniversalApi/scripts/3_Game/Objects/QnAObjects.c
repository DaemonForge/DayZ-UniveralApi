class QnAQuestion extends UApiObject_Base{ // Will be removed to allow for it to just use the generic UApiQuestion Object
	string question = "";
	
	void QnAQuestion(string Question){
		question = Question;
	}
	
	override string ToJson(){
		string jsonString = JsonFileLoader<QnAQuestion>.JsonMakeData(this);
		return jsonString;
	}
}

class QnAAnswer{
	string answer = "";
	float score = 0;
	string get(){
		return answer;
	}
}

class UApiQuestionRequest extends UApiObject_Base{
	string Question = "";
	
	void UApiQuestionRequest(string question){
		Question = question;
	}
	
	override string ToJson(){
		string jsonString = JsonFileLoader<UApiQuestionRequest>.JsonMakeData(this);
		return jsonString;
	}
}

class UApiRandomNumberRequest extends UApiObject_Base{
	int Count;
	
	void UApiRandomNumberRequest(int count){
		Count = count;
	}
	
	override string ToJson(){
		string jsonString = JsonFileLoader<UApiRandomNumberRequest>.JsonMakeData(this);
		return jsonString;
	}
}

