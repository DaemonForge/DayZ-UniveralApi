class CfgPatches
{
	class UFramework
	{
		requiredVersion=0.1;
		requiredAddons[]={
			"UFBase"
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

class CfgVehicles
{
	class Spraycan_ColorBase;
	class UF_SpraySkinCan_Base: Spraycan_ColorBase
	{
		scope=0;
		displayName="Admin Paint Spray Can";
		descriptionShort="A spray can used for painting ojects.";
		model="\dz\gear\consumables\spraycan.p3d";
		weight=100;
		itemSize[]={1,3};
		stackedUnit="ml";
		quantityBar=1;
		varQuantityInit=100;
		varQuantityMin=0;
		varQuantityMax=100;
		hiddenSelectionsTextures[]=
		{
			"_UFramework\data\RandomSpray.paa"
		};
	};
	class UF_Admin_Spraycan: UF_SpraySkinCan_Base
	{
		scope=2;
		displayName="Admin Paint Spray Can";
		descriptionShort="A spray can used for painting ojects.";
		varQuantityInit=1000;
		varQuantityMin=0;
		varQuantityMax=1000;
		hiddenSelectionsTextures[]=
		{
			"_UFramework\data\RandomSpray.paa"
		};
	};
	class UF_Spraycan: UF_SpraySkinCan_Base
	{
		scope=2;
		displayName="Paint Spray Can";
		descriptionShort="A spray can used for painting ojects. Can be used twice!";
		varQuantityInit=20;
		varQuantityMin=0;
		varQuantityMax=20;
		varQuantityDestroyOnMin = 1;
		hiddenSelectionsTextures[]=
		{
			"_UFramework\data\RandomSpray.paa"
		};
	};

};
