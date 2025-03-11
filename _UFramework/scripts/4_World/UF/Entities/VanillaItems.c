modded class TransmitterBase extends ItemTransmitter{
	override void OnUFSave(UEntityStore data){
		super.OnUFSave(data);
		
		data.Write("ChannelIndex", GetTunedFrequencyIndex());
	}
	
	override void OnUFLoad(UEntityStore data){
		super.OnUFLoad(data);
		
		SetFrequencyByIndex(data.GetInt("ChannelIndex"));
	}
}

modded class Edible_Base extends ItemBase
{

	override void OnUFSave(UEntityStore data){
		super.OnUFSave(data);
		
		data.Write("m_DecayTimer", m_DecayTimer );
		data.Write("m_LastDecayStage",  m_LastDecayStage );
		if (GetFoodStage()){
			data.Write("m_FoodStage", GetFoodStage().GetFoodStageType());
		}
	}
	
	override void OnUFLoad(UEntityStore data){
		super.OnUFLoad(data);
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
	override void OnUFSave(UEntityStore data){
		super.OnUFSave(data);
		
		int IsBloodTypeVisible = GetBloodTypeVisible();
		data.Write("m_IsBloodTypeVisible",  IsBloodTypeVisible );
	}
	
	override void OnUFLoad(UEntityStore data){
		super.OnUFLoad(data);
		
		int IsBloodTypeVisible = 0;
		if (data.Read("m_IsBloodTypeVisible", IsBloodTypeVisible ) && IsBloodTypeVisible == 1){
			SetBloodTypeVisible(true);
			SetSynchDirty();
		}
		
	}
}