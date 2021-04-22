class UApiJSONHandler<Class T>
{
	protected static ref JsonSerializer m_Serializer = new JsonSerializer;
	
	static bool FromString(string stringData, out T data)
	{
		string error;
		if (!m_Serializer)
			m_Serializer = new JsonSerializer;
		
		if (!m_Serializer.ReadFromString(data, stringData, error)) {
			Print("Error Creating Data from Json");
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
			Print("Error Creating JSON from Data");
			return "";
		}
		
		return stringData;
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
				Print("File At" + path + " could not be parsed");
			}
		} else {
			Print("File At" + path + " could not be opened");
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
		}
		CloseFile(fh);
	}

};