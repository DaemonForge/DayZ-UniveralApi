class UApiQueryObject{
	
	/*
		
	
	*/
	
	string Query = "{}"; //A mongo DB Query https://docs.mongodb.com/manual/reference/operator/meta/query/
	
	string OrderBy = "{}"; //The OrderBy for the Query https://docs.mongodb.com/manual/reference/operator/meta/orderby/
	
	string ReturnObject = ""; //This if you want to return a specific value within the document
	
	int MaxResults = -1;  //Max Number of Results to return, Note: dayz seems to crash at over 30mb in some of my tests
	
	
	
	void UApiQueryObject(string query = "{}", string orderBy = "{}", int maxResults = -1, string returnObject = ""){
		
		Query = query;
		OrderBy = orderBy;
		ReturnObject = returnObject;
		MaxResults = maxResults;
		
	}
	
	string ToJson(){
		return JsonFileLoader<UApiQueryObject>.JsonMakeData(this);;
	}
	
}
