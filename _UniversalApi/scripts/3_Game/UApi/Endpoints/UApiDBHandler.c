/* 
	Template DB Handler
	This is the newest Method in which modders can interact with the Universal API's MongoDB Endpoints providing access to modders to be
	able to build mods that utilize the Universal API Webservice.
	
	view full documentation https://github.com/daemonforge/DayZ-UniveralApi/wiki/Developer-Reference

	static autoptr UApiDBHandler<myClass> m_MyModHandler = new UApiDBHandler<myClass>("MyMod", PLAYER_DB); //PLAYER_DB or OBJECT_DB
	
	m_MyModHandler.Save("GUID", myObject); //returns Call ID
	m_MyModHandler.Load("GUID", player, "MyCallBackFunction"); //returns Call ID


	
*/

class UApiDBHandler<Class T> extends UApiDBHandlerBase{

	/*
	Load and Save
	
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
		Error2("[UAPI] DB HANDLER Save", "Error convertering to JSON or casting make sure you are passing the right class type");
		return -1;
	}
	override int Save(string oid, Class object, Class cbInstance, string cbFunction) {
		string jsonString = "{}";
		T obj; //Might not need Casting here but using it anyways
		if (Class.CastTo(obj, object) && UApiJSONHandler<T>.GetString(obj, jsonString)) {
			return UApi().db(Database).Save(Mod, oid, jsonString, new UApiCallback<T>(cbInstance, cbFunction));
		}
		Error2("[UAPI] DB HANDLER Save", "Error convertering to JSON or casting make sure you are passing the right class type");
		return -1;
	}
	
	
	
	
	override int Load(string oid, Class cbInstance, string cbFunction) {
		return UApi().db(Database).Load(Mod,oid, new UApiCallback<T>(cbInstance, cbFunction), "{}");
	}
	override int Load(string oid, Class cbInstance, string cbFunction, string defaultJson) {
		return UApi().db(Database).Load(Mod,oid, new UApiCallback<T>(cbInstance, cbFunction), defaultJson);
	}
	override int Load(string oid, Class cbInstance, string cbFunction, Class inObject) {
		string jsonString = "{}";
		T obj; //Might not need Casting here but using it anyways
		if (Class.CastTo(obj, inObject) && UApiJSONHandler<T>.GetString(obj, jsonString)) {
			autoptr UApiCallbackLoader<T> cb = new UApiCallbackLoader<T>(cbInstance, cbFunction);
			cb.SetObject(obj);
			return UApi().db(Database).Load(Mod, oid, cb, jsonString);
		} 
		Error2("[UAPI] DB HANDLER Load", "Error convertering to JSON or casting make sure you are passing the right class type");
		return -1;
	}
	
	
	
	/*
	
	Query
	
	
	Query Will return a UApiQueryResult with your Class Callback function for this would be
	
	//CALLBACK FUNCTION EXAMPLE
	protected void MyCallBackFunction(int cid, int status, string guid, UApiQueryResult<myClass> data) {
		if ( status == UAPI_SUCCESS ){
			//Do something with data
			array<autoptr myClass> results = data.GetResults();
		} else if (status == UAPI_EMPTY){ // no results

		}
	}
	
	Query Example "{ \"MyVar\": 3 }" would return all objects that have MyVar = 3
	Query Example "{ \"MyVar\": { \"$gte\": 5} }" would return all objects that have MyVar = 5 or higher
	Query Example "{ \"MyTStrringArray\": "somevalue" }" would return all objects that have a value of 'somevalue' inside the array
	Query Example "{ \"MySubObject.SubVar\": 3 }" would return all objects that have a SubVar = 3 inside a sub object this also works if the sub object was an array
	
	
	*/
	override int Query(UApiQueryBase query, Class cbInstance, string cbFunction) {
		return UApi().db(Database).Query(Mod,query,new UApiCallback<UApiQueryResult<T>>(cbInstance, cbFunction));
	}
	override int Query(string query, Class cbInstance, string cbFunction) {
		return UApi().db(Database).Query(Mod, new UApiDBQuery(query),new UApiCallback<UApiQueryResult<T>>(cbInstance, cbFunction));
	}
}




//just to be able to manage them in like an array or map?
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
	
	int Load(string oid, Class cbInstance, string cbFunction) {
		Error2("[UAPI] UApiDBHandlerBase LOAD","Incorrect Ussage class is not type of UApiDBHandler<T>");
		return -1;
	}
	
	int Load(string oid, Class cbInstance, string cbFunction, string defaultJson) {
		Error2("[UAPI] UApiDBHandlerBase LOAD","Incorrect Ussage class is not type of UApiDBHandler<T>");
		return -1;
	}
	
	int Load(string oid, Class cbInstance, string cbFunction, Class inObject){
		Error2("[UAPI] UApiDBHandlerBase LOAD","Incorrect Ussage class is not type of UApiDBHandler<T>");
		return -1;
	}
	
	
	/*
	
		LoadJson Returns string instead of object
	
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
		return Transaction(oid, element, value);
	}
	
	/*
		Transactions
	
	
		Updates a sub value inside the object in the database then returns the new value only works with floats or ints
		Sub objects can be used with dot notation aka MySubObject.SubObjectVar
		Will return status of UAPI_SUCCESS if operations was successful
	*/
	int Transaction(string oid, string element, float value) {
		return UApi().db(Database).Transaction(Mod,oid,element,value);
	}
	int Transaction(string oid, string element, float value, Class cbInstance, string cbFunction) {
		return UApi().db(Database).Transaction(Mod, oid, element, value, new UApiCallback<UApiTransactionResponse>(cbInstance, cbFunction));
	}
	int Transaction(string oid, string element, float value, float min, float max, Class cbInstance, string cbFunction) {
		return UApi().db(Database).Transaction(Mod, oid, element, value, min, max, new UApiCallback<UApiTransactionResponse>(cbInstance, cbFunction));
	}
	
	
	/*
	Update
	
		Updates a sub value inside the object in the database, can also use other operations
		https://github.com/daemonforge/DayZ-UniveralApi/blob/master/_UniversalApi/scripts/1_Core/Constants.c#L30
	
		Values can be in JSON format to update or push elements into arrays
		
		Sub objects can be used with dot notation aka MySubObject.SubObjectVar
		will return status of UAPI_SUCCESS if operations was successful
	*/
	int Update(string oid, string element, string value, string operation = UpdateOpts.SET) {
		return UApi().db(Database).Update(Mod, oid, element, value, operation);
	}
	int Update(string oid, string element, string value, string operation, Class cbInstance, string cbFunction) {	
		return UApi().db(Database).Update(Mod, oid, element, value, operation, new UApiCallback<UApiUpdateResponse>(cbInstance, cbFunction) );
	}
	
	
	int QueryUpdate(UApiQueryBase query, string element, string value, string operation = UpdateOpts.SET) {
		return UApi().db(Database).QueryUpdate(query, Mod, element, value, operation);
	}
	int QueryUpdate(UApiQueryBase query, string element, string value, string operation, Class cbInstance, string cbFunction) {	
		return UApi().db(Database).QueryUpdate(query, Mod, element, value, operation, new UApiCallback<UApiQueryUpdateResponse>(cbInstance, cbFunction) );
	}
	
	
	int Query(UApiQueryBase query, Class cbInstance, string cbFunction) {
		Error2("[UAPI] UApiDBHandlerBase QUERY","Incorrect Ussage class is not type of UApiDBHandler<T>");
		return -1;
	}
	int Query(string query, Class cbInstance, string cbFunction) {
		Error2("[UAPI] UApiDBHandlerBase QUERY","Incorrect Ussage class is not type of UApiDBHandler<T>");
		return -1;
	}
	
	
	/* 
		Call Cancel
		
		This allows you to cancel a call back to prevent access violations 
	*/
	void Cancel(int cid){
		UApi().RequestCallCancel(cid);
	}
}
