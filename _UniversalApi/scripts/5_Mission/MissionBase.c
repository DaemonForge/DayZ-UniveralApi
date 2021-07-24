modded class MissionBase extends MissionBaseWorld
{
	void MissionBase()
	{
		UApi();
	}
	
	override void UniversalApiReady(){
		super.UniversalApiReady();
		//A Safe Place to start pulling datadown from the WebServer 
		//(This will be called on init for the server, and after should be after the AuthToken Is received)
		
		//Don't forget to Super As
		//super.UniversalApiReady();
		
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.TestRandom, 10000, true);
	}
	
	void TestRandom(){
		
		Print("Random: " + Math.QRandomInt());
		Print("QRandomInt: 0-10 " + Math.QRandomInt(0,10));
		Print("QRandomInt: 0-100 " + Math.QRandomInt(0,100));
		Print("QRandomInt: 0-5 " + Math.QRandomInt(0,5));
		Print("QRandomFlip: " + Math.QRandomFlip());
		Print("QRandomFlip: " + Math.QRandomFlip());
		Print("QRandomFlip: " + Math.QRandomFlip());
		Print("QRandomFloat: 0-1 " + Math.QRandomFloat(0,1));
		Print("QRandomFloat: 0-1 " + Math.QRandomFloat(0,1));
		Print("QRandomFloat: 0-10 " + Math.QRandomFloat(0,10));
		Print("QRandomFloat: 5-10 " + Math.QRandomFloat(5,10));
		Print("QRandomFloat: 10000-100000 " + Math.QRandomFloat(10000,100000));
	}
}