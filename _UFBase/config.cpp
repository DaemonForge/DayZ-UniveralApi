#define UNIVERSALAPI
//This is to make sure the minium is loaded so that other mods can use properly
class CfgPatches
{
	class UFBase
	{
		requiredVersion=0.1;
		requiredAddons[]={
		};
	};
};

class CfgMods
{
	class UFBase
	{
		dir = "_UFBase";
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
					"_UFBase/scripts/Common"
					};
			}
			
			class gameScriptModule
			{
				value = "";
				files[] = {
					"_UFBase/scripts/Common",
					"_UFBase/scripts/3_Game"
					};
			}
			
			class worldScriptModule
			{
				value = "";
				files[] = {
					"_UFBase/scripts/Common",
					"_UFBase/scripts/4_World"
					};
			}

			class missionScriptModule
			{
				value = "";
				files[] = {
					"_UFBase/scripts/Common"
					};
			};
		};
	};
};