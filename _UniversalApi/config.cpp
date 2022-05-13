class CfgPatches
{
	class UniversalApi
	{
		requiredVersion=0.1;
		requiredAddons[]={
			"UAPIBase",
			"JM_CF_Scripts"
		};
	};
};

class CfgMods
{
	class UniversalApi
	{
		dir = "_UniversalApi";
		picture = "";
		action = "";
		hideName = 1;
		hidePicture = 1;
		name = "Universal Api";
		credits = "";
		author = "DaemonForge";
		authorID = "0"; 
		version = "1.3.2"; 
		extra = 0;
		type = "mod";
		
		dependencies[] = {"Core", "Game", "World", "Mission"};

		class defs
		{
			class engineScriptModule
			{
				value = "";
				files[] = {
					"_UniversalApi/scripts/1_Core"
					};
			}
			class gameScriptModule
			{
				value = "";
				files[] = {
					"_UniversalApi/scripts/3_Game"
					};
			}
			
			class worldScriptModule
			{
				value = "";
				files[] = {
					"_UniversalApi/scripts/4_World"
					};
			}

			class missionScriptModule
			{
				value = "";
				files[] = {
					"_UniversalApi/scripts/5_Mission"
					};
			};
		};
	};
};