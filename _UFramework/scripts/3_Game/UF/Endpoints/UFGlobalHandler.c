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
		return U().globals().Load(Mod, cbInstance, cbFunction, defaultJson));
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
