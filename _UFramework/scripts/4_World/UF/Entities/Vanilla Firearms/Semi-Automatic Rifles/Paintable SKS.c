class SKS extends SKS_Base
{
	override void InitSkins()
	{
		super.InitSkins();
		RegisterTextureAndMaterial("Camo", "_UFramework\\data\\Firearms\\SKS\\SKS_Camo_Co.paa","dz\\weapons\\firearms\\SKS\\data\\sks_painted.rvmat");
		RegisterTextureAndMaterial("Green", "dz\\weapons\\firearms\\SKS\\data\\sks_green_co.paa","dz\\weapons\\firearms\\SKS\\data\\sks_painted.rvmat");
		RegisterTextureAndMaterial("Black", "dz\\weapons\\firearms\\SKS\\data\\sks_black_co.paa","dz\\weapons\\firearms\\SKS\\data\\sks_painted.rvmat");
		RegisterTextureAndMaterial("666 SKS", "_UFramework\\data\\Firearms\\SKS\\TigerStripBobby\\RedTigerStripedBobbySKS_co.paa","dz\\weapons\\firearms\\SKS\\data\\sks_painted.rvmat");
		RegisterTextureAndMaterial("Strip", "dz\\weapons\\firearms\\SKS\\data\\sks_co.paa","dz\\weapons\\firearms\\SKS\\data\\sks.rvmat");
	}
	
};
