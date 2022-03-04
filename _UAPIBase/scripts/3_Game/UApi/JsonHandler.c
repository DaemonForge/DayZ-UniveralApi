class UApiJSONHandler<Class T>
{
	protected static ref JsonSerializer m_Serializer = new JsonSerializer;
	
	static bool FromString(string stringData, out T data)
	{
		string error;
		if (!m_Serializer)
			m_Serializer = new JsonSerializer;
		
		if (stringData == "" || !m_Serializer.ReadFromString(data, stringData, error)) {
			Print(stringData);
			Error2("[UAPI] FROM STRING", "Error Creating Data from Json : " + error);
			return false;
		}
		return true;
	}
	
	static string ToString(T data)
	{
		string stringData;
		if (!m_Serializer)
			m_Serializer = new JsonSerializer;

		if (!m_Serializer.WriteToString(data, false, stringData)) {
			Print(stringData);
			Error2("[UAPI] TO STRING", "Error Creating JSON from Data");
			return "{}";
		}
		
		return stringData;
	}
	
	static bool GetString(T data, out string stringData)
	{
		if (!m_Serializer)
			m_Serializer = new JsonSerializer;

		if (!m_Serializer.WriteToString(data, false, stringData)) {
			Print(stringData);
			Error2("[UAPI] GET STRING", "Error Creating JSON from Data");
			return false;
		}
		
		return true;
	}

	static void FromFile(string path, out T data)
	{
		if (!FileExist(path)) {
			Print("File At" + path + " could not be found");
			return;
		}

		FileHandle fh = OpenFile(path, FileMode.READ);
		string jsonData;
		string error;

		if (fh) {
			
			string line;
			while (FGets(fh, line) > 0) {
				jsonData = jsonData + "\n" + line;
			}

			CloseFile(fh);
			
			bool success = m_Serializer.ReadFromString(data, jsonData, error);
			
			if (error != "" || !success) {
				Error2("File At" + path + " could not be parsed",error);
			}
		} else {
			Error2("File At" + path + " could not be opened","");
		}
	}
	
	static void ToFile(string path, T data)
	{
		FileHandle fh = OpenFile(path, FileMode.WRITE);
			
		if (!fh) {
			Print("File At" + path + " could not be created");
			return;
		} 
		
		string jsonData;
		bool success = m_Serializer.WriteToString(data, true, jsonData);

		if (success && jsonData != string.Empty) {
			FPrintln(fh, jsonData);
		} else {
			Error2("File At" + path + " could not be saved","");
		}
		CloseFile(fh);
	}

};