class UCurrencyValue extends Managed {
	protected string m_TypeClass = "";
	protected int m_Value = 0;
	
	void UCurrencyValue(string itemType, int value){
		m_TypeClass = itemType;
		m_Value = value;
	}

	string TypeClass(){
		return m_TypeClass;
	}
	
	int Value(){
		return m_Value;
	}
}



typedef array<autoptr UCurrencyValue> UCurrencyBase;

class UCurrency extends UCurrencyBase{
	protected static autoptr map<string, autoptr UCurrency> m_UCurrencysMap = new map<string, autoptr UCurrency>;
	
	static UCurrency GetCurrency(string key){
		return m_UCurrencysMap.Get(key);
	}
	
	static int GetLastIndex(string key){
		return m_UCurrencysMap.Get(key).Count() - 1;
	}
	
	static int GetLowestDenominationValue(string key){
		return m_UCurrencysMap.Get(key).LowestDenominationValue();
	}
	
	static UCurrency Register(string key, TStringIntMap currency){
		autoptr UCurrency uCurrency = new UCurrency;
		uCurrency.Create(currency);
		m_UCurrencysMap.Set(key, uCurrency);
		return uCurrency;
	}
	
	static UCurrency InsertNew(string key, array<autoptr UCurrencyValue> values){
		autoptr UCurrency uCurrency = new UCurrency;
		for (int i = 0; i < values.Count(); i++){
			UCurrencyValue value = UCurrencyValue.Cast(values.Get(i));
			autoptr UCurrencyValue v = new UCurrencyValue(value.TypeClass(), value.Value());
			uCurrency.Insert( v );
		}
		m_UCurrencysMap.Set(key, uCurrency);
		return uCurrency;
	}
	
	static int GetAmount(UCurrencyValue MoneyObj, int amount){
		if (MoneyObj){
			return Math.Floor(amount / MoneyObj.Value());
		} 
		return 0;
	}
	
	static void UDebug(){
		Print("[UAPI] UCurrency Debug Start ---------");
		for (int i = 0; i < m_UCurrencysMap.Count(); i++){
			Print("- - - - - - - - - - -");
			Print(m_UCurrencysMap.GetKey(i));
			Print(m_UCurrencysMap.GetElement(i));
			m_UCurrencysMap.GetElement(i).Debug();
			Print("- - - - - - - - - - -");
		}
		Print("[UAPI] UCurrency Debug End  ---------");
	}
	
	protected bool m_IsSorted = false;
	protected bool m_CanUseRuined = true;
	
	void SetCanUseRuined(bool state = true){
		m_CanUseRuined = state;
	}
	
	bool CanUseRuined(){
		return m_CanUseRuined;
	}
	
	void SortCurrency(){
		if (m_IsSorted) return;
		array<autoptr UCurrencyValue> StartingValues =  new array<autoptr UCurrencyValue>;
		array<autoptr UCurrencyValue> SortedMoneyValues = new array<autoptr UCurrencyValue>;
		
		for (int h = 0; h < Count(); h++){
			StartingValues.Insert(Get(h));
		}
		while (StartingValues.Count() > 0){
			UCurrencyValue HighestValue = StartingValues.Get(0);
			for (int i = 1; i < StartingValues.Count(); i++){
				if (StartingValues.Get(i).Value() > HighestValue.Value()){
					HighestValue = StartingValues.Get(i);
				}
			}
			SortedMoneyValues.Insert(HighestValue);
			StartingValues.RemoveItem(HighestValue);
		}
		if (StartingValues.Count() == 1){
			SortedMoneyValues.Insert(StartingValues.Get(0));
		}
		Clear();
		for (int j = 0; j < SortedMoneyValues.Count(); j++){
			Insert(SortedMoneyValues.Get(j));
		}
		m_IsSorted = true;
	}
	
	
	UCurrencyValue GetHighestDenomination(int amount){
		SortCurrency();
		int LastIndex = Count() - 1;
		for (int i = 0; i < Count(); i++){
			if (GetAmount(Get(i), amount) > 0){
				return Get(i);
			}
		}
		return NULL;
	}
	
	string GetTypeClass(int idx){
		return Get(idx).TypeClass();
	}
	
	int GetValue(int idx){
		return Get(idx).Value();
	}
	
	
	void Add(string typeName, int value){
		Insert(new UCurrencyValue(typeName, value));
		m_IsSorted = false;
	}
	
	void Create(TStringIntMap values){
		if (!values) return;
		for (int i = 0; i < values.Count(); i++){
			Add( values.GetKey(i), values.GetElement(i));
		}
		SortCurrency();
	}
	int LastIndex(){
		return Count() - 1;
	}
	int LowestDenominationValue(){
		SortCurrency();
		return Get(LastIndex()).Value();
	}
	UCurrencyValue LowestDenomination(){
		SortCurrency();
		return Get(LastIndex());
	}
}