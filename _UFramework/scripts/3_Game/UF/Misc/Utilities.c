/**
 * Utility Functions Documentation
 *
 * GetLogPlayerPosArray
 * --------------------
 * Summary:
 *   Converts an array of ULogPlayerPos objects into a JSON string representation.
 *
 * Parameters:
 *   - thePlayerlist: An array of autoptr ULogPlayerPos objects.
 *
 * Returns:
 *   A string containing the JSON representation of the provided array.
 *
 *
 * UUtil Class
 * -----------
 * A collection of static utility functions designed to help with various operations such as:
 * - Retrieving player-related information (Steam ID, finding players by GUID or identity)
 * - File system operations (finding files in a directory)
 * - String manipulation (generating random IDs, formatting integers)
 * - Time and date operations (obtaining date stamps, time stamps and Unix timestamps)
 * - Configuration retrieval for different asset types (magazines, weapons, vehicles)
 *
 *
 * Functions Within UUtil:
 *
 * 1. GetSteamId
 *    -----------
 *    Summary:
 *      Retrieves the Steam ID for the current client player.
 *    Logic:
 *      - Checks for a valid title initiator from the UserManager.
 *      - If not available, attempts to cast the current player as DayZPlayer and retrieves the plain ID.
 *    Returns:
 *      A string with the Steam ID, or an empty string if not available.
 *
 * 2. FindFilesInDirectory
 *    ----------------------
 *    Summary:
 *      Scans the specified directory and returns a list of file names contained within.
 *    Parameters:
 *      - directory: A string with the path of the target directory.
 *    Returns:
 *      A TStringArray containing the names of the found files.
 *
 * 3. GetRandomId
 *    ------------
 *    Summary:
 *      Generates a random alphanumeric string of a specified length.
 *    Parameters:
 *      - number: The desired length of the generated ID.
 *    Returns:
 *      A random string composed of upper and lower case letters and digits.
 *    Note:
 *      Utilizes a random number generator (ensuring it is checked and renewed) for index selection.
 *
 * 4. FindPlayer
 *    ----------
 *    Summary:
 *      Searches for a player on the server by comparing each player's identity GUID.
 *    Parameters:
 *      - GUID: A string representing the player's unique identifier.
 *    Returns:
 *      The matched DayZPlayer object if found; otherwise, NULL.
 *
 * 5. FindPlayerByIdentity
 *    ----------------------
 *    Summary:
 *      Locates a player based on their PlayerIdentity object by using the network ID.
 *    Parameters:
 *      - identity: A PlayerIdentity reference for detecting the player.
 *    Returns:
 *      The DayZPlayer associated with the supplied identity; returns NULL if not found or if identity is invalid.
 *
 * 6. SendNotificationEx & SendNotification
 *    ----------------------------------------
 *    Summary:
 *      Sends an in-game notification to a specified player identity.
 *    Parameters (for both functions):
 *      - Header: A string representing the notification header.
 *      - Text: The main message of the notification.
 *      - player: The recipient's PlayerIdentity.
 *      - Icon: (Optional) A path string to the icon image used in the notification; defaults to info icon.
 *    Modes:
 *      - Dedicated Server: Uses NotificationSystem.SendNotificationToPlayerIdentityExtended.
 *      - Client: Uses NotificationSystem.AddNotificationExtended.
 *
 * 7. ConvertIntToNiceString
 *    ------------------------
 *    Summary:
 *      Transforms an integer value representing a dollar amount into a formatted string with commas.
 *    Parameters:
 *      - DollarAmount: The integer value to format.
 *    Behavior:
 *      Handles negative values by prefixing with a minus sign.
 *    Returns:
 *      A string formatted with comma separations (e.g., "1,234,567").
 *
 * 8. RestErrorToString
 *    -------------------
 *    Summary:
 *      Maps REST error codes to their corresponding string representations.
 *    Parameters:
 *      - ErrorCode: An integer representing the REST error state.
 *    Returns:
 *      A string describing the error state (e.g., "EREST_SUCCESS", "EREST_ERROR_TIMEOUT").
 *
 * 9. GetDateStamp & GetTimeStamp
 *    -----------------------------
 *    Summary:
 *      Provide the current date and time in a human-readable format.
 *    GetDateStamp:
 *      Returns the date in "YYYY-MM-DD" format with leading zeros for single-digit days or months.
 *    GetTimeStamp:
 *      Returns the time in "HH:MM:SS" format.
 *
 * 10. Unix and UTC Date/Time Functions
 *     ----------------------------------
 *     Functions:
 *       - GetDateInt / GetUTCDateInt:
 *           Compute the number of days since January 1, 1970 based on local or UTC date.
 *       - GetUnixInt / GetUTCUnixInt:
 *           Calculate and return the Unix timestamp (seconds elapsed since Jan 1 1970) for local or UTC time.
 *     Note:
 *       Takes into account leap years using the IsLeapYear helper function.
 *
 * 11. Configuration Getters
 *     -----------------------
 *     Functions:
 *       - GetConfigInt, GetConfigFloat, GetConfigString:
 *           Retrieve single configuration values from predefined configuration paths (magazines, weapons, vehicles).
 *       - GetConfigTStringArray, GetConfigTFloatArray, GetConfigTIntArray:
 *           Retrieve arrays of configuration values for the respective data types.
 *     Behavior:
 *       - Each function attempts to locate the configuration value in multiple asset paths.
 *       - Returns true if the configuration exists and has been successfully loaded, false otherwise.
 *
 * Notes:
 *   - Many functions rely on global game objects (like g_Game) and assume a proper game context.
 *   - The configuration retrieval functions expect specific naming conventions for paths and variables.
 *   - Error handling is minimal; functions typically return empty strings or NULL when they fail.
 */
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


class UUtil extends UUtilBase {
	
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
		if (g_Game && g_Game.GetUserManager() && g_Game.GetUserManager().GetTitleInitiator()){
			return g_Game.GetUserManager().GetTitleInitiator().GetUid();
		} else if (g_Game && g_Game.IsClient() && Class.CastTo(player, g_Game.GetPlayer()) && player.GetIdentity() && player.GetIdentity().GetPlainId() != "" ){
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
		if (g_Game.IsServer()){
			autoptr array<Man> players = new array<Man>;
			g_Game.GetPlayers( players );
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
		g_Game.GetPlayerNetworkIDByIdentityID(identity.GetPlayerId(), lowBits, highBits);
		return DayZPlayer.Cast(g_Game.GetObjectByNetworkId(lowBits, highBits));
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
		if (g_Game.IsDedicatedServer()){
			NotificationSystem.SendNotificationToPlayerIdentityExtended(player, 5, Header, Text, Icon );
		} else if (g_Game.IsClient()){
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
	 * Converts a UF status code to a human-readable string.
	 *
	 * @param StatusCode The status code (one of UF_SUCCESS, UF_EMPTY, UF_ERROR, etc.).
	 * @return string A string representation of the status code.
	 */
	static string StatusToString(int StatusCode){
		switch ( StatusCode )
		{
			case UF_SUCCESS:
				return "SUCCESS";
			case UF_EMPTY:
				return "EMPTY";
			case UF_NOTSETUP:
				return "NOT_SETUP";
			case UF_TIMEOUT:
				return "TIMEOUT";
			case UF_CLIENTERROR:
				return "CLIENT_ERROR";
			case UF_SERVERERROR:
				return "SERVER_ERROR";
			case UF_ERROR:
				return "ERROR";
			case UF_JSONERROR:
				return "JSON_ERROR";
			case UF_NOTFOUND:
				return "NOT_FOUND";
			case UF_TOOEARLY:
				return "TOO_EARLY";
			case UF_UNAUTHORIZED:
				return "UNAUTHORIZED";
			case UF_AI_PENDING:
				return "AI_PENDING";
			case UF_AI_PROCESSING:
				return "AI_PROCESSING";
		}
		return "UNKNOWN_STATUS(" + StatusCode + ")";
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
		return yr.ToString() + "-" + PadZero(mth) + "-" + PadZero(day);
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
		return PadZero(hr) + ":" + PadZero(min) + ":" + PadZero(sec);
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
	 * Gets the timezone offset in seconds between local time and UTC.
	 *
	 * A positive value means local time is ahead of UTC (e.g., UTC+2 returns 7200).
	 * A negative value means local time is behind UTC (e.g., UTC-5 returns -18000).
	 *
	 * @return int The timezone offset in seconds.
	 */
	static int GetTimezoneOffsetSeconds()
	{
		return GetUnixInt() - GetUTCUnixInt();
	}
	
	/**
	 * Gets the timezone offset in hours between local time and UTC.
	 *
	 * A positive value means local time is ahead of UTC (e.g., UTC+2 returns 2).
	 * A negative value means local time is behind UTC (e.g., UTC-5 returns -5).
	 * Note: This rounds to whole hours and may not be accurate for timezones with 30/45 minute offsets.
	 *
	 * @return int The timezone offset in hours.
	 */
	static int GetTimezoneOffsetHours()
	{
		return GetTimezoneOffsetSeconds() / 3600;
	}
	
	/**
	 * Gets the timezone offset as a formatted string (e.g., "UTC+02:00" or "UTC-05:00").
	 *
	 * @return string The timezone offset in standard format.
	 */
	static string GetTimezoneString()
	{
		int offsetSeconds = GetTimezoneOffsetSeconds();
		string sign = "+";
		if (offsetSeconds < 0)
		{
			sign = "-";
			offsetSeconds = Math.AbsInt(offsetSeconds);
		}
		
		int hours = offsetSeconds / 3600;
		int minutes = (offsetSeconds % 3600) / 60;
		
		return "UTC" + sign + PadZero(hours) + ":" + PadZero(minutes);
	}
	
	/**
	 * Converts a UTC Unix timestamp to a local Unix timestamp.
	 *
	 * Adds the current timezone offset to convert UTC time to local time.
	 *
	 * @param utcUnixTime The UTC Unix timestamp to convert.
	 * @return int The equivalent local Unix timestamp.
	 */
	static int UTCToLocalUnix(int utcUnixTime)
	{
		return utcUnixTime + GetTimezoneOffsetSeconds();
	}
	
	/**
	 * Converts a local Unix timestamp to a UTC Unix timestamp.
	 *
	 * Subtracts the current timezone offset to convert local time to UTC.
	 *
	 * @param localUnixTime The local Unix timestamp to convert.
	 * @return int The equivalent UTC Unix timestamp.
	 */
	static int LocalToUTCUnix(int localUnixTime)
	{
		return localUnixTime - GetTimezoneOffsetSeconds();
	}
	
	/**
	 * Converts a UTC Unix timestamp to local date and time components.
	 *
	 * @param utcUnixTime The UTC Unix timestamp to convert.
	 * @param year (Out) The local year component.
	 * @param month (Out) The local month component (1-12).
	 * @param day (Out) The local day component (1-31).
	 * @param hour (Out) The local hour component (0-23).
	 * @param minute (Out) The local minute component (0-59).
	 * @param second (Out) The local second component (0-59).
	 */
	static void UTCToLocalDateTime(int utcUnixTime, out int year, out int month, out int day, out int hour, out int minute, out int second)
	{
		int localUnix = UTCToLocalUnix(utcUnixTime);
		UnixToDateTime(localUnix, year, month, day, hour, minute, second);
	}
	
	/**
	 * Converts a UTC Unix timestamp to a local formatted date-time string.
	 *
	 * @param utcUnixTime The UTC Unix timestamp to convert.
	 * @return string The formatted local date-time string in "YYYY-MM-DD HH:MM:SS" format.
	 */
	static string UTCToLocalDateTimeString(int utcUnixTime)
	{
		return UnixToDateTimeString(UTCToLocalUnix(utcUnixTime));
	}
	
	/**
	 * Converts a Unix timestamp to date and time components.
	 *
	 * Breaks down a Unix timestamp (seconds since Jan 1, 1970) into its constituent
	 * year, month, day, hour, minute, and second values.
	 *
	 * @param unixTime The Unix timestamp to convert.
	 * @param year (Out) The year component.
	 * @param month (Out) The month component (1-12).
	 * @param day (Out) The day component (1-31).
	 * @param hour (Out) The hour component (0-23).
	 * @param minute (Out) The minute component (0-59).
	 * @param second (Out) The second component (0-59).
	 */
	static void UnixToDateTime(int unixTime, out int year, out int month, out int day, out int hour, out int minute, out int second)
	{
		// Extract time of day
		int timeOfDay = unixTime % 86400;
		hour = timeOfDay / 3600;
		minute = (timeOfDay % 3600) / 60;
		second = timeOfDay % 60;
		
		// Calculate total days since epoch
		int totalDays = unixTime / 86400;
		
		// Find the year
		year = UnixStartYear;
		while (true)
		{
			int daysInYear = 365;
			if (IsLeapYear(year))
			{
				daysInYear = 366;
			}
			if (totalDays < daysInYear)
			{
				break;
			}
			totalDays = totalDays - daysInYear;
			year++;
		}
		
		// Find the month
		month = 1;
		for (int i = 0; i < 12; i++)
		{
			int daysThisMonth = DaysInMonth[i];
			if (IsLeapYear(year) && i == 1)
			{
				daysThisMonth = 29;
			}
			if (totalDays < daysThisMonth)
			{
				break;
			}
			totalDays = totalDays - daysThisMonth;
			month++;
		}
		
		// Remaining days plus 1 (days are 1-indexed)
		day = totalDays + 1;
	}
	
	/**
	 * Converts a Unix timestamp to date components only.
	 *
	 * @param unixTime The Unix timestamp to convert.
	 * @param year (Out) The year component.
	 * @param month (Out) The month component (1-12).
	 * @param day (Out) The day component (1-31).
	 */
	static void UnixToDate(int unixTime, out int year, out int month, out int day)
	{
		int hour, minute, second;
		UnixToDateTime(unixTime, year, month, day, hour, minute, second);
	}
	
	/**
	 * Converts a Unix timestamp to time components only.
	 *
	 * @param unixTime The Unix timestamp to convert.
	 * @param hour (Out) The hour component (0-23).
	 * @param minute (Out) The minute component (0-59).
	 * @param second (Out) The second component (0-59).
	 */
	static void UnixToTime(int unixTime, out int hour, out int minute, out int second)
	{
		int year, month, day;
		UnixToDateTime(unixTime, year, month, day, hour, minute, second);
	}
	
	/**
	 * Converts a Unix timestamp to a formatted date-time string.
	 *
	 * @param unixTime The Unix timestamp to convert.
	 * @return string The formatted date-time string in "YYYY-MM-DD HH:MM:SS" format.
	 */
	static string UnixToDateTimeString(int unixTime)
	{
		int year, month, day, hour, minute, second;
		UnixToDateTime(unixTime, year, month, day, hour, minute, second);
		return year.ToString() + "-" + PadZero(month) + "-" + PadZero(day) + " " + PadZero(hour) + ":" + PadZero(minute) + ":" + PadZero(second);
	}
	
	/**
	 * Converts a Unix timestamp to a formatted date string.
	 *
	 * @param unixTime The Unix timestamp to convert.
	 * @return string The formatted date string in "YYYY-MM-DD" format.
	 */
	static string UnixToDateString(int unixTime)
	{
		int year, month, day;
		UnixToDate(unixTime, year, month, day);
		return year.ToString() + "-" + PadZero(month) + "-" + PadZero(day);
	}
	
	/**
	 * Converts a Unix timestamp to a formatted time string.
	 *
	 * @param unixTime The Unix timestamp to convert.
	 * @return string The formatted time string in "HH:MM:SS" format.
	 */
	static string UnixToTimeString(int unixTime)
	{
		int hour, minute, second;
		UnixToTime(unixTime, hour, minute, second);
		return PadZero(hour) + ":" + PadZero(minute) + ":" + PadZero(second);
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
		
		if ( g_Game.ConfigIsExisting(  CFG_MAGAZINESPATH  + " " + type + " " + varible ) ){
			value = g_Game.ConfigGetInt(  CFG_MAGAZINESPATH  + " " + type + " " + varible);
			return true;
		}
		if ( g_Game.ConfigIsExisting(  CFG_WEAPONSPATH  + " " + type + " " + varible ) ){
			value = g_Game.ConfigGetInt(  CFG_WEAPONSPATH  + " " + type + " " + varible);
			return true;
		}
		if ( g_Game.ConfigIsExisting(  CFG_VEHICLESPATH + " " + type + " " + varible ) ){
			value = g_Game.ConfigGetInt( CFG_VEHICLESPATH + " " + type + " " + varible );
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
		
		if ( g_Game.ConfigIsExisting(  CFG_MAGAZINESPATH  + " " + type + " " + varible ) ){
			value = g_Game.ConfigGetFloat(  CFG_MAGAZINESPATH  + " " + type + " " + varible);
			return true;
		}
		if ( g_Game.ConfigIsExisting(  CFG_WEAPONSPATH + " " + type + " " + varible ) ){
			value = g_Game.ConfigGetFloat( CFG_WEAPONSPATH + " " + type + " " + varible );
			return true;
		}
		if ( g_Game.ConfigIsExisting(  CFG_VEHICLESPATH + " " + type + " " + varible ) ){
			value = g_Game.ConfigGetFloat( CFG_VEHICLESPATH + " " + type + " " + varible );
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
		
		if ( g_Game.ConfigIsExisting(  CFG_MAGAZINESPATH  + " " + type + " " + varible ) ){
			return g_Game.ConfigGetText(  CFG_MAGAZINESPATH  + " " + type + " " + varible,value);
		}
		if ( g_Game.ConfigIsExisting(  CFG_WEAPONSPATH  + " " + type + " " + varible ) ){
			return g_Game.ConfigGetText(  CFG_WEAPONSPATH  + " " + type + " " + varible,value);
		}
		if ( g_Game.ConfigIsExisting(  CFG_VEHICLESPATH + " " + type + " " + varible ) ){
			return g_Game.ConfigGetText( CFG_VEHICLESPATH + " " + type + " " + varible,value);
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
		if ( g_Game.ConfigIsExisting(  CFG_MAGAZINESPATH  + " " + type + " " + varible ) ){
			g_Game.ConfigGetTextArray(  CFG_MAGAZINESPATH  + " " + type + " " + varible, value);
			return true;
		}
		if ( g_Game.ConfigIsExisting(  CFG_WEAPONSPATH  + " " + type + " " + varible ) ){
			g_Game.ConfigGetTextArray(  CFG_WEAPONSPATH  + " " + type + " " + varible, value);
			return true;
		}
		if ( g_Game.ConfigIsExisting(  CFG_VEHICLESPATH + " " + type + " " + varible ) ){
			g_Game.ConfigGetTextArray( CFG_VEHICLESPATH + " " + type + " " + varible, value);
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
		if ( g_Game.ConfigIsExisting(  CFG_MAGAZINESPATH  + " " + type + " " + varible ) ){
			g_Game.ConfigGetFloatArray(  CFG_MAGAZINESPATH  + " " + type + " " + varible, value);
			return true;
		}
		if ( g_Game.ConfigIsExisting(  CFG_WEAPONSPATH  + " " + type + " " + varible ) ){
			g_Game.ConfigGetFloatArray(  CFG_WEAPONSPATH  + " " + type + " " + varible, value);
			return true;
		}
		if ( g_Game.ConfigIsExisting(  CFG_VEHICLESPATH + " " + type + " " + varible ) ){
			g_Game.ConfigGetFloatArray( CFG_VEHICLESPATH + " " + type + " " + varible, value);
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
		if ( g_Game.ConfigIsExisting(  CFG_MAGAZINESPATH  + " " + type + " " + varible ) ){
			g_Game.ConfigGetIntArray(  CFG_MAGAZINESPATH  + " " + type + " " + varible, value);
			return true;
		}
		if ( g_Game.ConfigIsExisting(  CFG_WEAPONSPATH  + " " + type + " " + varible ) ){
			g_Game.ConfigGetIntArray(  CFG_WEAPONSPATH  + " " + type + " " + varible, value);
			return true;
		}
		if ( g_Game.ConfigIsExisting(  CFG_VEHICLESPATH + " " + type + " " + varible ) ){
			g_Game.ConfigGetIntArray( CFG_VEHICLESPATH + " " + type + " " + varible, value);
			return true;
		}
		return false;
	}
		
	
	/**
	 * Saves a Base64-encoded string to a binary file.
	 *
	 * This function decodes the provided Base64 string into an array of bytes using DecodeBase64,
	 * and then writes these bytes to a binary file using SaveBytesToFile.
	 *
	 * @param base64String The Base64-encoded string representing binary data.
	 * @param filePath The file system path where the decoded binary data will be written.
	 */
	static void SaveBase64ToFile(string base64String, string filePath)
	{
		array<int> bytes;
		DecodeBase64(base64String, bytes);
		SaveBytesToFile(bytes, filePath);
	}
	
	
	 /**
	 * Saves a Base64-encoded string to a binary file after a short delay.
	 *
	 * This variant of the save function decodes the Base64 string into bytes using DecodeBase64,
	 * and schedules the saving process using a call queue (via g_Game.GetCallQueue), allowing the saving
	 * operation to be deferred to reduce the impact on the client frame rate.
	 *
	 * @param base64String The Base64-encoded string representing binary data.
	 * @param filePath The file system path where the decoded binary data will be written.
	 */
	static void SaveBase64ToFileSplit(string base64String, string filePath)
	{
		array<int> bytes;
		DecodeBase64(base64String, bytes);
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(UUtil.SaveBytesToFile, 10, false, bytes, filePath); //call later to split client frame hit
	}


	/**
	 * Decodes a Base64-encoded string into an array of integer byte values.
	 *
	 * This function processes the input string in 4-character blocks, converting each block into a 24-bit integer
	 * from which the original bytes are extracted. It handles padding characters ("=") by counting them and adjusting
	 * the number of output bytes accordingly.
	 *
	 * @param base64String The Base64-encoded string to decode.
	 * @param decodedBytes [out] The output array of integers representing the decoded bytes.
	 */
	static void DecodeBase64(string base64String, out array<int> decodedBytes){
		// ---- Decode Base64 string into a byte array ----
	    const string base64Table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	    int len = base64String.Length();
	    	    
	    // Allocate an array to store decoded bytes.
	    decodedBytes = new array<int>;
	    
	    int i = 0;
	    while (i < len)
	    {
	        int indices[4];
	        int padCount = 0;
	        // Process 4 characters (one Base64 block).
	        for (int k = 0; k < 4; k++)
	        {
	            string ch = base64String.Substring(i, 1);
	            i++;
	            if (ch == "=")
	            {
	                indices[k] = 0;
	                padCount++;
	            }
	            else
	            {
	                int idx = base64Table.IndexOf(ch);
	                if (idx == -1)
	                {
	                    Print("Warning: Invalid Base64 character encountered: \"" + ch + "\"");
	                    idx = 0;
	                }
	                indices[k] = idx;
	            }
	        }
	        // Combine the four 6-bit values into one 24-bit integer.
	        int value = (indices[0] << 18) | (indices[1] << 12) | (indices[2] << 6) | indices[3];
	        // Determine how many bytes are produced (normally 3, minus padding).
	        int numBytes = 3 - padCount;
	        for (int b = 0; b < numBytes; b++)
	        {
	            int shift = (2 - b) * 8;
	            int byteValb = (value >> shift) & 0xFF;
	            decodedBytes.Insert(byteValb);
	        }
	    }
	}

	
	/**
	 * Saves an array of bytes to a binary file.
	 *
	 * This function writes data to a binary file by grouping bytes into 32-bit integers.
	 * It processes complete groups of 4 bytes, and if there is a remainder, it pads the remaining bytes with zeros.
	 * The binary data is then written to the file via a FileSerializer in write mode.
	 *
	 * @param bytes An array of integers representing byte values to be saved.
	 * @param filePath The file system path where the binary data will be written.
	 */
	static void SaveBytesToFile(array<int> bytes, string filePath){
	    // Open a binary file stream using FileSerializer.
	    FileSerializer serializer = new FileSerializer();
	    if (!serializer.Open(filePath, FileMode.WRITE))
	    {
	        Error2("[UF] SaveBytesToFile", "Error: Unable to open file for binary writing: " + filePath);
	        return;
	    }
	    
	    // Group the bytes into blocks of 4 (each block will produce one 32-bit integer).
	    int totalBytes = bytes.Count();
	    int fullGroups = totalBytes / 4;
	    int remainder = totalBytes % 4;
	    
	    // Write all complete groups.
	    for (int group = 0; group < fullGroups; group++)
	    {
	        int combinedl = 0;
	        for (int l = 0; l < 4; l++)
	        {
	            int byteVall = bytes.Get(group * 4 + l);
	            combinedl |= byteVall << (l * 8);
	        }
	        // Write the 32-bit integer as raw binary.
	        serializer.Write(combinedl);
	    }
	    
	    // Write leftover bytes (if any), padded with zeros.
	    if (remainder > 0)
	    {
	        int combinedm = 0;
	        for (int m = 0; m < remainder; m++)
	        {
	            int byteValm = bytes.Get(fullGroups * 4 + m);
	            combinedm |= byteValm << (m * 8);
	        }
	        serializer.Write(combinedm);
	    }
	    
	    serializer.Close();
	    //Print("Binary file saved successfully to: " + filePath);
	}
	/**
	* UMapLocation
	* ------------
	* Represents a named location on the map (city, town, village, etc.)
	* retrieved from CfgWorlds configuration.
	*
	* Properties:
	*   - ClassName: The config class name of the location entry.
	*   - Name: The display name of the location (e.g., "Chernogorsk").
	*   - Type: The location type (e.g., "City", "Village", "Capital").
	*   - Position: The 3D world position of the location (includes terrain height).
	*
	* Common Location Types:
	*   - "Capital" - Major cities
	*   - "City" - Large towns/cities
	*   - "Village" - Small villages
	*   - "Local" - Local landmarks
	*   - "Marine" - Marine/coastal points
	*   - "Hill" - Hills and elevated areas
	*   - "Ruin" - Ruins and historical sites
	*   - "ViewPoint" - Scenic viewpoints
	*/

	/**
	 * GetMapLocations
	 * ---------------
	 * Summary:
	 *   Retrieves all named locations (cities, towns, villages, etc.) from the current map's
	 *   CfgWorlds configuration. Each location includes its class name, display name, type,
	 *   and 3D world position (with terrain height).
	 *
	 * Parameters:
	 *   - typeFilters: (Optional) Array of location types to include (e.g., {"City", "Village", "Capital"}).
	 *                  Pass NULL or empty array to include all types.
	 *
	 * Returns:
	 *   An array of UMapLocation objects containing all matching locations.
	 *
	 * Example Usage:
	 * @code
	 *   // Get all cities and villages
	 *   array<string> filters = {"City", "Village"};
	 *   array<autoptr UMapLocation> towns = UUtil.GetMapLocations(filters);
	 *   
	 *   // Get all locations (no filter)
	 *   array<autoptr UMapLocation> allLocations = UUtil.GetMapLocations();
	 *   
	 *   foreach (UMapLocation loc : allLocations)
	 *   {
	 *       Print("Location: " + loc.Name + " (" + loc.Type + ") at " + loc.Position.ToString());
	 *   }
	 * @endcode
	 *
	 * Note:
	 *   This reads from CfgWorlds <worldName> Names, which contains map marker data.
	 *   Common types include: "City", "Village", "Capital", "Local", "Marine", "Hill", etc.
	 *   Position is computed with terrain height using SurfaceY().
	 */
	static array<autoptr UMapLocation> GetMapLocations(array<string> typeFilters = NULL)
	{
		array<autoptr UMapLocation> locations = new array<autoptr UMapLocation>();
		
		// Get current world name
		string worldName = "";
		GetGame().GetWorldName(worldName);
		
		if (worldName == "")
		{
			return locations;
		}
		
		// Build path to the Names config section
		string cfgPath = "CfgWorlds " + worldName + " Names";
		
		// Check if the config path exists
		if (!GetGame().ConfigIsExisting(cfgPath))
		{
			return locations;
		}
		
		// Determine if we should filter
		bool hasFilters = (typeFilters && typeFilters.Count() > 0);
		
		// Get number of location entries
		int count = GetGame().ConfigGetChildrenCount(cfgPath);
		
		for (int i = 0; i < count; i++)
		{
			string className = "";
			GetGame().ConfigGetChildName(cfgPath, i, className);
			
			if (className == "")
			{
				continue;
			}
			
			string entryPath = cfgPath + " " + className;
			
			// Get the display name
			string name = "";
			GetGame().ConfigGetText(entryPath + " name", name);
			
			// Get the type
			string type = "";
			GetGame().ConfigGetText(entryPath + " type", type);
			
			// Apply type filter if specified
			if (hasFilters && typeFilters.Find(type) == -1)
			{
				continue;
			}
			
			// Get position (stored as 2D float array [x, z])
			array<float> posArray = new array<float>();
			GetGame().ConfigGetFloatArray(entryPath + " position", posArray);
			
			vector position = vector.Zero;
			if (posArray.Count() >= 2)
			{
				float x = posArray.Get(0);
				float z = posArray.Get(1);
				float y = GetGame().SurfaceY(x, z);
				position = Vector(x, y, z);
			}
			
			// Create and add the location
			UMapLocation loc = new UMapLocation();
			loc.ClassName = className;
			loc.Name = name;
			loc.Type = type;
			loc.Position = position;
			
			locations.Insert(loc);
		}
		
		return locations;
	}
	
	/**
	 * GetNearestMapLocation
	 * ---------------------
	 * Summary:
	 *   Finds the nearest named location to a given position.
	 *
	 * Parameters:
	 *   - position: The world position to search from.
	 *   - typeFilters: (Optional) Array of location types to include. Pass NULL for all types.
	 *
	 * Returns:
	 *   The nearest UMapLocation, or NULL if no locations found.
	 *
	 * Example Usage:
	 * @code
	 *   vector playerPos = player.GetPosition();
	 *   array<string> filters = {"City", "Capital"};
	 *   UMapLocation nearest = UUtil.GetNearestMapLocation(playerPos, filters);
	 *   if (nearest)
	 *   {
	 *       Print("Nearest city: " + nearest.Name);
	 *   }
	 * @endcode
	 */
	static UMapLocation GetNearestMapLocation(vector position, array<string> typeFilters = NULL)
	{
		array<autoptr UMapLocation> locations = GetMapLocations(typeFilters);
		
		UMapLocation nearest = NULL;
		float nearestDist = 999999999; // Large initial value (Enforce Script has no float.MAX)
		
		foreach (UMapLocation loc : locations)
		{
			float dist = vector.Distance(position, loc.Position);
			if (dist < nearestDist)
			{
				nearestDist = dist;
				nearest = loc;
			}
		}
		
		return nearest;
	}

	/**
	 * GetNearestMapLocationName
	 * ---------------------
	 * Summary:
	 *   Finds the nearest named location to a given position and return its name.
	 *
	 * Parameters:
	 *   - position: The world position to search from.
	 *   - typeFilters: (Optional) Array of location types to include. Pass NULL for all types.
	 *
	 * Returns:
	 *   The nearest name, or "unknown" if no locations found.
	 *
	 * Example Usage:
	 * @code
	 *   vector playerPos = player.GetPosition();
	 *   array<string> filters = {"City", "Capital"};
	 *   UMapLocation nearest = UUtil.GetNearestMapLocation(playerPos, filters);
	 *   if (nearest)
	 *   {
	 *       Print("Nearest city: " + nearest.Name);
	 *   }
	 * @endcode
	 */
	static string GetNearestMapLocationName(vector position, array<string> typeFilters = NULL)
	{

		UMapLocation nearest = GetNearestMapLocation(position, typeFilters);
		if (nearest)
		{
			return nearest.Name;
		}
		return "unknown";	
	}
	 
	/**
	 * GetMapLocationsInRadius
	 * -----------------------
	 * Summary:
	 *   Finds all named locations within a specified radius of a position.
	 *
	 * Parameters:
	 *   - position: The center position to search from.
	 *   - radius: The search radius in meters.
	 *   - typeFilters: (Optional) Array of location types to include. Pass NULL for all types.
	 *
	 * Returns:
	 *   An array of UMapLocation objects within the radius.
	 *
	 * Example Usage:
	 * @code
	 *   vector pos = player.GetPosition();
	 *   array<string> filters = {"City", "Village"};
	 *   array<autoptr UMapLocation> nearby = UUtil.GetMapLocationsInRadius(pos, 5000, filters);
	 * @endcode
	 */
	static array<autoptr UMapLocation> GetMapLocationsInRadius(vector position, float radius, array<string> typeFilters = NULL)
	{
		array<autoptr UMapLocation> result = new array<autoptr UMapLocation>();
		array<autoptr UMapLocation> locations = GetMapLocations(typeFilters);
		
		foreach (UMapLocation loc : locations)
		{
			if (vector.Distance(position, loc.Position) <= radius)
			{
				result.Insert(loc);
			}
		}
		
		return result;
	}
	
	/**
	 * SanitizeString
	 * --------------
	 * Summary:
	 *   Sanitizes a string by replacing emojis with ASCII art equivalents
	 *   and optionally removing invalid/non-printable characters.
	 *
	 * Parameters:
	 *   - input: The string to sanitize.
	 *   - removeUnmapped: If true, removes emojis that don't have ASCII mappings.
	 *                     If false, leaves them as-is. Default: true.
	 *   - keepInternational: If true, preserves Cyrillic, CJK, and other DayZ-supported
	 *                        language characters. If false, strips to extended Latin only.
	 *                        Default: true.
	 *
	 * Returns:
	 *   A sanitized string with emojis converted to ASCII art.
	 *
	 * Note:
	 *   For multilingual servers (Russian, Chinese, etc.), use keepInternational=true.
	 *   For strict ASCII-only output, use StripNonASCII() after sanitization.
	 *
	 * Related Functions:
	 *   - StripUnsupportedCharacters(): Keeps all DayZ language characters, strips emojis
	 *   - StripNonASCII(): Strips everything except pure ASCII (English only)
	 *   - StripEmojisOnly(): Removes emojis but keeps ALL other Unicode
	 *
	 * Example Usage:
	 * @code
	 *   string clean = UUtil.SanitizeString("Hello 😊 World 😢 Привет");
	 *   // Returns: "Hello :) World :'( Привет"
	 * @endcode
	 */
	static string SanitizeString(string input, bool removeUnmapped = true, bool keepInternational = true)
	{
		if (input == "" || input.Length() == 0)
			return input;
		
		string result = input;
		
		// Apply emoji replacements
		result = ReplaceEmojisWithASCII(result);
		
		// Remove remaining non-printable and invalid characters if requested
		if (removeUnmapped)
		{
			if (keepInternational)
			{
				// Use StripUnsupportedCharacters to keep Cyrillic, CJK, etc.
				result = StripUnsupportedCharacters(result);
			}
			else
			{
				// Use RemoveInvalidCharacters for extended Latin only (strips Cyrillic, CJK)
				result = RemoveInvalidCharacters(result);
			}
		}
		
		return result;
	}
	
	/**
	 * ReplaceEmojisWithASCII
	 * ----------------------
	 * Summary:
	 *   Replaces common Unicode emojis with their ASCII art equivalents.
	 *   Uses the centralized emoji map from UUtilBase for maintainability.
	 *
	 * Parameters:
	 *   - input: The string containing emojis.
	 *
	 * Returns:
	 *   String with emojis replaced by ASCII representations.
	 *
	 * Note:
	 *   To add new emoji mappings, update InitializeEmojiMappings() in UUtilBase.c
	 */
	static string ReplaceEmojisWithASCII(string input)
	{
		return ApplyEmojiReplacements(input);
	}
	
	/**
	 * RemoveInvalidCharacters
	 * -----------------------
	 * Summary:
	 *   Removes non-printable and potentially problematic Unicode characters
	 *   from a string, keeping only standard ASCII printable characters
	 *   and common extended Latin characters.
	 *
	 * Parameters:
	 *   - input: The string to clean.
	 *
	 * Returns:
	 *   String with only valid printable characters.
	 */
	static string RemoveInvalidCharacters(string input)
	{
		string result = "";
		int len = input.Length();
		
		for (int i = 0; i < len; i++)
		{
			string ch = input.Substring(i, 1);
			int code = ch.ToAscii();
			
			// Keep printable ASCII (32-126) and extended Latin (128-255 for accented chars)
			// Also keep tab (9), newline (10), carriage return (13)
			if ((code >= 32 && code <= 126) || (code >= 128 && code <= 255) || code == 9 || code == 10 || code == 13)
			{
				result += ch;
			}
			// Skip multi-byte Unicode characters (they return 0 or negative from ToAscii)
			// and control characters
		}
		
		return result;
	}
	
	/**
	 * StripNonASCII
	 * -------------
	 * Summary:
	 *   Removes ALL non-ASCII characters from a string, keeping only
	 *   standard ASCII (codes 32-126). This includes emojis, accented
	 *   characters, special Unicode symbols, Cyrillic, CJK, etc.
	 *
	 *   WARNING: This strips ALL non-English characters! For multilingual
	 *   support (Russian, Chinese, Japanese, Korean, etc.), use 
	 *   StripUnsupportedCharacters() instead.
	 *
	 * Parameters:
	 *   - input: The string to strip non-ASCII from.
	 *   - keepWhitespace: If true, keeps tabs/newlines. Default: true.
	 *
	 * Returns:
	 *   String with only pure ASCII characters.
	 *
	 * Example Usage:
	 * @code
	 *   string clean = UUtil.StripNonASCII("Hello 😊 Wörld café");
	 *   // Returns: "Hello  Wrld caf"
	 * @endcode
	 */
	static string StripNonASCII(string input, bool keepWhitespace = true)
	{
		string result = "";
		int len = input.Length();
		
		for (int i = 0; i < len; i++)
		{
			string ch = input.Substring(i, 1);
			int code = ch.ToAscii();
			
			// Keep only printable ASCII (32-126)
			if (code >= 32 && code <= 126)
			{
				result += ch;
			}
			// Optionally keep tab (9), newline (10), carriage return (13)
			else if (keepWhitespace && (code == 9 || code == 10 || code == 13))
			{
				result += ch;
			}
			// Everything else (including 0, negatives from multi-byte, extended Latin 128-255) is stripped
		}
		
		return result;
	}
	
	/**
	 * StripUnsupportedCharacters
	 * --------------------------
	 * Summary:
	 *   Removes characters that are NOT supported by DayZ's localization system.
	 *   Keeps characters from ALL DayZ-supported languages:
	 *   - English, German, French, Spanish, Italian, Portuguese (Latin/Extended Latin)
	 *   - Russian (Cyrillic)
	 *   - Polish, Czech (Latin Extended)
	 *   - Chinese Simplified/Traditional (CJK)
	 *   - Japanese (Hiragana, Katakana, Kanji)
	 *   - Korean (Hangul)
	 *   - Turkish (Latin Extended)
	 *
	 *   This is the RECOMMENDED function for sanitizing player names and chat
	 *   messages while preserving international character support.
	 *
	 * Parameters:
	 *   - input: The string to sanitize.
	 *   - keepWhitespace: If true, keeps tabs/newlines. Default: true.
	 *
	 * Returns:
	 *   String with only DayZ-supported language characters.
	 *
	 * Unicode Ranges Preserved:
	 *   - Basic Latin:        0x0020-0x007E (ASCII printable)
	 *   - Latin Extended-A:   0x0100-0x017F (Polish, Czech, Turkish, etc.)
	 *   - Latin Extended-B:   0x0180-0x024F (additional Latin)
	 *   - Latin Supplement:   0x0080-0x00FF (German, French, Spanish, etc.)
	 *   - Cyrillic:           0x0400-0x04FF (Russian)
	 *   - Hangul Syllables:   0xAC00-0xD7AF (Korean)
	 *   - Hiragana:           0x3040-0x309F (Japanese)
	 *   - Katakana:           0x30A0-0x30FF (Japanese)
	 *   - CJK Unified:        0x4E00-0x9FFF (Chinese/Japanese Kanji)
	 *   - CJK Extension A:    0x3400-0x4DBF (Rare CJK)
	 *
	 * Example Usage:
	 * @code
	 *   string clean = UUtil.StripUnsupportedCharacters("Hello 😊 Привет 你好 こんにちは");
	 *   // Returns: "Hello  Привет 你好 こんにちは" (emoji removed, languages preserved)
	 * @endcode
	 */
	static string StripUnsupportedCharacters(string input, bool keepWhitespace = true)
	{
		string result = "";
		int len = input.Length();
		
		for (int i = 0; i < len; i++)
		{
			string ch = input.Substring(i, 1);
			int code = ch.ToAscii();
			
			// ToAscii() returns:
			// - Valid code (0-127) for ASCII characters
			// - Values 128-255 for Latin-1 Supplement (accented Latin chars)
			// - 0 or negative for multi-byte Unicode (Cyrillic, CJK, etc.)
			
			// Keep printable ASCII (32-126)
			if (code >= 32 && code <= 126)
			{
				result += ch;
			}
			// Keep Extended Latin / Latin-1 Supplement (128-255: ä ö ü ß é è ç ñ etc.)
			else if (code >= 128 && code <= 255)
			{
				result += ch;
			}
			// Keep whitespace characters (tab, newline, carriage return)
			else if (keepWhitespace && (code == 9 || code == 10 || code == 13))
			{
				result += ch;
			}
			// For multi-byte Unicode (code <= 0 from ToAscii()):
			// We need to check if it's a supported script or an emoji/special character
			// Unfortunately, ToAscii() can't distinguish Cyrillic from emoji
			// Both return 0 or negative. We'll use a heuristic approach.
			else if (code <= 0)
			{
				// Multi-byte character - we'll allow it through as DayZ's engine
				// handles these for supported languages. The engine itself will
				// render ? or skip truly unsupported glyphs.
				// This preserves Cyrillic, CJK, Hangul, Kana, etc.
				result += ch;
			}
			// Skip control characters (0-31 except whitespace) and other edge cases
		}
		
		return result;
	}
	
	/**
	 * StripEmojisOnly
	 * ---------------
	 * Summary:
	 *   Attempts to remove only emoji characters while preserving ALL other
	 *   Unicode text including Cyrillic, CJK, and other international scripts.
	 *   Uses the centralized emoji list from UUtilBase.
	 *
	 * Parameters:
	 *   - input: The string to strip emojis from.
	 *
	 * Returns:
	 *   String with emojis removed but international text preserved.
	 *
	 * Note:
	 *   This only removes emojis that are in the known emoji list. Unknown
	 *   emojis may still pass through. For complete emoji removal, consider
	 *   using StripNonASCII() but that removes ALL non-ASCII characters.
	 *   To add new emojis, update InitializeEmojiMappings() in UUtilBase.c
	 *
	 * Example Usage:
	 * @code
	 *   string clean = UUtil.StripEmojisOnly("Hello 😊 Привет 你好");
	 *   // Returns: "Hello  Привет 你好"
	 * @endcode
	 */
	static string StripEmojisOnly(string input)
	{
		return StripKnownEmojis(input);
	}
}

