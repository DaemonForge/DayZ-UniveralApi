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
	protected string Var;
	protected string Data;
	
	void UApiMetaData(string var, string data){
		Var = var;
		Data = data;
	}
	
	bool Is(string var){ return Var == var;}
	string ReadString(){return Data;}
	int ReadInt(){return Data.ToInt();	}
	float ReadFloat(){return Data.ToFloat();}
	vector ReadVector(){ return Data.ToVector(); }
}


class UApiZoneHealthData extends Managed{
	string m_Zone;
	float m_Health;
	
	void UApiZoneHealthData(string zone, float health){
		m_Zone = zone;
		m_Health = health;
	}
	
	bool Is(string zone){ return (m_Zone == zone); }
	string Zone(){ return m_Zone; }
	float Health(){ return m_Health; }
}
