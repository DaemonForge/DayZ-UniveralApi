class UApiQueryBase extends Managed{
	
	/*
		
	
	*/
	
	string Query = "{}"; //A mongo DB Query https://docs.mongodb.com/manual/reference/operator/meta/query/
	
	string OrderBy = "{}"; //The OrderBy for the Query https://docs.mongodb.com/manual/reference/operator/meta/orderby/
	
	string ReturnObject = ""; //This if you want to return a specific value within the document
	
	int MaxResults = -1;  //Max Number of Results to return, Note: dayz seems to crash at over 30mb in some of my tests
	
	bool FixQuery = false;  //This will correct queries to match the api's save structure in mongodb 
	
	
	string ToJson(){
		return JsonFileLoader<UApiQueryBase>.JsonMakeData(this);
	}
	
}

class UApiQueryObject extends UApiQueryBase {	
	
	
	void UApiQueryObject(string query = "{}", string orderBy = "{}", int maxResults = -1, string returnObject = ""){
		
		Query = query;
		OrderBy = orderBy;
		ReturnObject = returnObject;
		MaxResults = maxResults;
		FixQuery = false;
	}
	
	
}

class UApiDBQuery extends UApiQueryBase{
		
	void UApiDBQuery(string query = "{}", string orderBy = "{}", bool fixQuery = true, int maxResults = -1, string returnObject = ""){
		Query = query;
		OrderBy = orderBy;
		FixQuery = fixQuery;
		ReturnObject = returnObject;
		MaxResults = maxResults;
	}
	
}

class UApiDBQueryUpdate extends UApiObject_Base {
	string Element;
	string Operation = UpdateOpts.SET; // set | push | pull | unset | mul | rename | pullAll
	string Value;
	autoptr UApiQueryBase Query; 
	
	
	void UApiDBQueryUpdate(UApiQueryBase query, string element, string value, string operation = UpdateOpts.SET){
		Element = element;
		Value = value;
		Operation = operation;
		Query = query;
	}
	
	override string ToJson(){
		string jsonString = UApiJSONHandler<UApiDBQueryUpdate>.ToString(this);
		return jsonString;
	}
	
}
