/**
 * UDiscordUser class
 *
 * This class represents a Discord user and extends the StatusObject base class.
 *
 * Properties:
 * - id: A string representing the unique Discord user identifier.
 * - Username: The Discord user's display name.
 * - Discriminator: A string for differentiating users with the same username (commonly a numeric identifier).
 * - Avatar: A string representing the user's avatar identifier or URL.
 * - Roles: A dynamically allocated array (TStringArray) of role IDs assigned to the user.
 * - VoiceChannel: A string representing the identifier for the voice channel the user is connected to.
 *
 * Methods:
 * - HasRole(string roleid): Checks if the user has the specified role.
 *   - Returns true if the role is present in the Roles array; otherwise, returns false.
 */
class UDiscordUser extends StatusObject{
		
	string id;
	string Username;
	
	string GlobalName;
	string Avatar;
	
	autoptr TStringArray Roles;
	
	string VoiceChannel;
	
	bool HasRole(string roleid){
		if (!Roles) return false;
		return (Roles.Find(roleid) != -1);
	}
	
	int AddRole(string roleid){
		if (!id || !roleid) {
			Error("[UF] [UDiscordUser] Error: Cannot add role, user ID is invalid");
			return -1;
		}
		
		return U().ds().AddRole(id, roleid, this, "OnRoleAdded");
	}
	
	protected void OnRoleAdded(UDiscordUser user, string discordId) {
		if (user && user.Status == "Success") {
			// Create a deep copy of the roles array to avoid pointer issues
			if (!Roles) {
				Roles = new TStringArray();
			} else {
				Roles.Clear();
			}
			
			if (user.Roles) {
				for (int i = 0; i < user.Roles.Count(); i++) {
					Roles.Insert(user.Roles.Get(i));
				}
			}
			
			Print("[UF] [UDiscordUser] Role added successfully for user: " + id);
		} else {
			Error("[UF] [UDiscordUser] Failed to add role for user: " + id + " Error: " + user.Error);
		}
	}
	
	int RemoveRole(string roleid){
		if (!id || !roleid) {
			Error("[UF] [UDiscordUser] Error: Cannot remove role, user ID is invalid");
			return -1;
		}
		
		return U().ds().RemoveRole(id, roleid, this, "OnRoleRemoved");
	}
	
	protected void OnRoleRemoved(UDiscordUser user, string discordId) {
		if (user && user.Status == "Success") {
			// Create a deep copy of the roles array to avoid pointer issues
			if (!Roles) {
				Roles = new TStringArray();
			} else {
				Roles.Clear();
			}
			
			if (user.Roles) {
				for (int i = 0; i < user.Roles.Count(); i++) {
					Roles.Insert(user.Roles.Get(i));
				}
			}
			
			Print("[UF] [UDiscordUser] Role removed successfully for user: " + id);
		} else {
			Error("[UF] [UDiscordUser] Failed to remove role for user: " + id + " Error: " + user.Error);
		}
	}
}