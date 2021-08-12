modded class CarScript extends Car {

	override void OnUApiSave(UApiEntityStore data) {
		super.OnUApiSave(data);
		
		data.Write("m_FuelAmmount", m_FuelAmmount);
		data.Write("m_CoolantAmmount", m_CoolantAmmount);
		data.Write("m_OilAmmount", m_OilAmmount);
		data.Write("m_BrakeAmmount", m_BrakeAmmount);
		
		m_EngineHealth = GetHealth01("Engine", "");
		data.Write("m_EngineHealth", m_EngineHealth);
		m_FuelTankHealth = GetHealth01("FuelTank", "");
		data.Write("m_FuelTankHealth", m_FuelTankHealth);
	}
	
	override void OnUApiLoad(UApiEntityStore data){
		super.OnUApiLoad(data);
		
		data.Read("m_FuelAmmount", m_FuelAmmount);
		data.Read("m_CoolantAmmount", m_CoolantAmmount);
		data.Read("m_OilAmmount", m_OilAmmount);
		data.Read("m_BrakeAmmount", m_BrakeAmmount);
		
		if (data.Read("m_EngineHealth", m_EngineHealth)){
			SetHealth01("Engine", "", m_EngineHealth);
		}
		if (data.Read("m_FuelTankHealth", m_FuelTankHealth)){
			SetHealth01("FuelTank", "", m_FuelTankHealth);
		}
	}

}