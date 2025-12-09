# Universal Framework - Texture System

## Overview

The Texture System provides dynamic texture and material management for items, allowing runtime skin changes, color variations, and paintable items without requiring additional model files.

## Key Features

- Register multiple textures/materials per item
- Named skin variants
- Per-player skin restrictions
- Network synchronization of skin state
- Persistence through server restarts
- Random skin selection on spawn

## Implementation

The system is implemented via `modded class ItemBase` in `_UFramework/scripts/4_World/UF/Entities/ItemBase.c`.

### Core Methods

```enforce
modded class ItemBase {
    // Registration (call in InitSkins override)
    void RegisterTextureAndMaterial(string name, string Tpath = "", string Mpath = "", TStringArray ids = NULL);
    void RegisterTextureAndMaterialArray(string name, TStringArray Tpath, TStringArray Mpath = NULL, TStringArray ids = NULL);
    
    // Override this to register your skins don't foget to call super.InitSkins()
    void InitSkins();
    
    // Getters
    int GetCurrentSkinIdx();              // Current skin index (-1 if none)
    int GetTextureCount();                // Number of registered skins
    int GetIndexByName(string name);      // Get skin index by name
    string GetTextureName(int textureID); // Get skin name by index
    TStringArray GetNames();              // Get all skin names
    TStringArray GetTexture(int textureID);   // Get texture paths for skin
    TStringArray GetMaterial(int textureID);  // Get material paths for skin
    TStringArray GetAllowedIds(int textureID); // Get allowed player IDs for skin
    bool CanPaint();                      // True if item has multiple skins
    
    // Setters
    void SetTexture(int index);           // Apply skin by index (syncs to clients)
    
    // Utility
    int GetNextTendancy(int Tendancy);    // Get next skin index (skips current)
    void RefreshTextures();               // Reapply current texture
    void SkinMgr_debug();                 // Debug print all skin info
}
```

## Registering Skins

Override `InitSkins()` in your item class to register available textures.

### Single Texture/Material

```enforce
class MyPaintableItem extends ItemBase {
    
    override void InitSkins() {
        super.InitSkins();
        // RegisterTextureAndMaterial(name, texturePath, materialPath, allowedPlayerIds)
        RegisterTextureAndMaterial("Default", "", "");  // Original texture
        RegisterTextureAndMaterial("Red", 
            "#(argb,8,8,3)color(0.8,0,0,1.0,CO)", 
            "");
        RegisterTextureAndMaterial("Blue",
            "#(argb,8,8,3)color(0,0,0.8,1.0,CO)",
            "");
        RegisterTextureAndMaterial("Custom",
            "MyMod\\data\\custom_texture.paa",
            "MyMod\\data\\custom_material.rvmat");
    }
}
```

### Multiple Texture Slots

For items with multiple texture slots (selection sets):

```enforce
class MyMultiTextureItem extends ItemBase {
    
    override void InitSkins() {
        super.InitSkins();
        // RegisterTextureAndMaterialArray(name, textureArray, materialArray, allowedIds)
        
        // Each array index corresponds to a selection set on the model
        TStringArray redTextures = {
            "#(argb,8,8,3)color(0.8,0,0,1.0,CO)",   // Selection 0
            "#(argb,8,8,3)color(0.8,0,0,1.0,CO)",   // Selection 1
            "#(argb,8,8,3)color(0.6,0,0,1.0,CO)"    // Selection 2
        };
        RegisterTextureAndMaterialArray("Red", redTextures);
        
        TStringArray blueTextures = {
            "#(argb,8,8,3)color(0,0,0.8,1.0,CO)",
            "#(argb,8,8,3)color(0,0,0.8,1.0,CO)",
            "#(argb,8,8,3)color(0,0,0.6,1.0,CO)"
        };
        RegisterTextureAndMaterialArray("Blue", blueTextures);
    }
}
```

### Restricted Skins

Limit certain skins to specific player IDs (Steam IDs):

```enforce
override void InitSkins() {
        super.InitSkins();
    RegisterTextureAndMaterial("Default", "", "");
    RegisterTextureAndMaterial("Common", "path/common.paa", "");
    
    // VIP-only skin
    TStringArray vipPlayers = {"76561198012345678", "76561198087654321"};
    RegisterTextureAndMaterial("VIP Gold", "path/vip_gold.paa", "", vipPlayers);
    
    // Staff-only skin  
    TStringArray staffPlayers = {"76561198011111111"};
    RegisterTextureAndMaterial("Staff", "path/staff.paa", "", staffPlayers);
}
```

## Setting Textures

### Programmatic Change

```enforce
ItemBase item = GetPlayerWeapon();
if (item && item.CanPaint()) {
    item.SetTexture(2);  // Set to skin index 2
}
```

### By Name

```enforce
int idx = item.GetIndexByName("Red");
if (idx >= 0) {
    item.SetTexture(idx);
}
```

### Cycle Through Skins

```enforce
void NextSkin(ItemBase item) {
    if (!item || !item.CanPaint()) return;
    
    int current = item.GetCurrentSkinIdx();
    int next = item.GetNextTendancy(current);
    item.SetTexture(next);
}
```

## Querying Skin Info

```enforce
ItemBase item = GetItem();

// Check if paintable
if (item.CanPaint()) {
    
    // Get available skin count
    int count = item.GetTextureCount();
    
    // Get current skin index
    int current = item.GetCurrentSkinIdx();
    
    // Get skin names
    TStringArray names = item.GetNames();
    
    // Get specific skin name
    string name = item.GetTextureName(current);
    
    // Get texture paths for a skin
    TStringArray textures = item.GetTexture(0);
    
    // Get material paths for a skin
    TStringArray materials = item.GetMaterial(0);
    
    // Check allowed players for a skin
    TStringArray allowed = item.GetAllowedIds(2);
}
```

## Complete Example: Colored Armbandslored Armbands

```enforce
class ArmBand_Colorful extends ArmBand_ColorBase {
    
    override void InitSkins() {
        // Solid colors using procedural textures
        RegisterTextureAndMaterialArray("White", {
            "#(argb,8,8,3)color(0.65,0.65,0.65,1.0,CO)",
            "#(argb,8,8,3)color(0.65,0.65,0.65,1.0,CO)"
        });
        
        RegisterTextureAndMaterialArray("Red", {
            "#(argb,8,8,3)color(0.8,0,0,1.0,CO)",
            "#(argb,8,8,3)color(0.8,0,0,1.0,CO)"
        });
        
        RegisterTextureAndMaterialArray("Blue", {
            "#(argb,8,8,3)color(0,0,0.8,1.0,CO)",
            "#(argb,8,8,3)color(0,0,0.8,1.0,CO)"
        });
        
        RegisterTextureAndMaterialArray("Green", {
            "#(argb,8,8,3)color(0,0.5,0,1.0,CO)",
            "#(argb,8,8,3)color(0,0.5,0,1.0,CO)"
        });
        
        RegisterTextureAndMaterialArray("Yellow", {
            "#(argb,8,8,3)color(0.8,0.8,0,1.0,CO)",
            "#(argb,8,8,3)color(0.8,0.8,0,1.0,CO)"
        });
        
        RegisterTextureAndMaterialArray("Orange", {
            "#(argb,8,8,3)color(0.8,0.4,0,1.0,CO)",
            "#(argb,8,8,3)color(0.8,0.4,0,1.0,CO)"
        });
    }
}
```

## Paint Kit Action

Create an action to paint items:

```enforce
class ActionPaintItem : ActionSingleUseBase {
    
    override void OnExecuteServer(ActionData action_data) {
        ItemBase paintKit = ItemBase.Cast(action_data.m_MainItem);
        ItemBase target = ItemBase.Cast(action_data.m_Target.GetObject());
        
        if (target && target.CanPaint()) {
            int nextSkin = target.GetNextTendancy(target.GetCurrentSkinIdx());
            target.SetTexture(nextSkin);
            
            // Consume paint kit durability
            paintKit.AddHealth("", "", -10);
        }
    }
    
    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item) {
        ItemBase targetItem = ItemBase.Cast(target.GetObject());
        return targetItem && targetItem.CanPaint();
    }
}
```

## Debug

```enforce
void DebugSkins(ItemBase item) {
    Print("=== Skin Debug ===");
    Print("Can Paint: " + item.CanPaint());
    Print("Skin Count: " + item.GetTextureCount());
    Print("Current Skin: " + item.GetCurrentSkinIdx());
    
    for (int i = 0; i < item.GetTextureCount(); i++) {
        Print("Skin " + i + ": " + item.GetTextureName(i));
    }
    
    // Full debug
    item.SkinMgr_debug();
}
```

## Client-Side Skin Override

You can override which skin is displayed on the client by overriding `GetCurrentSkinIdx()`. This is useful for:
- Displaying different textures for certain skins locally (e.g., higher-res versions)
- Player preferences (substituting one color for another)
- Client-side skin packs that remap skins

### Override by Skin Name

Override `GetCurrentSkinIdx()` to return a different skin index based on the current skin's name:

```enforce
modded class MyItem {
    
    override int GetCurrentSkinIdx() {
        int idx = super.GetCurrentSkinIdx();
        
        // Client-side only override
        if (GetGame().IsClient() /* && some custom condition */) {
            return GetIndexByName("Blue");
        }
        return idx;
    }
}
```


> **Note:** Client-side overrides only affect visual display. The server still tracks and syncs the original skin index. Other players will see the server-synced skin, not your local override.

## Best Practices

1. **Always include a default skin** - index 0 should be the original texture
2. **Use procedural colors** for simple color changes - faster than texture files
3. **Match array sizes** - texture and material arrays should have same count
4. **Initialize in InitSkins()** - called from constructor
5. **Test on server** - ensure sync works correctly
6. **Consider performance** - many texture slots can impact performance
7. **Client overrides are local only** - other players won't see your custom textures
