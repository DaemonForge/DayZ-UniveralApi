/**
 * UCreateChannelObject
 *
 * Represents a request to create a new channel with a specified name and options.
 *
 * Members:
 *   - string Name: The name of the channel to be created.
 *   - UChannelCreateOptions Options: Options detailing properties of the channel creation,
 *     such as type and reason. If no options are provided, defaults are used.
 *
 * Constructor:
 *   - UCreateChannelObject(string name, UChannelCreateOptions options = NULL):
 *       Initializes the new channel creation object with the given name and optional options.
 *
 * Methods:
 *   - ToJson():
 *       Serializes the current object to a JSON string using UJSONHandler.
 */


/**
 * UUpdateChannelObject
 *
 * Represents a request to update an existing channel along with a reason and update options.
 *
 * Members:
 *   - string Reason: The reason provided for the update.
 *   - UChannelUpdateOptions Options: Options specifying the updates to apply to the channel.
 *
 * Constructor:
 *   - UUpdateChannelObject(string reason, UChannelUpdateOptions options):
 *       Initializes a channel update object with a provided reason and update options.
 *
 * Methods:
 *   - ToJson():
 *       Converts the current object state into a JSON string using UJSONHandler.
 */


/**
 * UChannelUpdateOptions
 *
 * Inherits from UChannelOptions and provides additional properties for updating a channel.
 *
 * Members:
 *   - string name: The updated channel name.
 *
 * Constructor:
 *   - UChannelOptions(string Reason, string Name, string Topic = ""):
 *       Initializes update options with a reason, a new name, and an optional topic.
 */


/**
 * UChannelCreateOptions
 *
 * Inherits from UChannelOptions and holds options specifically for channel creation.
 *
 * Members:
 *   - string type: The type of channel to create (default is "text").
 *
 * Constructor:
 *   - UChannelCreateOptions(string Reason, string Type = "text", string Topic = ""):
 *       Initializes the create options with a reason, a specified type, and an optional topic.
 */


/**
 * UChannelOptions
 *
 * Base class representing common options for channel operations (creation and update).
 *
 * Members:
 *   - string reason: Reason for the action (default "Created Via DayZ").
 *   - string topic: The topic or description for the channel.
 *   - bool nsfw: Indicates if the channel is marked as not safe for work.
 *   - string parent: Identifier for the parent category or channel.
 *   - array<autoptr UChannelPermissions> permissionOverwrites: List of permission overwrites for the channel.
 *   - int position: The position of the channel (default -1).
 *   - int rateLimitPerUser: The rate limit per user (default -1).
 *
 * Methods:
 *   - AddPerm(string id, string perm, bool isAllow = true):
 *       Adds or updates a single permission for the specified role or user.
 *   - SetPerms(string id, TStringArray perms, bool isAllow = true):
 *       Sets multiple permissions for the specified role or user.
 */


/**
 * UChannelPermissions
 *
 * Represents permission overwrites for a specific role or user in a channel.
 *
 * Members:
 *   - string id: Identifier for the role or user.
 *   - TStringArray allow: List of permissions that are allowed.
 *   - TStringArray deny: List of permissions that are denied.
 *
 * Constructor:
 *   - UChannelPermissions(string Id, TStringArray Allow, TStringArray Deny):
 *       Initializes the permission object with the provided allowed and denied permission lists.
 */
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