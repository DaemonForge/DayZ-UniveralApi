# DayZ Universal Api
 
The Universal Api Mod and Webservice this is my attempt to make a Universal Api Backend for the server hosters, allowing for a robust, easy to use and set up backend. and for modders an easy to use API wrapper to help them make hived and cross-server mods more easily. With the mod and API I handle authentication and client tokens allowing you to ensure that the data you are accessing and using is secured, preventing rouge and unwanted access to the APIs.

All the Modder/Developer documentation is available here https://github.com/daemonforge/DayZ-UniveralApi/wiki/Developer-Reference
_(I will accept pull requests if someone wants to add features for there own ideas)_
### Database Functions
- Load/Save Objects(JSON)
- Update Objects (can set sub Elements/Variables including Pull and Push from Arrays) 
- Transactions (increment float/ints in database with constancy (aka from multiple servers at once)
- Queries (MongoDB Queries)
### Discord Functions 
- Simple Web Interface for players to connect there steam accounts to discord (customizable for server owners) 
- Add/Remove Roles
- Get User (Returns an object with current Roles and Discord ID/Name)
- Send Message (Sends DM from bot to player, player doesn't have to be connected to DayZ server at the time)
- Create/Edit/Delete Channels
- Send Message to Channels
- Get Messages from Channels
- Move User to Voice Channel
- Mute User in Voice Channel
- Kick User from Voice Channel
- Get User's current Voice Channel
### Other Stuff
- Quantum Random Numbers
- Toxicity Checker (TensorFlow Toxicity)
- Translate (Libre Translate)
- And More
