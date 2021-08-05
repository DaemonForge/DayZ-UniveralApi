static string GetLogPlayerPosArray(array<autoptr UApiLogPlayerPos> thePlayerlist){
	return JsonFileLoader<array<autoptr UApiLogPlayerPos>>.JsonMakeData(thePlayerlist);
}

