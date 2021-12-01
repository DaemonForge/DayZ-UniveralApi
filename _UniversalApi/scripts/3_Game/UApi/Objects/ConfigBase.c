/*
Config Base Class

   This is just a template on how you could build your config classes you can inherit this 
but at the end of the day this is just a template to help modders newer to API something 
to start from

*/
class UApiConfigBase : RestCallback {
	protected bool m_DataReceived = false;
	
	
	void Load(){
		m_DataReceived = false;
		SetDefaults();//Set the Defaults so that way, when you load if this its the server Requesting the data it will create it based on the defaults
		
		/*
		Global Configs
		UApi().Rest().GlobalsLoad("MODNAME", this, this.ToJson());
		*/
	}
	
	void Load( string ID){
		m_DataReceived = false;
		SetDefaults();//Set the Defaults so that way, when you load if this its the server Requesting the data it will create it based on the defaults
		
		/*
		Player Configs
		UApi().Rest().PlayerLoad("MODNAME", ID, this, this.ToJson());
		
		Item Configs / Party Configs / anything that could have an ID Number
		UApi().Rest().ItemLoad("MODNAME", ID, this, this.ToJson());
		*/
	}
	

	
	void Save(){
		/* 
		if (GetGame().IsServer()){	//By Default the API is configure to only allow save operations from the server AUTH
		
			Global Configs
			UApi().Rest().GlobalsSave("MODNAME", this.ToJson());
		
			Player Configs
			UApi().Rest().PlayerSave("MODNAME", PlayerGUID, this.ToJson());
		
			Item Configs / Party Configs / anything that could have an ID Number
			UApi().Rest().ItemSave("MODNAME", ItemId, this.ToJson());
		
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
		string jsonString = UApiJSONHandler<UApiConfigBase>.ToString(this);
		Print("[UAPI] Error You didn't override ToJson: " + jsonString); 
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
		JsonFileLoader<UApiConfigBase>.JsonLoadData(data, this);
		if (this){
			OnDataReceive();
		} else {
			Print("[UAPI] CallBack Failed errorCode: Invalid Data");
		}
	};
	
	
		
	// This Are Called by the API System on errors from the API System
	override void OnError(int errorCode) {
		Print("[UAPI] CallBack Failed errorCode: " + UApi().ErrorToString(errorCode));		
	};
	
	override void OnTimeout() {
		Print("[UAPI] CallBack Failed errorCode: Timeout");
	};
}