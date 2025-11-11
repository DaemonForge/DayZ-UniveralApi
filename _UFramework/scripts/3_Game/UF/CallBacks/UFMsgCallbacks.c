class UFMsgStringCallback extends UFCallbackBase{
	
	override void OnError(int errorCode, int cid) {
		if (GetInstance() && Function != "") {
			g_Game.GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, TStringArray>(cid, errorCode, OID, NULL));
		}
	}
	
	override void OnSuccess(string jsonData, int cid) {
		if (GetInstance() && Function != ""){
			autoptr UReadMsgString obj;
			if (UJSONHandler<UReadMsgString>.FromString(jsonData, obj)){
				int rstatus = UF_SUCCESS;
				StatusObject sobj;
				if (Class.CastTo(sobj, obj)){
					switch (sobj.Status) {
						case "NotFound":
							rstatus = UF_NOTFOUND;
							break;
						case "Empty":
							rstatus = UF_EMPTY;
							break;
						case "Error":
							rstatus = UF_ERROR;
							break;
						case "NoPerms":
							rstatus = UF_UNAUTHORIZED;
							break;
						case "NoAuth":
							rstatus = UF_UNAUTHORIZED;
							break;
						case "InvalidAuth":
							rstatus = UF_UNAUTHORIZED;
							break;
						case "NotSetup":
							rstatus = UF_NOTSETUP;
							break;
					}
				}
				g_Game.GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, TStringArray>(cid, rstatus, OID, obj.GetMessages()));
			} else {
				g_Game.GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, TStringArray>(cid, UF_JSONERROR, OID, NULL));
			}
		}
	}

}

class UFMsgCallback<Class T> extends UFCallbackBase{
	
	override void OnError(int errorCode, int cid) {
		if (GetInstance() && Function != "") {
			g_Game.GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, array<T>>(cid, errorCode, OID, NULL));
		}
	}
	
	override void OnSuccess(string jsonData, int cid) {
		if (GetInstance() && Function != ""){
			autoptr UReadMsg<T> obj;
			if (UJSONHandler<UReadMsg<T>>.FromString(jsonData, obj)){
				int rstatus = UF_SUCCESS;
				StatusObject sobj;
				if (Class.CastTo(sobj, obj)){
					switch (sobj.Status) {
						case "NotFound":
							rstatus = UF_NOTFOUND;
							break;
						case "Empty":
							rstatus = UF_EMPTY;
							break;
						case "Error":
							rstatus = UF_ERROR;
							break;
						case "NoPerms":
							rstatus = UF_UNAUTHORIZED;
							break;
						case "NoAuth":
							rstatus = UF_UNAUTHORIZED;
							break;
						case "InvalidAuth":
							rstatus = UF_UNAUTHORIZED;
							break;
						case "NotSetup":
							rstatus = UF_NOTSETUP;
							break;
					}
				}
				g_Game.GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, array<T>>(cid, rstatus, OID, obj.GetMessages()));
			} else {
				g_Game.GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, array<T>>(cid, UF_JSONERROR, OID, NULL));
			}
		}
	}

}
