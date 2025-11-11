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

class UDBGlobalHandler<Class T> extends UDBGlobalHandlerBase{

	/*
	Load and Save
	
	CALLBACK FUNCTION EXAMPLE
	protected void MyCallBackFunction(int cid, int status, string mod, myClass data) {
		if ( status == UF_SUCCESS ){
			//Do something with data
		}
	}*/
		
	override int Save(Class object) {
		string jsonString = "{}";
		T obj; //Might not need Casting here but using it anyways
		if (Class.CastTo(obj, object) && UJSONHandler<T>.GetString(obj, jsonString)) {
			return U().globals().Save(Mod,jsonString);
		}
		Error2("[UF] DB HANDLER Save", "Error convertering to JSON or casting make sure you are passing the right class type");
		return -1;
	}
	override int Save(Class object, Class cbInstance, string cbFunction) {
		string jsonString = "{}";
		T obj; //Might not need Casting here but using it anyways
		if (Class.CastTo(obj, object) && UJSONHandler<T>.GetString(obj, jsonString)) {
			return U().globals().Save(Mod, jsonString, new UFCallback<T>(cbInstance, cbFunction));
		}
		Error2("[UF] DB HANDLER Save", "Error convertering to JSON or casting make sure you are passing the right class type");
		return -1;
	}
	
	
	
	override int Load(Class cbInstance, string cbFunction) {
		return U().globals().Load(Mod,new UFCallback<T>(cbInstance, cbFunction), "{}");
	}
	override int Load(Class cbInstance, string cbFunction, string defaultJson) {
		return U().globals().Load(Mod,new UFCallback<T>(cbInstance, cbFunction), defaultJson);
	}
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


//just to be able to manage them in like an array or map?
class UDBGlobalHandlerBase extends Managed {
	
	string Mod = "";
	
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
	int LoadJson(Class cbInstance, string cbFunction, string defaultJson = "{}") {
		return U().globals().Load(Mod, cbInstance, cbFunction, defaultJson);
	}
	
	int Increment(string element, float value = 1){
		return Transaction(element, value);
	}
	
	/*
		Transactions
	
	
		Updates a sub value inside the object in the database then returns the new value only works with floats or ints
		Sub objects can be used with dot notation aka MySubObject.SubObjectVar
		Will return status of UF_SUCCESS if operations was successful
	*/
	int Transaction(string element, float value) {
		return U().globals().Transaction(Mod,element,value);
	}
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
	int Update(string element, string value, string operation = UpdateOpts.SET) {
		return U().globals().Update(Mod, element, value, operation);
	}
	int Update(string element, string value, string operation, Class cbInstance, string cbFunction) {	
		return U().globals().Update(Mod, element, value, operation, new UFCallback<UDBUpdateResponse>(cbInstance, cbFunction) );
	}
	
	
	/* 
		Call Cancel
		
		This allows you to cancel a call back to prevent access violations 
	*/
	static void Cancel(int cid){
		U().RequestCallCancel(cid);
	}
}
