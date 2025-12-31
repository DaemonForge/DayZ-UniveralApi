/*
Config Base Class

   This is just a template on how you could build your config classes you can inherit this 
but at the end of the day this is just a template to help modders newer to API something 
to start from

*/
class UFConfigBase : UFRestCallBackBase {
	protected bool m_DataReceived = false;
	
	
	void Load(){
		m_DataReceived = false;
		SetDefaults();//Set the Defaults so that way, when you load if this its the server Requesting the data it will create it based on the defaults
		
		/*
		Global Configs
		U().Rest().GlobalsLoad("MODNAME", this, this.ToJson());
		*/
	}
	
	void Load( string ID){
		m_DataReceived = false;
		SetDefaults();//Set the Defaults so that way, when you load if this its the server Requesting the data it will create it based on the defaults
		
		/*
		Player Configs
		U().Rest().PlayerLoad("MODNAME", ID, this, this.ToJson());
		
		Item Configs / Party Configs / anything that could have an ID Number
		U().Rest().ItemLoad("MODNAME", ID, this, this.ToJson());
		*/
	}
	

	
	void Save(){
		/* 
		if (g_Game.IsServer()){	//By Default the API is configure to only allow save operations from the server AUTH
		
			Global Configs
			U().Rest().GlobalsSave("MODNAME", this.ToJson());
		
			Player Configs
			U().Rest().PlayerSave("MODNAME", PlayerGUID, this.ToJson());
		
			Item Configs / Party Configs / anything that could have an ID Number
			U().Rest().ItemSave("MODNAME", ItemId, this.ToJson());
		
		}
		*/
	}
	
	void SetDefaults(){
		/*
	
		  This is to set the defaults for the mod before requesting a load so that way 
		if it doesn't exsit the API will create the file
	
		*/
	}
	
	string ToJson(){
		// Override and Replace with your class Name
		string jsonString = UJSONHandler<UFConfigBase>.ToString(this);
		UFLog.Err("You didn't override ToJson: " + jsonString); 
		return jsonString;
	}
	
	
	
	
	void SetDataReceived(bool dataReceived = true){
		m_DataReceived = dataReceived;
	}
	
	bool DataReceived(){
		return m_DataReceived;
	}
	
	void OnDataReceive(){
		SetDataReceived();
		/*
		if(ModVersion != CurrentVersion){
			DoSome Code Upgrade
		
			Save(); //Resave the upgrade Version Back to the server
		}
		*/
	}
	
	
	// This is called by the API System on the successfull response from the API
	override void OnSuccess(string data, int dataSize) {
		JsonFileLoader<UFConfigBase>.JsonLoadData(data, this);
		if (this){
			OnDataReceive();
		} else {
			UFLog.Err("CallBack Failed errorCode: Invalid Data");
		}
		//dont' call super or it will delete the object
	};
	
	
		
	// This Are Called by the API System on errors from the API System
	override void OnError(int errorCode) {
		UFLog.Err("CallBack Failed errorCode: " + U().ErrorToString(errorCode));
	};
	
	override void OnTimeout() {
		UFLog.Err("CallBack Failed errorCode: Timeout");
	};
}