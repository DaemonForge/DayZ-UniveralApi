class StatusObject extends Managed {
    string Status =  "Pending"; //Ok or Error
	string Error =  "Not an Error Object";
	
	
}

class UApiDiscordStatusObject extends StatusObject {
	
	string oid;

}

class UApiDiscordMessagesResponse extends StatusObject {
	
	ref array<UApiDiscordMessage> Messages;
	
}