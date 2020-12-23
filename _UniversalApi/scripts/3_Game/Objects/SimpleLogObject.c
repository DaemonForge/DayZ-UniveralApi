class UApiLogBase{
	

	
	string Log = "";
	
	string GUID = "";
	
	array<float> Position;
	
	
	void UApiLogBase(string log, string playerGUID = "", vector pos = vector.Zero){
		Log = log;
		GUID = playerGUID;
		if (pos != vector.Zero){
			Position.Insert(pos[0]);
			Position.Insert(pos[1]);
			Position.Insert(pos[2]);
		}
	}
	
	string ToJson(){
		return JsonFileLoader<UApiLogBase>.JsonMakeData(this);
	}
	
}

static string GetLogPlayerPosArray(array<ref UApiLogPlayerPos> thePlayerlist){
	return JsonFileLoader<array<ref UApiLogPlayerPos>>.JsonMakeData(thePlayerlist);
}


class UApiLogPlayerPos {
	string Log = "PlayerPos";
	
	string GUID = "";
	
	array<float> Position;
	float Speed;
	
	bool InTransport;
	
	void UApiLogPlayerPos(string playerGUID, vector pos = vector.Zero, float speed = 0, bool inTransport = false) {
		GUID = playerGUID;
		if (pos != vector.Zero){
			Position.Insert(pos[0]);
			Position.Insert(pos[1]);
			Position.Insert(pos[2]);
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
	ref array<float> Position;
	float Distance;
	
	string KilledBy = "";
	string KilledByGUID = "";
	ref array<float> KilledByPosition;
	
	float StatWater;
	float StatEnergy;
	int BleedingSources;
	
	void UApiLogKilled(string playerGUID = "", vector pos = vector.Zero, string killedBy = "", vector killedByPos = vector.Zero){
		GUID = playerGUID;
		if (pos != vector.Zero){
			Position.Insert(pos[0]);
			Position.Insert(pos[1]);
			Position.Insert(pos[2]);
		}
		KilledBy = killedBy;
		if (killedByPos != Vector(0,0,0)){
			KilledByPosition.Insert(killedByPos[0]);
			KilledByPosition.Insert(killedByPos[1]);
			KilledByPosition.Insert(killedByPos[2]);
		}
		if (killedByPos != Vector(0,0,0) && pos != vector.Zero){
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