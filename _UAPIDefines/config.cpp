class CfgPatches
{
	class UAPIDefines
	{
		requiredVersion=0.1;
		requiredAddons[]={
		};
	};
};

class CfgMods
{
	class UAPIDefines
	{
		dir = "_UAPIDefines";
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
					"_UAPIDefines/scripts/Common"
					};
			}
			
			class worldScriptModule
			{
				value = "";
				files[] = {
					"_UAPIDefines/scripts/Common"
					};
			}

			class missionScriptModule
			{
				value = "";
				files[] = {
					"_UAPIDefines/scripts/Common"
					};
			};
		};
	};
};