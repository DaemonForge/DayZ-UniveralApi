modded class DayZGame extends CGame
{
	protected autoptr UApiDiscordUser m_discordUser;
	
	UApiDiscordUser DiscordUser(){
		if (!IsClient()){
			Error2("[UAPI] DiscordInfo", "Can't get discord info from server you must request it from the api directly only client caches");
			return NULL;
		}
		return m_discordUser;
	}
	
	
	protected void CBCacheDiscordInfo(int cid, int status, string oid, UApiDiscordUser data){
		if (IsClient() && status == UAPI_SUCCESS){
			if (Class.CastTo(m_discordUser, data)){
				Print("[UAPI] Discord is set up and cached " + m_discordUser.Username + "#" +  m_discordUser.Discriminator);
			}
		}
		if (IsClient() && status == UAPI_NOTSETUP && UApiConfig().PromptDiscordOnConnect >= 1){
			OpenURL(UApi().ds().Link());
		}
	}
	
	protected void CBQnAChatMessageSilent(int cid, int status, string oid, QnAAnswer data){
		if (status == UAPI_SUCCESS && data){
			if (data.get() != "null" && data.get() != "error" &&  data.get() != "ERROR" &&  data.get() != ""){
				UApiQnAMaker().ProcessAnswer(data.get());
			} else {
				UApiQnAMaker().SendRespone("Sorry couldn't find the an answer to your question? Try rephrasing it or asking a real person");
			}
		}
	}
	
	protected void CBQnAChatMessage(int cid, int status, string oid, QnAAnswer data){
		if (status == UAPI_SUCCESS && data){
			if (data.get() != "null" && data.get() != "error" &&  data.get() != "ERROR" &&  data.get() != ""){
				UApiQnAMaker().ProcessAnswer(data.get());
			}
		}
	}
	
	//Client side function to get the steam id
	string GetSteamId(){
		DayZPlayer player;
		if (GetUserManager() && GetUserManager().GetTitleInitiator()){
			return GetUserManager().GetTitleInitiator().GetUid();
		} else if (IsClient() && Class.CastTo(player, GetPlayer()) && player.GetIdentity() && player.GetIdentity().GetPlainId() != "" ){
			return player.GetIdentity().GetPlainId();
		} 
		return "";
	}
}