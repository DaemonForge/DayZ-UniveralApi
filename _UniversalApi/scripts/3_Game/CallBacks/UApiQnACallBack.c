class UApiQnACallBack : RestCallback
{
	
	protected bool m_AlwaysAnswer = false;
	
	void SetAlwaysAnswer(bool alwaysAnswer = true){
		m_AlwaysAnswer = alwaysAnswer;
	}
	
	override void OnError(int errorCode) {
		if (m_AlwaysAnswer){
			UApiQnAMaker().SendRespone("Sorry Something went wrong try asking again later");
		}
	};
	
	override void OnTimeout() {
		if (m_AlwaysAnswer){
			UApiQnAMaker().SendRespone("Sorry Something went wrong try asking again later");
		}
	};
	override void OnSuccess(string data, int dataSize) {
		autoptr QnAAnswer AnswerObj;
		JsonFileLoader<QnAAnswer>.JsonLoadData(data, AnswerObj);
		if (AnswerObj.get() != "null" && AnswerObj.get() != "error" &&  AnswerObj.get() != "ERROR" &&  AnswerObj.get() != ""){
			UApiQnAMaker().ProcessAnswer(AnswerObj.get());
		} else if (m_AlwaysAnswer) {
			UApiQnAMaker().SendRespone("Sorry couldn't find the an answer to your question? Try rephrasing it or asking a real person");
		}
	};
};