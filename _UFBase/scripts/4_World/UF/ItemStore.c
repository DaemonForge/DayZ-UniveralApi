/*
 * Class: UEntityStore
 * Description: 
 *   UEntityStore extends UFObject_Base and provides serialization, deserialization,
 *   and metadata management functionality for an EntityAI. It is used to capture the state
 *   of an entity, including its type, health, physical properties, cargo contents, weapon or vehicle
 *   specifics, and any additional metadata. The class also supports saving and retrieving health
 *   data for specific zones of an entity.
 *
 * Members:
 *   - m_Type: Identifier for the type of entity.
 *   - m_pid1, m_pid2, m_pid3, m_pid4: Supplemental ID values (purpose defined by context).
 *   - m_Health: Health value of the entity; a negative value implies uninitialized or invalid health.
 *   - m_Quantity, m_Wet, m_Tempature, m_Energy: Various physical properties of the entity.
 *   - m_LiquidType: Type identifier for the liquid, if applicable.
 *   - m_Slot, m_Idx, m_Row, m_Col: Positional indices that could be used for inventory or grid positioning.
 *   - m_Flip: Indicates if the entity has been flipped.
 *   - m_IsInHands: Flag to denote if the entity is currently held.
 *   - m_IsOn: Flag to indicate if the entity is switched on.
 *   - m_QuickBarSlot: Slot index for quick access/inventory bar.
 *   - m_Agents: Likely represents contamination or agent values (game-specific usage).
 *   - m_Cleanness: Represents the cleanliness or dirtiness level of the entity.
 *   - m_HealthZones: An array managing health data for different zones of the entity.
 *   - m_Cargo: An array storing nested UEntityStore objects, representing items contained within.
 *   - m_IsMagazine: Flag indicating if the entity functions as a magazine.
 *   - m_MagAmmo: Array to manage ammunition data for magazines.
 *   - m_IsWeapon: Flag to mark the entity as a weapon.
 *   - m_IsVehicle: Flag to mark the entity as a vehicle.
 *   - m_FireModes: Array listing available fire modes for a weapon.
 *   - m_ChamberedRound: Represents the current round chambered in a weapon.
 *   - m_MetaData: Array holding key-value metadata entries (UMetaData) supporting various data types.
 *
 * Methods:
 *   - UEntityStore(EntityAI item = NULL):
 *       Constructor that initializes the entity store. If an EntityAI is provided,
 *       the SaveEntity method is invoked to capture its state.
 *
 *   - ~UEntityStore():
 *       Destructor that correctly disposes of dynamically allocated arrays for cargo, ammo, fire modes,
 *       chambered rounds, and metadata to prevent memory leaks.
 *
 *   - SaveEntity(EntityAI item, bool recursive = true):
 *       Serializes the provided EntityAI's state into this UEntityStore.
 *       The 'recursive' parameter, when true, indicates that nested properties (such as cargo)
 *       may also be serialized.
 *
 *   - Create(EntityAI parent = NULL, bool RestoreOrginalLocation = true):
 *       Creates a new EntityAI instance based on the stored state. An optional parent entity may be provided,
 *       and RestoreOrginalLocation determines whether the original location is preserved during creation.
 *
 *   - CreateAtPos(vector Pos, vector Ori = "0 0 0"):
 *       Creates an EntityAI instance at the specified position (and orientation, if provided).
 *
 *   - LoadEntity(EntityAI item):
 *       Loads/stores the previously serialized data into the existing EntityAI instance passed as argument.
 *
 *   - ToJson():
 *       Converts this UEntityStore instance to a JSON string representation.
 *
 *   - IsValid():
 *       Checks if the entity store holds valid data, ensuring that m_Type is not empty and m_Health is non-negative.
 *
 *   - Write(...):
 *       A series of overloaded methods allowing metadata (m_MetaData) to be stored.
 *       Supports BOOL, INT, FLOAT, VECTOR, STRING and arrays of those types.
 *       Attempts to store a non-primitive class will emit an error.
 *
 *   - Read(...):
 *       Overloaded methods to retrieve metadata from m_MetaData based on a provided key.
 *       These methods extract values as BOOL, INT, FLOAT, VECTOR, STRING, or corresponding arrays.
 *
 *   - GetInt/GetFloat/GetVector/GetString:
 *       Helper methods to retrieve single metadata values of specified type directly by key.
 *
 *   - SaveZoneHealth(string zone, float health):
 *       Stores health value for a given zone by inserting a new UZoneData entry in m_HealthZones.
 *
 *   - ReadZoneHealth(string zone, out float health):
 *       Retrieves the health value for a specific zone from the m_HealthZones array.
 *
 * Notes:
 *   - The class relies on other types such as EntityAI, UMetaData, UZoneData, UAmmoData,
 *     and possibly JsonFileLoader, which are part of the broader system.
 *   - Some method implementations (e.g., SaveEntity, Create, CreateAtPos, LoadEntity) are placeholders,
 *     intended to be implemented with logic specific to the application.
 *   - Memory management for dynamically allocated arrays is handled explicitly in the destructor.
 */
class UEntityStore extends UFObject_Base {
	
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
	protected autoptr array<autoptr UZoneData> m_HealthZones;
	
	protected autoptr array<autoptr UEntityStore> m_Cargo;
	
	bool m_IsMagazine;
	autoptr array<autoptr UAmmoData> m_MagAmmo;
	bool m_IsWeapon;
	bool m_IsVehicle;
	autoptr array<int> m_FireModes;
	autoptr UAmmoData m_ChamberedRound;
	
	protected autoptr array<autoptr UMetaData> m_MetaData;
	
	void UEntityStore(EntityAI item = NULL){
		if (!item) return;
		SaveEntity(item, true);
	}
	
	void ~UEntityStore(){
		delete m_Cargo;
		delete m_MagAmmo;
		delete m_ChamberedRound;
		delete m_FireModes;
		delete m_MetaData;
	}
	
	void SaveEntity(notnull EntityAI item, bool recursive = true ){
		
	}
	
	EntityAI Create(EntityAI parent = NULL, bool RestoreOrginalLocation = true){
		return null;
	}
	
	EntityAI CreateAtPos(vector Pos, vector Ori = "0 0 0"){
		return null;
	}
	
	void LoadEntity(EntityAI item){
		
	}
		
	override string ToJson(){
		string jsonString = JsonFileLoader<UEntityStore>.JsonMakeData(this);
		return jsonString;
	}
	
	bool IsValid(){
		return m_Type != "" && m_Health >= 0;
	}
	
	bool Write(string var, bool data){
		if (!m_MetaData) { m_MetaData = new array<autoptr UMetaData>;}
		m_MetaData.Insert(new UMetaData(var, data.ToString()));
		return true;
	}
	bool Write(string var, int data){
		if (!m_MetaData) { m_MetaData = new array<autoptr UMetaData>;}
		m_MetaData.Insert(new UMetaData(var, data.ToString()));
		return true;
	}
	bool Write(string var, float data){
		if (!m_MetaData) { m_MetaData = new array<autoptr UMetaData>;}
		m_MetaData.Insert(new UMetaData(var, data.ToString()));
		return true;
	}
	bool Write(string var, vector data){
		if (!m_MetaData) { m_MetaData = new array<autoptr UMetaData>;}
		m_MetaData.Insert(new UMetaData(var, data.ToString()));
		return true;
	}
	bool Write(string var, TStringArray data){
		if (!m_MetaData) { m_MetaData = new array<autoptr UMetaData>;}
		for (int ii = 0; ii < data.Count(); ii++){
			m_MetaData.Insert(new UMetaData(var, data.Get(ii)));
		}
		return true;
	}
	bool Write(string var, TIntArray data){
		if (!m_MetaData) { m_MetaData = new array<autoptr UMetaData>;}
		for (int ii = 0; ii < data.Count(); ii++){
			m_MetaData.Insert(new UMetaData(var, data.Get(ii).ToString()));
		}
		return true;
	}
	bool Write(string var, TBoolArray data){
		if (!m_MetaData) { m_MetaData = new array<autoptr UMetaData>;}
		for (int ii = 0; ii < data.Count(); ii++){
			m_MetaData.Insert(new UMetaData(var, data.Get(ii).ToString()));
		}
		return true;
	}
	bool Write(string var, TFloatArray data){
		if (!m_MetaData) { m_MetaData = new array<autoptr UMetaData>;}
		for (int ii = 0; ii < data.Count(); ii++){
			m_MetaData.Insert(new UMetaData(var, data.Get(ii).ToString()));
		}
		return true;
	}
	bool Write(string var, TVectorArray data){
		if (!m_MetaData) { m_MetaData = new array<autoptr UMetaData>;}
		for (int ii = 0; ii < data.Count(); ii++){
			m_MetaData.Insert(new UMetaData(var, data.Get(ii).ToString()));
		}
		return true;
	}
	bool Write(string var, string data){
		if (!m_MetaData) { m_MetaData = new array<autoptr UMetaData>;}
		m_MetaData.Insert(new UMetaData(var, data));
		return true;
	}
	bool Write(string var, Class data){
		Error("[UF] Trying to save undefined data class to " + var + " for " + m_Type + " try converting to a string before saving");
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
		Error("[UF] Trying to read undefined data class for " + var + " for " + m_Type + " try converting to a string before saving");
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
		if (!m_HealthZones){m_HealthZones = new array<autoptr UZoneData>}
		m_HealthZones.Insert(new UZoneData(zone, health));
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
