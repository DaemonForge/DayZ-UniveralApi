class ActionPaintItemCB : ActionContinuousBaseCB
{
	override void CreateActionComponent()
	{
		m_ActionData.m_ActionComponent = new CAContinuousTime( 2 );//120
	};
}

class ActionPaintItemBase: ActionContinuousBase
{
    void ActionPaintItemBase()
	{
		m_CallbackClass = ActionPaintItemCB;
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONFB_INTERACT;
		m_FullBody = true;
		m_StanceMask = DayZPlayerConstants.STANCEMASK_ERECT;
		m_SpecialtyWeight = UASoftSkillsWeight.ROUGH_HIGH;
    };

	int SpamCounter = 0;
    string TendancyText = "";
	string AvaibleTextureName = "";
    override string GetText()
	{
		
        return "Paint " + AvaibleTextureName;
    };

	override bool ActionCondition( PlayerBase player, ActionTarget target, ItemBase item )
	{
		UF_SpraySkinCan_Base Spray = UF_SpraySkinCan_Base.Cast( item  );
		ItemBase ntarget = ItemBase.Cast( target.GetObject() );
		if (ntarget && Spray && ntarget.CanPaint() )
		{
			if (Spray.GetPaintTendancy() >= ntarget.GetTextureCount())
			{
				Spray.SetPaintTedancy(1);
			}
			AvaibleTextureName = ntarget.GetTextureName(Spray.GetPaintTendancy());
			
			
			return true;
		}
		return false;
	}

	override void OnFinishProgressServer( ActionData action_data )
	{
		ItemBase ntarget = ItemBase.Cast( action_data.m_Target.GetObject() );
		PlayerBase player = PlayerBase.Cast(action_data.m_Player);
		UF_SpraySkinCan_Base Spray = UF_SpraySkinCan_Base.Cast(action_data.m_MainItem);
		
		
		
		if( ntarget && Spray )
		{
			ntarget.SetTexture(Spray.GetPaintTendancy());
			MinusQuantity( action_data.m_MainItem, player );
		}
		

	}
	void MinusQuantity( ItemBase item, PlayerBase player )
	{
		item.AddQuantity(-10, true);// some reason repeats the -# twice no clue.
	}
}