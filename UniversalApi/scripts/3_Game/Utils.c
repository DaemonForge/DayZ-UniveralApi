Man GameApiFindPlayer(string GUID){
	ref array<Man> players = new array<Man>;
	GetGame().GetPlayers( players );
	for (int i = 0; i < player.Count(); i++){
		if (players.Get(i).GetID() == GUID && players.Get(i).GetIdentity()){
			return Man.Cast(players.Get(i));
		}
	}
	return NULL;
}