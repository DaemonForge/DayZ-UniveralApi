class UApiCreateChannelObject extends UApiObject_Base{

	string Name = "new-channel";
	
	ref UApiChannelCreateOptions Options;
	
	
	void UApiCreateChannelObject(string name, UApiChannelCreateOptions options = NULL){
		Name = name;
		if (!options){
			Options = new UApiChannelCreateOptions("Created Via DayZ");
		} else {
			Options = UApiChannelCreateOptions.Cast(options);
		}
	}
	
	
	override string ToJson(){
		string jsonString = JsonFileLoader<UApiCreateChannelObject>.JsonMakeData(this);
		return jsonString;
	}

}

class UApiUpdateChannelObject extends UApiObject_Base{
	
	string Reason = "";
	ref UApiChannelUpdateOptions Options;
	
	void UApiUpdateChannelObject(string reason, UApiChannelUpdateOptions options){
		Reason = reason;
		Options = UApiChannelUpdateOptions.Cast(options);
	}
	
	override string ToJson(){
		string jsonString = JsonFileLoader<UApiUpdateChannelObject>.JsonMakeData(this);
		return jsonString;
	}
}



class DSPerms {
	
	static string ADD_REACTIONS = "ADD_REACTIONS"; // (add new reactions to messages)
	static string VIEW_AUDIT_LOG = "VIEW_AUDIT_LOG";
	static string PRIORITY_SPEAKER = "PRIORITY_SPEAKER";
	static string STREAM = "STREAM";
	static string VIEW_CHANNEL = "VIEW_CHANNEL";
	static string SEND_MESSAGES = "SEND_MESSAGES";
	static string SEND_TTS_MESSAGES = "SEND_TTS_MESSAGES";
	static string MANAGE_MESSAGES = "MANAGE_MESSAGES"; // (delete messages and reactions)
	static string EMBED_LINKS = "EMBED_LINKS"; // (links posted will have a preview embedded)
	static string ATTACH_FILES = "ATTACH_FILES"; 
	static string READ_MESSAGE_HISTORY = "READ_MESSAGE_HISTORY"; // (view messages that were posted prior to opening Discord)
	static string MENTION_EVERYONE = "MENTION_EVERYONE";
	static string USE_EXTERNAL_EMOJIS = "USE_EXTERNAL_EMOJIS"; // (use emojis from different guilds)
	static string CONNECT = "CONNECT"; // (connect to a voice channel)
	static string USE_VAD = "USE_VAD"; // (use voice activity detection)
	static string SPEAK = "SPEAK"; // (speak in a voice channel)
	static string CREATE_INSTANT_INVITE = "CREATE_INSTANT_INVITE"; // (create invitations to the guild)
	
	
	//Since there is no functions to manage the discord these permission are kinda usless but keeping them encase something changes in the future
	static string ADMINISTRATOR = "ADMINISTRATOR";// (implicitly has all permissions, and bypasses all channel overwrites)
	static string KICK_MEMBERS = "KICK_MEMBERS";
	static string BAN_MEMBERS = "BAN_MEMBERS";
	static string MANAGE_CHANNELS = "MANAGE_CHANNELS"; //(edit and reorder channels)
	static string MANAGE_GUILD = "MANAGE_GUILD"; //  (edit the guild information, region, etc.)
	static string VIEW_GUILD_INSIGHTS = "VIEW_GUILD_INSIGHTS";
	static string MUTE_MEMBERS = "MUTE_MEMBERS"; //(mute members across all voice channels)
	static string DEAFEN_MEMBERS = "DEAFEN_MEMBERS"; //(deafen members across all voice channels)
	static string MOVE_MEMBERS = "MOVE_MEMBERS"; //(move members between voice channels)
	static string CHANGE_NICKNAME = "CHANGE_NICKNAME";
	static string MANAGE_NICKNAMES = "MANAGE_NICKNAMES"; //(change other members' nicknames)
	static string MANAGE_ROLES = "MANAGE_ROLES";
	static string MANAGE_WEBHOOKS = "MANAGE_WEBHOOKS";
	static string MANAGE_EMOJIS = "MANAGE_EMOJIS";	
}




class UApiChannelUpdateOptions extends UApiChannelOptions {
	string name;
	
	void UApiChannelOptions(string Reason, string Name, string Topic = ""){
		name = Name;
		reason =  Reason;
		topic = Topic;
	}
	
}

class UApiChannelCreateOptions extends UApiChannelOptions{
	string type;

	void UApiChannelCreateOptions(string Reason, string Type = "text", string Topic = ""){
		type = Type;
	}
	
}

class UApiChannelOptions extends Managed {
	string reason = "Created Via DayZ";
	string topic;
	bool nsfw;
	string parent;
	ref array<ref UApiChannelPermissions> permissionOverwrites;
	int position = -1;
	int rateLimitPerUser = -1;
	
	
	void AddPerm(string id, string perm, bool isAllow = true){
		if (!permissionOverwrites){
			permissionOverwrites = new array<ref UApiChannelPermissions>;
		}
		bool added = false;
		for (int i = 0; i < permissionOverwrites.Count(); i++){
			if (permissionOverwrites.Get(i) && permissionOverwrites.Get(i).id == id){
				if (isAllow){
					added = true;
					permissionOverwrites.Get(i).allow.Insert(perm);
				} else {
					added = true;
					permissionOverwrites.Get(i).deny.Insert(perm);
				}
			}
		} 
		if (!added){
			if (isAllow){
				permissionOverwrites.Insert(new UApiChannelPermissions(id, { perm }, NULL));
			} else {
				permissionOverwrites.Insert(new UApiChannelPermissions(id, NULL, { perm }));
			}
		}
	}
	
	void SetPerms(string id, TStringArray perms, bool isAllow = true){
		if (!permissionOverwrites){
			permissionOverwrites = new array<ref UApiChannelPermissions>;
		}
		bool added = false;
		for (int i = 0; i < permissionOverwrites.Count(); i++){
			if (permissionOverwrites.Get(i) && permissionOverwrites.Get(i).id == id){
				if (isAllow){
					added =true;
					permissionOverwrites.Get(i).allow = perms;
				} else {
					added =true;
					permissionOverwrites.Get(i).deny = perms;
				}
			}
		} 
		
		if (!added){
			if (isAllow){
				permissionOverwrites.Insert(new UApiChannelPermissions(id, perms, NULL));
			} else {
				permissionOverwrites.Insert(new UApiChannelPermissions(id, NULL, perms));
			}
		}
		
	}
}


class UApiChannelPermissions extends Managed{

	string id;
	ref TStringArray allow;
	ref TStringArray deny;
	
	void UApiChannelPermissions(string Id, TStringArray Allow, TStringArray Deny){
		id = Id;
		allow = Allow;
		deny = Deny;
	}
}