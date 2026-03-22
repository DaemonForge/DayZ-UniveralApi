/**
 * UDBQueryResult<T> Class
 *
 * Template class for database query results. Contains an array of objects of type T
 * that match the query criteria, along with a count of results.
 *
 * This is the return type for UF().db(OBJECT_DB).Query() operations when using typed callbacks.
 *
 * Usage Example:
 *   // In callback:
 *   void OnQueryResult(int cid, int status, string oid, UDBQueryResult<MyClass> results) {
 *       if (status == UF_SUCCESS) {
 *           array<autoptr MyClass> items = results.GetResults();
 *           Print("Found " + results.Count() + " items");
 *           foreach (autoptr MyClass item : items) {
 *               // Process each item
 *           }
 *       } else if (status == UF_EMPTY) {
 *           Print("No results found");
 *       }
 *   }
 *
 * @see UF().db(OBJECT_DB).Query() for query operations
 * @see UDBQuery for query construction
 */
class UDBQueryResult<Class T> : StatusObject {
	
	autoptr array<autoptr T> Results;
	int Count;
	
	
	/**
	 * CreateFrom
	 *
	 * Static factory method to create a UDBQueryResult from a JSON string.
	 * Useful for manual JSON parsing if needed.
	 *
	 * @param stringData The JSON string containing query results
	 * @return A populated UDBQueryResult<T> instance, or NULL on error
	 */
	static UDBQueryResult<T> CreateFrom(string  stringData){
		UDBQueryResult<T> returnval;
		if (UJSONHandler<UDBQueryResult<T>>.FromString( stringData, returnval)){
			return returnval;
		} 
		Error("[UF] Failed to create Query Results");
		return NULL;
	}
	
	/**
	 * FromJson
	 *
	 * Populates this instance from a JSON string.
	 *
	 * @param stringData The JSON string containing query results
	 * @return True if successful, false on error
	 */
	bool FromJson(string stringData) {
		return UJSONHandler<UDBQueryResult<T>>.FromString( stringData, this);
	}

	/**
	 * GetResults
	 *
	 * Returns the array of result objects matching the query.
	 *
	 * @return Array of autoptr<T> containing all matching results
	 */
	array<autoptr T> GetResults(){
		return Results;
	}
	
	/**
	 * Count
	 *
	 * Returns the number of results in the query.
	 *
	 * @return The count of results in the Results array
	 */
	int Count(){
		return Results.Count();
	}
}
