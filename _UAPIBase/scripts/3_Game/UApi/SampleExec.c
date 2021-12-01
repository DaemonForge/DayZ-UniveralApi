/* Sample ScriptBase Class for us with 
class UApiScriptTest extends UApiScriptBase
{
	
	void UApiScriptTest(){
		Print("UApiScriptTest Created" );
	}
	void ~UApiScriptTest(){
		Print("UApiScriptTest Destroyed" );
	}


    override void Init() //This runs if using UScriptExec.SpawnScriptBase
    {
        // intialization code can go here
		Print("UApiScriptTest Init" );
    }
	
	override void Test(){
		Print("UApiScriptTest Successfull Test");
	}

}
*/


/* Sample Script for Run (Supports returning int, bool, and string
static int Main(){
	Print("Random Number");
	return Math.QRandom();
}
*/