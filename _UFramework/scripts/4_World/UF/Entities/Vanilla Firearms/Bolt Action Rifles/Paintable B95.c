modded class B95_base
{
	override void InitSkins()
	{
		super.InitSkins();
		RegisterTextureAndMaterial("Camo", "_UFramework\\data\\Firearms\\B95\\B95Camo_co.paa","dz\\weapons\\firearms\\B95\\data\\b95_painted.rvmat");
		RegisterTextureAndMaterial("Black", "DZ\\weapons\\firearms\\B95\\data\\b95_black_co.paa","dz\\weapons\\firearms\\B95\\data\\b95_painted.rvmat");
		RegisterTextureAndMaterial("Green", "DZ\\weapons\\firearms\\B95\\data\\b95_green_co.paa","dz\\weapons\\firearms\\B95\\data\\b95_painted.rvmat");
		RegisterTextureAndMaterial("Strip", "dz\\weapons\\firearms\\B95\\data\\b95_co.paa","dz\\weapons\\firearms\\B95\\data\\b95.rvmat");
		
	}
};
