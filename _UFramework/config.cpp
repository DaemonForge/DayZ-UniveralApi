class CfgPatches
{
	class UFramework
	{
		requiredVersion=0.1;
		requiredAddons[]={
			"UFBase",
			"JM_CF_Scripts"
		};
	};
};

class CfgMods
{
	class UFramework
	{
		dir = "_UFramework";
		picture = "";
		action = "";
		hideName = 1;
		hidePicture = 1;
		name = "Universal Framework";
		credits = "";
		author = "DaemonForge";
		authorID = "0"; 
		version = "1.3.2"; 
		extra = 0;
		type = "mod";
		defines[] = {
			"UNIVERSALFRAMEWORK",
			"UNIVERSALFRAMEWORK_STABLE"
		};
		dependencies[] = {"Core", "Game", "World", "Mission"};
		class defs
		{
			class engineScriptModule
			{
				value = "";
				files[] = {
					"_UFramework/scripts/1_Core"
					};
			}
			class gameScriptModule
			{
				value = "";
				files[] = {
					"_UFramework/scripts/3_Game"
					};
			}
			
			class worldScriptModule
			{
				value = "";
				files[] = {
					"_UFramework/scripts/4_World"
					};
			}

			class missionScriptModule
			{
				value = "";
				files[] = {
					"_UFramework/scripts/5_Mission"
					};
			};
		};
	};
};