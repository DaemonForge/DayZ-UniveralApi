# DayZ Universal Framework

**Framework for DayZ Modding**

The **DayZ Universal Framework** expands what is possible in DayZ modding by bridging the gap between your game server and external services. It enables powerful features that require persistent data storage, external API communication, and cross-server synchronization.

By providing a standardized API for these complex tasks, the framework allows modders and server owners to implement advanced systems like global economies, intelligent NPCs, and unified player stats without building custom backend infrastructure from scratch.

**Why use Universal Framework?**
*   **For Modders**: Stop reinventing the wheel. Get instant access to MongoDB, Discord, and AI APIs with one line of Enforce Script. Focus on gameplay, not backend infrastructure.
*   **For Hosts**: Run true "Hived" clusters where players, items, and economy travel seamlessly between maps.
*   **For Communities**: Engage players with deep Discord integration, intelligent AI NPCs, and persistent web-based stats.

It handles the boring stuff—authentication, security, rate-limiting, and database connections—so you can build the cool stuff.

### 🌟 V2: A Complete Overhaul
Version 2 represents a massive leap forward from the original V1 (Universal API). We've rebuilt the entire backend service from the ground up to be faster, more stable, and easier to use.

*   **Standalone Application**: No more complex Node.js setup. V2 runs as a standalone executable (Electron on Windows, Binary on Linux).
*   **Visual Management**: The new Windows Dashboard lets you configure everything—database, keys, AI settings—with a GUI. No more JSON errors.
*   **AI Revolution**: Full support for OpenAI's latest features including context-aware Chat, Knowledge Bases (RAG), and Assistants.
*   **Performance**: Rewritten core logic for better concurrency and stability under load.
*   **Developer Experience**: Simplified API endpoints and better error messages for modders.

### 🏗️ Architecture
The system functions as a bridge between your DayZ server and external services:
1. **DayZ Mod (`@UFramework`)**: Runs on the server/client, exposing a clean API to other mods.
2. **Backend Service (`UFService`)**: Node.js application that handles heavy lifting (Database, AI, Discord).
3. **MongoDB**: Stores all persistent data (players, objects, globals) securely.

### 🚀 Getting Started

#### 1. Backend Service
*   **Windows**: Download and run the `UniversalFrameworkService-Setup.exe`. It provides an easy-to-use Dashboard for configuration.
*   **Linux**: Run the provided install script or download the binary executable. Run as a systemd service.
*   **MongoDB**: Required for the backend. The Windows installer can set this up for you.

#### 2. DayZ Server
*   Install the `@UFramework` mod on your server.
*   Configure `UFramework.json` in your server profile to point to your Backend Service URL.
*   Add authentication keys from the Backend Service to your server config.

#### 3. Mod Integration
Add `UFramework` to your `requiredAddons[]` in `config.cpp`.
```cpp
// Wait for framework to be ready before making calls
override void UFrameworkReady() {
    // Example: Load player data from MongoDB
    U().Player().Load(playerIdentity, this, "OnLoadParams");
}
```

### 💾 Database Functions (MongoDB)
- **Object Storage**: Load/Save complex JSON objects
- **Player Data**: Dedicated persistent player storage
- **Globals**: Server-wide or cross-server global state
- **Transactions**: Atomic increments and updates across multiple servers
- **Advanced Queries**: Full MongoDB query support to find data efficiently

### 🤖 AI Features (OpenAI)
- **AI Chat**: conversational NPCs with context awareness
- **Knowledge Bases**: Attach document collections (lore, rules) for AI retrieval
- **Assistants API**: Stateful AI agents with long-term memory
- **Text-to-Speech**: Generate audio for dynamic voice lines
- **Image Generation**: Generate textures or images on the fly

### 🎮 Discord Integration
- **Account Linking**: Web interface for Steam ↔ Discord linking
- **Role Management**: Add/Remove roles based on in-game events
- **Messaging**: Send DMs to players (online or offline)
- **Channel Management**: Create, edit, and delete channels
- **Voice Control**: Move, mute, or kick users in voice channels

### 🛠️ Management Tools
- **Windows Dashboard**: Electron-based GUI app for easy management
- **Config Editor**: Visual editor for all service settings
- **KB Manager**: Drag-and-drop document management for AI Knowledge Bases
- **Globals Editor**: View and edit server global variables

### ⚡ Other Features
- **Quantum Random Numbers** (ANU QRNG)
- **Toxicity Checker** (Chat moderation)
- **Service Monitoring & Logging**
- **Server Health Checks**

_(Pull requests are welcome for new features and improvements!)_
