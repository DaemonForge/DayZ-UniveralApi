/* 
	Template Global Handler
	This is the newest Method in which modders can interact with the Universal API's MongoDB Endpoints providing access to modders to be
	able to build mods that utilize the Universal API Webservice.
	
	view full documentation https://github.com/daemonforge/DayZ-UniveralApi/wiki/Developer-Reference

	static autoptr UApiGlobalHandler<myClass> m_MyModHandler = new UApiGlobalHandler<myClass>("MyMod");
	
	m_MyModHandler.Save(myObject); //returns Call ID
	m_MyModHandler.Load(player, "MyCallBackFunction"); //returns Call ID


	
*/

class UApiGlobalHandler<Class T> extends UApiGlobalHandlerBase{

	/*
	Load and Save
	
	CALLBACK FUNCTION EXAMPLE
	protected void MyCallBackFunction(int cid, int status, string mod, myClass data) {
		if ( status == UAPI_SUCCESS ){
			//Do something with data
		}
	}*/
		
	override int Save(Class object) {
		string jsonString = "{}";
		T obj; //Might not need Casting here but using it anyways
		if (Class.CastTo(obj, object) && UApiJSONHandler<T>.GetString(obj, jsonString)) {
			return UApi().globals().Save(Mod,jsonString);
		}
		Error2("[UAPI] DB HANDLER Save", "Error convertering to JSON or casting make sure you are passing the right class type");
		return -1;
	}
	override int Save(Class object, Class cbInstance, string cbFunction) {
		string jsonString = "{}";
		T obj; //Might not need Casting here but using it anyways
		if (Class.CastTo(obj, object) && UApiJSONHandler<T>.GetString(obj, jsonString)) {
			return UApi().globals().Save(Mod, jsonString, new UApiCallback<T>(cbInstance, cbFunction));
		}
		Error2("[UAPI] DB HANDLER Save", "Error convertering to JSON or casting make sure you are passing the right class type");
		return -1;
	}
	
	
	
	override int Load(Class cbInstance, string cbFunction) {
		return UApi().globals().Load(Mod,new UApiCallback<T>(cbInstance, cbFunction), "{}");
	}
	override int Load(Class cbInstance, string cbFunction, string defaultJson) {
		return UApi().globals().Load(Mod,new UApiCallback<T>(cbInstance, cbFunction), defaultJson);
	}
	override int Load(Class cbInstance, string cbFunction, Class inObject) {
		string jsonString = "{}";
		T obj; //Might not need Casting here but using it anyways
		if (Class.CastTo(obj, inObject) && UApiJSONHandler<T>.GetString(obj, jsonString)) {
			autoptr UApiCallbackLoader<T> cb = new UApiCallbackLoader<T>(cbInstance, cbFunction);
			cb.SetObject(obj);
			return UApi().globals().Load(Mod, cb, jsonString);
		} 
		Error2("[UAPI] DB HANDLER Load", "Error convertering to JSON or casting make sure you are passing the right class type");
		return -1;
	}
	override int LoadSelf(Class cbInstance, string cbFunction = "") {
		string jsonString = "{}";
		T obj; //Might not need Casting here but using it anyways
		if (Class.CastTo(obj, cbInstance) && UApiJSONHandler<T>.GetString(obj, jsonString)) {
			autoptr UApiCallbackLoader<T> cb = new UApiCallbackLoader<T>(cbInstance, cbFunction);
			cb.SetObject(obj);
			return UApi().globals().Load(Mod, cb, jsonString);
		} 
		Error2("[UAPI] DB HANDLER LoadSelf", "Error convertering to JSON or casting make sure you are passing the right class type");
		return -1;
	}
}


//just to be able to manage them in like an array or map?
class UApiGlobalHandlerBase extends Managed {
	
	string Mod = "";
	
	void UApiGlobalHandlerBase(string mod){
		Mod = mod;
	}
	
	int Save(Class object) {
		Error2("[UAPI] UApiGlobalHandlerBase SAVE","Incorrect Ussage class is not type of UApiDBHandler<T>");
		return -1;
	}
	
	int Save(Class object, Class cbInstance, string cbFunction) {
		Error2("[UAPI] UApiGlobalHandlerBase SAVE","Incorrect Ussage class is not type of UApiDBHandler<T>");
		return -1;
	}
	
	int Load(Class cbInstance, string cbFunction) {
		Error2("[UAPI] UApiGlobalHandlerBase LOAD","Incorrect Ussage class is not type of UApiDBHandler<T>");
		return -1;
	}
	
	int Load(Class cbInstance, string cbFunction, string defaultJson) {
		Error2("[UAPI] UApiGlobalHandlerBase LOAD","Incorrect Ussage class is not type of UApiDBHandler<T>");
		return -1;
	}
	
	int Load(Class cbInstance, string cbFunction, Class inObject){
		Error2("[UAPI] UApiGlobalHandlerBase LOAD","Incorrect Ussage class is not type of UApiDBHandler<T>");
		return -1;
	}
	int LoadSelf(Class cbInstance, string cbFunction = ""){
		Error2("[UAPI] UApiGlobalHandlerBase LOAD","Incorrect Ussage class is not type of UApiDBHandler<T>");
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
	int LoadJson(Class cbInstance, string cbFunction, string defaultJson = "{}") {
		return UApi().globals().Load(Mod, cbInstance, cbFunction, defaultJson));
	}
	
	int Increment(string element, float value = 1){
		return Transaction(element, value);
	}
	
	/*
		Transactions
	
	
		Updates a sub value inside the object in the database then returns the new value only works with floats or ints
		Sub objects can be used with dot notation aka MySubObject.SubObjectVar
		Will return status of UAPI_SUCCESS if operations was successful
	*/
	int Transaction(string element, float value) {
		return UApi().globals().Transaction(Mod,element,value);
	}
	int Transaction(string element, float value, Class cbInstance, string cbFunction) {
		return UApi().globals().Transaction(Mod, element, value, new UApiCallback<UApiTransactionResponse>(cbInstance, cbFunction));
	}
	//int Transaction(string element, float value, float min, float max, Class cbInstance, string cbFunction) {
	//	return UApi().globals().Transaction(Mod, element, value, min, max, new UApiCallback<UApiTransactionResponse>(cbInstance, cbFunction));
	//}
	
	
	/*
	Update
	
		Updates a sub value inside the object in the database, can also use other operations
		https://github.com/daemonforge/DayZ-UniveralApi/blob/master/_UniversalApi/scripts/1_Core/Constants.c#L30
	
		Values can be in JSON format to update or push elements into arrays
		
		Sub objects can be used with dot notation aka MySubObject.SubObjectVar
		will return status of UAPI_SUCCESS if operations was successful
	*/
	int Update(string element, string value, string operation = UpdateOpts.SET) {
		return UApi().globals().Update(Mod, element, value, operation);
	}
	int Update(string element, string value, string operation, Class cbInstance, string cbFunction) {	
		return UApi().globals().Update(Mod, element, value, operation, new UApiCallback<UApiUpdateResponse>(cbInstance, cbFunction) );
	}
	
	
	/* 
		Call Cancel
		
		This allows you to cancel a call back to prevent access violations 
	*/
	static void Cancel(int cid){
		UApi().RequestCallCancel(cid);
	}
}
