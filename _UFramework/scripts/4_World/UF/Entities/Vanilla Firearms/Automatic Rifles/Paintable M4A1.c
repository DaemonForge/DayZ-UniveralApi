modded class M4A1_Base
{
	override void InitSkins()
	{
		super.InitSkins();
		RegisterTextureAndMaterial("Vanilla", "dz\\weapons\\firearms\\m4\\data\\m4_body_co.paa","dz\\weapons\\firearms\\m4\\data\\m4_body.rvmat");
		RegisterTextureAndMaterial("Green", "dz\\weapons\\firearms\\m4\\data\\m4_body_green_co.paa","dz\\weapons\\firearms\\m4\\data\\m4_body_camo.rvmat");
	}
};
