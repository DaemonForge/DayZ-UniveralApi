static string GetLogPlayerPosArray(array<autoptr UApiLogPlayerPos> thePlayerlist){
	return JsonFileLoader<array<autoptr UApiLogPlayerPos>>.JsonMakeData(thePlayerlist);
}


class UUtil extends Managed {
	
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
	
	static string GetRandomId(int number){
		UApi().CheckAndRenewQRandom();
		TStringArray Chars = {"A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z","a","b","c","d","e","f","g","h","i","j","k","l","m","n","o","p","q","r","s","t","u","v","w","x","y","z","0","1","2","3","4","5","6","7","8","9"};
		string id = "";
		for (int i = 0; i <= number; i++){
			int idx = Math.QRandomInt(0,(Chars.Count() - 1));
			id = id + Chars.Get(idx);
		}
		return id;
	}
	
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
	
	//Simple function for finding a player based on their identity
	static DayZPlayer FindPlayerByIdentity(PlayerIdentity identity) {
		if (!identity)
			return NULL;

		int highBits;
		int lowBits;
		GetGame().GetPlayerNetworkIDByIdentityID(identity.GetPlayerId(), lowBits, highBits);
		return DayZPlayer.Cast(GetGame().GetObjectByNetworkId(lowBits, highBits));
	}
	
	static void SendNotificationEx(string Header, string Text, PlayerIdentity player, string Icon = "_UniversalApi\\images\\info.edds") {
		if (GetGame().IsDedicatedServer()){
			NotificationSystem.SendNotificationToPlayerIdentityExtended(player, 5, Header, Text, Icon );
		} else if (GetGame().IsClient()){
			NotificationSystem.AddNotificationExtended(5, Header, Text, Icon);
		}
	}
	
	static void SendNotification(string Header, string Text, PlayerIdentity player, string Icon = "_UniversalApi\\images\\info.edds") {
		if (!player) return;
		SendNotificationEx(Header,Text,player,Icon);
	}
	
	
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
	static int GetUnixInt() {
		int hr, min, sec;
		GetHourMinuteSecond(hr, min, sec);
		return (GetDateInt() * 86400) + (hr * 3600) + (min * 60) + sec;
	}
	
	// Gets the current unix time stamp at UTC
	//Due to int.MAX will break on Tue Jan 19 2038 03:14:07
	static int GetUTCUnixInt() {
		int hr, min, sec;
		GetHourMinuteSecondUTC(hr, min, sec);
		return (GetUTCDateInt() * 86400) + (hr * 3600) + (min * 60) + sec;
	}
	
	
	
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