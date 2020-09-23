class QnAQuestion{
	string question = "";
	
	void QnAQuestion(string Question){
		question = Question;
	}
	
	string ToJson(){
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

