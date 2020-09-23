modded class ChatInputMenu
{
	override bool OnChange(Widget w, int x, int y, bool finished)
	{
		if (UApiConfig().QnAEnabled){
			if (!finished) return false;
			string question = m_edit_box.GetText();
			if (question.Length() > 3){
				int lastIndex = question.Length() - 1;
				bool silentQuestion = (question.Substring(0,1) == "?" );
				bool generalQuestion = (question.Substring(lastIndex,1) == "?" );
				if (question != "" && (silentQuestion || generalQuestion))	{
					string rdyQuestion = question;
					if (silentQuestion){
						rdyQuestion = question.Substring(1,lastIndex);
					}
					Print("Question: \"" + question + "\" Set to API");
					UApi().Rest().QnA(rdyQuestion, silentQuestion);
					if (silentQuestion){
						m_close_timer.Run(0.1, this, "Close");
						return true;
					}
				}
			}
		}
		return super.OnChange(w, x, y, finished);
	}
};