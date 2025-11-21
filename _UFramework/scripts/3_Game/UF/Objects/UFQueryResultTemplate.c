class UDBQueryResult<Class T> : StatusObject {
	
	autoptr array<autoptr T> Results;
	int Count;
	
	
	static UDBQueryResult<T> CreateFrom(string  stringData){
		UDBQueryResult<T> returnval;
		if (UJSONHandler<UDBQueryResult<T>>.FromString( stringData, returnval)){
			return returnval;
		} 
		Error("[UF] Failed to create Query Results");
		return NULL;
	}
	
	bool FromJson(string stringData) {
		return UJSONHandler<UDBQueryResult<T>>.FromString( stringData, this);
	}

	array<autoptr T> GetResults(){
		return Results;
	}
	
	int Count(){
		return Results.Count();
	}
}
