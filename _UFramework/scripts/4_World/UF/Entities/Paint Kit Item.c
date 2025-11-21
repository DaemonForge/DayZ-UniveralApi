class UF_SpraySkinCan_Base extends ItemBase
{
	protected int m_PaintTendancy = 0;
	
	override void EEInit()
	{
      super.EEInit();
      RegisterNetSyncVariableInt("m_PaintTendancy");
    };
	
	override void SetActions()
	{
		super.SetActions();
		AddAction(ActionPaintItemBase);
		AddAction(ActionChangePaintAction);
	}
	
	
	
	
	void SetPaintTedancy(int Tendancy = 1) {
		m_PaintTendancy = Tendancy;
		SetSynchDirty();
	}
	
	int GetPaintTendancy() {
      	return m_PaintTendancy;
    };
	
}

class UF_Admin_Spraycan extends UF_SpraySkinCan_Base
{
}

class UF_Spraycan extends UF_SpraySkinCan_Base
{
}