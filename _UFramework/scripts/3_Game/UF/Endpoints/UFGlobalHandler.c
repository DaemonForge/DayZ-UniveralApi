/*
	Universal Framework Global Handler Documentation

	Overview:
	This documentation outlines the functionality of the Global Handler classes which provide an interface to interact
	with the Universal Framework's MongoDB web service endpoints used in the modding environment. These classes allow for
	saving and loading mod data, as well as updating and doing transactions on subelements within the saved JSON objects.
	
	-----------------------------------------------------------------------------
	Class: UDBGlobalHandler<Class T>
	-----------------------------------------------------------------------------
	Description:
	A template-based subclass of UDBGlobalHandlerBase designed for handling JSON conversions for objects of type T.
	It provides overloaded methods to save and load objects to/from the database along with callback capabilities.

	Provided Methods:
	1. Save(Class object)
	   - Converts the provided object of type T to a JSON string using UJSONHandler<T>.
	   - Calls the globals().Save method to store the JSON string under the specified mod.
	   - Returns a call ID or -1 if conversion or casting fails.
	   
	2. Save(Class object, Class cbInstance, string cbFunction)
	   - Similar to Save(Class object) but includes a callback via UFCallback<T> for post-save operations.
	   - Returns a call ID or -1 on error.
	   
	3. Load(Class cbInstance, string cbFunction)
	   - Initiates a load operation from the database using a default JSON string.
	   - Calls globals().Load with the mod and callback.
	   - Returns the call ID.
	   
	4. Load(Class cbInstance, string cbFunction, string defaultJson)
	   - Similar to the first Load but allows a custom default JSON string.
	   - Returns the call ID.
	   
	5. Load(Class cbInstance, string cbFunction, Class inObject)
	   - Attempts to cast the provided inObject to type T and convert it to a JSON string.
	   - Uses a specialized UFCallbackLoader<T> to manage the callback with the given object.
	   - Returns the call ID or -1 if conversion or casting fails.
	   
	6. LoadSelf(Class cbInstance, string cbFunction = "")
	   - Uses the callback instance itself as the source to derive a JSON string via type T conversion.
	   - Uses UFCallbackLoader<T> to handle the callback.
	   - Returns the call ID or -1 if conversion or casting fails.

	-----------------------------------------------------------------------------
	Class: UDBGlobalHandlerBase
	-----------------------------------------------------------------------------
	Description:
	The base class for managing mod-specific database interactions. It implements default error responses for methods that
	should be overridden in subclasses and provides common functions for executing update and transaction calls on the data.

	Provided Methods:
	1. Save(Class object) & Save(Class object, Class cbInstance, string cbFunction)
	   - Default implementations that log errors. They must be overridden by a subclass (i.e., UDBGlobalHandler<T>) to function.
	   
	2. Load methods (overloads):
	   - Provide default error responses if improperly used.
	   
	3. LoadJson(Class cbInstance, string cbFunction, string defaultJson = "{}")
	   - Loads raw JSON text from the database for the given mod using a callback.
	   - Returns the call ID.
	   
	4. Increment & Transaction methods:
	   - Alter sub-values (floats or integers only) inside the mod database object.
	   - Increase a given element by a specified float value.
	   - Transaction methods allow optional callback handling for change confirmation.
	   
	5. Update methods:
	   - Update specific sub-elements of the mod data using provided JSON values.
	   - Support various operations (default is UpdateOpts.SET) to change or push values.
	   - Callback versions exist for asynchronous handling.
	   
	6. Cancel(int cid)
	   - Static method to cancel an ongoing database call callback to prevent potential access violations.

	General Notes:
	- All operations interact with the global database via the U().globals() interface.
	- JSON conversion operations utilize helper class UJSONHandler<T>.
	- Callbacks are implemented through UFCallback<T> and UFCallbackLoader<T> which pass the call ID, status, mod
	  identifier, and the JSON data or data object to the callback function.
	- Error handling is centralized via Error2 calls to notify debug information when operations fail due to casting
	  or JSON conversion issues.
	
	Usage:
	Instantiate the UDBGlobalHandler<T> with the appropriate data class and mod identifier to enable saving and loading via
	the Universal Framework’s MongoDB endpoint. The provided overloads provide flexibility to handle callback-based logic
	tailored to the mod's requirements.
*/
/* 
	Template Global Handler
	This is the newest Method in which modders can interact with the Universal Framework's MongoDB Endpoints providing access to modders to be
	able to build mods that utilize the Universal Framework Webservice.
	
	view full documentation https://github.com/daemonforge/DayZ-UniveralApi/wiki/Developer-Reference

	static autoptr UDBGlobalHandler<myClass> m_MyModHandler = new UDBGlobalHandler<myClass>("MyMod");
	
	m_MyModHandler.Save(myObject); //returns Call ID
	m_MyModHandler.Load(player, "MyCallBackFunction"); //returns Call ID


	
*/

/**
 * UDBGlobalHandler<T>
 * Template-based handler for managing global mod data in MongoDB.
 * Provides type-safe Save/Load operations with automatic JSON serialization.
 * 
 * @tparam T The data class type to manage
 * 
 * @code
 * class MyModData { int score; string name; }
 * static autoptr UDBGlobalHandler<MyModData> m_Handler = new UDBGlobalHandler<MyModData>("MyMod");
 * 
 * // Save
 * autoptr MyModData data = new MyModData();
 * data.score = 100;
 * m_Handler.Save(data, this, "OnSaved");
 * 
 * // Load
 * m_Handler.Load(this, "OnLoaded");
 * @endcode
 */
class UDBGlobalHandler<Class T> extends UDBGlobalHandlerBase{

	/*
	Load and Save
	
	CALLBACK FUNCTION EXAMPLE
	protected void MyCallBackFunction(int cid, int status, string mod, myClass data) {
		if ( status == UF_SUCCESS ){
			//Do something with data
		}
	}*/
		
	/**
	 * Saves an object to the global database for this mod
	 * @param object The object to save (must be of type T)
	 * @return int Call ID, or -1 on error
	 */
	override int Save(Class object) {
		string jsonString = "{}";
		T obj; //Might not need Casting here but using it anyways
		if (Class.CastTo(obj, object) && UJSONHandler<T>.GetString(obj, jsonString)) {
			return U().globals().Save(Mod,jsonString);
		}
		Error2("[UF] DB HANDLER Save", "Error convertering to JSON or casting make sure you are passing the right class type");
		return -1;
	}
	
	/**
	 * Saves an object to the global database with a callback
	 * @param object The object to save (must be of type T)
	 * @param cbInstance The callback instance
	 * @param cbFunction The callback function name: void OnCallback(int cid, int status, string mod, T data)
	 * @return int Call ID, or -1 on error
	 */
	override int Save(Class object, Class cbInstance, string cbFunction) {
		string jsonString = "{}";
		T obj; //Might not need Casting here but using it anyways
		if (Class.CastTo(obj, object) && UJSONHandler<T>.GetString(obj, jsonString)) {
			return U().globals().Save(Mod, jsonString, new UFCallback<T>(cbInstance, cbFunction));
		}
		Error2("[UF] DB HANDLER Save", "Error convertering to JSON or casting make sure you are passing the right class type");
		return -1;
	}
	
	
	
	/**
	 * Loads the global mod data from database
	 * @param cbInstance The callback instance
	 * @param cbFunction The callback function name: void OnCallback(int cid, int status, string mod, T data)
	 * @return int Call ID
	 */
	override int Load(Class cbInstance, string cbFunction) {
		return U().globals().Load(Mod,new UFCallback<T>(cbInstance, cbFunction), "{}");
	}
	
	/**
	 * Loads the global mod data from database with a default JSON fallback
	 * @param cbInstance The callback instance
	 * @param cbFunction The callback function name
	 * @param defaultJson Default JSON string to use if no data exists
	 * @return int Call ID
	 */
	override int Load(Class cbInstance, string cbFunction, string defaultJson) {
		return U().globals().Load(Mod,new UFCallback<T>(cbInstance, cbFunction), defaultJson);
	}
	
	/**
	 * Loads data into an existing object instance
	 * @param cbInstance The callback instance
	 * @param cbFunction The callback function name
	 * @param inObject The object to load data into (must be of type T)
	 * @return int Call ID, or -1 on error
	 */
	override int Load(Class cbInstance, string cbFunction, Class inObject) {
		string jsonString = "{}";
		T obj; //Might not need Casting here but using it anyways
		if (Class.CastTo(obj, inObject) && UJSONHandler<T>.GetString(obj, jsonString)) {
			autoptr UFCallbackLoader<T> cb = new UFCallbackLoader<T>(cbInstance, cbFunction);
			cb.SetObject(obj);
			return U().globals().Load(Mod, cb, jsonString);
		} 
		Error2("[UF] DB HANDLER Load", "Error convertering to JSON or casting make sure you are passing the right class type");
		return -1;
	}
	
	/**
	 * Loads data into the callback instance itself (cbInstance must be of type T)
	 * Useful when a class wants to load its own data from the database
	 * @param cbInstance The instance to load data into (must be type T)
	 * @param cbFunction Optional callback function name (can be empty string)
	 * @return int Call ID, or -1 on error
	 */
	override int LoadSelf(Class cbInstance, string cbFunction = "") {
		string jsonString = "{}";
		T obj; //Might not need Casting here but using it anyways
		if (Class.CastTo(obj, cbInstance) && UJSONHandler<T>.GetString(obj, jsonString)) {
			autoptr UFCallbackLoader<T> cb = new UFCallbackLoader<T>(cbInstance, cbFunction);
			cb.SetObject(obj);
			return U().globals().Load(Mod, cb, jsonString);
		} 
		Error2("[UF] DB HANDLER LoadSelf", "Error convertering to JSON or casting make sure you are passing the right class type");
		return -1;
	}
}


/**
 * UDBGlobalHandlerBase
 * Base class for global database handlers. Should not be used directly.
 * Use UDBGlobalHandler<T> instead for type-safe operations.
 * 
 * Provides base implementations for Save/Load/Update/Transaction operations.
 */
class UDBGlobalHandlerBase extends Managed {
	
	string Mod = "";
	
	/**
	 * Constructor
	 * @param mod The mod identifier used as the database key
	 */
	void UDBGlobalHandlerBase(string mod){
		Mod = mod;
	}
	
	int Save(Class object) {
		Error2("[UF] UDBGlobalHandlerBase SAVE","Incorrect Ussage class is not type of UDBHandler<T>");
		return -1;
	}
	
	int Save(Class object, Class cbInstance, string cbFunction) {
		Error2("[UF] UDBGlobalHandlerBase SAVE","Incorrect Ussage class is not type of UDBHandler<T>");
		return -1;
	}
	
	int Load(Class cbInstance, string cbFunction) {
		Error2("[UF] UDBGlobalHandlerBase LOAD","Incorrect Ussage class is not type of UDBHandler<T>");
		return -1;
	}
	
	int Load(Class cbInstance, string cbFunction, string defaultJson) {
		Error2("[UF] UDBGlobalHandlerBase LOAD","Incorrect Ussage class is not type of UDBHandler<T>");
		return -1;
	}
	
	int Load(Class cbInstance, string cbFunction, Class inObject){
		Error2("[UF] UDBGlobalHandlerBase LOAD","Incorrect Ussage class is not type of UDBHandler<T>");
		return -1;
	}
	int LoadSelf(Class cbInstance, string cbFunction = ""){
		Error2("[UF] UDBGlobalHandlerBase LOAD","Incorrect Ussage class is not type of UDBHandler<T>");
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
	 * Loads global mod data as raw JSON string (no automatic deserialization)
	 * @param cbInstance The callback instance
	 * @param cbFunction The callback function: void OnCallback(int cid, int status, string mod, string jsonData)
	 * @param defaultJson Default JSON to return if no data exists (default: "{}")
	 * @return int Call ID
	 */
	int LoadJson(Class cbInstance, string cbFunction, string defaultJson = "{}") {
		return U().globals().Load(Mod, cbInstance, cbFunction, defaultJson);
	}
	
	/**
	 * Increments a numeric field in the stored object
	 * Shorthand for Transaction(element, value)
	 * @param element The field path (supports dot notation for nested objects)
	 * @param value Amount to increment by (default: 1)
	 * @return int Call ID
	 */
	int Increment(string element, float value = 1){
		return Transaction(element, value);
	}
	
	/*
		Transactions
	
		Updates a sub value inside the object in the database then returns the new value only works with floats or ints
		Sub objects can be used with dot notation aka MySubObject.SubObjectVar
		Will return status of UF_SUCCESS if operations was successful
	*/
	
	/**
	 * Atomically increments/decrements a numeric field and returns the new value
	 * Only works with numeric fields (int/float)
	 * @param element The field path (supports dot notation: "stats.kills")
	 * @param value Amount to add (can be negative for decrement)
	 * @return int Call ID
	 */
	int Transaction(string element, float value) {
		return U().globals().Transaction(Mod,element,value);
	}
	
	/**
	 * Atomically increments/decrements a numeric field with callback
	 * @param element The field path
	 * @param value Amount to add
	 * @param cbInstance The callback instance
	 * @param cbFunction The callback function: void OnCallback(int cid, int status, string mod, UDBTransactionResponse data)
	 * @return int Call ID
	 */
	int Transaction(string element, float value, Class cbInstance, string cbFunction) {
		return U().globals().Transaction(Mod, element, value, new UFCallback<UDBTransactionResponse>(cbInstance, cbFunction));
	}
	//int Transaction(string element, float value, float min, float max, Class cbInstance, string cbFunction) {
	//	return U().globals().Transaction(Mod, element, value, min, max, new UFCallback<UDBTransactionResponse>(cbInstance, cbFunction));
	//}
	
	
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
	 * @param element The field path (supports dot notation: "player.inventory")
	 * @param value The new value as JSON string
	 * @param operation The update operation (SET, PUSH, PULL, etc. - see UpdateOpts in Constants.c)
	 * @return int Call ID
	 * @see UpdateOpts
	 */
	int Update(string element, string value, string operation = UpdateOpts.SET) {
		return U().globals().Update(Mod, element, value, operation);
	}
	
	/**
	 * Updates a specific field with callback
	 * @param element The field path
	 * @param value The new value as JSON string
	 * @param operation The update operation
	 * @param cbInstance The callback instance
	 * @param cbFunction The callback function: void OnCallback(int cid, int status, string mod, UDBUpdateResponse data)
	 * @return int Call ID
	 */
	int Update(string element, string value, string operation, Class cbInstance, string cbFunction) {	
		return U().globals().Update(Mod, element, value, operation, new UFCallback<UDBUpdateResponse>(cbInstance, cbFunction) );
	}
	
	
	/* 
		Call Cancel
		
		This allows you to cancel a call back to prevent access violations 
	*/
	
	/**
	 * Cancels a pending callback to prevent access violations
	 * Useful when the callback instance is being destroyed
	 * @param cid The call ID to cancel
	 */
	static void Cancel(int cid){
		U().RequestCallCancel(cid);
	}
}
