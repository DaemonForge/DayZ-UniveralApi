class UApiQueryResult<Class T> : StatusObject {
	
	autoptr array<ref T> Results;
	int Count;
	
	
	static UApiQueryResult<T> CreateFrom(string  stringData){
		UApiQueryResult<T> returnval;
		if (UApiJSONHandler<UApiQueryResult<T>>.FromString( stringData, returnval)){
			return returnval;
		} 
		Error("[UAPI] Failed to create Query Results");
		return NULL;
	}
	
	bool FromJson(string stringData) {
		return UApiJSONHandler<UApiQueryResult<T>>.FromString( stringData, this);
	}

	
	array<ref T> GetResults(){
		return Results;
	}
}
