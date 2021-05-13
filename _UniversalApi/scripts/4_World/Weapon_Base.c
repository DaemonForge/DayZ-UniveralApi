modded class Weapon_Base extends Weapon {
	
	override void OnUApiSave(UApiEntityStore data){
		super.OnUApiSave(data);
		
	}
	
	override void OnUApiLoad(UApiEntityStore data){
		int i;
		super.OnUApiLoad(data);
		Print("===========================================================================================================");
		Print("===========================================================================================================");
		Print("[UAPI] [INFO] Validating and Repairing the Weapon Unless this is just before a crash this was not the cause");
		Print("-----------------------------------------------------------------------------------------------------------");
			ValidateAndRepair();
			int dummy_version = int.MAX;
			PlayerBase parentPlayer = PlayerBase.Cast(GetHierarchyRootPlayer());
			if (!parentPlayer)
				dummy_version -= 1;
			ScriptReadWriteContext ctxdata = new ScriptReadWriteContext;
			OnStoreSave(ctxdata.GetWriteContext());
			OnStoreLoad(ctxdata.GetReadContext(), dummy_version);
			/*GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(weap.ValidateAndRepair,100,false);*/
			for (int mi = 0; mi < GetMuzzleCount(); ++mi)
			{
				if (data.m_ChamberedRound){
					Print("[UAPI] Pushing Round to Chamber");
					PushCartridgeToChamber(mi, data.m_ChamberedRound.dmg(), data.m_ChamberedRound.cartTypeName());
				}
				for (i = 0; i < data.m_MagAmmo.Count(); i++){
					if (i > data.m_Quantity) {break;}
					PushCartridgeToInternalMagazine( mi, data.m_MagAmmo.Get(i).dmg(), data.m_MagAmmo.Get(i).cartTypeName());
				}
			}
			SetStepZeroing(GetCurrentMuzzle(), data.GetInt("m_Zeroing"));
			SetZoom(data.GetFloat("m_Zoom"));
			DryFire(GetCurrentMuzzle());
		Print("===========================================================================================================");
		Print("===========================================================================================================");
			if (data.GetInt("m_IsJammed") == 1){
				Print("Setting SetJammed");
				SetJammed(true);
			}
			if (data.m_FireModes){
				for (i = 0; i < data.m_FireModes.Count(); ++i)
				{
					SetCurrentMode(i, data.m_FireModes.Get(i));
				}
			}
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Call(this.SendUApiWeaponAfterLoadClient, data.m_QuickBarSlot);
	}
	
	void SendUApiWeaponAfterLoadClient(int quickBarSlot){
		GetGame().RemoteObjectTreeDelete(this);
		GetGame().RemoteObjectTreeCreate(this);
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Call(UApiFixRemote, quickBarSlot);
	}
	
	void UApiFixRemote(int quickBarSlot){
		//array<EntityAI> items = new array<EntityAI>;
		/*GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);
			if (items && items.Count() > 0){
				for (int i = 0; i < items.Count(); i++){
					EntityAI child_item = EntityAI.Cast(items.Get(i));
					if (child_item ){
						GetGame().RemoteObjectCreate(child_item);
					}
				}
			}*/
		if (quickBarSlot >= 0){
			PlayerBase HoldingPlayer;
			if (Class.CastTo(HoldingPlayer, GetHierarchyRootPlayer())){
				HoldingPlayer.SetQuickBarEntityShortcut(this, quickBarSlot);
			}
		}
		RPCSingleParam(155494166, new Param1<bool>( true ), true, NULL);
	}

	override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		super.OnRPC(sender, rpc_type, ctx);
		if (rpc_type == 155494166 && GetGame().IsClient()) {
			Param1<bool> data;
			if (ctx.Read(data))	{
				Print("[UAPI] OnRPC" + GetType());
				if (data.param1 && GetGame().IsClient()){
					UApiWeaponAfterLoadClient();
				}
			}
		}
	}
	
	void UApiWeaponAfterLoadClient(){
		int i;
		//if (!data){return;}
		if (!GetGame().IsMultiplayer() || GetGame().IsServer()){return;}
		Print("===========================================================================================================");
		Print("===========================================================================================================");
		Print("[UAPI] [INFO] Validating and Repairing the Weapon Unless this is just before a crash this was not the cause");
		Print("-----------------------------------------------------------------------------------------------------------");
			ValidateAndRepair();
			/*int dummy_version = int.MAX;
			PlayerBase parentPlayer = PlayerBase.Cast(GetHierarchyRootPlayer());
			if (!parentPlayer)
				dummy_version -= 1;
			ScriptReadWriteContext ctx = new ScriptReadWriteContext;
			OnStoreSave(ctx.GetWriteContext());
			OnStoreLoad(ctx.GetReadContext(), dummy_version);
			for (int mi = 0; mi < GetMuzzleCount(); ++mi)
			{
				if (data.m_ChamberedRound){
					Print("[UAPI] Pushing Round to Chamber");
					PushCartridgeToChamber(mi,  data.m_ChamberedRound.dmg(),  data.m_ChamberedRound.cartTypeName());
				}
				for (i = 0; i < data.m_MagAmmo.Count(); i++){
					if (i > data.m_Quantity) {break;}
					PushCartridgeToInternalMagazine( mi,  data.m_MagAmmo.Get(i).dmg(),  data.m_MagAmmo.Get(i).cartTypeName());
				}
			}
		//GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Call(this.AfterStoreLoad);
		SetStepZeroing(GetCurrentMuzzle(), data.GetInt("Vanilla", "m_Zeroing"));
		SetZoom(data.GetFloat("Vanilla", "m_Zoom"));*/
		Print("===========================================================================================================");
		Print("===========================================================================================================");
			/*if (data.GetInt("Vanilla", "m_IsJammed") == 1){
				Print("Setting SetJammed");
				SetJammed(true);
			}
			if (data.m_FireModes){
				for (i = 0; i < data.m_FireModes.Count(); ++i)
				{
					SetCurrentMode(i, data.m_FireModes.Get(i));
				}
			}*/
	}
	
}