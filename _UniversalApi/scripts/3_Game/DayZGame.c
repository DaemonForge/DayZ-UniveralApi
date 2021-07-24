modded class DayZGame extends CGame
{
	
	protected void CBQnAChatMessageSilent(int cid, int status, string oid, string data){
		if (status == UAPI_SUCCESS){
			autoptr QnAAnswer AnswerObj;
			JsonFileLoader<QnAAnswer>.JsonLoadData(data, AnswerObj);
			if (AnswerObj.get() != "null" && AnswerObj.get() != "error" &&  AnswerObj.get() != "ERROR" &&  AnswerObj.get() != ""){
				UApiQnAMaker().ProcessAnswer(AnswerObj.get());
			} else {
				UApiQnAMaker().SendRespone("Sorry couldn't find the an answer to your question? Try rephrasing it or asking a real person");
			}
		}
	}
	
	protected void CBQnAChatMessage(int cid, int status, string oid, string data){
		if (status == UAPI_SUCCESS){
			autoptr QnAAnswer AnswerObj;
			JsonFileLoader<QnAAnswer>.JsonLoadData(data, AnswerObj);
			if (AnswerObj.get() != "null" && AnswerObj.get() != "error" &&  AnswerObj.get() != "ERROR" &&  AnswerObj.get() != ""){
				UApiQnAMaker().ProcessAnswer(AnswerObj.get());
			}
		}
	}
}