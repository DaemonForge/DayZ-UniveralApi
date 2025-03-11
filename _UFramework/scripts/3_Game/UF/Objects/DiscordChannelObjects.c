class UCreateChannelObject extends UFObject_Base{

	string Name = "new-channel";
	
	autoptr UChannelCreateOptions Options;
	
	
	void UCreateChannelObject(string name, UChannelCreateOptions options = NULL){
		Name = name;
		if (!options){
			Options = new UChannelCreateOptions("Created Via DayZ");
		} else {
			Options = UChannelCreateOptions.Cast(options);
		}
	}
	
	
	override string ToJson(){
		string jsonString = UJSONHandler<UCreateChannelObject>.ToString(this);
		return jsonString;
	}

}

class UUpdateChannelObject extends UFObject_Base{
	
	string Reason = "";
	autoptr UChannelUpdateOptions Options;
	
	void UUpdateChannelObject(string reason, UChannelUpdateOptions options){
		Reason = reason;
		Options = UChannelUpdateOptions.Cast(options);
	}
	
	override string ToJson(){
		string jsonString = UJSONHandler<UUpdateChannelObject>.ToString(this);
		return jsonString;
	}
}


class UChannelUpdateOptions extends UChannelOptions {
	string name;
	
	void UChannelOptions(string Reason, string Name, string Topic = ""){
		name = Name;
		reason =  Reason;
		topic = Topic;
	}
	
}

class UChannelCreateOptions extends UChannelOptions{
	string type;

	void UChannelCreateOptions(string Reason, string Type = "text", string Topic = ""){
		type = Type;
	}
	
}

class UChannelOptions extends Managed {
	string reason = "Created Via DayZ";
	string topic;
	bool nsfw;
	string parent;
	autoptr array<autoptr UChannelPermissions> permissionOverwrites;
	int position = -1;
	int rateLimitPerUser = -1;
	
	
	void AddPerm(string id, string perm, bool isAllow = true){
		if (!permissionOverwrites){
			permissionOverwrites = new array<autoptr UChannelPermissions>;
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
				permissionOverwrites.Insert(new UChannelPermissions(id, { perm }, NULL));
			} else {
				permissionOverwrites.Insert(new UChannelPermissions(id, NULL, { perm }));
			}
		}
	}
	
	void SetPerms(string id, TStringArray perms, bool isAllow = true){
		if (!permissionOverwrites){
			permissionOverwrites = new array<autoptr UChannelPermissions>;
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
				permissionOverwrites.Insert(new UChannelPermissions(id, perms, NULL));
			} else {
				permissionOverwrites.Insert(new UChannelPermissions(id, NULL, perms));
			}
		}
		
	}
}

class UChannelPermissions extends Managed{

	string id;
	autoptr TStringArray allow;
	autoptr TStringArray deny;
	
	void UChannelPermissions(string Id, TStringArray Allow, TStringArray Deny){
		id = Id;
		allow = Allow;
		deny = Deny;
	}
}