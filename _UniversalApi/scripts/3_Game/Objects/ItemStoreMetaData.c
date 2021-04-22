class UApiAmmoData extends Managed{
	protected int m_cartIndex;
	protected float m_dmg;
	protected string m_cartTypeName;
	
	void UApiAmmoData(int cartIndex, float dmg, string cartTypeName){
		m_cartIndex = cartIndex;
		m_dmg = dmg;
		m_cartTypeName = cartTypeName;
	}
	
	int cartIndex(){return m_cartIndex;}	
	float dmg(){return m_dmg;}
	string cartTypeName(){return m_cartTypeName;}
}

class UApiMetaData extends Managed{
	string Mod;
	string Var;
	string Data;
	
	void UApiMetaData(string mod, string var, string data){
		Mod = mod;
		Var = var;
		Data = data;
	}
	
	bool Is(string mod, string var){ return Mod == mod && Var == var;}
	string ReadString(){return Data;}
	int ReadInt(){return Data.ToInt();	}
	float ReadFloat(){return Data.ToFloat();}
	vector ReadVector(){ return Data.ToVector(); }
}