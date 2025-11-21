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
		Print("[UF] Attempting to Cache Discord info cid" + cid + " status: " + status);
		if (IsClient() && status == UF_SUCCESS){
			if (Class.CastTo(m_discordUser, data)){
				Print("[UF] Discord is set up and cached " + m_discordUser.GlobalName);
				U().ds().DownloadAvatar(GetDayZGame().GetSteamId()); //will use auth key to get the GUID
				g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.UpdateDiscordLoggedInWidget, 1500, false);
			}
		}
		if (IsClient() && status == UF_NOTSETUP && UFConfig().PromptDiscordOnConnect >= 1){
			Print("[UF] [Discord] Prompt on connect configured and no Discord info found");
			GetDiscordLoggedInWidget().ShowAvatar();
		}
	}
	
	protected bool m_UpdateDiscordWidgetShouldRetry = true;
	protected void UpdateDiscordLoggedInWidget(){
		Print("[UF] [Discord] UpdateDiscordLoggedInWidget");
		if (m_discordUser && GetDiscordLoggedInWidget()){
			if (FileExist("$saves:discordme.edds")){
				GetDiscordLoggedInWidget().ShowAvatar();
				GetDiscordLoggedInWidget().UpdateData(m_discordUser.GlobalName, "$saves:discordme.edds");
				m_UpdateDiscordWidgetShouldRetry = false;
			} else {
				GetDiscordLoggedInWidget().ShowAvatar();
				GetDiscordLoggedInWidget().UpdateData(m_discordUser.GlobalName, "_UFramework/images/discord.edds");
				if (m_UpdateDiscordWidgetShouldRetry){
					g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.UpdateDiscordLoggedInWidget, 3500, false);
					m_UpdateDiscordWidgetShouldRetry = false;
				}
			}
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