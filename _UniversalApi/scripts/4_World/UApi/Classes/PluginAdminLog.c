modded class PluginAdminLog extends PluginBase
{
	override void PlayerKilled( PlayerBase player, Object source )  
	{
		if (UApiConfig().EnableBuiltinLogging != 2){
			super.PlayerKilled( player, source );
		}
		
		if ( UApiConfig().EnableBuiltinLogging != 0 && player && source && player.GetIdentity() )
		{
			UApiLogKilled logobj;
			if ( player == source ){ // deaths not caused by another object (starvation, dehydration)
				logobj = new UApiLogKilled(player.GetIdentity().GetId(), player.GetPosition());
			} else {
				logobj = new UApiLogKilled(player.GetIdentity().GetId(), player.GetPosition(), source.GetType(), source.GetPosition() );
			}
			
			PlayerStat<float>				p_StatWater = player.GetStatWater();
			PlayerStat<float>				p_StatEnergy = player.GetStatEnergy();
			BleedingSourcesManagerServer	p_BleedMgr = player.GetBleedingManagerServer();
			if ( logobj && p_StatWater && p_StatEnergy && p_BleedMgr ) {
				logobj.AddStats(p_StatWater.Get(),p_StatEnergy.Get(), p_BleedMgr.GetBleedingSourcesCount());
			} else if ( logobj &&  p_StatWater && p_StatEnergy ) {
				logobj.AddStats(p_StatWater.Get(),p_StatEnergy.Get());	
			}
			EntityAI theSource = EntityAI.Cast(source);
			if (logobj && theSource ) {  // player			
				PlayerBase sourcePlayer = PlayerBase.Cast( theSource.GetHierarchyParent() );
				if (sourcePlayer && sourcePlayer.GetIdentity()) {
					logobj.ByPlayer(sourcePlayer.GetIdentity().GetId());
				}
			}
			
			if (logobj){
				UApi().Rest().Log(logobj.ToJson());
			}
		}
	}
	
	/*
	void PlayerHitBy( TotalDamageResult damageResult, int damageType, PlayerBase player, EntityAI source, int component, string dmgZone, string ammo ) // PlayerBase.c 
	{
		super.PlayerHitBy( damageResult, damageType, player, source, component, dmgZone, ammo );
		if ( player && source )		
		{
			m_PlayerPrefix = this.GetPlayerPrefix( player ,  player.GetIdentity() ) + "[HP: " + player.GetHealth().ToString() + "]";
			m_HitMessage = this.GetHitMessage( damageResult, component, dmgZone, ammo );
			
			switch ( damageType )
			{
				case DT_CLOSE_COMBAT:	// Player melee, animals, infected 
				
					if ( m_HitFilter != 1 && ( source.IsZombie() || source.IsAnimal() ) )  // Infected & Animals
					{
						m_DisplayName = source.GetDisplayName();
												
						LogPrint( m_PlayerPrefix + " hit by " + m_DisplayName + m_HitMessage );	
					}			
					else if ( source.IsPlayer() )				// Fists
					{
						m_Source = PlayerBase.Cast( source );
						m_PlayerPrefix2 = this.GetPlayerPrefix( m_Source ,  m_Source.GetIdentity() );
					
						LogPrint( m_PlayerPrefix + " hit by " + m_PlayerPrefix2 + m_HitMessage );
					}
					else if ( source.IsMeleeWeapon() )			// Melee weapons
					{				
						m_ItemInHands = source.GetDisplayName();		
						m_Source = PlayerBase.Cast( source.GetHierarchyParent() );
						m_PlayerPrefix2 = this.GetPlayerPrefix( m_Source ,  m_Source.GetIdentity() );
			
						LogPrint( m_PlayerPrefix + " hit by " + m_PlayerPrefix2 + m_HitMessage + " with " + m_ItemInHands );				
					}
					else
					{
						m_DisplayName = source.GetType();
					
						LogPrint( m_PlayerPrefix + " hit by " + m_DisplayName + m_HitMessage );					
					} 
					break;
				
				case DT_FIRE_ARM:	// Player ranged
				
					if ( source.IsWeapon() )
					{
						m_ItemInHands = source.GetDisplayName();				
						m_Source = PlayerBase.Cast( source.GetHierarchyParent() );
						m_PlayerPrefix2 = this.GetPlayerPrefix( m_Source ,  m_Source.GetIdentity() );
						m_Distance = vector.Distance( player.GetPosition(), m_Source.GetPosition() );
					
						LogPrint( m_PlayerPrefix + " hit by " + m_PlayerPrefix2 + m_HitMessage + " with " + m_ItemInHands + " from " + m_Distance + " meters ");
					}
					else 
					{
						m_DisplayName = source.GetType();
					
						LogPrint( m_PlayerPrefix + " hit by " + m_DisplayName + m_HitMessage );			
					}
					break;
				
				case DT_EXPLOSION:	// Explosion
				
					LogPrint( m_PlayerPrefix + " hit by explosion (" + ammo + ")" );
					break;
						
				case DT_STUN: 		// unused atm
				
					LogPrint( m_PlayerPrefix + " stunned by " + ammo );
					break;
						
				case DT_CUSTOM:		// Others (Vehicle hit, fall, fireplace, barbed wire ...)
								
					if ( ammo == "FallDamage" )			// Fall
					{
						LogPrint( m_PlayerPrefix + " hit by " + ammo );	
					}
					else if ( source.GetType() == "AreaDamageBase" )  
					{
						EntityAI parent = EntityAI.Cast( source );
						if ( parent )
						{
							LogPrint( m_PlayerPrefix + " hit by " + parent.GetType() + " with " + ammo );	
						}
					}
					else
					{
						m_DisplayName = source.GetType();
										
						LogPrint( m_PlayerPrefix + " hit by " + m_DisplayName + " with " + ammo );
					}
					break;
											
				default:
				
					LogPrint("DEBUG: PlayerHitBy() unknown damageType: " + ammo );
					break;
			}
		}
		else
		{
			LogPrint("DEBUG: player/source does not exist");
		} 
	}
	
	*/
	
	void OnPlacementComplete( Man player, ItemBase item ) // ItemBase.c
	{
		if (UApiConfig().EnableBuiltinLogging != 2){
			super.OnPlacementComplete( player, item);
		}
		
		if ( UApiConfig().EnableBuiltinLogging != 0)
		{
			PlayerBase thePlayer = PlayerBase.Cast( player );
			if (thePlayer && thePlayer.GetIdentity()){	
				string Item = "";
				ItemBase mainItem = ItemBase.Cast(item);
				if (mainItem){
					Item = mainItem.GetType();
				}
				
				UApiLogMisc logobj = new UApiLogMisc("OnPlacementComplete", thePlayer.GetIdentity().GetId(), thePlayer.GetPosition(), "PlaceItem", Item);
				if (logobj){
					UApi().Rest().Log(logobj.ToJson());
				}
			}
		}
	}
	
	void OnContinouousAction( ActionData action_data )	// ActionContinouousBase.c
	{
		if (UApiConfig().EnableBuiltinLogging != 2){
			super.OnContinouousAction(action_data);
		}
		
		if ( UApiConfig().EnableBuiltinLogging != 0 && action_data)
		{				
			PlayerBase player = PlayerBase.Cast( action_data.m_Player );
			if (player && player.GetIdentity()){
				string ActionName = action_data.m_Action.Type().ToString();
				string Target = "";
				if (action_data.m_Target){
					Object targetObj;
					if (Class.CastTo(targetObj, action_data.m_Target.GetObject()) || Class.CastTo(targetObj, action_data.m_Target.GetParent())){
						Target = targetObj.GetType();
					}
				}
				string Item = "";
				if (action_data.m_MainItem){
					ItemBase mainItem = ItemBase.Cast(action_data.m_MainItem);
					if (mainItem){
						Item = mainItem.GetType();
					}
				}
				UApiLogMisc logobj = new UApiLogMisc("OnContinouousAction", player.GetIdentity().GetId(), player.GetPosition(), ActionName, Item, Target);
				if (logobj){
					UApi().Rest().Log(logobj.ToJson());
				}
			}
		}
	}
	override void Suicide( PlayerBase player )  // EmoteManager.c 
	{
		if (UApiConfig().EnableBuiltinLogging != 2){
			super.Suicide( player );
		}
		if ( UApiConfig().EnableBuiltinLogging != 0 && player && player.GetIdentity() )
		{
			UApiLogKilled logobj = new UApiLogKilled(player.GetIdentity().GetId(), player.GetPosition(), "Suicide");
			
			PlayerStat<float>				p_StatWater = player.GetStatWater();
			PlayerStat<float>				p_StatEnergy = player.GetStatEnergy();
			BleedingSourcesManagerServer	p_BleedMgr = player.GetBleedingManagerServer();
			if ( logobj && p_StatWater && p_StatEnergy && p_BleedMgr ) {
				logobj.AddStats(p_StatWater.Get(),p_StatEnergy.Get(), p_BleedMgr.GetBleedingSourcesCount());
			} else if ( logobj &&  p_StatWater && p_StatEnergy ) {
				logobj.AddStats(p_StatWater.Get(),p_StatEnergy.Get());	
			}
			if (logobj){
				UApi().Rest().Log(logobj.ToJson());
			}
		}
	}
	
	override void BleedingOut( PlayerBase player )  // Bleeding.c
	{
		if (UApiConfig().EnableBuiltinLogging != 2){
			super.BleedingOut( player );
		}
		if ( UApiConfig().EnableBuiltinLogging != 0 &&  player && player.GetIdentity() )
		{
			UApiLogKilled logobj = new UApiLogKilled(player.GetIdentity().GetId(), player.GetPosition(), "BleedingOut");
			
			PlayerStat<float>				p_StatWater = player.GetStatWater();
			PlayerStat<float>				p_StatEnergy = player.GetStatEnergy();
			BleedingSourcesManagerServer	p_BleedMgr = player.GetBleedingManagerServer();
			if ( logobj && p_StatWater && p_StatEnergy && p_BleedMgr ) {
				logobj.AddStats(p_StatWater.Get(),p_StatEnergy.Get(), p_BleedMgr.GetBleedingSourcesCount());
			} else if ( logobj &&  p_StatWater && p_StatEnergy ) {
				logobj.AddStats(p_StatWater.Get(),p_StatEnergy.Get());	
			}
			if (logobj){
				UApi().Rest().Log(logobj.ToJson());
			}
		}
	}
	
	override void PlayerList() {
		if (UApiConfig().EnableBuiltinLogging != 2){
			super.PlayerList();
		}
		if (UApiConfig().EnableBuiltinLogging != 0 ){
			thread DoUApiPlayerListLog(); //To stop any extra server lag
		}
	}
	
	void DoUApiPlayerListLog(){
		array<Man> theManList = new array<Man>;
		GetGame().GetPlayers( theManList );
		array<autoptr UApiLogPlayerPos> thePlayerList = new array<autoptr UApiLogPlayerPos>;
		if ( m_PlayerArray.Count() != 0 ) {	
			for (int i = 0; i < theManList.Count(); i++ ) {
				PlayerBase thePlayer = PlayerBase.Cast(theManList.Get(i));
				if (thePlayer && thePlayer.GetIdentity()) { 
					float MovementSpeed = 0;
					if (thePlayer.GetCommand_Move()){
						MovementSpeed = thePlayer.GetCommand_Move().GetCurrentMovementSpeed();
					}
					thePlayerList.Insert(new UApiLogPlayerPos(thePlayer.GetIdentity().GetId(), thePlayer.GetPosition(), MovementSpeed, thePlayer.IsInTransport()) );
				}					
			}
		}
		if (thePlayerList && thePlayerList.Count() > 0){
			UApi().Rest().LogBulk(GetLogPlayerPosArray(thePlayerList));
		}
	}
}
