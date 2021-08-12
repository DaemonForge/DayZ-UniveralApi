class UApiLogBase{
	string Log = "";
	string GUID = "";
	vector Position;
	
	
	void UApiLogBase(string log, string playerGUID = "", vector pos = vector.Zero){
		Log = log;
		GUID = playerGUID;
		if (pos != vector.Zero){
			Position = pos;
		}
	}
	
	string ToJson(){
		return JsonFileLoader<UApiLogBase>.JsonMakeData(this);
	}
	
}

class UApiLogMisc{
	string Log = "";
	string Action = "";
	string Item = "";
	string Target = "";
	string GUID = "";
	
	vector Position;
	
	
	void UApiLogMisc(string log, string playerGUID = "", vector pos = vector.Zero, string action = "", string item = "", string target = ""){
		Log = log;
		GUID = playerGUID;
		if (pos != vector.Zero){
			Position = pos;
		}
		Item = item;
		Action = action;
		Target = target;
	}
	
	string ToJson(){
		return JsonFileLoader<UApiLogMisc>.JsonMakeData(this);
	}
	
}

class UApiLogPlayerPos {
	string Log = "PlayerPos";
	
	string GUID = "";
	
	vector Position;
	float Speed;
	
	bool InTransport;
	
	void UApiLogPlayerPos(string playerGUID, vector pos = vector.Zero, float speed = 0, bool inTransport = false) {
		GUID = playerGUID;
		if (pos != vector.Zero){
			Position = pos;
		}
		Speed = speed;
		InTransport = inTransport;
	}
	
	string ToJson(){
		return JsonFileLoader<UApiLogPlayerPos>.JsonMakeData(this);
	}
	
}


class UApiLogKilled{
	
	string Log = "PlayerKilled";
	
	string GUID = "";
	vector Position;
	float Distance;
	
	string KilledBy = "";
	string KilledByGUID = "";
	vector KilledByPosition;
	
	float StatWater;
	float StatEnergy;
	int BleedingSources;
	
	void UApiLogKilled(string playerGUID = "", vector pos = vector.Zero, string killedBy = "", vector killedByPos = vector.Zero){
		GUID = playerGUID;
		if (pos != vector.Zero){
			Position = pos;
		}
		KilledBy = killedBy;
		if (killedByPos != vector.Zero){
			KilledByPosition = killedByPos;
		}
		if (killedByPos != vector.Zero && pos != vector.Zero){
			Distance = vector.Distance(pos, killedByPos);
		}
	}

	void AddStats(float statWater, float statEnergy, int bleedingSources = -1){
		
		StatWater = statWater;
		StatEnergy = statEnergy;
		if (bleedingSources != -1){
			BleedingSources = bleedingSources;
		}
	}
	
	void ByPlayer(string killedByGUID){
		KilledByGUID = killedByGUID;
	}
	
	string ToJson(){
		return JsonFileLoader<UApiLogKilled>.JsonMakeData(this);;
	}
	
}