modded class PlayerBase extends ManBase{
	
	
	int GetQuickBarEntityIndex(EntityAI entity){
		return m_QuickBarBase.FindEntityIndex(entity);
	}
	
}