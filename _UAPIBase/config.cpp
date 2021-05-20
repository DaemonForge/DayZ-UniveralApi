#define UNIVERSALAPI
//This is to make sure the minium is loaded so that other mods can use properly
class CfgPatches
{
	class UAPIBase
	{
		requiredVersion=0.1;
		requiredAddons[]={
		};
	};
};

class CfgMods
{
	class UAPIBase
	{
		dir = "_UAPIBase";
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
					"_UAPIBase/scripts/Common",
					"_UAPIBase/scripts/3_Game"
					};
			}
			
			class worldScriptModule
			{
				value = "";
				files[] = {
					"_UAPIBase/scripts/Common",
					"_UAPIBase/scripts/4_World"
					};
			}

			class missionScriptModule
			{
				value = "";
				files[] = {
					"_UAPIBase/scripts/Common"
					};
			};
		};
	};
};