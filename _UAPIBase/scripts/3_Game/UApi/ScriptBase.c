//From Wardog and DaOne

class UApiScriptBase
{

    void UApiScriptBase()
    {
        // can have a constructor
    }

    void ~UApiScriptBase()
    {
        // can have a destructor
    }

    void Init()
    {
        // intialization code can go here
    }
	
	void Test(){
		Print(ClassName() + " Test Successfull");
	}

}

