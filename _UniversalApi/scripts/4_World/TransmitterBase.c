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
