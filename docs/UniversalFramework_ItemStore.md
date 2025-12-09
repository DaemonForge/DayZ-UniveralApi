# Universal Framework - Item Store

## Overview

The Item Store (`UEntityStore`) provides complete serialization and deserialization of DayZ entities including items, weapons, vehicles, and their full state - cargo, attachments, damage, ammunition, and custom properties.

## Key Features

- Full entity state serialization to JSON
- Recursive cargo/attachment handling
- Weapon chamber and magazine state
- Vehicle state preservation
- Damage zone tracking
- Custom property storage via `Write`/`Read`
- Position and orientation restoration

## UEntityStore Class

Located in `_UFramework/scripts/4_World/UF/ItemStore.c`.

### Core Methods

```enforce
class UEntityStore extends UFObject_Base {
    // Save entity state
    void SaveEntity(EntityAI entity, bool saveCargo = true);
    
    // Create entity from saved state
    EntityAI Create(EntityAI parent, bool inLocation = true);
    EntityAI CreateAtPos(vector pos, vector ori = "0 0 0");
    
    // Custom data storage (use in OnUFSave/OnUFLoad overrides)
    void Write(string key, int value);
    void Write(string key, float value);
    void Write(string key, bool value);
    void Write(string key, string value);
    void Write(string key, vector value);
    
    bool Read(string key, out int value);
    bool Read(string key, out float value);
    bool Read(string key, out bool value);
    bool Read(string key, out string value);
    bool Read(string key, out vector value);
}

// Override these in your items for custom data
modded class ItemBase {
    override void OnUFSave(UEntityStore data);
    override void OnUFLoad(UEntityStore data);
}
```

## Saving Entities

### Save Single Entity

```enforce
// Save an item to JSON
ItemBase item = GetPlayerWeapon();
UEntityStore store = new UEntityStore();
store.SaveEntity(item, true);  // true = include cargo recursively

// Convert to JSON
string json;
if (UJSONHandler<UEntityStore>.GetString(store, json)) {
    // Save to database
    U().db().Save("MyMod", "item_" + itemId, json);
}
```

### Save Without Cargo

```enforce
// Save just the item, not its contents
store.SaveEntity(item, false);
```

### Save Player Inventory

```enforce
void SavePlayerInventory(PlayerBase player) {
    array<EntityAI> items = new array<EntityAI>;
    player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);
    
    array<autoptr UEntityStore> inventory = new array<autoptr UEntityStore>;
    
    foreach (EntityAI item : items) {
        if (item.GetHierarchyParent() == player) {  // Only top-level items
            UEntityStore store = new UEntityStore();
            store.SaveEntity(item, true);
            inventory.Insert(store);
        }
    }
    
    string json;
    if (UJSONHandler<array<autoptr UEntityStore>>.GetString(inventory, json)) {
        U().db(PLAYER_DB).Save("Inventory", player.GetIdentity().GetPlainId(), json);
    }
}
```

## Creating Entities

### Create in Inventory

```enforce
void LoadPlayerInventory(PlayerBase player) {
    U().db(PLAYER_DB).Load("Inventory", player.GetIdentity().GetPlainId(), 
        new UFCallback<array<autoptr UEntityStore>>(this, "OnInventoryLoaded", player.GetIdentity().GetPlainId()));
}

void OnInventoryLoaded(int cid, int status, string oid, array<autoptr UEntityStore> inventory) {
    PlayerBase player = PlayerBase.Cast(UUtil.FindPlayer(oid));
    if (!player || status != UF_SUCCESS || !inventory) return;
    
    foreach (UEntityStore store : inventory) {
        store.Create(player, true);  // Create with original location
    }
}
```

### Create at Position

```enforce
void SpawnItemAtPosition(UEntityStore store, vector position, vector orientation) {
    EntityAI item = store.CreateAtPos(position, orientation);
    if (item) {
        Print("Spawned: " + item.GetType());
    }
}
```

### Create Options

```enforce
// Create in parent's inventory (respects original slot/position)
EntityAI item = store.Create(parent, true);

// Create ignoring original location
EntityAI item = store.Create(parent, false);

// Create at world position
EntityAI item = store.CreateAtPos(position);
EntityAI item = store.CreateAtPos(position, orientation);
```

## Custom Properties

Extend items to save/load custom mod data:

```enforce
class MyCustomItem extends ItemBase {
    protected int m_CustomLevel;
    protected string m_CustomName;
    
    override void OnUFSave(UEntityStore data) {
        super.OnUFSave(data);
        data.Write("CustomLevel", m_CustomLevel);
        data.Write("CustomName", m_CustomName);
    }
    
    override void OnUFLoad(UEntityStore data) {
        super.OnUFLoad(data);
        if (!data.Read("CustomLevel", m_CustomLevel)) m_CustomLevel = 1;
        if (!data.Read("CustomName", m_CustomName)) m_CustomName = "";
    }
}
```

## Complete Example: Storage System

```enforce
class StorageChest {
    protected string m_StorageId;
    
    void SaveContents(EntityAI container) {
        array<autoptr UEntityStore> contents = new array<autoptr UEntityStore>;
        
        array<EntityAI> items = new array<EntityAI>;
        container.GetInventory().EnumerateInventory(InventoryTraversalType.LEVELORDER, items);
        
        foreach (EntityAI item : items) {
            if (item == container) continue;
            if (container.GetInventory().HasEntityInCargo(item) || 
                container.GetInventory().HasAttachment(item)) {
                
                UEntityStore store = new UEntityStore();
                store.SaveEntity(item, true);
                contents.Insert(store);
            }
        }
        
        string json;
        if (UJSONHandler<array<autoptr UEntityStore>>.GetString(contents, json)) {
            U().db().Save("Storage", m_StorageId, json);
        }
    }
    
    void LoadContents(EntityAI container) {
        U().db().Load("Storage", m_StorageId, 
            new UFCallback<array<autoptr UEntityStore>>(this, "OnContentsLoaded"));
    }
    
    void OnContentsLoaded(int cid, int status, string oid, array<autoptr UEntityStore> contents) {
        if (status != UF_SUCCESS || !contents) return;
        
        EntityAI container = GetContainer();
        if (!container) return;
        
        foreach (UEntityStore store : contents) {
            store.Create(container, true);
        }
    }
}
```

### Player Loadout System

```enforce
class LoadoutManager {
    
    void SaveLoadout(PlayerBase player, string loadoutName) {
        array<autoptr UEntityStore> loadout = new array<autoptr UEntityStore>;
        
        // Save equipped items
        array<EntityAI> items = new array<EntityAI>;
        player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);
        
        foreach (EntityAI item : items) {
            if (item.GetHierarchyParent() == player) {
                UEntityStore store = new UEntityStore();
                store.SaveEntity(item, true);
                loadout.Insert(store);
            }
        }
        
        string json;
        if (UJSONHandler<array<autoptr UEntityStore>>.GetString(loadout, json)) {
            U().db(PLAYER_DB).Save("Loadout_" + loadoutName, 
                player.GetIdentity().GetPlainId(), json);
        }
    }
    
    void ApplyLoadout(PlayerBase player, string loadoutName) {
        // Clear current inventory first
        ClearInventory(player);
        
        // Load saved loadout
        U().db(PLAYER_DB).Load("Loadout_" + loadoutName,
            player.GetIdentity().GetPlainId(),
            new UFCallback<array<autoptr UEntityStore>>(this, "OnLoadoutLoaded", 
                player.GetIdentity().GetPlainId()));
    }
    
    void OnLoadoutLoaded(int cid, int status, string oid, array<autoptr UEntityStore> loadout) {
        PlayerBase player = PlayerBase.Cast(UUtil.FindPlayer(oid));
        if (!player || status != UF_SUCCESS || !loadout) return;
        
        foreach (UEntityStore store : loadout) {
            store.Create(player, true);
        }
    }
    
    protected void ClearInventory(PlayerBase player) {
        array<EntityAI> items = new array<EntityAI>;
        player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);
        
        foreach (EntityAI item : items) {
            if (item != player) {
                GetGame().ObjectDelete(item);
            }
        }
    }
}
```

## Best Practices

1. **Use recursive save** for complete state (`SaveEntity(item, true)`)
2. **Handle null items** in cargo - some slots may be empty
3. **Override OnUFSave/OnUFLoad** for custom item data
4. **Test weapon restoration** - magazines and chambers are complex
5. **Clean up before loading** - especially for inventory systems
6. **Verify entity creation** - Create can return null
7. **Consider performance** - large inventories = large JSON
