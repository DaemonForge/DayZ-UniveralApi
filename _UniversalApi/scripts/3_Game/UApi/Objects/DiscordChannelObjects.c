class UApiCreateChannelObject extends UApiObject_Base{

	string Name = "new-channel";
	
	autoptr UApiChannelCreateOptions Options;
	
	
	void UApiCreateChannelObject(string name, UApiChannelCreateOptions options = NULL){
		Name = name;
		if (!options){
			Options = new UApiChannelCreateOptions("Created Via DayZ");
		} else {
			Options = UApiChannelCreateOptions.Cast(options);
		}
	}
	
	
	override string ToJson(){
		string jsonString = UApiJSONHandler<UApiCreateChannelObject>.ToString(this);
		return jsonString;
	}

}

class UApiUpdateChannelObject extends UApiObject_Base{
	
	string Reason = "";
	autoptr UApiChannelUpdateOptions Options;
	
	void UApiUpdateChannelObject(string reason, UApiChannelUpdateOptions options){
		Reason = reason;
		Options = UApiChannelUpdateOptions.Cast(options);
	}
	
	override string ToJson(){
		string jsonString = UApiJSONHandler<UApiUpdateChannelObject>.ToString(this);
		return jsonString;
	}
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
	autoptr array<autoptr UApiChannelPermissions> permissionOverwrites;
	int position = -1;
	int rateLimitPerUser = -1;
	
	
	void AddPerm(string id, string perm, bool isAllow = true){
		if (!permissionOverwrites){
			permissionOverwrites = new array<autoptr UApiChannelPermissions>;
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
			permissionOverwrites = new array<autoptr UApiChannelPermissions>;
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
	autoptr TStringArray allow;
	autoptr TStringArray deny;
	
	void UApiChannelPermissions(string Id, TStringArray Allow, TStringArray Deny){
		id = Id;
		allow = Allow;
		deny = Deny;
	}
}