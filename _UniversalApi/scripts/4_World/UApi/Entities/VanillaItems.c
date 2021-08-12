modded class TransmitterBase extends ItemTransmitter{
	override void OnUApiSave(UApiEntityStore data){
		super.OnUApiSave(data);
		
		data.Write("ChannelIndex", GetTunedFrequencyIndex());
	}
	
	override void OnUApiLoad(UApiEntityStore data){
		super.OnUApiLoad(data);
		
		SetFrequencyByIndex(data.GetInt("ChannelIndex"));
	}
}

modded class Edible_Base extends ItemBase
{

	override void OnUApiSave(UApiEntityStore data){
		super.OnUApiSave(data);
		
		data.Write("m_DecayTimer", m_DecayTimer );
		data.Write("m_LastDecayStage",  m_LastDecayStage );
		if (GetFoodStage()){
			data.Write("m_FoodStage", GetFoodStage().GetFoodStageType());
		}
	}
	
	override void OnUApiLoad(UApiEntityStore data){
		super.OnUApiLoad(data);
		if (!data.Read("m_DecayTimer", m_DecayTimer )){
			m_DecayTimer = 0.0;
		}
		if (!data.Read("m_LastDecayStage", m_LastDecayStage )){
			m_LastDecayStage = FoodStageType.NONE;
		}
		int foodStageType;
		if (data.Read("m_FoodStage",foodStageType) && GetFoodStage()){
			GetFoodStage().ChangeFoodStage(foodStageType);
		}
	}
	
}
	
modded class BloodContainerBase extends ItemBase
{	
	override void OnUApiSave(UApiEntityStore data){
		super.OnUApiSave(data);
		
		int IsBloodTypeVisible = GetBloodTypeVisible();
		data.Write("m_IsBloodTypeVisible",  IsBloodTypeVisible );
	}
	
	override void OnUApiLoad(UApiEntityStore data){
		super.OnUApiLoad(data);
		
		int IsBloodTypeVisible = 0;
		if (data.Read("m_IsBloodTypeVisible", IsBloodTypeVisible ) && IsBloodTypeVisible == 1){
			SetBloodTypeVisible(true);
			SetSynchDirty();
		}
		
	}
}