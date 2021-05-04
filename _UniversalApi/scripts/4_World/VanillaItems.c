modded class TransmitterBase extends ItemTransmitter{
	override void OnUApiSave(UApiEntityStore data){
		super.OnUApiSave(data);
		
		data.WriteInt("Vanilla", "ChannelIndex", GetTunedFrequencyIndex());
	}
	
	override void OnUApiLoad(UApiEntityStore data){
		super.OnUApiLoad(data);
		
		SetFrequencyByIndex(data.GetInt("Vanilla", "ChannelIndex"));
	}
}

modded class Edible_Base extends ItemBase
{

	override void OnUApiSave(UApiEntityStore data){
		super.OnUApiSave(data);
				
		data.WriteFloat("Vanilla", "m_DecayTimer", m_DecayTimer );
		data.WriteInt("Vanilla", "m_LastDecayStage",  m_LastDecayStage );
		data.WriteInt("Vanilla", "m_FoodStage", GetFoodStage().GetFoodStageType());
	}
	
	override void OnUApiLoad(UApiEntityStore data){
		super.OnUApiLoad(data);
		if (!data.ReadFloat("Vanilla", "m_DecayTimer", m_DecayTimer )){
			m_DecayTimer = 0.0;
		}
		if (!data.ReadInt("Vanilla", "m_LastDecayStage", m_LastDecayStage )){
			m_LastDecayStage = FoodStageType.NONE;
		}
		int foodStageType;
		if (data.ReadInt("Vanilla", "m_FoodStage",foodStageType)){
			GetFoodStage().ChangeFoodStage(foodStageType);
		}
	}
	
}
	
modded class BloodContainerBase extends ItemBase
{	
	override void OnUApiSave(UApiEntityStore data){
		super.OnUApiSave(data);
		
		int IsBloodTypeVisible = GetBloodTypeVisible();
		data.WriteInt("Vanilla", "m_IsBloodTypeVisible",  IsBloodTypeVisible );
	}
	
	override void OnUApiLoad(UApiEntityStore data){
		super.OnUApiLoad(data);
		
		int IsBloodTypeVisible = 0;
		if (data.ReadInt("Vanilla", "m_IsBloodTypeVisible", IsBloodTypeVisible ) && IsBloodTypeVisible == 1){
			SetBloodTypeVisible(true);
			SetSynchDirty();
		}
		
	}
}