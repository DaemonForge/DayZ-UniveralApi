# API Endpoints - Discord

All endpoints share the base URL: `http://<ServerIP>:<Port>/Discord`

## Player Management

### POST /AddRole/:GUID
Adds a Discord role to a player.
- **URL**: `/Discord/AddRole/:GUID`
- **Method**: `POST`
- **Body**:
  ```json
  {
      "Role": "REPLACE_WITH_ROLE_ID"
  }
  ```
- **Response**:
  ```json
  {
      "Status": "Success",
      "Error": "",
      "Roles": ["ROLE_ID_1", "ROLE_ID_2"],
      "VoiceChannel": "VOICE_CHANNEL_ID",
      "id": "DISCORD_USER_ID"
  }
  ```

### POST /RemoveRole/:GUID
Removes a Discord role from a player.
- **URL**: `/Discord/RemoveRole/:GUID`
- **Method**: `POST`
- **Body**:
  ```json
  {
      "Role": "REPLACE_WITH_ROLE_ID"
  }
  ```

### GET /Get/:GUID
Gets Discord information for a player, including their roles and current voice channel.
- **URL**: `/Discord/Get/:GUID`
- **Method**: `GET`
- **Response**:
  ```json
  {
      "Status": "Success", 
      "Error": "",
      "username": "User", 
      "globalName": "Display Name", 
      "Roles": ["..."],
      "VoiceChannel": "..."
  }
  ```

### GET /Check/:GUID
Checks if a player has linked their Discord account.
- **URL**: `/Discord/Check/:GUID`
- **Method**: `GET`
- **Response**:
  ```json
  {
      "Status": "Success",
      "HasDiscord": true
  }
  ```

### GET /CheckRole/:GUID/:ROLEID
Checks if a player has a specific role.
- **URL**: `/Discord/CheckRole/:GUID/:ROLEID`
- **Method**: `GET`
- **Response**:
  ```json
  {
      "Status": "Success",
      "HasRole": true
  }
  ```

### POST /SetNickname/:GUID
Sets the player's nickname in the Discord server.
- **URL**: `/Discord/SetNickname/:GUID`
- **Method**: `POST`
- **Body**:
  ```json
  {
      "Nickname": "New Name"
  }
  ```

### POST /Send/:GUID
Sends a Direct Message (DM) to the player's Discord account.
- **URL**: `/Discord/Send/:GUID`
- **Method**: `POST`
- **Body**: 
  ```json
  {
      "Message": "Hello from the server!"
  }
  ```
  *Note: Message can also be a Discord Embed object.*

## Voice Management

### POST /Mute/:GUID
Mutes or Unmutes the player in the specific voice channel.
- **URL**: `/Discord/Mute/:GUID`
- **Method**: `POST`
- **Body**:
  ```json
  {
      "State": 1  // 1 = Mute, 0 = Unmute
  }
  ```

### POST /Kick/:GUID
Kicks the player from their current voice channel.
- **URL**: `/Discord/Kick/:GUID`
- **Method**: `POST`
- **Body**:
  ```json
  {
      "Text": "Optional reason"
  }
  ```

### POST /Move/:GUID/:ChannelID
Moves the player to a different voice channel.
- **URL**: `/Discord/Move/:GUID/:ChannelID`
- **Method**: `POST`
- **URL Parameters**:
  - `GUID`: Player's Steam/Identity GUID.
  - `ChannelID`: The ID of the generic voice channel to move them to.

### GET /GetChannel/:GUID
Gets the ID of the voice channel the player is currently in.
- **URL**: `/Discord/GetChannel/:GUID`
- **Method**: `GET`
- **Response**:
  ```json
  {
      "Status": "Success",
      "oid": "CHANNEL_ID"
  }
  ```

## Channel Management

### POST /Channel/Create
Creates a new Grid/Group channel (Text + Voice).
- **URL**: `/Discord/Channel/Create`
- **Method**: `POST`
- **Body**:
  ```json
  {
      "Name": "New Channel",
      "Type": 0, // 0 = Text, 2 = Voice
      "Category": "CATEGORY_ID", // Optional
      "UserLimit": 5 // Optional (Voice only)
  }
  ```

### POST /Channel/Edit/:ChannelID
Edits an existing channel.
- **URL**: `/Discord/Channel/Edit/:ChannelID`
- **Method**: `POST`
- **Body**:
  ```json
  {
      "Reason": "Roleplay purpose",
      "Options": {
          "name": "New Name",
          "userLimit": 10
      }
  }
  ```

### DELETE /Channel/Delete/:ChannelID
Deletes a channel.
- **URL**: `/Discord/Channel/Delete/:ChannelID`
- **Method**: `DELETE`

### POST /Channel/Send/:ChannelID
Sends a message to a specific channel.
- **URL**: `/Discord/Channel/Send/:ChannelID`
- **Method**: `POST`
- **Body**:
  ```json
  {
      "Message": "Message content"
  }
  ```

### GET /Channel/Messages/:ChannelID
Fetches the last 50 messages from a channel.
- **URL**: `/Discord/Channel/Messages/:ChannelID`
- **Method**: `GET`
- **Response**:
  ```json
  {
      "Status": "Success",
      "Messages": [
          {
              "id": "MSG_ID",
              "content": "Hello",
              "author": { "username": "User", "id": "..." }
          }
      ]
  }
  ```

### POST /Channel/Invite/:ChannelID
Creates an invite link for a channel.
- **URL**: `/Discord/Channel/Invite/:ChannelID`
- **Method**: `POST`
- **Body**:
  ```json
  {
      "maxAge": 86400,
      "maxUses": 1
  }
  ```
- **Response**:
  ```json
  {
      "Status": "Success",
      "Invite": "https://discord.gg/..."
  }
  ```

## Tags
`operators`, `api`, `endpoints`, `discord`, `integration`, `reference`, `doc-usage`
