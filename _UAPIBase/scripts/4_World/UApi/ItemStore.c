class UApiEntityStore extends UApiObject_Base {
	
	string m_Type = "";
	int m_pid1;
	int m_pid2;
	int m_pid3;
	int m_pid4;
	float m_Health = -1;
	float m_Quantity;
	float m_Wet;
	float m_Tempature;
	float m_Energy;
	int m_LiquidType;
	int m_Slot;
	int m_Idx;
	int m_Row;
	int m_Col;
	bool m_Flip;
	bool m_IsInHands;
	bool m_IsOn;
	int m_QuickBarSlot;
	int m_Agents;
	int m_Cleanness;
	protected autoptr array<autoptr UApiZoneHealthData> m_HealthZones;
	
	protected autoptr array<autoptr UApiEntityStore> m_Cargo;
	
	bool m_IsMagazine;
	autoptr array<autoptr UApiAmmoData> m_MagAmmo;
	bool m_IsWeapon;
	bool m_IsVehicle;
	autoptr array<int> m_FireModes;
	autoptr UApiAmmoData m_ChamberedRound;
	
	protected autoptr array<autoptr UApiMetaData> m_MetaData;
	
	void UApiEntityStore(EntityAI item = NULL){
		if (!item) return;
		SaveEntity(item, true);
	}
	
	void ~UApiEntityStore(){
		delete m_Cargo;
		delete m_MagAmmo;
		delete m_ChamberedRound;
		delete m_FireModes;
		delete m_MetaData;
	}
	
	void SaveEntity(notnull EntityAI item, bool recursive = true ){
		
	}
	
	EntityAI Create(EntityAI parent = NULL, bool RestoreOrginalLocation = true){
		
	}
	
	EntityAI CreateAtPos(vector Pos, vector Ori = "0 0 0"){
		
	}
	
	void LoadEntity(EntityAI item){
		
	}
	
	
	override string ToJson(){
		string jsonString = JsonFileLoader<UApiEntityStore>.JsonMakeData(this);
		return jsonString;
	}
	
	bool IsValid(){
		return m_Type != "" && m_Health >= 0;
	}
	
	bool Write(string var, bool data){
		if (!m_MetaData) { m_MetaData = new array<autoptr UApiMetaData>;}
		m_MetaData.Insert(new UApiMetaData(var, data.ToString()));
		return true;
	}
	bool Write(string var, int data){
		if (!m_MetaData) { m_MetaData = new array<autoptr UApiMetaData>;}
		m_MetaData.Insert(new UApiMetaData(var, data.ToString()));
		return true;
	}
	bool Write(string var, float data){
		if (!m_MetaData) { m_MetaData = new array<autoptr UApiMetaData>;}
		m_MetaData.Insert(new UApiMetaData(var, data.ToString()));
		return true;
	}
	bool Write(string var, vector data){
		if (!m_MetaData) { m_MetaData = new array<autoptr UApiMetaData>;}
		m_MetaData.Insert(new UApiMetaData(var, data.ToString()));
		return true;
	}
	bool Write(string var, TStringArray data){
		if (!m_MetaData) { m_MetaData = new array<autoptr UApiMetaData>;}
		for (int ii = 0; ii < data.Count(); ii++){
			m_MetaData.Insert(new UApiMetaData(var, data.Get(ii)));
		}
		return true;
	}
	bool Write(string var, TIntArray data){
		if (!m_MetaData) { m_MetaData = new array<autoptr UApiMetaData>;}
		for (int ii = 0; ii < data.Count(); ii++){
			m_MetaData.Insert(new UApiMetaData(var, data.Get(ii).ToString()));
		}
		return true;
	}
	bool Write(string var, TBoolArray data){
		if (!m_MetaData) { m_MetaData = new array<autoptr UApiMetaData>;}
		for (int ii = 0; ii < data.Count(); ii++){
			m_MetaData.Insert(new UApiMetaData(var, data.Get(ii).ToString()));
		}
		return true;
	}
	bool Write(string var, TFloatArray data){
		if (!m_MetaData) { m_MetaData = new array<autoptr UApiMetaData>;}
		for (int ii = 0; ii < data.Count(); ii++){
			m_MetaData.Insert(new UApiMetaData(var, data.Get(ii).ToString()));
		}
		return true;
	}
	bool Write(string var, TVectorArray data){
		if (!m_MetaData) { m_MetaData = new array<autoptr UApiMetaData>;}
		for (int ii = 0; ii < data.Count(); ii++){
			m_MetaData.Insert(new UApiMetaData(var, data.Get(ii).ToString()));
		}
		return true;
	}
	bool Write(string var, string data){
		if (!m_MetaData) { m_MetaData = new array<autoptr UApiMetaData>;}
		m_MetaData.Insert(new UApiMetaData(var, data));
		return true;
	}
	bool Write(string var, Class data){
		Error("[UAPI] Trying to save undefined data class to " + var + " for " + m_Type + " try converting to a string before saving");
		return false;
	}
	
	
	bool Read(string var, out bool data){
		for(int i = 0; i < m_MetaData.Count(); i++){
			if (m_MetaData.Get(i) && m_MetaData.Get(i).Is(var)){
				data = m_MetaData.Get(i).ReadInt();
				return true;
			}
		}
		return false;
	}
	bool Read(string var, out int data){
		for(int i = 0; i < m_MetaData.Count(); i++){
			if (m_MetaData.Get(i) && m_MetaData.Get(i).Is(var)){
				data = m_MetaData.Get(i).ReadInt();
				return true;
			}
		}
		return false;
	}
	bool Read(string var, out float data){
		for(int i = 0; i < m_MetaData.Count(); i++){
			if (m_MetaData.Get(i) && m_MetaData.Get(i).Is(var)){
				data = m_MetaData.Get(i).ReadFloat();
				return true;
			}
		}
		return false;
	}
	bool Read(string var, out vector data){
		for(int i = 0; i < m_MetaData.Count(); i++){
			if (m_MetaData.Get(i) && m_MetaData.Get(i).Is(var)){
				data = m_MetaData.Get(i).ReadVector();
				return true;
			}
		}
		return false;
	}
	bool Read(string var, out TStringArray data){
		bool found = false;
		for(int i = 0; i < m_MetaData.Count(); i++){
			if (m_MetaData.Get(i) && m_MetaData.Get(i).Is(var)){
				data.Insert(m_MetaData.Get(i).ReadString());
				found = true;
			}
		}
		return found;
	}
	bool Read(string var, out TIntArray data){
		bool found = false;
		for(int i = 0; i < m_MetaData.Count(); i++){
			if (m_MetaData.Get(i) && m_MetaData.Get(i).Is(var)){
				data.Insert(m_MetaData.Get(i).ReadInt());
				found = true;
			}
		}
		return found;
	}
	bool Read(string var, out TFloatArray data){
		bool found = false;
		for(int i = 0; i < m_MetaData.Count(); i++){
			if (m_MetaData.Get(i) && m_MetaData.Get(i).Is(var)){
				data.Insert(m_MetaData.Get(i).ReadFloat());
				found = true;
			}
		}
		return found;
	}
	bool Read(string var, out TBoolArray data){
		bool found = false;
		for(int i = 0; i < m_MetaData.Count(); i++){
			if (m_MetaData.Get(i) && m_MetaData.Get(i).Is(var)){
				bool value = (m_MetaData.Get(i).ReadInt());
				data.Insert(value);
				found = true;
			}
		}
		return found;
	}
	bool Read(string var, out TVectorArray data){
		bool found = false;
		for(int i = 0; i < m_MetaData.Count(); i++){
			if (m_MetaData.Get(i) && m_MetaData.Get(i).Is(var)){
				data.Insert(m_MetaData.Get(i).ReadVector());
				found = true;
			}
		}
		return found;
	}
	bool Read(string var, out string data){
		for(int i = 0; i < m_MetaData.Count(); i++){
			if (m_MetaData.Get(i) && m_MetaData.Get(i).Is(var)){
				data = m_MetaData.Get(i).ReadString();
				return true;
			}
		}
		return false;
	}
	bool Read(string var, out Class data){
		Error("[UAPI] Trying to read undefined data class for " + var + " for " + m_Type + " try converting to a string before saving");
		return false;
	}
	
	int GetInt(string var){
		for(int i = 0; i < m_MetaData.Count(); i++){
			if (m_MetaData.Get(i) && m_MetaData.Get(i).Is(var)){return m_MetaData.Get(i).ReadInt();}
		}
		return 0;
	}
	float GetFloat(string var){
		for(int i = 0; i < m_MetaData.Count(); i++){
			if (m_MetaData.Get(i) && m_MetaData.Get(i).Is(var)){ return m_MetaData.Get(i).ReadFloat(); }
		}
		return 0;
	}
	vector GetVector(string var){
		for(int i = 0; i < m_MetaData.Count(); i++){
			if (m_MetaData.Get(i) && m_MetaData.Get(i).Is(var)){ return m_MetaData.Get(i).ReadVector(); }
		}
		return Vector(0,0,0);
	}
	string GetString(string var){
		for (int i = 0; i < m_MetaData.Count(); i++){
			if (m_MetaData.Get(i) && m_MetaData.Get(i).Is(var)){ return m_MetaData.Get(i).ReadString(); }
		}
		return "";
	}
	
	
	bool SaveZoneHealth(string zone, float health){
		if (!m_HealthZones){m_HealthZones = new array<autoptr UApiZoneHealthData>}
		m_HealthZones.Insert(new UApiZoneHealthData(zone, health));
		return true;
	}
	bool ReadZoneHealth(string zone, out float health){
		for (int i = 0; i < m_HealthZones.Count(); i++){
			if (m_HealthZones.Get(i) && m_HealthZones.Get(i).Is(zone)){ 
				health = m_HealthZones.Get(i).Health();
				return true;
			}
		}
		return false;
	}
}
