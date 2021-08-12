class UApiQueryResult<Class T> : StatusObject {
	
	autoptr array<autoptr T> Results;
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

	array<autoptr T> GetResults(){
		return Results;
	}
}
