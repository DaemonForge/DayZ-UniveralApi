/* Sample ScriptBase Class for us with 
class UFScriptTest extends UFScriptBase
{
	
	void UFScriptTest(){
		Print("UFScriptTest Created" );
	}
	void ~UFScriptTest(){
		Print("UFScriptTest Destroyed" );
	}


    override void Init() //This runs if using UScriptExec.SpawnScriptBase
    {
        // intialization code can go here
		Print("UFScriptTest Init" );
    }
	
	override void Test(){
		Print("UFScriptTest Successfull Test");
	}

}
*/


/* Sample Script for Run (Supports returning int, bool, and string
static int Main(){
	Print("Random Number");
	return Math.QRandom();
}
*/