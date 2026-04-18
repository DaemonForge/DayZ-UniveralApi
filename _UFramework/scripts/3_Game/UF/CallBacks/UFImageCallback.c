class UFDLDiscordAvatarCallback : UFRestCallBackBase
{	
    string m_oid;
	
	void UFDLDiscordAvatarCallback(string oid){
		m_oid = oid;
	}
	
	override void OnError(int errorCode) {
		UFLog.Info("[UFDLDiscordAvatarCallback] Save of a File Failed errorCode: " + UUtil.RestErrorToString(errorCode) + "(" + errorCode + ")");
		
		super.OnError(errorCode);
	};
	override void OnTimeout() {
		UFLog.Info("[UFDLDiscordAvatarCallback] Save of a File Timeout");
		super.OnTimeout();
	};
	
	override void OnSuccess(string data, int dataSize) {
        if (data && dataSize > 1 && data != "" && data != "Error")
        {
			string filename =  "$saves:" + m_oid + ".edds";
			if (FileExist(filename)){
				DeleteFile(filename);
			}
        	UFLog.Debug("[UFDLDiscordAvatarCallback] Saving " + filename + " Size: " + dataSize);
            UUtil.SaveBase64ToFileSplit(data, filename);

			super.OnSuccess(data,dataSize);
			return
        }
        UFLog.Err("[UFDLDiscordAvatarCallback] an error occured");
		super.OnSuccess(data,dataSize);
	}

	
}