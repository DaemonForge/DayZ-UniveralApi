class ActionChangePaintAction: ActionSingleUseBase
{
    void ActionChangePaintAction()
	{
        m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_OPENDOORFW;
        m_StanceMask = DayZPlayerConstants.STANCEMASK_CROUCH | DayZPlayerConstants.STANCEMASK_ERECT;
        //m_HUDCursorIcon = CursorIcons.OpenDoors;
    };

    override void CreateConditionComponents()
	{
        m_ConditionItem = new CCINone;
        m_ConditionTarget = new CCTCursor;
    };

    override string GetText()
	{
        return "Switch Paint";
    };

    override bool ActionCondition( PlayerBase player, ActionTarget target, ItemBase item )
	  {
		ItemBase ntarget = ItemBase.Cast( target.GetObject() );
		if (ntarget && ntarget.CanPaint()){
			return true;
		}
		return false;
    };
    
	override void OnStartServer( ActionData action_data )
    {
		 ItemBase ntarget = ItemBase.Cast( action_data.m_Target.GetObject() );
		
		
        UF_SpraySkinCan_Base Spray = UF_SpraySkinCan_Base.Cast(action_data.m_MainItem);
        if (Spray && ntarget)
		{
      		int currentT = Spray.GetPaintTendancy();
        	int nextT = ntarget.GetNextTendancy( currentT );
            Spray.SetPaintTedancy( nextT );
        }
    };
};