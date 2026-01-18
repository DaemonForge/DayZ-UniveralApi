# Universal Framework - Player Money Handling

## Overview

Physical currency system using inventory items as money. Configure item types as currency denominations, then use PlayerBase methods to manage player wealth through inventory.

## Currency Configuration

Register currencies with the `UCurrency` class:

```enforce
// Register currency types (item class name -> value)
TStringIntMap money = new TStringIntMap();
money.Set("Paper_1Dollar", 1);
money.Set("Paper_5Dollar", 5);
money.Set("Paper_20Dollar", 20);
money.Set("Paper_100Dollar", 100);

UCurrency.Register("dollars", money);
```

## UCurrencyValue Class

```enforce
class UCurrencyValue {
    string TypeClass();  // Item class name
    int Value();         // Monetary value
}
```

## PlayerBase Methods

All methods require a currency key (registered via `UCurrency.Register`):

```enforce
// Get total balance from inventory items
int UGetPlayerBalance(string key);

// Add money (spawns items in inventory or ground)
// Returns: 0=success, 1=some dropped on ground, 2=invalid amount
int UAddMoney(string key, int Amount);

// Remove money (deletes items, makes change if needed)
// Returns: 0=success, 1=made change, 2=invalid amount
int URemoveMoney(string key, int Amount);
```

## Usage Examples

### Check Balance

```enforce
PlayerBase player = PlayerBase.Cast(GetPlayer());
int balance = player.UGetPlayerBalance("dollars");
Print("Balance: $" + balance);
```

### Give Money

```enforce
int result = player.UAddMoney("dollars", 150);
if (result == 0) {
    Print("Added to inventory");
} else if (result == 1) {
    Print("Some dropped on ground (inventory full)");
}
```

### Take Money

```enforce
int balance = player.UGetPlayerBalance("dollars");
int price = 50;

if (balance >= price) {
    int result = player.URemoveMoney("dollars", price);
    if (result == 0 || result == 1) {
        // Purchase successful
        SpawnItem(player, "Apple");
    }
} else {
    NotifyPlayer(player, "Insufficient funds");
}
```

### Shop Example

```enforce
class SimpleShop {
    
    void Purchase(PlayerBase player, string itemClass, int price) {
        int balance = player.UGetPlayerBalance("dollars");
        
        if (balance < price) {
            SendNotification("Shop", "Need $" + price + ", have $" + balance, player.GetIdentity());
            return;
        }
        
        int result = player.URemoveMoney("dollars", price);
        if (result <= 1) {  // 0 or 1 = success
            EntityAI item = GetGame().CreateObject(itemClass, player.GetPosition(), false, false, true);
            SendNotification("Shop", "Purchased " + itemClass, player.GetIdentity());
        }
    }
}
```

## Notes

- **Physical inventory items** - Currency exists as actual items in player inventory
- **Synchronous operations** - Methods execute immediately on server
- **Automatic change** - `URemoveMoney` handles breaking larger bills
- **Ground overflow** - Items drop to ground if inventory is full
- **Ruined items** - Configurable whether ruined currency is accepted
- **Multiple currencies** - Register different keys for different currency types

## Tags
`economy`, `currency`, `inventory`, `transactions`, `shops`, `player-balance`, `reference`, `doc-usage`, `modder`
