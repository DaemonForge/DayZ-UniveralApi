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



# 🚨 CRITICAL: ENFORCE SCRIPT IS NOT C++ - YOU WILL FAIL WITHOUT TOOLS

## ⛔ STOP! READ THIS FIRST!

**Enforce Script LOOKS like C++ but it is NOT.** It has dozens of quirks, missing features, and unique syntax that WILL break your code if you guess.

**FACTS:**
- ❌ No ternary operator (`a ? b : c` does NOT work)
- ❌ No `static_cast`, `dynamic_cast` - must use `Class.CastTo()`
- ❌ No templates, no lambdas, no auto keyword
- ❌ `extends` does NOT modify vanilla classes - you need `modded class`
- ❌ Missing `override` keyword = silent failure, not an error
- ❌ Forgetting `super.Method()` = broken inheritance chain

**Your code WILL NOT WORK if you write from memory.** You MUST use the tools below.

---

## 🔥 YOU HAVE FULL TOOL ACCESS - USE IT!

You have access to the complete DayZ modding toolkit. **USE THESE TOOLS FOR EVERY TASK.**

### 📚 KNOWLEDGE BASE TOOLS (Start Here!)

#### `search_dayz_kb` - YOUR FIRST STOP FOR EVERY TASK
**What it does:** Searches a curated knowledge base of DayZ modding tutorials, best practices, and expert guides.

**Why it's critical:** The KB contains solutions to problems the community has already solved. It knows about quirks, pitfalls, and the RIGHT way to implement features.

**Parameters:**
- `query` (required): What you're trying to do or learn about
- `limit` (optional): Number of results (default: 5)

**Example usage:**
```
search_dayz_kb query:"how to create custom item"
search_dayz_kb query:"modded class override method"
search_dayz_kb query:"RPC server client communication"
search_dayz_kb query:"player inventory manipulation"
search_dayz_kb query:"config.cpp CfgVehicles syntax"
```

**USE THIS BEFORE WRITING ANY CODE!**

---

#### `list_dayz_kb` - See All Available KB Articles
**What it does:** Returns a list of all available knowledge base articles.

**When to use:** When you want to browse what documentation is available, or find the exact name of an article.

**Parameters:** None

---

#### `get_dayz_kb_article` - Get a Specific KB Article
**What it does:** Retrieves the full content of a specific KB article by name.

**Parameters:**
- `name` (required): The exact article name from `list_dayz_kb`

**Example:**
```
get_dayz_kb_article name:"modded_class_overview"
get_dayz_kb_article name:"custom_actions"
```

---

### 🔍 SOURCE CODE SEARCH TOOLS (Find Exact Implementations!)

#### `search_dayz_files` - FIND THE ACTUAL VANILLA CODE
**What it does:** Vector search across ALL DayZ source files to find relevant code, classes, and implementations.

**Why it's critical:** You MUST see the actual vanilla implementation before overriding methods. The method signature, parameters, and return type must match EXACTLY.

**Parameters:**
- `query` (required): What to search for (class names, method names, concepts)
- `limit` (optional): Number of results (default: 5)
- `includeMods` (optional): Include community mod code (default: false)

**Example usage:**
```
search_dayz_files query:"PlayerBase OnConnect"
search_dayz_files query:"ItemBase EEKilled"
search_dayz_files query:"override void Init"
search_dayz_files query:"GetGame().CreateObject"
search_dayz_files query:"RPC SendRPC"
```

**ALWAYS use this to find method signatures before overriding!**

---

#### `get_dayz_file_by_class` - Get File Content by Class Name
**What it does:** Finds and returns the file(s) containing a specific class, including the first 3 parts of content.

**Parameters:**
- `className` (required): The class name to find
- `includeMods` (optional): Include community mods

**Example:**
```
get_dayz_file_by_class className:"PlayerBase"
get_dayz_file_by_class className:"ItemBase"
get_dayz_file_by_class className:"CarScript"
```

---

#### `get_dayz_file_parts` - Get Parts List for Large Files
**What it does:** Large files are split into parts. This returns the list of available parts for a file.

**Parameters:**
- `filePath` (required): The file path

**Example:**
```
get_dayz_file_parts filePath:"scripts/4_world/entities/manbase/playerbase.c"
```

---

#### `get_dayz_file_part` - Get Specific Part of a File
**What it does:** Retrieves a specific part of a large file.

**Parameters:**
- `filePath` (required): The file path
- `partNumber` (required): Part number (1-based)

**Example:**
```
get_dayz_file_part filePath:"scripts/4_world/entities/manbase/playerbase.c" partNumber:2
```

---

#### `search_in_dayz_file` - Search Within a Specific File
**What it does:** Text search within a specific file to find methods, variables, or patterns.

**Parameters:**
- `filePath` (required): The file path
- `searchTerm` (required): Text to search for (case-insensitive)

**Example:**
```
search_in_dayz_file filePath:"scripts/4_world/entities/itembase.c" searchTerm:"OnInventoryEnter"
```

---

#### `get_dayz_credits` - Check API Credits
**What it does:** Returns your remaining DaemonForge API credits.

---

## 📋 MANDATORY WORKFLOW - FOLLOW THIS EVERY TIME!

### Step 1: Research the Feature (BEFORE writing ANY code)
```
search_dayz_kb query:"[what you want to build]"
```
Read the KB articles. They contain the RIGHT way to do things.

### Step 2: Find the Vanilla Implementation
```
search_dayz_files query:"[class or method you need]"
get_dayz_file_by_class className:"[TargetClass]"
```
You MUST see the original code to override correctly.

### Step 3: Understand the Class Structure
```
lsp_find_class className:"[TargetClass]"
lsp_get_class_hierarchy className:"[TargetClass]"
lsp_get_class_methods className:"[TargetClass]"
```

### Step 4: Check for Known Quirks
```
list_enforce_quirks search:"[topic]"
get_enforce_context section:"[topic]"
```

### Step 5: Write Code
NOW you can write code, using the information you gathered.

### Step 6: VALIDATE (Required!)
```
lsp_get_diagnostics filePath:"[your file]"
check_class_references classNames:["Class1", "Class2"]
```
**DO NOT present code with compile errors!**

---

## 🛠️ COMPLETE TOOL REFERENCE

### 📚 DayZ Knowledge Base Tools
| Tool | Description |
|------|-------------|
| `search_dayz_kb` | Vector search the knowledge base for tutorials, guides, best practices |
| `list_dayz_kb` | List all available KB articles |
| `get_dayz_kb_article` | Get full content of a specific KB article |

### 🔍 DayZ Source Code Tools
| Tool | Description |
|------|-------------|
| `search_dayz_files` | Vector search across all DayZ source files |
| `get_dayz_file_by_class` | Get file content by class name |
| `get_dayz_file_parts` | Get list of parts for a large file |
| `get_dayz_file_part` | Get specific part of a file |
| `search_in_dayz_file` | Search within a specific file |
| `get_dayz_credits` | Check remaining API credits |

### 🏗️ Mod Scaffolding Tools
| Tool | Description |
|------|-------------|
| `scaffold_mod` | Generate complete mod folder structure with all required files |
| `generate_item_config` | Generate config.cpp CfgVehicles entry for new items |
| `list_base_classes` | List common base classes with their properties |
| `find_parent_class` | Find the parent class of a given class |
| `check_class_references` | Verify that class names exist in vanilla |
| `list_inventory_slots` | List valid inventory slot names |

### 🔬 Code Intelligence (LSP) Tools
| Tool | Description |
|------|-------------|
| `lsp_find_class` | Find a class definition by exact name |
| `lsp_search_classes` | Search for classes matching a pattern |
| `lsp_get_class_hierarchy` | Get complete inheritance hierarchy |
| `lsp_find_child_classes` | Find all classes extending a base class |
| `lsp_get_class_methods` | Get all methods defined in a class |
| `lsp_find_method_overrides` | Find all overrides of a method |
| `lsp_search_symbols` | Search for any symbol type |
| `lsp_get_file_outline` | Get structure/outline of a file |
| `lsp_go_to_definition` | Navigate to symbol definition |
| `lsp_find_references` | Find all usages of a symbol |
| `lsp_get_symbol_info` | Get detailed info about a symbol |
| `lsp_get_completions` | Get code completion suggestions |
| `lsp_get_diagnostics` | **CRITICAL** - Get compile errors/warnings |

### 📂 Project Drive Tools (P:\ Access)
| Tool | Description |
|------|-------------|
| `project_read_file` | Read vanilla source file content |
| `project_list_files` | List files in vanilla directories |
| `project_grep` | Search vanilla code with text/regex |

### 🧠 Quirks & Context Tools
| Tool | Description |
|------|-------------|
| `get_enforce_context` | Get syntax reference for a topic |
| `list_context_sections` | List all available context sections |
| `get_enforce_warnings` | Get common Enforce Script pitfalls to avoid |
| `refresh_context` | Reload context after adding new .insc files |
| `list_enforce_quirks` | List known Enforce Script quirks |
| `get_enforce_quirk` | Get details on a specific quirk |
| `save_enforce_quirk` | Save a new quirk you discovered |

---

## ⚠️ ENFORCE SCRIPT TRAPS - MEMORIZE THESE!

### ❌ Things That DO NOT Work (Even Though They Look Like C++)

| What You Might Try | Why It Fails | What To Do Instead |
|-------------------|--------------|---------------------|
| `x = a ? b : c;` | No ternary operator | `if (a) x = b; else x = c;` |
| `PlayerBase.Cast(entity)` | Wrong cast syntax | `Class.CastTo(player, entity);` |
| `class MyPlayer extends PlayerBase` | Creates NEW class, doesn't modify vanilla | `modded class PlayerBase` |
| `void OnInit() { ... }` | Missing override keyword | `override void OnInit() { ... }` |
| `override void OnInit() { MyCode(); }` | Missing super call | `super.OnInit(); MyCode();` |
| `static MyClass instance;` | Static member syntax differs | Use singleton pattern with function |
| `auto x = GetSomething();` | No auto keyword | Explicitly declare type |
| `[](int x) { return x * 2; }` | No lambdas | Use named functions |
| `template<T>` | No templates | Use specific types or Managed |

### ✅ Correct Patterns

**Casting:**
```cpp
PlayerBase player;
if (Class.CastTo(player, entity)) {
    // player is now valid
}
```

**Modifying Vanilla Classes:**
```cpp
modded class PlayerBase {
    override void Init() {
        super.Init();  // ALWAYS call super FIRST!
        // Your modifications here
    }
}
```

**Null Checks:**
```cpp
if (object && object.IsValid()) {
    // Safe to use object
}
```

---

## 📁 MOD STRUCTURE

```
YourMod/
├── mod.cpp              # Mod registration (name, author, version)
├── config.cpp           # CfgPatches, CfgVehicles, CfgMods
└── Scripts/
    ├── 3_Game/          # Core game systems (rare to modify)
    ├── 4_World/         # Items, players, entities, vehicles (most common)
    │   └── YourMod/     # Your script files go here
    └── 5_Mission/       # Mission logic, UI, menus
```

Use `scaffold_mod modName:"YourMod"` to generate this automatically!

---

## ✅ FINAL CHECKLIST - DO NOT SKIP!

Before presenting ANY code to the user, verify:

- [ ] ✅ Used `search_dayz_kb` to find the right approach
- [ ] ✅ Used `search_dayz_files` to find vanilla implementation
- [ ] ✅ Method signatures match vanilla EXACTLY
- [ ] ✅ Used `lsp_get_diagnostics` - **ZERO compile errors**
- [ ] ✅ All `override` methods call `super.MethodName()` first
- [ ] ✅ Used `modded class` (NOT `extends`) for vanilla modifications
- [ ] ✅ Checked `list_enforce_quirks` for known issues
- [ ] ✅ No ternary operators, no auto, no lambdas
- [ ] ✅ Using `Class.CastTo()` for all casts

---

## 📚 CONTEXT SECTIONS

Use `get_enforce_context section:"name"` for detailed syntax reference:

| Category | Available Sections |
|----------|-------------------|
| **Core Language** | `variables`, `functions`, `operators`, `control_flow`, `arrays`, `enums` |
| **OOP** | `oop`, `inheritance`, `overrides`, `modded_class_overview`, `modded_class_advanced` |
| **Patterns** | `patterns`, `base_classes`, `modules`, `lifecycle` |
| **Networking** | `networking_overview`, `networking_rpc`, `networking_netsync` |
| **UI** | `ui_widget_types`, `ui_widget_code`, `ui_quick_reference` |
| **Systems** | `actions`, `inventory`, `config`, `file_structure`, `persistence` |

Use `list_context_sections` to see all available sections with descriptions.

