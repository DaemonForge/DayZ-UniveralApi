modded class FAL_Base
{
	override void InitSkins() {
		super.InitSkins();
		RegisterTextureAndMaterial("Vanilla", "DZ\\weapons\\firearms\\fal\\data\\fal_co.paa","DZ\\weapons\\firearms\\fal\\data\\fal.rvmat");
		RegisterTextureAndMaterial("Camo", "_UFramework\\data\\Firearms\\Fal\\Fal_Camo_Co.paa","DZ\\weapons\\firearms\\fal\\data\\fal.rvmat");
	}
};
