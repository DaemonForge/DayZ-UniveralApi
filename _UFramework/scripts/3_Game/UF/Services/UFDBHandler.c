/*
Documentation for Universal Framework Database Handlers

Overview:
	This module provides a generic framework for modders to interact with the Universal Framework's MongoDB endpoints. It simplifies operations such as saving, loading, querying, updating, and transacting on database objects by abstracting JSON conversion, database calls, and asynchronous callbacks.

Classes:

1. UDBHandler<T>
	 - A templated database handler that extends UDBHandlerBase.
	 - Handles conversion of class objects to JSON using UJSONHandler<T> for persistence in the database.
	 - Methods:
			 • Save(string oid, Class object)
				 - Converts the provided object to JSON and calls the database Save operation.
				 - Returns a call ID on success; logs an error if conversion or casting fails.
			 
			 • Save(string oid, Class object, Class cbInstance, string cbFunction)
				 - Similar to Save(object) but includes a callback through UFCallback<T> to notify upon completion.
			 
			 • Load(string oid, Class cbInstance, string cbFunction)
				 - Loads an object from the database identified by 'oid' and invokes the specified callback.
			 
			 • Load(string oid, Class cbInstance, string cbFunction, string defaultJson)
				 - Loads an object, using 'defaultJson' if no existing record is found.
			 
			 • Load(string oid, Class cbInstance, string cbFunction, Class inObject)
				 - Loads an object using an instance (inObject) to determine its type; performs JSON conversion before invoking the callback through UFCallbackLoader<T>.
			 
			 • Query(UDBQueryBase query, Class cbInstance, string cbFunction) & Query(string query, Class cbInstance, string cbFunction)
				 - Executes a query based on either a UDBQueryBase instance or a JSON query string.
				 - Returns a UDBQueryResult<T> via the callback function, containing matching results.

2. UDBHandlerBase
	 - Base class for database handlers, managing common properties and methods.
	 - Contains:
			 • Mod: A string identifier for the mod (or service) using this handler.
			 • Database: Indicates which database to target (e.g., PLAYER_DB).
	 - Provides default stub implementations for Save, Load, and Query methods that log errors if misused directly.
	 - Additional Utility Methods:
			 • LoadJson(string oid, Class cbInstance, string cbFunction, string defaultJson)
				 - Loads a JSON string from the database, allowing the caller to handle JSON conversion externally.
			 
			 • Increment(string oid, string element, float value = 1)
				 - Convenience function wrapping a numeric transaction to increment a sub-value.
			 
			 • Transaction(string oid, string element, float value)
				 - Performs atomic numerical transactions on a specific sub-value inside the database record.
				 - Overloaded to support callbacks and defined value boundaries.
			 
			 • Update(string oid, string element, string value, string operation)
				 - Updates a sub-value within the JSON object using a specified update operation (e.g., SET).
				 - Overloaded to support callback notifications.
			 
			 • QueryUpdate(UDBQueryBase query, string element, string value, string operation)
				 - Applies an update operation to a group of objects identified by the query.
				 - Includes overloaded versions to support asynchronous callbacks.
			 
			 • Cancel(int cid)
				 - Cancels an in-progress callback-based database operation to prevent access violations.

Callbacks:
	- Callback mechanisms (via UFCallback and UFCallbackLoader<T>) enable asynchronous handling of the Save, Load, Query, Transaction, and Update operations.
	- Typical callback function signatures include parameters such as call ID, status code (e.g., UF_SUCCESS, UF_EMPTY), a unique identifier (GUID), and the resultant data (object, JSON string, or query result).

Usage:
	- Create an instance of UDBHandler for a specific mod and database:
				static autoptr UDBHandler<myClass> m_MyModHandler = new UDBHandler<myClass>("MyMod", PLAYER_DB);
	- Save and Load operations support both synchronous operations and asynchronous operations with callbacks:
				m_MyModHandler.Save("GUID", myObject);
				m_MyModHandler.Load("GUID", player, "MyCallBackFunction");
	- Queries are executed by passing a JSON-based query string or a UDBQueryBase instance to retrieve results matching specified conditions.

Notes:
	- Ensure that objects passed to Save and Load operations can be properly cast to the expected template type T.
	- Error logging is employed to detect improper usage or failed JSON conversion/casting.
	- This abstraction allows mod developers to integrate web-service based database operations into mods without having to handle underlying MongoDB interactions directly.
*/
/* 
	Template DB Handler
	This is the newest Method in which modders can interact with the Universal Framework's MongoDB Endpoints providing access to modders to be
	able to build mods that utilize the Universal Framework Webservice.
	
	view full documentation https://github.com/daemonforge/DayZ-UniveralApi/wiki/Developer-Reference

	static autoptr UDBHandler<myClass> m_MyModHandler = new UDBHandler<myClass>("MyMod", PLAYER_DB); //PLAYER_DB or OBJECT_DB
	
	m_MyModHandler.Save("GUID", myObject); //returns Call ID
	m_MyModHandler.Load("GUID", player, "MyCallBackFunction"); //returns Call ID


	
*/

/**
 * UDBHandler<T>
 * Template-based database handler for managing mod-specific player or object data.
 * Provides type-safe CRUD operations with automatic JSON serialization.
 * 
 * @tparam T The data class type to manage
 * 
 * @code
 * class PlayerData { int kills; int deaths; }
 * static autoptr UDBHandler<PlayerData> m_Handler = new UDBHandler<PlayerData>("MyMod", PLAYER_DB);
 * 
 * // Save
 * m_Handler.Save(playerGUID, playerData, this, "OnSaved");
 * 
 * // Load
 * m_Handler.Load(playerGUID, this, "OnLoaded");
 * 
 * // Query
 * m_Handler.Query("{\"kills\": {\"$gte\": 10}}", this, "OnQueryResults");
 * @endcode
 */
class UDBHandler<Class T> extends UDBHandlerBase{

	/*
	Load and Save
	
	CALLBACK FUNCTION EXAMPLE
	protected void MyCallBackFunction(int cid, int status, string guid, myClass data) {
		if ( status == UF_SUCCESS ){
			//Do something with data
		}
	}*/
	
	
	/**
	 * Saves an object to the database
	 * @param oid Object identifier (e.g., player GUID, object ID)
	 * @param object The object to save (must be of type T)
	 * @return int Call ID, or -1 on error
	 */
	override int Save(string oid, Class object) {
		string jsonString = "{}";
		T obj; //Might not need Casting here but using it anyways
		if (Class.CastTo(obj, object) && UJSONHandler<T>.GetString(obj, jsonString)) {
			return UF().db(Database).Save(Mod, oid, jsonString);
		}
		Error2("[UF] DB HANDLER Save", "Error convertering to JSON or casting make sure you are passing the right class type");
		return -1;
	}
	
	/**
	 * Saves an object to the database with callback notification
	 * @param oid Object identifier
	 * @param object The object to save (must be of type T)
	 * @param cbInstance The callback instance
	 * @param cbFunction The callback function: void OnCallback(int cid, int status, string oid, T data)
	 * @return int Call ID, or -1 on error
	 */
	override int Save(string oid, Class object, Class cbInstance, string cbFunction) {
		string jsonString = "{}";
		T obj; //Might not need Casting here but using it anyways
		if (Class.CastTo(obj, object) && UJSONHandler<T>.GetString(obj, jsonString)) {
			return UF().db(Database).Save(Mod, oid, jsonString, new UFCallback<T>(cbInstance, cbFunction));
		}
		Error2("[UF] DB HANDLER Save", "Error convertering to JSON or casting make sure you are passing the right class type");
		return -1;
	}
	
	
	
	
	/**
	 * Loads an object from the database
	 * @param oid Object identifier
	 * @param cbInstance The callback instance
	 * @param cbFunction The callback function: void OnCallback(int cid, int status, string oid, T data)
	 * @return int Call ID
	 */
	override int Load(string oid, Class cbInstance, string cbFunction) {
		return UF().db(Database).Load(Mod,oid, new UFCallback<T>(cbInstance, cbFunction), "{}");
	}
	
	/**
	 * Loads an object from the database with default JSON fallback
	 * @param oid Object identifier
	 * @param cbInstance The callback instance
	 * @param cbFunction The callback function
	 * @param defaultJson Default JSON to use if object doesn't exist
	 * @return int Call ID
	 */
	override int Load(string oid, Class cbInstance, string cbFunction, string defaultJson) {
		return UF().db(Database).Load(Mod,oid, new UFCallback<T>(cbInstance, cbFunction), defaultJson);
	}
	
	/**
	 * Loads data into an existing object instance
	 * @param oid Object identifier
	 * @param cbInstance The callback instance
	 * @param cbFunction The callback function
	 * @param inObject The object to load data into (must be of type T)
	 * @return int Call ID, or -1 on error
	 */
	override int Load(string oid, Class cbInstance, string cbFunction, Class inObject) {
		string jsonString = "{}";
		T obj; //Might not need Casting here but using it anyways
		if (Class.CastTo(obj, inObject) && UJSONHandler<T>.GetString(obj, jsonString)) {
			autoptr UFCallbackLoader<T> cb = new UFCallbackLoader<T>(cbInstance, cbFunction);
			cb.SetObject(obj);
			return UF().db(Database).Load(Mod, oid, cb, jsonString);
		} 
		Error2("[UF] DB HANDLER Load", "Error convertering to JSON or casting make sure you are passing the right class type");
		return -1;
	}
	
	
	
	/*
	
	Query
	
	
	Query Will return a UDBQueryResult with your Class Callback function for this would be
	
	//CALLBACK FUNCTION EXAMPLE
	protected void MyCallBackFunction(int cid, int status, string guid, UDBQueryResult<myClass> data) {
		if ( status == UF_SUCCESS ){
			//Do something with data
			array<autoptr myClass> results = data.GetResults();
		} else if (status == UF_EMPTY){ // no results

		}
	}
	
	Query Example "{ \"MyVar\": 3 }" would return all objects that have MyVar = 3
	Query Example "{ \"MyVar\": { \"$gte\": 5} }" would return all objects that have MyVar = 5 or higher
	Query Example "{ \"MyTStrringArray\": "somevalue" }" would return all objects that have a value of 'somevalue' inside the array
	Query Example "{ \"MySubObject.SubVar\": 3 }" would return all objects that have a SubVar = 3 inside a sub object this also works if the sub object was an array
	
	
	*/
	
	/**
	 * Executes a database query using a UDBQueryBase object
	 * @param query The query object with filters and options
	 * @param cbInstance The callback instance
	 * @param cbFunction The callback function: void OnCallback(int cid, int status, string oid, UDBQueryResult<T> results)
	 * @return int Call ID
	 */
	override int Query(UDBQueryBase query, Class cbInstance, string cbFunction) {
		return UF().db(Database).Query(Mod,query,new UFCallback<UDBQueryResult<T>>(cbInstance, cbFunction));
	}
	
	/**
	 * Executes a database query using a JSON query string
	 * @param query JSON query string (e.g., "{\"kills\": {\"$gte\": 10}}")
	 * @param cbInstance The callback instance
	 * @param cbFunction The callback function: void OnCallback(int cid, int status, string oid, UDBQueryResult<T> results)
	 * @return int Call ID
	 */
	override int Query(string query, Class cbInstance, string cbFunction) {
		return UF().db(Database).Query(Mod, new UDBQuery(query),new UFCallback<UDBQueryResult<T>>(cbInstance, cbFunction));
	}
}




//just to be able to manage them in like an array or map?
class UDBHandlerBase extends Managed {
	
	string Mod = "";
	int Database = PLAYER_DB;
	protected int m_lastCall = -1;
	
	void UDBHandlerBase(string mod, int database = PLAYER_DB){
		Mod = mod;
		Database = database;
	}
	
	/**
	 * Checks whether the framework is ready to make API calls.
	 * Destructor - cancels last pending call
	 */
	void ~UDBHandlerBase(){
		if (m_lastCall > 0) Cancel(m_lastCall);
	}
	
	/**
	 * Stub method - must be overridden by UDBHandler<T>
	 */
	int Save(string oid, Class object) {
		Error2("[UF] UDBHandlerBase SAVE","Incorrect Ussage class is not type of UDBHandler<T>");
		return -1;
	}
	
	/**
	 * Stub method - must be overridden by UDBHandler<T>
	 */
	int Save(string oid, Class object, Class cbInstance, string cbFunction) {
		Error2("[UF] UDBHandlerBase SAVE","Incorrect Ussage class is not type of UDBHandler<T>");
		return -1;
	}
	
	/**
	 * Stub method - must be overridden by UDBHandler<T>
	 */
	int Load(string oid, Class cbInstance, string cbFunction) {
		Error2("[UF] UDBHandlerBase LOAD","Incorrect Ussage class is not type of UDBHandler<T>");
		return -1;
	}
	
	/**
	 * Stub method - must be overridden by UDBHandler<T>
	 */
	int Load(string oid, Class cbInstance, string cbFunction, string defaultJson) {
		Error2("[UF] UDBHandlerBase LOAD","Incorrect Ussage class is not type of UDBHandler<T>");
		return -1;
	}
	
	/**
	 * Stub method - must be overridden by UDBHandler<T>
	 */
	int Load(string oid, Class cbInstance, string cbFunction, Class inObject){
		Error2("[UF] UDBHandlerBase LOAD","Incorrect Ussage class is not type of UDBHandler<T>");
		return -1;
	}
	
	
	/*
	
		LoadJson Returns string instead of object
	
		CALLBACK FUNCTION EXAMPLE
		protected void MyCallBackFunction(int cid, int status, string guid, string data) {
			if ( status == UF_SUCCESS ){
				//Do something with data you use 
				autoptr myClass obj;
				if (UJSONHandler<myClass>.GetString(obj, data)){
					
				}
			}
		}
	*/
	
	/**
	 * Loads raw JSON from database without automatic deserialization
	 * @param oid Object identifier
	 * @param cbInstance The callback instance
	 * @param cbFunction The callback function: void OnCallback(int cid, int status, string oid, string jsonData)
	 * @param defaultJson Default JSON if object doesn't exist
	 * @return int Call ID
	 */
	int LoadJson(string oid, Class cbInstance, string cbFunction, string defaultJson = "{}") {
		return UF().db(Database).Load(Mod, oid, cbInstance, cbFunction, defaultJson);
	}
	
	/**
	 * Increments a numeric field by a value
	 * Shorthand for Transaction(oid, element, value)
	 * @param oid Object identifier
	 * @param element Field path (supports dot notation)
	 * @param value Amount to increment by (default: 1)
	 * @return int Call ID
	 */
	int Increment(string oid, string element, float value = 1){
		return Transaction(oid, element, value);
	}
	
	/*
		Transactions
	
	
		Updates a sub value inside the object in the database then returns the new value only works with floats or ints
		Sub objects can be used with dot notation aka MySubObject.SubObjectVar
		Will return status of UF_SUCCESS if operations was successful
	*/
	
	/**
	 * Atomically increments/decrements a numeric field
	 * @param oid Object identifier
	 * @param element Field path (supports dot notation: "stats.kills")
	 * @param value Amount to add (can be negative)
	 * @return int Call ID
	 */
	int Transaction(string oid, string element, float value) {
		return UF().db(Database).Transaction(Mod,oid,element,value);
	}
	
	/**
	 * Atomically increments/decrements a numeric field with callback
	 * @param oid Object identifier
	 * @param element Field path
	 * @param value Amount to add
	 * @param cbInstance The callback instance
	 * @param cbFunction The callback function: void OnCallback(int cid, int status, string oid, UDBTransactionResponse data)
	 * @return int Call ID
	 */
	int Transaction(string oid, string element, float value, Class cbInstance, string cbFunction) {
		return UF().db(Database).Transaction(Mod, oid, element, value, new UFCallback<UDBTransactionResponse>(cbInstance, cbFunction));
	}
	
	/**
	 * Atomically increments/decrements with min/max bounds
	 * @param oid Object identifier
	 * @param element Field path
	 * @param value Amount to add
	 * @param min Minimum allowed value
	 * @param max Maximum allowed value
	 * @param cbInstance The callback instance
	 * @param cbFunction The callback function
	 * @return int Call ID
	 */
	int Transaction(string oid, string element, float value, float min, float max, Class cbInstance, string cbFunction) {
		return UF().db(Database).Transaction(Mod, oid, element, value, min, max, new UFCallback<UDBTransactionResponse>(cbInstance, cbFunction));
	}
	
	
	/*
	Update
	
		Updates a sub value inside the object in the database, can also use other operations
		https://github.com/daemonforge/DayZ-UniveralApi/blob/master/_UFramework/scripts/1_Core/Constants.c#L30
	
		Values can be in JSON format to update or push elements into arrays
		
		Sub objects can be used with dot notation aka MySubObject.SubObjectVar
		will return status of UF_SUCCESS if operations was successful
	*/
	
	/**
	 * Updates a specific field in the stored object
	 * @param oid Object identifier
	 * @param element Field path (supports dot notation)
	 * @param value The new value as JSON string
	 * @param operation The update operation (SET, PUSH, PULL, etc. - see UpdateOpts in Constants.c)
	 * @return int Call ID
	 * @see UpdateOpts
	 */
	int Update(string oid, string element, string value, string operation = UpdateOpts.SET) {
		return UF().db(Database).Update(Mod, oid, element, value, operation);
	}
	
	/**
	 * Updates a specific field with callback
	 * @param oid Object identifier
	 * @param element Field path
	 * @param value The new value as JSON string
	 * @param operation The update operation
	 * @param cbInstance The callback instance
	 * @param cbFunction The callback function: void OnCallback(int cid, int status, string oid, UDBUpdateResponse data)
	 * @return int Call ID
	 */
	int Update(string oid, string element, string value, string operation, Class cbInstance, string cbFunction) {	
		return UF().db(Database).Update(Mod, oid, element, value, operation, new UFCallback<UDBUpdateResponse>(cbInstance, cbFunction) );
	}
	
	
	/**
	 * Updates a field across all objects matching a query
	 * @param query The query to match objects
	 * @param element Field path to update
	 * @param value The new value as JSON string
	 * @param operation The update operation (default: SET)
	 * @return int Call ID
	 */
	int QueryUpdate(UDBQueryBase query, string element, string value, string operation = UpdateOpts.SET) {
		return UF().db(Database).QueryUpdate(query, Mod, element, value, operation);
	}
	
	/**
	 * Updates a field across all objects matching a query with callback
	 * @param query The query to match objects
	 * @param element Field path to update
	 * @param value The new value as JSON string
	 * @param operation The update operation
	 * @param cbInstance The callback instance
	 * @param cbFunction The callback function: void OnCallback(int cid, int status, string oid, UDBQueryUpdateResponse data)
	 * @return int Call ID
	 */
	int QueryUpdate(UDBQueryBase query, string element, string value, string operation, Class cbInstance, string cbFunction) {	
		return UF().db(Database).QueryUpdate(query, Mod, element, value, operation, new UFCallback<UDBQueryUpdateResponse>(cbInstance, cbFunction) );
	}
	
	
	/**
	 * Stub method - must be overridden by UDBHandler<T>
	 */
	int Query(UDBQueryBase query, Class cbInstance, string cbFunction) {
		Error2("[UF] UDBHandlerBase QUERY","Incorrect Ussage class is not type of UDBHandler<T>");
		return -1;
	}
	
	/**
	 * Stub method - must be overridden by UDBHandler<T>
	 */
	int Query(string query, Class cbInstance, string cbFunction) {
		Error2("[UF] UDBHandlerBase QUERY","Incorrect Ussage class is not type of UDBHandler<T>");
		return -1;
	}
	
	
	/* 
		Call Cancel
		
		This allows you to cancel a call back to prevent access violations 
	*/
	
	/**
	 * Cancels a pending callback to prevent access violations
	 * @param cid The call ID to cancel
	 */
	void Cancel(int cid){
		UF().RequestCallCancel(cid);
	}
}
