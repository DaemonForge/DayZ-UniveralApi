# Universal Framework - Mod Initial Setup

## Overview
This guide covers how to set up your development environment and integrate the **Universal Framework** into your DayZ mod.

## Prerequisites
1.  **Work Drive Setup**: Ensure you have a valid `P:` drive setup for DayZ modding.
2.  **Dependencies**: Your mod must depend on `UniversalApi` (Workshop ID: `2984102636`).

---

## 1. Adding the Dependency

In your mod's `config.cpp`, you must add `UFramework` to your `requiredAddons`.

```cpp
class CfgPatches
{
	class YourModName_Scripts
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]=
		{
			"DZ_Data",
			"UFramework" // <--- CRITICAL: Adds dependency
		};
	};
};
```

This ensures the Universal Framework scripts load **before** your mod scripts.

---

## 2. Setting Up Your Scripts

Because `UFramework` loads its core definitions in `1_Core`, `3_Game`, and `4_World`, you can access them from corresponding layers in your mod.

### Best Practice: `UFrameworkReady`

Do **NOT** attempt to use `U()` or make API calls in `Init()`, `Constructor()`, or `Start()`. The framework needs time to authenticate with the backend.

Instead, override `UFrameworkReady()` in your Mission class.

#### For Server-Side Logic (MissionServer.c)

```enforce
modded class MissionServer
{
    override void UFrameworkReady()
    {
        super.UFrameworkReady(); // ALWAYS call super!
        
        Print("[YourMod] Framework is ready! Server ID: " + U().GetServerID());
        
        // Safe to start loading data
        U().globals().Load("YourMod", "Config", this, "OnConfigLoaded");
    }
}

## Tags
`modder`, `setup`, `dependency`, `uFrameworkReady`, `getting-started`, `how-to`, `reference`, `doc-usage`
```

#### For Client-Side Logic (MissionGameplay.c)

```enforce
modded class MissionGameplay
{
    override void UFrameworkReady()
    {
        super.UFrameworkReady();
        
        Print("[YourMod] Client authenticated!");
        
        // Safe to use player-specific endpoints
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (player)
        {
            string uid = player.GetIdentity().GetId();
            U().db(PLAYER_DB).Load("YourMod", uid, this, "OnPlayerStatsLoaded");
        }
    }
}
```

---

## 3. Configuration (Server Admin Side)

While you (the modder) write the code, the **server admin** must configure the connection. Your mod should not require hardcoded URLs.

The server admin handles:
1.  Installing `@UniversalApi` + `UFService`.
2.  Editing `$profile:UF/UFramework.json`.
3.  Generating auth tokens.

You typically **do not** need to include any JSON config files in your mod PBO.

---

## 4. Debugging

### Enable Debug Mode
You can enable verbal logging during development:

```enforce
// In your initialization
U().SetDebug(true); 
```

### Checking Status
Always verify the connection status before critical operations:

```enforce
if (!U().IsOnline())
{
    Print("Warning: Universal Framework is offline!");
    return;
}
```

---

## Next Steps
- Learn about [Database Operations](UniversalFramework_DBHandler_Basics.md)
- Integrate [Discord Features](UniversalFramework_Discord.md)
- Create [AI NPCs](UniversalFramework_AIChat.md)
