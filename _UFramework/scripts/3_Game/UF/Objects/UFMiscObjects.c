class UTextObject extends UFObject_Base{
	
	string Text = "";
	
	void UTextObject(string text){
		Text = text;
	}
	
	override string ToJson(){
		string jsonString = JsonFileLoader<UTextObject>.JsonMakeData(this);
		return jsonString;
	}
}

class URandomNumberResponse extends StatusObject {
	
	autoptr TIntArray Numbers;
	
}

class URandomNumberRequest extends UFObject_Base{
	int Count;
	
	void URandomNumberRequest(int count){
		Count = count;
	}
	
	override string ToJson(){
		string jsonString = JsonFileLoader<URandomNumberRequest>.JsonMakeData(this);
		return jsonString;
	}
}


/**
 * UMapLocation
 * ------------
 * Represents a named location on the map (city, town, village, etc.)
 * retrieved from CfgWorlds configuration.
 *
 * Properties:
 *   - ClassName: The config class name of the location entry.
 *   - Name: The display name of the location (e.g., "Chernogorsk").
 *   - Type: The location type (e.g., "City", "Village", "Capital").
 *   - Position: The 3D world position of the location (includes terrain height).
 *
 * Common Location Types:
 *   - "Capital" - Major cities
 *   - "City" - Large towns/cities
 *   - "Village" - Small villages
 *   - "Local" - Local landmarks
 *   - "Marine" - Marine/coastal points
 *   - "Hill" - Hills and elevated areas
 *   - "Ruin" - Ruins and historical sites
 *   - "ViewPoint" - Scenic viewpoints
 */
class UMapLocation
{
	string ClassName;
	string Name;
	string Type;
	vector Position;
}