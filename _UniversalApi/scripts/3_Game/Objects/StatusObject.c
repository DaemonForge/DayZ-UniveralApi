class StatusObject extends Managed {
	
    string Status =  "Pending"; //Success or Error
	string Error =  "Not an Error Object";
	
	
}

class UApiDiscordStatusObject extends StatusObject {
	
	string oid;

}

class UApiDiscordMessagesResponse extends StatusObject {
	
	ref array<UApiDiscordMessage> Messages;
	
}

class UApiToxicityResponse extends StatusObject {
	
	float IdentityAttack;
	float Insult;
	float Obscene;
	float SevereToxicity;
	float SexualExplicit;
	float Threat;
	float Toxicity;
	
}
