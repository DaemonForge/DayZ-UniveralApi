modded class DayZGame extends CGame
{
	protected autoptr UDiscordUser m_discordUser;
	
	UDiscordUser DiscordUser(){
		if (!IsClient()){
			Error2("[UF] DiscordInfo", "Can't get discord info from server you must request it from the api directly only client caches");
			return NULL;
		}
		return m_discordUser;
	}
	
	
	protected void CBCacheDiscordInfo(int cid, int status, string oid, UDiscordUser data){
		Print("[UF] Attempting to Cache Discord info cid"+cid + " status: " + status);
		if (IsClient() && status == UF_SUCCESS){
			if (Class.CastTo(m_discordUser, data)){
				Print("[UF] Discord is set up and cached " + m_discordUser.Username + "#" +  m_discordUser.Discriminator);
			}
		}
		if (IsClient() && status == UF_NOTSETUP && UFConfig().PromptDiscordOnConnect >= 1){
			Print("[UF] [Discord] Prompt on connect configured and no Discord info found");
			OpenURL(U().ds().Link());
		}
	}
	
	override void OnUpdate(bool doSim, float timeslice){
		super.OnUpdate(doSim, timeslice);
		if (UFramework.isGlobalInit()){
			U().Cron().onUpdate();
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