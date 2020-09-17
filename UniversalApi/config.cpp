class CfgPatches
{
	class UniversalApi
	{
		requiredVersion=0.1;
		requiredAddons[]={
			"JM_CF_Scripts"
		};
	};
};

class CfgMods
{
	class UniversalApi
	{
		dir = "UniversalApi";
		picture = "";
		action = "";
		hideName = 1;
		hidePicture = 1;
		name = "Universal Api";
		credits = "";
		author = "DaemonForge";
		authorID = "0"; 
		version = "1.0.0"; 
		extra = 0;
		type = "mod";
		
		dependencies[] = {"Game", "World", "Mission"};

		class defs
		{
			class gameScriptModule
			{
				value = "";
				files[] = {
					"UniversalApi/scripts/Common",
					"UniversalApi/scripts/3_Game"
					};
			}
			
			class worldScriptModule
			{
				value = "";
				files[] = {
					"UniversalApi/scripts/Common",
					"UniversalApi/scripts/4_World"
					};
			}

			class missionScriptModule
			{
				value = "";
				files[] = {
					"UniversalApi/scripts/Common",
					"UniversalApi/scripts/5_Mission"
					};
			};
		};
	};
};