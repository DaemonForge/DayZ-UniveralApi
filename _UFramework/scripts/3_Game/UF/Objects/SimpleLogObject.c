class ULogBase extends UFObject_Base {
	string Log = "";
	string GUID = "";
	vector Position;
	
	
	void ULogBase(string log, string playerGUID = "", vector pos = vector.Zero){
		Log = log;
		GUID = playerGUID;
		if (pos != vector.Zero){
			Position = pos;
		}
	}
	
	override string ToJson(){
		return JsonFileLoader<ULogBase>.JsonMakeData(this);
	}
	
}

class ULogMisc extends UFObject_Base {
	string Log = "";
	string Action = "";
	string Item = "";
	string Target = "";
	string GUID = "";
	
	vector Position;
	
	
	void ULogMisc(string log, string playerGUID = "", vector pos = vector.Zero, string action = "", string item = "", string target = ""){
		Log = log;
		GUID = playerGUID;
		if (pos != vector.Zero){
			Position = pos;
		}
		Item = item;
		Action = action;
		Target = target;
	}
	
	override string ToJson(){
		return JsonFileLoader<ULogMisc>.JsonMakeData(this);
	}
	
}

class ULogPlayerPos extends UFObject_Base {
	string Log = "PlayerPos";
	
	string GUID = "";
	
	vector Position;
	float Speed;
	
	bool InTransport;
	
	void ULogPlayerPos(string playerGUID, vector pos = vector.Zero, float speed = 0, bool inTransport = false) {
		GUID = playerGUID;
		if (pos != vector.Zero){
			Position = pos;
		}
		Speed = speed;
		InTransport = inTransport;
	}
	
	override string ToJson(){
		return JsonFileLoader<ULogPlayerPos>.JsonMakeData(this);
	}
	
}


class ULogKilled extends Managed {
	
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
	
	void ULogKilled(string playerGUID = "", vector pos = vector.Zero, string killedBy = "", vector killedByPos = vector.Zero){
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
		return JsonFileLoader<ULogKilled>.JsonMakeData(this);;
	}
	
}