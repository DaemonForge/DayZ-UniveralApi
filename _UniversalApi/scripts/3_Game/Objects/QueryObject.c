class UApiQueryObject{
	
	/*
		
	
	*/
	
	string Query = "{}"; //A mongo DB Query https://docs.mongodb.com/manual/reference/operator/meta/query/
	
	string OrderBy = "{}"; //The OrderBy for the Query https://docs.mongodb.com/manual/reference/operator/meta/orderby/
	
	string ReturnObject = ""; //
	
	int MaxResults = -1;
	
	string ToJson(){
		return JsonFileLoader<UApiQueryObject>.JsonMakeData(this);;
	}
	
}
