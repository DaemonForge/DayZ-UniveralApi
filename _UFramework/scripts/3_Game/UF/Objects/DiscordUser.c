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
	
	string Discriminator;
	string Avatar;
	
	autoptr TStringArray Roles;
	
	string VoiceChannel;
	
	bool HasRole(string roleid){
		if (!Roles) return false;
		return (Roles.Find(roleid) != -1);
	}
	
}