# Universal Framework - API Utilities

## Overview

The API endpoint (`UApiEndpoint`) via `U().Api()` provides external service integrations including server queries, cryptocurrency prices, random numbers, and text-to-speech.

## Accessing the API Endpoint

```enforce
UApiEndpoint api = U().Api();
```

## Permissions

All external API endpoints allow both server and player access:

| Operation | Server | Player (Client) |
|-----------|--------|----------------|
| SteamQuery | ✅ | ✅ |
| CryptoPrice / CryptoConvert / Crypto | ✅ | ✅ |
| RandomNumbers | ✅ | ✅ |
| TTS Generate / Status / Download | ✅ | ✅ |

> **Note:** These endpoints are rate-limited to prevent abuse.

---

## Steam Server Query

Query DayZ/Steam server status information.

### SteamQuery

```enforce
// Query server status - returns UFServerStatus object
U().Api().SteamQuery(ip, queryPort, this, "OnServerStatus");

void OnServerStatus(int cid, int status, string oid, UFServerStatus server) {
    if (status == UF_SUCCESS && server) {
        Print("Server: " + server.Name);
        Print("Players: " + server.Players + "/" + server.MaxPlayers);
        Print("Map: " + server.GameMap);
        Print("Version: " + server.ServerVersion);
    }
}
```

### UFServerStatus Response Object

```enforce
class UFServerStatus extends StatusObject {
    string IP;
    int GamePort;
    int QueryPort;
    string Name;              // Server name
    string ServerVersion;     // DayZ version
    int Players;              // Current players
    int QueuePlayers;         // Players in queue
    int MaxPlayers;           // Max player slots
    string GameTime;          // In-game time
    string GameMap;           // Map name (e.g., "chernarusplus")
    bool Password;            // Password protected
    bool FirstPerson;         // First person only
}
```

### Usage Example: Server Status Display

```enforce
class ServerMonitor {
    protected string m_IP = "192.168.1.100";
    protected string m_QueryPort = "27016";
    
    void CheckStatus() {
        U().Api().SteamQuery(m_IP, m_QueryPort, this, "OnStatus");
    }
    
    void OnStatus(int cid, int status, string oid, UFServerStatus server) {
        if (status == UF_SUCCESS && server) {
            string msg = server.Name + " - " + server.Players + "/" + server.MaxPlayers;
            if (server.QueuePlayers > 0) {
                msg += " (+" + server.QueuePlayers + " in queue)";
            }
            Print(msg);
        } else {
            Print("Server offline or unreachable");
        }
    }
}
```

---

## Cryptocurrency Prices

Get live cryptocurrency market prices and conversions.

### CryptoPrice

Get the current price of one crypto in terms of another:

```enforce
// Get BTC price in USD
U().Api().CryptoPrice("BTC", "USD", this, "OnPrice");

void OnPrice(int cid, int status, string oid, UCryptoConvertResult result) {
    if (status == UF_SUCCESS && result) {
        Print("1 BTC = $" + result.Value + " USD");
    }
}
```

### CryptoConvert

Convert a specific amount between cryptocurrencies:

```enforce
// Convert 0.5 ETH to USD
U().Api().CryptoConvert("ETH", "USD", 0.5, this, "OnConvert");

void OnConvert(int cid, int status, string oid, UCryptoConvertResult result) {
    if (status == UF_SUCCESS && result) {
        Print("0.5 ETH = $" + result.Value + " USD");
    }
}
```

### Crypto (Multiple Prices)

Get multiple crypto prices at once:

```enforce
// Get BTC, ETH, LTC prices in USD
TStringArray coins = {"BTC", "ETH", "LTC"};
U().Api().Crypto(coins, "USD", this, "OnCryptoPrices");

void OnCryptoPrices(int cid, int status, string oid, UCryptoResults result) {
    if (status == UF_SUCCESS && result) {
        map<string, float> prices = result.Get();
        foreach (string coin, float price : prices) {
            Print(coin + " = $" + price);
        }
    }
}
```

### Response Objects

```enforce
class UCryptoConvertResult extends StatusObject {
    float Value;
    
    float Get();  // Returns the converted value
}

class UCryptoResults extends StatusObject {
    map<string, float> Get();  // Returns map of coin -> price
}
```

### Usage Example: In-Game Economy

```enforce
class CryptoEconomy {
    protected float m_BTCPrice;
    
    void UpdatePrices() {
        U().Api().CryptoPrice("BTC", "USD", this, "OnBTCPrice");
    }
    
    void OnBTCPrice(int cid, int status, string oid, UCryptoConvertResult result) {
        if (status == UF_SUCCESS && result) {
            m_BTCPrice = result.Value;
        }
    }
    
    float ConvertBTCToGameCurrency(float btc) {
        return btc * m_BTCPrice * 100;  // 1 USD = 100 game currency
    }
}
```

---

## Random Numbers

Get cryptographically random numbers from the service.

```enforce
// Get 100 random numbers (max 4096)
U().Api().RandomNumbers(100, this, "OnRandom");

void OnRandom(int cid, int status, string oid, URandomNumberResponse result) {
    if (status == UF_SUCCESS && result) {
        array<int> numbers = result.Numbers;
        Print("Got " + numbers.Count() + " random numbers");
    }
}
```

**Note:** The framework automatically maintains a pool of random numbers via `Math.QRandomInt()`. You typically don't need to call this directly.

---

## Text-to-Speech (TTS)

See **AIVoice.md** for TTS documentation. TTS methods are also available via `U().Api()`:

```enforce
U().Api().TTSGenerate(voiceId, message, this, "OnGenerated");
U().Api().TTSStatus(ttsId, this, "OnStatus");
U().Api().TTSDownload(ttsId, this, "OnDownloaded");
U().Api().TTSPlay(ttsId);  // Download and play automatically
```

---

## API Status

Check if the backend service is responding:

```enforce
U().Api().Status(this, "OnApiStatus");

void OnApiStatus(int cid, int status, string oid, UFStatus result) {
    if (status == UF_SUCCESS && result) {
        Print("API Version: " + result.Version);
    }
}
```
