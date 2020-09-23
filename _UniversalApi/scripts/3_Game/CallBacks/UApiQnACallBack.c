class UApiQnACallBack : RestCallback
{
	
	protected bool m_AlwaysAnswer = false;
	
	void SetAlwaysAnswer(bool alwaysAnswer = true){
		m_AlwaysAnswer = alwaysAnswer;
	}
	
	override void OnError(int errorCode) {
		if (m_AlwaysAnswer){
			GetGame().Chat("BOT: Sorry Something went wrong try asking again later", "colorImportant");
		}
	};
	
	override void OnTimeout() {
		if (m_AlwaysAnswer){
			GetGame().Chat("BOT: Sorry Something went wrong try asking again later", "colorImportant");
		}
	};
	override void OnSuccess(string data, int dataSize) {
		ref QnAAnswer AnswerObj;
		JsonFileLoader<QnAAnswer>.JsonLoadData(data, AnswerObj);
		if (AnswerObj.get() != "null" && AnswerObj.get() != "error" &&  AnswerObj.get() != "ERROR" &&  AnswerObj.get() != ""){
			string message = "BOT: " + AnswerObj.get();
			GetGame().Chat(message, "");
		} else if (m_AlwaysAnswer) {
			GetGame().Chat("BOT: Sorry couldn't find the an answer to your question? Try rephrasing it or asking a real person", "colorImportant");
		}
	};
};