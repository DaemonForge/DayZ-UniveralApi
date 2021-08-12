class StatusObject extends Managed {
	
    string Status =  "Pending"; //Success or Error
	string Error =  "Not an Error Object";
	
}


class UApiStatus extends StatusObject {
	
    string Version =  "0.0.0";
	string Discord =  "Disabled";
	string Translate =  "Disabled";
	autoptr TStringArray Wit;
	autoptr TStringArray QnA;
	autoptr TStringArray LUIS;
	
	int CheckVersion(string version){
		if (version == Version){
			return 0;
		}
		TStringArray ModVerMap = {};
		TStringArray ApiVerMap = {};
		version.Split(".", ModVerMap);
		Version.Split(".", ApiVerMap);
		int ModMajor = ModVerMap.Get(0).ToInt();
		int ModMinor = ModVerMap.Get(1).ToInt();
		int ModPatch = ModVerMap.Get(2).ToInt();
		int ApiMajor = ApiVerMap.Get(0).ToInt();
		int ApiMinor = ApiVerMap.Get(1).ToInt();
		int ApiPatch = ApiVerMap.Get(2).ToInt();
		if (ModMajor > ApiMajor){
			return 3;
		} 
		if (ModMajor < ApiMajor){
			return -3;
		}
		if (ModMinor > ApiMinor){
			return 2;
		} 
		if (ModMinor < ApiMinor){
			return -2;
		} 
		if (ModPatch > ApiPatch){
			return 1;
		}
		if (ModPatch < ApiPatch){
			return -1;
		}
		return 0;
	}
	
}
