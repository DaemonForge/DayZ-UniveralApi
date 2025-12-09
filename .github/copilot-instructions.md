# DayZ Universal API - Copilot Instructions

## Project Overview
This is a **two-component system** providing backend services for DayZ modding:
1. **DayZ Mod** (`_UFramework`, `_UFBase`) - Enforce Script (`.c` files) running inside DayZ client/server
2. **Node.js Service** (`UFServerServiceV2`) - Express.js REST API + Discord bot + MongoDB backend

## Architecture

### Communication Flow
```
DayZ Mod (Enforce Script) <--REST API--> UFServerService (Node.js) <---> MongoDB / Discord / OpenAI
```

The mod uses DayZ's built-in `RestApi` to make HTTP calls to the service. All requests go through authentication (server auth tokens or player auth tokens).

### Configuration Files
- **Mod Config**: `$profile:UF/UFramework.json` - Contains `ServerURL`, `ServerID`, `ServerAuth`
- **Service Config**: `UFServerServiceV2/config.json` - MongoDB, Discord bot token, OpenAI API key, server auth keys

### Mod Structure (`_UFramework/scripts/`)
- **`1_Core/`** - Core engine scripts
- **`3_Game/UF/`** - Main framework (game layer)
  - **`UFramework.c`** - Singleton hub (`U()`) providing access to all endpoints
  - **`UniversalRest.c`** - Static REST utilities for HTTP calls
  - **`Misc/ConfigLoader.c`** - `UFConfig()` singleton for mod configuration
  - **`Endpoints/`** - API endpoint wrappers:
    - `UFDBEndpoint.c` - Database operations (Save/Load/Query/Transaction/Update)
    - `UFDSEndpoint.c` - Discord operations (roles, DMs, channels, voice)
    - `UFAIChatEndpoint.c` - OpenAI chat sessions
    - `UFMsgEndpoint.c` - Messaging system
    - `UFAPIEndpoint.c` - Generic API forwarding
  - **`CallBacks/`** - Callback system for async REST responses
    - `UFCallbackBase` - Base class for all callbacks (in `ObjectCallBack.c`)
    - `UNestedCallBack` - Wrapper for chaining callbacks
  - **`Services/`** - Higher-level abstractions:
    - `AIChatAgent.c` - AI agent classes (`UFAIChatAgent`, `UAIChatAgent<T>`)
    - `CronManager.c` - Scheduled task manager
    - `Logger.c` - Logging utilities
- **`4_World/`** - World-layer scripts
- **`5_Mission/`** - Mission-layer scripts

### Service Structure (`UFServerServiceV2/`)
- **`app.js`** - Express server setup, clustering, route mounting
- **`controllers/`** - REST route handlers:
  - `object.js` - Object database CRUD
  - `player.js` - Player database operations
  - `global.js` - Global state management
  - `aiChat.js` - OpenAI chat sessions (create, send, poll)
  - `aiAssistant.js` - OpenAI assistants
  - `tts.js`, `images.js` - Media generation
- **`models/`** - MongoDB data access layer
- **`discord/`** - Discord.js bot and route handlers
- **`auth/`** - JWT-based authentication (server keys + player tokens)

## Developer Workflows

### Service Development
```bash
cd UFServerServiceV2
npm run start        # Start Electron app
npm run tunnel       # Run cloudflared tunnel for dev
```

### Key Dependencies
- MongoDB must be running locally (`mongodb://localhost:27017`)
- Discord bot token required for Discord features
- OpenAI API key required for AI features

## Key Patterns

### Callback Pattern (Mod Side)
All async operations use callbacks. Two styles:
```enforce
// Style 1: Class instance + function name
U().db().Load("MyMod", "player123", this, "OnPlayerLoaded");
void OnPlayerLoaded(int cid, int status, string oid, string data){ ... }

// Style 2: UFCallbackBase subclass
U().db().Load("MyMod", "player123", new MyCallback());
```

### Endpoint Access (Mod Side)
```enforce
U().db()      // Database operations (OBJECT_DB or PLAYER_DB)
U().ds()      // Discord operations
U().AI()      // AI Chat endpoint
U().Msg()     // Messaging
U().globals() // Global state
U().Cron()    // Scheduled tasks
```

### Authentication
- **Server Auth**: API key in config, sent as header token
- **Player Auth**: JWT tokens issued per-player, validated against MongoDB

## Language & Style

### Enforce Script (`.c` files)
- C-like syntax with classes, no namespaces
- Use `autoptr` for automatic memory management (prevents leaks)
- Avoid `ref` - use `autoptr` instead
- Naming conflicts: DayZ has classes like `Param1<T>`, `Param2<T>` - don't use these as variable names
- Class.CastTo() for safe downcasting: `Class.CastTo(m_Agent, agent);`
- `extends Managed` for garbage-collected classes

### Node.js Service
- Express.js routes with middleware chains
- Async/await for database and external API calls
- Winston logging via `createLogger(global.logger, 'module')`
- MongoDB via native driver (not Mongoose)

## AI Chat System
- **UFAIChatEndpoint** - Low-level endpoint for create/send/poll
- **UFAIChatAgent** - High-level agent returning string responses
- **UAIChatAgent<T>** - Templated agent returning typed JSON responses
- Sessions stored in MongoDB, polling for async OpenAI responses
- Context system: `UAIChatContext` for structured context blocks
- Tool support: `UAIChatToolDef` for function calling

## Common Operations

### Database (Mod)
```enforce
U().db().Save("ModName", "objectId", jsonString, this, "OnSaved");
U().db().Load("ModName", "objectId", this, "OnLoaded");
U().db().Transaction("ModName", "objectId", "field", 1.0); // atomic increment
```

### Discord (Mod)
```enforce
U().ds().AddRole(playerGUID, "RoleId", this, "OnRoleDone");
U().ds().UserSend(playerGUID, "Hello!", this, "OnSent");
U().ds().ChannelSend("channelId", "Server message");
```

### AI Chat (Mod)
```enforce
class MyAgent extends UFAIChatAgent {
    override string SystemInstructions(){ return "You are helpful."; }
}
autoptr MyAgent agent = new MyAgent();
agent.Chat("Hello", this, "OnResponse");
```

## File Conventions
- Mod scripts: `UF*.c` prefix for framework files
- Callbacks: `*CallBack.c` suffix
- Endpoints: `UF*Endpoint.c` pattern
- Service routes: `/controllers/*.js`
- Service models: `/models/*.js`

## Important Notes
- The mod runs in both client and server contexts - use `GetGame().IsServer()` to check
- REST callbacks are async - never block waiting for responses
- Always validate status codes in callbacks before using data
- MongoDB collections: `Objects`, `Players`, `Globals`, `AIChats`, `AIMessages`



# DayZ MCP Tools Usage Guide

You have access to specialized DayZ modding tools via MCP. This guide helps you use them effectively to write correct Enforce Script code.

---

## 🎮 DayZ Modding Overview

**DayZ** uses the **Enfusion engine** with **Enforce Script**, a C-like scripting language with unique patterns for modding. Key concepts:

- **Modded Classes**: Inject behavior into existing game classes without editing originals
- **Script Modules**: Scripts load in order (1_Core → 5_Mission), placement matters
- **Client-Server Architecture**: Code runs on both sides, must handle networking
- **Config Files**: Items, vehicles, recipes defined in config.cpp

**Common Mod Types:**
- **Items**: New weapons, tools, clothing (extend ItemBase, Clothing, Weapon_Base)
- **Player Modifications**: Custom behaviors, actions (modded class PlayerBase)
- **Mission Scripts**: Server events, spawn systems (modded class MissionServer)
- **UI/HUD**: Custom interfaces (client-side scripts in 5_Mission)

---

## 🔧 Available Tools

### Reference & Context
| Tool | Purpose |
|------|---------|
| `get_enforce_context` | Enforce Script syntax reference (see sections below) |
| `list_context_sections` | List all available context sections with descriptions |
| `get_enforce_warnings` | Common syntax pitfalls to avoid |

### Context Sections (use with `get_enforce_context section:"..."`)

**Language Basics:**
| Section | Description |
|---------|-------------|
| `inheritance` | How to create new classes using extends keyword |
| `overrides` | How to properly override methods using override and super |
| `modifiers` | Access levels (private, protected) and variable modifiers (ref, autoptr) |
| `variables` | Variable declaration, primitive types (int, float, bool, string, vector) |
| `functions` | Function declaration, parameters (out, inout), return types |
| `operators` | Arithmetic, comparison, logical operators (NO ternary!) |
| `control_flow` | If/else, switch, for, foreach, while loops |
| `arrays` | Static arrays and dynamic array<T> with methods |
| `maps` | Key-value storage using map<K,V> |
| `enums` | Enum declaration and usage |
| `oop` | Classes, constructors, this/super, managed classes |

**Modded Classes:**
| Section | Description |
|---------|-------------|
| `modded_class_overview` | Quick reference for modded class basics, key rules, mod chaining |
| `modded_class_advanced` | Advanced techniques, constants, private members, best practices |
| `modded_class_examples` | Real-world examples: hygiene, stamina, recoil, nitro, earplugs |
| `modding_core_classes` | How to mod MissionServer, MissionGameplay, PlayerBase, CarScript |
| `custom_actions` | Custom player actions: SingleUse, Interact, Continuous patterns |

**Networking:**
| Section | Description |
|---------|-------------|
| `networking_overview` | Server/client architecture, detection, quick reference table |
| `networking_classes` | Key classes: DayZGame, MissionServer, MissionGameplay, PlayerBase |
| `networking_constraints` | Server-side vs client-side scripting limitations and rules |
| `networking_scenarios` | What runs where: inventory, player status, world, UI, persistence |
| `networking_rpc` | Detailed RPC patterns for server-client communication |
| `networking_netsync` | NetSync variables for automatic state synchronization |

**DayZ Systems:**
| Section | Description |
|---------|-------------|
| `patterns` | Essential code patterns: safe casting, server/client checks |
| `base_classes` | Key classes to extend: ItemBase, PlayerBase, EntityAI, etc. |
| `modules` | Load order of script modules and where to place files |
| `lifecycle` | Important methods for items, players, and missions |
| `entity_management` | Spawning, manipulating, and deleting entities |
| `inventory` | Inventory manipulation, cargo, attachments |
| `plugins` | Creating and registering custom plugins |
| `config` | Structure of config.cpp, CfgPatches, CfgVehicles |
| `file_structure` | Standard mod folder layout and organization |
| `actions` | How to create and register custom actions |
| `crafting` | How to define and register crafting recipes |
| `persistence` | Saving and loading data using storage and JSON |
| `events` | ScriptInvoker events and custom event patterns |
| `timers` | Timer class, CallLater patterns, delayed execution |
| `best_practices` | Code organization, performance, common mistakes |
| `limitations` | Enforce Script quirks and unsupported features |
| `debugging` | Print statements, logging, debugging tools |
| `pbo` | How mods are packaged and loaded |

### Search & Discovery
| Tool | Purpose |
|------|---------|
| `search_dayz_kb` | Community tutorials and best practices |
| `search_dayz_files` | Search actual DayZ source code |

### Mod Scaffolding & Generation
| Tool | Purpose |
|------|---------|
| `scaffold_mod` | Generate complete mod folder structure |
| `generate_item_config` | Create CfgVehicles entry for new items |
| `find_parent_class` | Find parent class for any DayZ class |
| `check_class_references` | Validate that referenced classes exist |
| `list_base_classes` | List known base classes with properties |
| `list_inventory_slots` | List valid inventory slot names |

### Quirks (Long-Term Memory)
| Tool | Purpose |
|------|---------|
| `list_enforce_quirks` | List known quirks and gotchas |
| `get_enforce_quirk` | Get full details of a quirk by ID |
| `save_enforce_quirk` | Save newly discovered quirks |

### Code Intelligence (LSP)
| Tool | Purpose |
|------|---------|
| `lsp_get_diagnostics` | Get compile errors/warnings for a file |
| `lsp_get_file_outline` | Get class/method structure of a file |
| `lsp_go_to_definition` | Find where a symbol is defined |
| `lsp_find_references` | Find all usages of a symbol |
| `lsp_get_symbol_info` | Get type info for a symbol |
| `lsp_get_completions` | Get code completions at a position |

---

## 📋 Required Workflow

### BEFORE Writing Code:

1. **Check for relevant quirks first**
   ```
   list_enforce_quirks search:"topic you're working on"
   ```

2. **Get the right context section**
   ```
   get_enforce_context section:"modded_class"
   get_enforce_context section:"lifecycle"
   get_enforce_context section:"networking"
   ```

3. **Find the original implementation before overriding**
   ```
   search_dayz_files "ClassName MethodName"
   find_parent_class className:"ClassName"
   ```

4. **For new items, scaffold correctly**
   ```
   scaffold_mod modName:"MyMod" includeExampleItem:true
   generate_item_config className:"MyItem" displayName:"My Item" baseClass:"ItemBase"
   ```

### BEFORE Finishing:

5. **Validate class references**
   ```
   check_class_references classNames:["ParentClass", "OtherClass"]
   ```

6. **Always validate with diagnostics**
   ```
   lsp_get_diagnostics for each file you created/modified
   ```
   Do not present code with errors.

7. **Save any new quirks discovered**
   ```
   save_enforce_quirk summary:"..." details:"..." tags:["syntax", "compiler"]
   ```

---

## 🎯 Tool Usage Patterns

**Starting a new mod:**
```
scaffold_mod modName:"MyMod" authorName:"Me" includeExampleItem:true includeExampleScript:true
```

**Adding a new item:**
```
list_base_classes                          → Find appropriate base class
generate_item_config className:"..." ...   → Generate config entry
get_enforce_context section:"config"       → Understand config structure
```

**Modifying existing behavior:**
```
search_dayz_files "ClassName"              → Find the class
find_parent_class className:"ClassName"    → Understand hierarchy
get_enforce_context section:"modded_class" → Get modded class syntax
list_enforce_quirks tag:"inheritance"      → Check known issues
```

**Understanding how something works:**
```
search_dayz_kb "topic"                     → Get tutorials first
search_dayz_files "ClassName"              → Find actual implementation
get_enforce_context section:"lifecycle"    → Understand method timing
```

**When something doesn't work:**
```
lsp_get_diagnostics                        → Check for compile errors
get_enforce_warnings                       → Check syntax pitfalls
list_enforce_quirks search:"..."           → Check if it's a known quirk
```

---

## 🧠 Quirks System

The quirks system stores knowledge about unexpected Enforce Script behaviors. **Always check before coding, always save when you discover something new.**

**Useful tags:** `syntax`, `inheritance`, `networking`, `compiler`, `lifecycle`, `null`, `casting`, `inventory`, `ui`, `performance`

**Example save:**
```
save_enforce_quirk
  summary: "Ternary operator doesn't work for assignments"
  details: "Enforce Script doesn't support x = a ? b : c syntax. Use if/else instead. The compiler will not error but behavior is undefined."
  tags: ["syntax", "compiler"]
```

---

## ✅ Pre-Completion Checklist

Before presenting code:
- [ ] Got appropriate context sections for the task
- [ ] Ran `lsp_get_diagnostics` - no errors
- [ ] Checked `list_enforce_quirks` for related issues
- [ ] Used `search_dayz_files` to verify override signatures
- [ ] Validated class references with `check_class_references`
- [ ] Saved any new quirks discovered

---

## ⚠️ Critical DayZ Modding Rules

### Modded Classes (MOST IMPORTANT!)

**To modify an existing vanilla class, use `modded class`:**
```cpp
modded class PlayerBase {
    override void Init() {
        super.Init();  // ALWAYS call super first!
        // Your custom code here
    }
}
```

**Do NOT use `extends` for existing classes:**
```cpp
// ❌ WRONG - This creates a NEW class, doesn't modify existing
class MyPlayer extends PlayerBase { }

// ✅ RIGHT - This modifies the existing PlayerBase everywhere
modded class PlayerBase { }
```

### Override Rules

1. **Always use the `override` keyword** - Compiler catches typos
2. **Always call `super.MethodName()`** - Preserves original behavior and mod compatibility
3. **Match exact method signatures** - Use `search_dayz_files` to find the original

```cpp
modded class ItemBase {
    override void OnInventoryEnter(Man player) {
        super.OnInventoryEnter(player);  // Call parent first
        // Your code after
    }
}
```

### Script Module Placement

Place your scripts in the **same module as the class you're modding**:

| Class | Module | Your Script Goes In |
|-------|--------|---------------------|
| `PlayerBase` | 4_World | `Scripts/4_World/` |
| `ItemBase` | 4_World | `Scripts/4_World/` |
| `MissionServer` | 5_Mission | `Scripts/5_Mission/` |
| `DayZGame` | 3_Game | `Scripts/3_Game/` |

### Client-Server Architecture

```cpp
// Code that should only run on server
if (GetGame().IsServer()) {
    // Spawn items, process game logic
    GetGame().CreateObject("ItemClassName", position, false, false, true);
}

// Code that should only run on client
if (GetGame().IsClient()) {
    // UI updates, local effects
}
```

### Common Patterns

**Safe Casting (ALWAYS use this):**
```cpp
PlayerBase player;
if (Class.CastTo(player, someEntity)) {
    player.DoSomething();
}
```

**Creating Objects (server-side):**
```cpp
EntityAI obj = GetGame().CreateObject("ClassName", position, false, false, true);
```

### Syntax Pitfalls

```cpp
// ❌ NO ternary assignments
x = condition ? a : b;

// ✅ Use if/else instead
if (condition) { x = a; } else { x = b; }
```

```cpp
// ❌ NO inline single-statement if
if (x) DoA(); DoB();  // DoB always runs!

// ✅ Use braces
if (x) { DoA(); DoB(); }
```

---

## 📁 Mod File Structure

```
YourMod/
├── mod.cpp                 # Mod registration
├── config.cpp              # CfgPatches, CfgMods, CfgVehicles
├── Scripts/
│   ├── 3_Game/             # Game logic (rare)
│   ├── 4_World/            # Entities, items, players (most common)
│   │   ├── MyModdedPlayer.c
│   │   └── MyNewItem.c
│   └── 5_Mission/          # Mission, UI scripts
├── data/                   # Models, textures (optional)
└── keys/                   # Server signature keys
```

Use `scaffold_mod` to generate this structure automatically!
