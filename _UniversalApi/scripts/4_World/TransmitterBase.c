modded class TransmitterBase extends ItemTransmitter{
	override void OnUApiSave(EntityStore data){
		super.OnUApiSave(data);
		
		data.WriteInt("Vanilla", "ChannelIndex", GetTunedFrequencyIndex());
	}
	
	override void OnUApiLoad(EntityStore data){
		super.OnUApiLoad(data);
		
		SetFrequencyByIndex(data.GetInt("Vanilla", "ChannelIndex"));
	}
}

modded class Edible_Base extends ItemBase
{

	override void OnUApiSave(EntityStore data){
		super.OnUApiSave(data);
				
		data.WriteFloat("Vanilla", "m_DecayTimer", m_DecayTimer );
		data.WriteInt("Vanilla", "m_LastDecayStage",  m_LastDecayStage );
	}
	
	override void OnUApiLoad(EntityStore data){
		super.OnUApiLoad(data);
		if (!data.ReadFloat("Vanilla", "m_DecayTimer", m_DecayTimer )){
			m_DecayTimer = 0.0;
		}
		if (!data.ReadInt("Vanilla", "m_LastDecayStage", m_LastDecayStage )){
			m_LastDecayStage = FoodStageType.NONE;
		}
	}
	
}
	
modded class BloodContainerBase extends ItemBase
{	
	override void OnUApiSave(EntityStore data){
		super.OnUApiSave(data);
		
		int IsBloodTypeVisible = GetBloodTypeVisible();
		data.WriteInt("Vanilla", "m_IsBloodTypeVisible",  IsBloodTypeVisible );
	}
	
	override void OnUApiLoad(EntityStore data){
		super.OnUApiLoad(data);
		
		int IsBloodTypeVisible = 0;
		if (data.ReadInt("Vanilla", "m_IsBloodTypeVisible", IsBloodTypeVisible ) && IsBloodTypeVisible == 1){
			SetBloodTypeVisible(true);
		}
		
	}
}