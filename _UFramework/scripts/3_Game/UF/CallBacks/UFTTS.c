/*
 * UFDownloadAudio Class:
 * -----------------------
 * Inherits from: UFRestCallBackBase
 *
 * Description:
 *   Handles the process of downloading and saving audio content.
 *   This class primarily validates and processes the base64 encoded data received from a REST call,
 *   saves it as an MP4 file in a designated saves directory, and logs success or error messages.
 *
 * Constructor:
 *   UFDownloadTTS(string oid)
 *     - Initializes the instance with a unique identifier (oid) that is later used to generate the filename.
 *
 * Overridden Methods:
 *   OnError(int errorCode)
 *     - Logs an error message indicating that the file save operation has failed,
 *       providing the error code for debugging.
 *     - Invokes the parent class’s OnError method to ensure further error handling.
 *
 *   OnTimeout()
 *     - Logs a timeout message indicating that the file save operation timed out.
 *     - Calls the parent class’s OnTimeout to allow for standard timeout processing.
 *
 *   OnSuccess(string data, int dataSize)
 *     - Validates that the incoming data is non-empty, of sufficient size, and not an error indicator.
 *     - Constructs a filename using the provided oid and a fixed ".mp4" extension.
 *     - Saves the base64 encoded file data using UUtil.SaveBase64ToFileSplit.
 *     - Logs a success message after saving the file.
 *     - Calls the parent class’s OnSuccess method for any additional processing.
 *     - If validation fails, logs an error message and delegates handling to the parent class.
 */
class UFDownloadTTS : UFRestCallBackBase
{
    string m_oid;
	
	void UFDownloadTTS(string oid){
		m_oid = oid;
	}
	
	override void OnError(int errorCode) {
		Print("[UF] [UFDownloadAudio] Save of a File Failed errorCode: " + errorCode);
		
		super.OnError(errorCode);
	};
	override void OnTimeout() {
		Print("[UF] [UFDownloadAudio] Save of a File Timeout");
		super.OnTimeout();
	};
	
	override void OnSuccess(string data, int dataSize) {
        if (data && dataSize > 1 && data != "" && data != "Error")
        {
			int ln = m_oid.Length() - 12;
			int rnd = Math.RandomInt(100,990);
			string filename =  "$saves:" + m_oid + ".mp4";
            UUtil.SaveBase64ToFileSplit(data, filename);
			Print("[UF] [UFDownloadAudio] File '" + filename + "' saved successfully.");

			super.OnSuccess(data,dataSize);
			return
        }
        Print("[UF] [UFDownloadAudio] an error occured");
		super.OnSuccess(data,dataSize);
	}

	
}

/*
 * UDLTTSNestedCallback Class:
 * --------------------------------
 * Inherits from: UNestedCallBack
 *
 * Description:
 *   Serves as an intermediary callback handling nested responses for audio file operations.
 *   Specifically, it deals with additional validation and file saving when the async request
 *   returns a success response.
 *
 * Overridden Methods:
 *   OnSuccess(string data, int dataSize)
 *     - Checks if the original call has been canceled; if so, logs this event and calls the parent callback.
 *     - Validates the size and content of the data to ensure that it is not empty.
 *     - Constructs a filename using the oid from the associated primary callback.
 *     - Saves the base64 encoded data using UUtil.SaveBase64ToFile.
 *     - Logs a success message after saving the file.
 *     - Delegates further processing to the parent class’s OnSuccess method.
 */
class UDLTTSNestedCallback : UNestedCallBack
{
	override void OnSuccess(string data, int dataSize) {
		if (U().IsCallCanceled(m_UFid)){
			Print("[UF] Call " + m_UFid + " not called as it was requested to be canceled - OnSuccess");
			super.OnSuccess(data, dataSize);
			return;
		}
		if (dataSize <= 1 || data == "" ){
			GetCB().OnError(UF_EMPTY, m_UFid);
			super.OnSuccess(data, dataSize);
			return;
		}
		int rnd = Math.RandomInt(100,990);
		string filename =  "$saves:" + GetCB().GetOID() + ".mp4";
        UUtil.SaveBase64ToFile(data, filename);
		Print("[UF] [UFDownloadTTS] File '" + filename + "' saved successfully.");

		super.OnSuccess(data,dataSize);
	}
}


/*
 * UDLTTSCallback Class:
 * -------------------------
 * Inherits from: UFCallbackBase
 *
 * Description:
 *   Provides callback functionality that integrates with the game’s scripting engine.
 *   It formats and forwards responses from audio file operations to specified game script functions.
 *
 * Overridden Methods:
 *   OnError(int errorCode, int cid)
 *     - If there is a valid target instance and function defined,
 *       invokes the designated game script function, passing:
 *         - Callback id (cid)
 *         - Error code
 *         - Object identifier (OID)
 *         - A literal "Error" string.
 *
 *   OnSuccess(string jsonData, int cid)
 *     - If a valid instance and function exist,
 *       invokes the game script function with:
 *         - Callback id (cid)
 *         - A success constant (UF_SUCCESS)
 *         - Object identifier (OID)
 *         - A literal "Success" string.
 */
class UDLTTSCallback : UFCallbackBase {
	
	override void OnError(int errorCode, int cid) {
		if (GetInstance() && Function != ""){
			g_Game.GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, string>(cid, errorCode, OID, "Error"));
		}
	}
		
	override void OnSuccess(string jsonData, int cid) {
		if (GetInstance() && Function != ""){
			g_Game.GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, string>(cid, UF_SUCCESS, OID, "Success"));
		}
	}
}

class UTTSStatusCallback : UFCallbackBase {
	
	override void OnError(int errorCode, int cid) {
		if (GetInstance() && Function != ""){
			g_Game.GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, string>(cid, errorCode, OID, "Error"));
		}
	}
		
	override void OnSuccess(string jsonData, int cid) {
		int rstatus = UF_JSONERROR;
		StatusObject obj;
		if (GetInstance() && Function != ""){
			if (UJSONHandler<StatusObject>.FromString(jsonData, obj)){
				if (obj){
					switch (obj.Status) {
						case "NotFound":
							rstatus = UF_NOTFOUND;
							break;
						case "Empty":
							rstatus = UF_EMPTY;
							break;
						case "Error":
							rstatus = UF_ERROR;
							break;
						case "NoAuth":
							rstatus = UF_UNAUTHORIZED;
							break;
						case "NotSetup":
							rstatus = UF_NOTSETUP;
							break;
					}
					g_Game.GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, string>(cid, rstatus, OID, obj.Status));
					return;
				}
			}
		}
		OnError(rstatus, cid);
	}
}

class UGenTTSCallback : UFCallbackBase {
	
	override void OnError(int errorCode, int cid) {
		if (GetInstance() && Function != ""){
			g_Game.GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, string>(cid, errorCode, OID, "Error"));
		}
	}
		
	override void OnSuccess(string jsonData, int cid) {
		if (GetInstance() && Function != ""){
			g_Game.GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, string>(cid, UF_SUCCESS, OID, "Success"));
		}
	}
}