class UDBQueryBase extends Managed{
	
	/*
		
	
	*/
	
	string Query = "{}"; //A mongo DB Query https://docs.mongodb.com/manual/reference/operator/meta/query/
	
	string OrderBy = "{}"; //The OrderBy for the Query https://docs.mongodb.com/manual/reference/operator/meta/orderby/
	
	string ReturnObject = ""; //This if you want to return a specific value within the document
	
	int MaxResults = -1;  //Max Number of Results to return, Note: dayz seems to crash at over 30mb in some of my tests
	
	bool FixQuery = false;  //This will correct queries to match the api's save structure in mongodb 
	
	
	string ToJson(){
		return JsonFileLoader<UDBQueryBase>.JsonMakeData(this);
	}
	
}

class UDBQueryObject extends UDBQueryBase {	
	
	
	void UDBQueryObject(string query = "{}", string orderBy = "{}", int maxResults = -1, string returnObject = ""){
		
		Query = query;
		OrderBy = orderBy;
		ReturnObject = returnObject;
		MaxResults = maxResults;
		FixQuery = true;
	}
	
	
}

class UDBQuery extends UDBQueryBase{
		
	void UDBQuery(string query = "{}", string orderBy = "{}", bool fixQuery = true, int maxResults = -1, string returnObject = ""){
		Query = query;
		OrderBy = orderBy;
		FixQuery = fixQuery;
		ReturnObject = returnObject;
		MaxResults = maxResults;
	}
	
}

class UDBQueryUpdate extends UFObject_Base {
	string Element;
	string Operation = UpdateOpts.SET; // set | push | pull | unset | mul | rename | pullAll
	string Value;
	autoptr UDBQueryBase Query; 
	
	
	void UDBQueryUpdate(UDBQueryBase query, string element, string value, string operation = UpdateOpts.SET){
		Element = element;
		Value = value;
		Operation = operation;
		Query = query;
	}
	
	override string ToJson(){
		string jsonString = UJSONHandler<UDBQueryUpdate>.ToString(this);
		return jsonString;
	}
	
}
