/**
 * Converts an array of ULogPlayerPos objects into a JSON string.
 *
 * This function leverages the JsonFileLoader's JsonMakeData method to serialize an array
 * of ULogPlayerPos pointers into its JSON representation.
 *
 * @param thePlayerlist Array containing autopointers to ULogPlayerPos objects.
 * @return              A JSON string that represents the provided array.
 */
static string GetLogPlayerPosArray(array<autoptr ULogPlayerPos> thePlayerlist){
	return JsonFileLoader<array<autoptr ULogPlayerPos>>.JsonMakeData(thePlayerlist);
}


class UUtil extends Managed {
	
	/**
	 * Gets the Steam ID of the current player.
	 * 
	 * For client side:
	 *  - If the game has a valid TitleInitiator, returns its UID.
	 *  - Otherwise, if the client and player identity exist, returns the plain ID.
	 *  - Returns an empty string if none of these conditions are met.
	 *
	 * @return string The Steam ID as a string, or empty if not found.
	 */
	 
	//Client side function to get the steam id
	static string GetSteamId(){
		DayZPlayer player;
		if (GetGame() && GetGame().GetUserManager() && GetGame().GetUserManager().GetTitleInitiator()){
			return GetGame().GetUserManager().GetTitleInitiator().GetUid();
		} else if (GetGame() && GetGame().IsClient() && Class.CastTo(player, GetGame().GetPlayer()) && player.GetIdentity() && player.GetIdentity().GetPlainId() != "" ){
			return player.GetIdentity().GetPlainId();
		} 
		return "";
	}
	
	/**
	 * Finds and returns an array of file names located in the specified directory.
	 *
	 * The function builds a search pattern by appending "\*" to the directory path,
	 * then iterates through the matching files. Only valid file attributes result in an insertion.
	 *
	 * @param directory The path to the directory in which to search for files.
	 * @return TStringArray An array of file names found in the directory.
	 */
	//Return an array of file names for all the files in the specified directory
	static TStringArray FindFilesInDirectory(string directory)  { 
		TStringArray fileList = new TStringArray;
		
		string		fileName;
		int		fileAttr;
		int		flags;
		//Add \ to directory path and add search parameter (*)
		string pathpattern = directory + "\\*";
		
		//Search for files in file directory
		FindFileHandle fileHandler = FindFile(pathpattern, fileName, fileAttr, flags);
		
		bool found = true;
	    while ( found ) {//while there are files loop through looking for more
		    if ( fileAttr ) {
		        	//If file exsit add to array
		        	fileList.Insert(fileName);
		    }
			found = FindNextFile(fileHandler, fileName, fileAttr);
	    }
		return fileList; 
	};
	
	/**
	 * Generates a random id string of a specified length.
	 *
	 * Uses a predefined character array including uppercase, lowercase letters, and digits.
	 * A loop selects random characters from the array to construct the id.
	 * Note: The loop iterates from 0 to number inclusive, resulting in (number+1) characters.
	 *
	 * @param number The number determining the length of the generated id.
	 * @return string The generated random id.
	 */
	//Generate a random id string of a specified length
	static string GetRandomId(int number){
		U().CheckAndRenewQRandom();
		TStringArray Chars = {"A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z","a","b","c","d","e","f","g","h","i","j","k","l","m","n","o","p","q","r","s","t","u","v","w","x","y","z","0","1","2","3","4","5","6","7","8","9"};
		string id = "";
		for (int i = 0; i <= number; i++){
			int idx = Math.QRandomInt(0,(Chars.Count() - 1));
			id = id + Chars.Get(idx);
		}
		return id;
	}
	
	/**
	 * Finds a player on the server based on their GUID.
	 *
	 * Iterates over all connected players and casts each to a DayZPlayer.
	 * If a player's identity exists and matches the provided GUID, that player is returned.
	 *
	 * @param GUID The unique identifier for the player.
	 * @return DayZPlayer The player with the matching GUID, or NULL if the player is not found.
	 */
	//Simple function for finding a player based on their GUID
	static DayZPlayer FindPlayer(string GUID){
		if (GetGame().IsServer()){
			autoptr array<Man> players = new array<Man>;
			GetGame().GetPlayers( players );
			for (int i = 0; i < players.Count(); i++){
				DayZPlayer player = DayZPlayer.Cast(players.Get(i));
				if (player.GetIdentity() && player.GetIdentity().GetId() == GUID ){
					return player;
				}
			}
		}
		return NULL;
	}
	
	/**
	 * Finds a player by their identity.
	 *
	 * Retrieves the network ID corresponding to the player's identity, then returns the player object
	 * associated with that network ID.
	 *
	 * @param identity The PlayerIdentity to find.
	 * @return DayZPlayer The corresponding player, or NULL if not found.
	 */
	//Simple function for finding a player based on their identity
	static DayZPlayer FindPlayerByIdentity(PlayerIdentity identity) {
		if (!identity)
			return NULL;

		int highBits;
		int lowBits;
		GetGame().GetPlayerNetworkIDByIdentityID(identity.GetPlayerId(), lowBits, highBits);
		return DayZPlayer.Cast(GetGame().GetObjectByNetworkId(lowBits, highBits));
	}
	
	 
	/**
	 * Sends a notification to a player with extended options.
	 *
	 * This function checks if the runtime is dedicated server or client:
	 *  - On a dedicated server, it calls the NotificationSystem to send a notification to the player identity.
	 *  - On a client, it adds a notification directly.
	 *
	 * @param Header The title/header of the notification.
	 * @param Text The message text of the notification.
	 * @param player The target player's identity.
	 * @param Icon (Optional) The path to an icon to display with the notification. Defaults to "_UFramework\images\info.edds".
	 */
	static void SendNotificationEx(string Header, string Text, PlayerIdentity player, string Icon = "_UFramework\\images\\info.edds") {
		if (GetGame().IsDedicatedServer()){
			NotificationSystem.SendNotificationToPlayerIdentityExtended(player, 5, Header, Text, Icon );
		} else if (GetGame().IsClient()){
			NotificationSystem.AddNotificationExtended(5, Header, Text, Icon);
		}
	}
	
	 
	/**
	 * Sends a notification to a player.
	 *
	 * This function is a wrapper for SendNotificationEx that first checks if the player identity exists.
	 *
	 * @param Header The title/header of the notification.
	 * @param Text The message text of the notification.
	 * @param player The target player's identity.
	 * @param Icon (Optional) The path to an icon for the notification. Defaults to "_UFramework\images\info.edds".
	 */
	static void SendNotification(string Header, string Text, PlayerIdentity player, string Icon = "_UFramework\\images\\info.edds") {
		if (!player) return;
		SendNotificationEx(Header,Text,player,Icon);
	}
	
	
	 
	/**
	 * Converts an integer (e.g., a dollar amount) into a nice formatted string with commas.
	 *
	 * Takes into account negative values and uses substring manipulation to insert commas at the appropriate positions.
	 *
	 * @param DollarAmount The integer value to format.
	 * @return string The formatted string representation of the integer.
	 */
	static string ConvertIntToNiceString(int DollarAmount){
		string prefix = "";
		string NiceString = "";
		if (DollarAmount < 0){
			prefix = "-";
		}
		string OrginalString = Math.AbsInt(DollarAmount).ToString();
		if (OrginalString.Length() <= 3){
			return prefix + OrginalString;
		} 
		int StrLen = OrginalString.Length() - 3;
		string StrSelection = OrginalString.Substring(StrLen,3);
		NiceString = StrSelection;
		while (StrLen > 3){
			StrLen = StrLen - 3;
			StrSelection = OrginalString.Substring(StrLen,3);
			NiceString = StrSelection + "," + NiceString;
		}
		StrSelection = OrginalString.Substring(0,StrLen);
		NiceString = StrSelection + "," + NiceString;
		return prefix + NiceString;
	}
	
	 
	/**
	 * Converts a REST error code to its corresponding string representation.
	 *
	 * The function uses a switch-case structure to map REST error states (such as pending, success, various errors)
	 * to their string equivalents.
	 *
	 * @param ErrorCode The REST error code (one of ERestResultState values).
	 * @return string A string representation of the error code, or "UNDEFINED_ERROR" if unknown.
	 */
	static string RestErrorToString(int ErrorCode){
		switch ( ErrorCode )
		{
			case ERestResultState.EREST_EMPTY:
				return "EREST_EMPTY";
			case ERestResultState.EREST_PENDING:
				return "EREST_PENDING";
			case ERestResultState.EREST_FEEDING:
				return "EREST_FEEDING";
			case ERestResultState.EREST_SUCCESS:
				return "EREST_SUCCESS";
			case ERestResultState.EREST_ERROR:
				return "EREST_ERROR";
			case ERestResultState.EREST_ERROR_CLIENTERROR:
				return "EREST_ERROR_CLIENTERROR";
			case ERestResultState.EREST_ERROR_SERVERERROR:
				return "EREST_ERROR_SERVERERROR";
			case ERestResultState.EREST_ERROR_APPERROR:
				return "EREST_ERROR_APPERROR";
			case ERestResultState.EREST_ERROR_TIMEOUT:
				return "EREST_ERROR_TIMEOUT";
			case ERestResultState.EREST_ERROR_NOTIMPLEMENTED:
				return "EREST_ERROR_NOTIMPLEMENTED";
			case ERestResultState.EREST_ERROR_UNKNOWN:
				return "EREST_ERROR_UNKNOWN";
		}
		return "UNDEFINED_ERROR";
	}
	
	 
	/**
	 * Gets the current date stamp in the format "YYYY-MM-DD".
	 *
	 * Retrieves the current year, month, and day, ensuring that single digit days or months are padded with a leading zero.
	 *
	 * @return string The current date stamp.
	 */
	static string GetDateStamp() {
		int yr, mth, day;
		GetYearMonthDay(yr, mth, day);
		string sday = day.ToString();
		if (sday.Length() == 1){
			sday = "0" + sday;
		}
		
		string smth = mth.ToString();
		if (smth.Length() == 1){
			smth = "0" + mth.ToString();
		}
		
		return yr.ToString() + "-" + smth + "-" + sday;
	}
	 
	/**
	 * Gets the current time stamp in the format "HH:MM:SS".
	 *
	 * Retrieves the current hour, minute, and second, with single digit values padded with a leading zero.
	 *
	 * @return string The current time stamp.
	 */
	static string GetTimeStamp() {
		int hr, min, sec;
		GetHourMinuteSecond(hr, min, sec);
		
		string ssec = sec.ToString();
		if (ssec.Length() == 1){
			ssec = "0" + ssec;
		}
		string smin = min.ToString();
		if (smin.Length() == 1){
			smin = "0" + smin;
		}
		string shr = hr.ToString();
		if (shr.Length() == 1) {
			shr = "0" + shr;
		}
		return  shr + ":" + smin + ":" + ssec;
	}
	
	protected static int UnixStartYear = 1970;
	protected static int DaysInMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	
	/**
	 * Determines if a given year is a leap year.
	 *
	 * Checks if the year is divisible by 4, 100, and 400 to determine if it is a leap year.
	 *
	 * @param year The year to check.
	 * @return bool True if the year is a leap year, false otherwise.
	 */
	protected static bool IsLeapYear(int year){
		if (year % 4 == 0) {
	        if (year % 100 == 0) {
	            if (year % 400 == 0) {
	                return true;
	            }
	    		return false;
			}
	        return true;
	    }
	    return false;
	}
	
	//Get Days since JAN 01 1970
	/**
	 * Calculates the number of days passed since January 1, 1970, based on the current local date.
	 *
	 * This function takes into account the days in the current month and leap years.
	 *
	 * @return int The count of days since January 1, 1970 in local time.
	 */
	static int GetDateInt() {
		int yr, mth, day;
		GetYearMonthDay(yr, mth, day);
		int count = day;
		for (int i = 0; i < (mth - 1); i++){
			count = count + DaysInMonth[i];
			if (IsLeapYear(yr) && i == 1){
				count++;
			}
		}
		count = count + Math.Floor((yr - UnixStartYear) * 365.25);
		return count;
	}
	
	//Get Days since JAN 01 1970
	 
	/**
	 * Calculates the number of days passed since January 1, 1970, based on the current UTC date.
	 *
	 * Similar to GetDateInt but using UTC values.
	 *
	 * @return int The count of days since January 1, 1970 in UTC.
	 */
	static int GetUTCDateInt() {
		int yr, mth, day;
		GetYearMonthDayUTC(yr, mth, day);
		int count = day;
		for (int i = 0; i < (mth - 1); i++){
			count = count + DaysInMonth[i];
			if (IsLeapYear(yr) && i == 1){
				count++;
			}
		}
		count = count + Math.Floor((yr - UnixStartYear) * 365.25);
		return count;
	}
	
	//Gets the current unix time stamp for the current server timezone
	//Due to int.MAX will break on Tue Jan 19 2038 03:14:07
	/**
	 * Gets the current Unix timestamp for the server's local time.
	 *
	 * Computes the number of seconds since January 1, 1970, by combining the date integer and the current time.
	 * Note: The use of int may lead to issues on Tue Jan 19 2038 03:14:07 due to int.MAX limitations.
	 *
	 * @return int The Unix timestamp for local time.
	 */
	static int GetUnixInt() {
		int hr, min, sec;
		GetHourMinuteSecond(hr, min, sec);
		return (GetDateInt() * 86400) + (hr * 3600) + (min * 60) + sec;
	}
	
	// Gets the current unix time stamp at UTC
	//Due to int.MAX will break on Tue Jan 19 2038 03:14:07
	 
	/**
	 * Gets the current Unix timestamp for UTC.
	 *
	 * Similar in computation to GetUnixInt but based on UTC date and time.
	 *
	 * @return int The Unix timestamp for UTC.
	 */
	static int GetUTCUnixInt() {
		int hr, min, sec;
		GetHourMinuteSecondUTC(hr, min, sec);
		return (GetUTCDateInt() * 86400) + (hr * 3600) + (min * 60) + sec;
	}
	
	
	
	 
	/**
	 * Retrieves an integer configuration value from the game's configuration.
	 *
	 * Searches multiple configuration paths in a specific order:
	 * 1. Magazines path
	 * 2. Weapons path
	 * 3. Vehicles path
	 *
	 * If the configuration exists in one of these paths, the value is output and the function returns true.
	 *
	 * @param type The type/category of the configuration.
	 * @param varible The specific configuration variable name.
	 * @param value (Out) The retrieved integer value.
	 * @return bool True if the configuration value was found and output, false otherwise.
	 */
	static bool GetConfigInt(string type, string varible, out int value){
		
		if ( GetGame().ConfigIsExisting(  CFG_MAGAZINESPATH  + " " + type + " " + varible ) ){
			value = GetGame().ConfigGetInt(  CFG_MAGAZINESPATH  + " " + type + " " + varible);
			return true;
		}
		if ( GetGame().ConfigIsExisting(  CFG_WEAPONSPATH  + " " + type + " " + varible ) ){
			value = GetGame().ConfigGetInt(  CFG_WEAPONSPATH  + " " + type + " " + varible);
			return true;
		}
		if ( GetGame().ConfigIsExisting(  CFG_VEHICLESPATH + " " + type + " " + varible ) ){
			value = GetGame().ConfigGetInt( CFG_VEHICLESPATH + " " + type + " " + varible );
			return true;
		}
		return false;
	}
	 
	/**
	 * Retrieves a float configuration value from the game's configuration.
	 *
	 * Searches in the configuration paths of magazines, weapons, then vehicles.
	 * Outputs the value if found.
	 *
	 * @param type The type/category of the configuration.
	 * @param varible The specific configuration variable name.
	 * @param value (Out) The retrieved float value.
	 * @return bool True if the configuration value was found, false otherwise.
	 */
	static bool GetConfigFloat(string type, string varible, out float value){
		
		if ( GetGame().ConfigIsExisting(  CFG_MAGAZINESPATH  + " " + type + " " + varible ) ){
			value = GetGame().ConfigGetFloat(  CFG_MAGAZINESPATH  + " " + type + " " + varible);
			return true;
		}
		if ( GetGame().ConfigIsExisting(  CFG_WEAPONSPATH + " " + type + " " + varible ) ){
			value = GetGame().ConfigGetFloat( CFG_WEAPONSPATH + " " + type + " " + varible );
			return true;
		}
		if ( GetGame().ConfigIsExisting(  CFG_VEHICLESPATH + " " + type + " " + varible ) ){
			value = GetGame().ConfigGetFloat( CFG_VEHICLESPATH + " " + type + " " + varible );
			return true;
		}
		return false;
	}
	 
	/**
	 * Retrieves a string configuration value from the game's configuration.
	 *
	 * Checks multiple configuration paths (magazines, weapons, vehicles), and outputs the value if it exists.
	 *
	 * @param type The type/category of the configuration.
	 * @param varible The configuration variable name.
	 * @param value (Out) The retrieved configuration string.
	 * @return bool True if the configuration was found, false otherwise.
	 */
	static bool GetConfigString(string type, string varible, out string value){
		
		if ( GetGame().ConfigIsExisting(  CFG_MAGAZINESPATH  + " " + type + " " + varible ) ){
			return GetGame().ConfigGetText(  CFG_MAGAZINESPATH  + " " + type + " " + varible,value);
		}
		if ( GetGame().ConfigIsExisting(  CFG_WEAPONSPATH  + " " + type + " " + varible ) ){
			return GetGame().ConfigGetText(  CFG_WEAPONSPATH  + " " + type + " " + varible,value);
		}
		if ( GetGame().ConfigIsExisting(  CFG_VEHICLESPATH + " " + type + " " + varible ) ){
			return GetGame().ConfigGetText( CFG_VEHICLESPATH + " " + type + " " + varible,value);
		}
		return false;
	}
	 
	/**
	 * Retrieves a configuration value as an array of strings.
	 *
	 * Searches the specific configuration paths for magazines, weapons, and vehicles.
	 * If found, outputs the string array.
	 *
	 * @param type The type/category of the configuration.
	 * @param varible The specific configuration variable name.
	 * @param value (Out) The retrieved TStringArray of configuration values.
	 * @return bool True if the configuration value was found, false otherwise.
	 */
	static bool GetConfigTStringArray(string type, string varible, out TStringArray value){
		if ( GetGame().ConfigIsExisting(  CFG_MAGAZINESPATH  + " " + type + " " + varible ) ){
			GetGame().ConfigGetTextArray(  CFG_MAGAZINESPATH  + " " + type + " " + varible, value);
			return true;
		}
		if ( GetGame().ConfigIsExisting(  CFG_WEAPONSPATH  + " " + type + " " + varible ) ){
			GetGame().ConfigGetTextArray(  CFG_WEAPONSPATH  + " " + type + " " + varible, value);
			return true;
		}
		if ( GetGame().ConfigIsExisting(  CFG_VEHICLESPATH + " " + type + " " + varible ) ){
			GetGame().ConfigGetTextArray( CFG_VEHICLESPATH + " " + type + " " + varible, value);
			return true;
		}
		return false;
	}
	 
	/**
	 * Retrieves a configuration value as an array of floats.
	 *
	 * Looks into the configuration paths for magazines, weapons, and vehicles.
	 * Outputs the float array if the configuration exists.
	 *
	 * @param type The type/category of the configuration.
	 * @param varible The configuration variable name.
	 * @param value (Out) The retrieved TFloatArray of configuration values.
	 * @return bool True if the value was successfully retrieved, false otherwise.
	 */
	static bool GetConfigTFloatArray(string type, string varible, out TFloatArray value){
		if ( GetGame().ConfigIsExisting(  CFG_MAGAZINESPATH  + " " + type + " " + varible ) ){
			GetGame().ConfigGetFloatArray(  CFG_MAGAZINESPATH  + " " + type + " " + varible, value);
			return true;
		}
		if ( GetGame().ConfigIsExisting(  CFG_WEAPONSPATH  + " " + type + " " + varible ) ){
			GetGame().ConfigGetFloatArray(  CFG_WEAPONSPATH  + " " + type + " " + varible, value);
			return true;
		}
		if ( GetGame().ConfigIsExisting(  CFG_VEHICLESPATH + " " + type + " " + varible ) ){
			GetGame().ConfigGetFloatArray( CFG_VEHICLESPATH + " " + type + " " + varible, value);
			return true;
		}
		return false;
	}
	 
	/**
	 * Retrieves a configuration value as an array of integers.
	 *
	 * Checks magazines, weapons, and vehicles configuration paths.
	 * Outputs the integer array if the configuration exists.
	 *
	 * @param type The type/category of the configuration.
	 * @param varible The configuration variable name.
	 * @param value (Out) The retrieved TIntArray of configuration values.
	 * @return bool True if the configuration value was found, false otherwise.
	 */
	static bool GetConfigTIntArray(string type, string varible, out TIntArray value){
		if ( GetGame().ConfigIsExisting(  CFG_MAGAZINESPATH  + " " + type + " " + varible ) ){
			GetGame().ConfigGetIntArray(  CFG_MAGAZINESPATH  + " " + type + " " + varible, value);
			return true;
		}
		if ( GetGame().ConfigIsExisting(  CFG_WEAPONSPATH  + " " + type + " " + varible ) ){
			GetGame().ConfigGetIntArray(  CFG_WEAPONSPATH  + " " + type + " " + varible, value);
			return true;
		}
		if ( GetGame().ConfigIsExisting(  CFG_VEHICLESPATH + " " + type + " " + varible ) ){
			GetGame().ConfigGetIntArray( CFG_VEHICLESPATH + " " + type + " " + varible, value);
			return true;
		}
		return false;
	}
	
	
}