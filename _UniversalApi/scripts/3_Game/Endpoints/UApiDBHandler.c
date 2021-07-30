/* Just an idea?? for future implmentations 

	static autoptr UApiDBHandler<myClass> m_MyModHandler = new UApiDBHandler<myClass>("MyMod", PLAYER_DB); //PLAYER_DB or OBJECT_DB
	
	m_MyModHandler.Save("GUID", myObject); //returns Call ID
	m_MyModHandler.Load("GUID", player, "MyCallBackFunction"); //returns Call ID
	
*/


class UApiDBHandler<Class T> extends UApiDBHandlerBase{

	//Load and Save
	/*
	CALLBACK FUNCTION EXAMPLE
	protected void MyCallBackFunction(int cid, int status, string guid, myClass data) {
		if ( status == UAPI_SUCCESS ){
			//Do something with data
		}
	}*/
	
	override int Save(string oid, Class object) {
		string jsonString = "{}";
		T obj; //Might not need Casting here but using it anyways
		if (Class.CastTo(obj, object) && UApiJSONHandler<T>.GetString(obj, jsonString)) {
			return UApi().db(Database).Save(Mod, oid, jsonString);
		}
		return -1;
	}
	
	override int Save(string oid, Class object, Class cbInstance, string cbFunction) {
		string jsonString = "{}";
		T obj; //Might not need Casting here but using it anyways
		if (Class.CastTo(obj, object) && UApiJSONHandler<T>.GetString(obj, jsonString)) {
			return UApi().db(Database).Save(Mod, oid, jsonString, new UApiCallback<T>(cbInstance, cbFunction));
		}
		return -1;
	}
	
	override int Load(string oid, Class cbInstance, string cbFunction, string defaultJson = "{}") {
		return UApi().db(Database).Load(Mod,oid, new UApiCallback<T>(cbInstance, cbFunction, oid), defaultJson);
	}
	
	override int Load(string oid, Class cbInstance, string cbFunction, Class defaultObject) {
		string jsonString = "{}";
		T obj; //Might not need Casting here but using it anyways
		if (Class.CastTo(obj, defaultObject) && UApiJSONHandler<T>.GetString(obj, jsonString)) {
			return UApi().db(Database).Load(Mod, oid, new UApiCallback<T>(cbInstance, cbFunction, oid), jsonString);
		}
		return -1;
	}
	
	
	//Query Will return a UApiQueryResult with your Class Callback function for this would be
	/*
	CALLBACK FUNCTION EXAMPLE
	protected void MyCallBackFunction(int cid, int status, string guid, UApiQueryResult<myClass> data) {
		if ( status == UAPI_SUCCESS ){
			//Do something with data
			array<autoptr myClass> results = data.GetResults();
		} else if (status == UAPI_EMPTY){ // no results

		}
	}
	*/
	override int Query(UApiQueryBase query, Class cbInstance, string cbFunction) {
		return UApi().db(Database).Query(Mod,query,new UApiCallback<UApiQueryResult<T>>(cbInstance, cbFunction, oid));
	}
	override int Query(string query, Class cbInstance, string cbFunction) {
		return UApi().db(Database).Query(Mod, new UApiDBQuery(query),new UApiCallback<UApiQueryResult<T>>(cbInstance, cbFunction, oid));
	}
}




//just to be able to manage them in like an array or map maybe later?
class UApiDBHandlerBase extends Managed {
	
	string Mod = "";
	int Database = PLAYER_DB;
	
	void UApiDBHandlerBase(string mod, int database = PLAYER_DB){
		Mod = mod;
		Database = database;
	}
	
	int Save(string oid, Class object) {
		Error2("[UAPI] UApiDBHandlerBase SAVE","Incorrect Ussage class is not type of UApiDBHandler<T>");
		return -1;
	}
	
	int Save(string oid, Class object, Class cbInstance, string cbFunction) {
		Error2("[UAPI] UApiDBHandlerBase SAVE","Incorrect Ussage class is not type of UApiDBHandler<T>");
		return -1;
	}
	
	int Load(string oid, Class cbInstance, string cbFunction, string defaultJson = "{}") {
		Error2("[UAPI] UApiDBHandlerBase LOAD","Incorrect Ussage class is not type of UApiDBHandler<T>");
		return -1;
	}
	
	int Load(string oid, Class cbInstance, string cbFunction, Class defaultObject) {
		Error2("[UAPI] UApiDBHandlerBase LOAD","Incorrect Ussage class is not type of UApiDBHandler<T>");
		return -1;
	}
	
	//LoadJson Returns string instead of object
	/*
		CALLBACK FUNCTION EXAMPLE
		protected void MyCallBackFunction(int cid, int status, string guid, string data) {
			if ( status == UAPI_SUCCESS ){
				//Do something with data you use 
				autoptr myClass obj;
				if (UApiJSONHandler<myClass>.GetString(obj, data)){
					
				}
			}
		}
	*/
	int LoadJson(string oid, Class cbInstance, string cbFunction, string defaultJson = "{}") {
		return UApi().db(Database).Load(Mod, oid, cbInstance, cbFunction, defaultJson));
	}
	
	int Increment(string oid, string element, float value = 1){
		return Transaction(Mod, element, value);
	}
	
	int Transaction(string oid, string element, float value) {
		return UApi().db(Database).Transaction(Mod,oid,element,value);
	}
	
	int Transaction(string oid, string element, float value, Class cbInstance, string cbFunction) {
		return UApi().db(Database).Transaction(Mod, oid, element, value, new UApiCallback<UApiTransactionResponse>(cbInstance, cbFunction));
	}	
		
	int Update(string oid, string element, string value, string operation = UpdateOpts.SET) {
		return UApi().db(Database).Update(Mod, oid, element, value, operation);
	}
	
	int Update(string oid, string element, string value, string operation, Class cbInstance, string cbFunction) {	
		return UApi().db(Database).Update(Mod, oid, element, value, operation, new UApiCallback<UApiUpdateResponse>(cbInstance, cbFunction) );
	}
	
	int Query(UApiQueryBase query, Class cbInstance, string cbFunction) {
		Error2("[UAPI] UApiDBHandlerBase QUERY","Incorrect Ussage class is not type of UApiDBHandler<T>");
		return -1;
	}
	int Query(string query, Class cbInstance, string cbFunction) {
		Error2("[UAPI] UApiDBHandlerBase QUERY","Incorrect Ussage class is not type of UApiDBHandler<T>");
		return -1;
	}
}