# Universal Framework - String Utilities

## Overview

String manipulation utilities including formatting, sanitization, emoji handling, and regex-like pattern matching.

> **Related Documentation:**
> - [Core Utilities](UniversalFramework_Utilities.md) - Player, Time, Notifications, Config
> - [File Utilities](UniversalFramework_Utilities_Files.md) - File operations, JSON handling
> - [Map Utilities](UniversalFramework_Utilities_Map.md) - Map locations, nearest city

## Table of Contents

- [String Formatting](#string-formatting)
- [String Sanitization](#string-sanitization)
- [Regex-Like Pattern Matching](#regex-like-pattern-matching)

---

## String Formatting

```enforce
// Format integer with commas: 1234567 -> "1,234,567"
static string ConvertIntToNiceString(int DollarAmount);
```

### Usage

```enforce
string formatted = UUtil.ConvertIntToNiceString(1000000);  // "1,000,000"
string negative = UUtil.ConvertIntToNiceString(-5000);     // "-5,000"
```

---

## String Sanitization

Functions for cleaning and sanitizing user input, handling emojis, and ensuring string compatibility across DayZ's supported languages.

### Available Functions

| Function | Purpose | Keeps |
|----------|---------|-------|
| `SanitizeString()` | Main sanitization - converts emojis to ASCII art | All DayZ languages by default |
| `StripUnsupportedCharacters()` | **Recommended** for multilingual - removes emojis | Cyrillic, CJK, Hangul, Latin |
| `StripEmojisOnly()` | Removes only known emojis | All other Unicode |
| `RemoveInvalidCharacters()` | Removes non-printable characters | ASCII + Extended Latin |
| `StripNonASCII()` | **Strict English-only** | ASCII 32-126 only |
| `ReplaceEmojisWithASCII()` | Converts emojis to ASCII art | Doesn't remove anything |

### Function Signatures

```enforce
// Main sanitization function - converts emojis to ASCII art
static string SanitizeString(string input, bool removeUnmapped = true, bool keepInternational = true);

// Recommended for multilingual - keeps all DayZ language characters
static string StripUnsupportedCharacters(string input, bool keepWhitespace = true);

// Removes only known emojis, preserves all other Unicode
static string StripEmojisOnly(string input);

// Removes non-printable chars, keeps ASCII + Extended Latin (128-255)
static string RemoveInvalidCharacters(string input);

// Strict ASCII only (32-126) - English only!
static string StripNonASCII(string input, bool keepWhitespace = true);

// Convert emojis to ASCII art equivalents (:), XD, :'(, etc.)
static string ReplaceEmojisWithASCII(string input);
```

### DayZ Supported Languages

These functions correctly handle all DayZ-supported language character sets:

| Language | Script/Characters |
|----------|-------------------|
| English | Basic Latin (ASCII) |
| German | Latin + umlauts (Ã¤ Ã¶ Ã¼ ÃŸ) |
| French | Latin + accents (Ã© Ã¨ Ãª Ã  Ã§ Ã´) |
| Spanish | Latin + (Ã± Ã¡ Ã© Ã­ Ã³ Ãº Ã¼ Â¿ Â¡) |
| Russian | **Cyrillic** (Ð-Ð¯Ð°-Ñ) |
| Polish | Latin + (Ä… Ä‡ Ä™ Å‚ Å„ Ã³ Å› Åº Å¼) |
| Czech | Latin + (Ã¡ Ä Ä Ã© Ä› Ã­ Åˆ Ã³ Å™ Å¡ Å¥ Ãº Å¯ Ã½ Å¾) |
| Italian | Latin + (Ã  Ã¨ Ã© Ã¬ Ã² Ã¹) |
| Portuguese | Latin + (Ã£ Ãµ Ã¡ Ã© Ã§ Ã¢ Ãª Ã´) |
| Chinese (Simplified) | **CJK Characters** |
| Chinese (Traditional) | **CJK Characters** |
| Japanese | **Hiragana, Katakana, Kanji** |
| Korean | **Hangul** |
| Turkish | Latin + (Ã§ ÄŸ Ä± Ã¶ ÅŸ Ã¼ Ä°) |

### Usage Examples

```enforce
// For multilingual servers (keeps Russian, Chinese, Japanese, Korean, etc.)
string clean = UUtil.SanitizeString("Hello ðŸ˜Š ÐŸÑ€Ð¸Ð²ÐµÑ‚ ä½ å¥½ ã“ã‚“ã«ã¡ã¯");
// Returns: "Hello :) ÐŸÑ€Ð¸Ð²ÐµÑ‚ ä½ å¥½ ã“ã‚“ã«ã¡ã¯"

// Recommended for player names/chat - preserves all languages, removes emojis
string safeName = UUtil.StripUnsupportedCharacters("PlayerðŸ˜ˆåå‰Ð˜Ð³Ñ€Ð¾Ðº");
// Returns: "Playeråå‰Ð˜Ð³Ñ€Ð¾Ðº"

// Just remove emojis, keep everything else
string noEmoji = UUtil.StripEmojisOnly("Hello ðŸ˜Š World ðŸŽ‰ ÐŸÑ€Ð¸Ð²ÐµÑ‚");
// Returns: "Hello  World  ÐŸÑ€Ð¸Ð²ÐµÑ‚"

// Convert emojis to ASCII art (without removing unmapped)
string asAscii = UUtil.ReplaceEmojisWithASCII("Hello ðŸ˜Š you're ðŸ˜Ž cool");
// Returns: "Hello :) you're B) cool"

// Strict ASCII-only (English servers only!)
string asciiOnly = UUtil.StripNonASCII("Hello WÃ¶rld ÐŸÑ€Ð¸Ð²ÐµÑ‚ ä½ å¥½");
// Returns: "Hello Wrld "  (WARNING: removes ALL non-English!)

// Extended Latin only (removes Cyrillic/CJK but keeps accented chars)
string latinOnly = UUtil.RemoveInvalidCharacters("CafÃ© ÐŸÑ€Ð¸Ð²ÐµÑ‚ ä½ å¥½");
// Returns: "CafÃ© "
```

### Emoji to ASCII Art Mappings

The `ReplaceEmojisWithASCII()` and `SanitizeString()` functions convert 150+ emojis to ASCII equivalents:

| Emoji | ASCII | Category |
|-------|-------|----------|
| ðŸ˜€ ðŸ˜ƒ ðŸ˜„ | `:D` | Happy |
| ðŸ™‚ ðŸ˜Š â˜ºï¸ | `:)` | Smile |
| ðŸ˜‰ | `;)` | Wink |
| ðŸ˜¢ ðŸ˜¥ | `:'(` | Sad |
| ðŸ˜­ | `T_T` | Crying |
| ðŸ˜‚ ðŸ¤£ ðŸ˜† | `XD` | Laughing |
| ðŸ˜ | `<3_<3` | Love eyes |
| ðŸ˜Ž | `B)` | Cool |
| ðŸ˜› ðŸ˜‹ | `:P` | Tongue |
| ðŸ˜œ | `;P` | Wink tongue |
| ðŸ¤” | `:-?` | Thinking |
| ðŸ˜ ðŸ˜‘ | `:-\|` | Neutral |
| ðŸ˜® ðŸ˜¯ | `:O` | Surprised |
| ðŸ˜± | `D:` | Shocked |
| ðŸ˜¡ ðŸ˜  | `>:(` | Angry |
| â¤ï¸ ðŸ’• ðŸ’– | `<3` | Hearts |
| ðŸ‘ | `+1` | Thumbs up |
| ðŸ‘Ž | `-1` | Thumbs down |
| ðŸ‘‹ [NO]‹ | `o/` | Wave |
| ðŸ‘Œ | `OK` | OK hand |
| [NO]Œï¸ | `V` | Peace |
| ðŸ¤˜ ðŸ¤Ÿ | `\m/` | Rock on |
| ðŸ”¥ | `*fire*` | Fire |
| â­ ðŸŒŸ | `*` | Stars |
| [YES] [NO]”ï¸ | `check` | Checkmark |
| âŒ [NO]— | `X` | X mark |

### Practical Examples

```enforce
// Sanitize chat message for display
void OnChatMessage(string playerName, string message) {
    // Clean message - convert emojis to ASCII art, keep international chars
    string cleanMsg = UUtil.SanitizeString(message);
    
    // Display in chat
    DisplayChatMessage(playerName, cleanMsg);
}

// Validate player name on connect
void OnPlayerConnect(PlayerIdentity identity) {
    string originalName = identity.GetName();
    
    // Remove potentially problematic characters while keeping international names
    string safeName = UUtil.StripUnsupportedCharacters(originalName);
    
    if (safeName != originalName) {
        Print("[Security] Player name sanitized: " + originalName + " -> " + safeName);
    }
}

// Log with ASCII-safe format (for systems that can't handle Unicode)
void LogToExternalSystem(string message) {
    // Convert to pure ASCII for legacy systems
    string asciiMsg = UUtil.StripNonASCII(message);
    ExternalLogger.Send(asciiMsg);
}
```

---

## Regex-Like Pattern Matching

The `URegexLikePattern` class provides regex-like pattern matching for Enforce Script. Since DayZ doesn't have native regex support, this class implements a subset of common regex features with optimizations for performance.

### Supported Features

| Pattern | Description | Example |
|---------|-------------|---------|
| `^` | Anchor at start of string | `^Admin` matches "Admin_John" |
| `$` | Anchor at end of string | `\.p3d$` matches "house.p3d" |
| `.` | Any single character | `a.c` matches "abc", "a1c" |
| `\x` | Escape meta-character | `\.` matches literal "." |
| `\d` | Digit `[0-9]` | `\d+` matches "123" |
| `\D` | Non-digit `[^0-9]` | `\D+` matches "abc" |
| `\w` | Word char `[A-Za-z0-9_]` | `\w+` matches "hello_123" |
| `\W` | Non-word char | `\W` matches "@", " ", etc. |
| `\s` | Whitespace `[ \t\n\r]` | `\s+` matches spaces/tabs |
| `\S` | Non-whitespace | `\S+` matches non-space runs |
| `[abc]` | Character class | `[aeiou]` matches vowels |
| `[a-z]` | Range in class | `[A-Za-z]` matches letters |
| `[^a-z]` | Negated class | `[^0-9]` matches non-digits |
| `X*` | Zero or more (greedy) | `a*` matches "", "a", "aaa" |
| `X+` | One or more (greedy) | `a+` matches "a", "aaa" (not "") |
| `X?` | Zero or one | `colou?r` matches "color", "colour" |

### Case-Insensitive Matching

Pass `true` as the second parameter for case-insensitive matching:

```enforce
URegexLikePattern rx = new URegexLikePattern("hello", true);
rx.Match("HELLO WORLD");  // true
rx.Match("Hello There");  // true
```

### Not Supported

- Groups `( )` - no capturing or grouping
- Alternation `|` - use multiple patterns or character classes instead
- Lazy quantifiers `*?`, `+?`, `??` - only greedy quantifiers
- Lookahead/lookbehind assertions
- Backreferences

### Class API

```enforce
class URegexLikePattern
{
    // Constructor
    void URegexLikePattern(string pattern, bool caseInsensitive = false);
    
    // Validation
    bool IsValid();                          // Check if pattern compiled successfully
    string GetError();                       // Get compilation error message
    string GetPattern();                     // Get original pattern string
    
    // Matching
    bool Match(string text);                 // Search for pattern anywhere in text
    bool MatchFull(string text);             // Match entire string (implicit ^...$)
    int CountMatches(string text);           // Count non-overlapping matches
    
    // Match results (after successful Match())
    int GetMatchStart();                     // Start position of last match
    int GetMatchEnd();                       // End position (exclusive) of last match
    string GetMatchedString(string text);    // Extract the matched substring
}
```

### Factory Functions

```enforce
// Create a compiled regex pattern object
URegexLikePattern UCreateRegex(string pattern, bool caseInsensitive = false);

// Quick one-shot match (creates temporary pattern)
bool URegexMatch(string pattern, string text, bool caseInsensitive = false);

// Quick full-string match
bool URegexMatchFull(string pattern, string text, bool caseInsensitive = false);

// Count occurrences of pattern in text
int URegexCount(string pattern, string text, bool caseInsensitive = false);
```

### Basic Examples

```enforce
// Match admin names (starts with Admin_, alphanumeric)
URegexLikePattern rxAdmin = UCreateRegex("^Admin_[A-Za-z0-9_]+$");
rxAdmin.Match("Admin_John42");      // true
rxAdmin.Match("Player_Bob");        // false
rxAdmin.Match("Admin_");            // false (+ requires at least one char)

// Match file extensions
URegexLikePattern rxP3D = UCreateRegex(".*\\.p3d$");
rxP3D.Match("house_small.p3d");     // true
rxP3D.Match("house.p3d.backup");    // false

// Case-insensitive extension matching
URegexLikePattern rxP3DIgnore = UCreateRegex("\\.p3d$", true);
rxP3DIgnore.Match("model.P3D");     // true
rxP3DIgnore.Match("model.p3d");     // true

// Search inside string (no ^ anchor)
URegexLikePattern rxBase = UCreateRegex("Base_[0-9][0-9]");
rxBase.Match("This has Base_01 somewhere");  // true
rxBase.Match("No base here");                // false

// Using shorthand classes
URegexLikePattern rxDigits = UCreateRegex("^\\d+$");
rxDigits.Match("12345");            // true
rxDigits.Match("123abc");           // false

// Simple email pattern
URegexLikePattern rxEmail = UCreateRegex("\\w+@\\w+\\.\\w+");
rxEmail.Match("test@example.com");  // true

// Get match position and extract
URegexLikePattern rx = UCreateRegex("\\d+");
if (rx.Match("I have 42 apples")) {
    int start = rx.GetMatchStart();           // 7
    int end = rx.GetMatchEnd();               // 9
    string matched = rx.GetMatchedString("I have 42 apples");  // "42"
}

// Count matches
int count = URegexCount("\\d+", "I have 3 apples and 42 oranges");  // 2
```

### Content Filter Examples

Character classes can contain special characters that are normally regex metacharacters:

```enforce
// Special chars are LITERAL inside [ ]
// $ | . are all literal inside character classes!

// Match leetspeak variations
URegexLikePattern rxBadWord = UCreateRegex("b[a@4]d", true);
rxBadWord.Match("bad");   // true
rxBadWord.Match("b@d");   // true
rxBadWord.Match("b4d");   // true

// | is literal inside [ ], not alternation
URegexLikePattern rx = UCreateRegex("[i1!|l]");
rx.Match("i");   // true
rx.Match("1");   // true
rx.Match("|");   // true - matches pipe character!
rx.Match("l");   // true

// $ is literal inside [ ]
URegexLikePattern rxMoney = UCreateRegex("[s5$]");
rxMoney.Match("s");   // true
rxMoney.Match("$");   // true

// Dash at start or end of class is literal
URegexLikePattern rxDash = UCreateRegex("[-a-z]");  // Matches - or a-z
rxDash.Match("-");    // true
rxDash.Match("a");    // true
```

### Practical Examples

```enforce
// Validate player name format
bool IsValidPlayerName(string name) {
    if (name.Length() < 3 || name.Length() > 20)
        return false;
    
    // Must start with letter, contain only alphanumeric and underscore
    URegexLikePattern rx = UCreateRegex("^[A-Za-z]\\w+$");
    return rx.IsValid() && rx.Match(name);
}

// Check for spam patterns (case-insensitive)
bool ContainsSpam(string message) {
    URegexLikePattern rxSpam = UCreateRegex("free.*gift", true);
    return rxSpam.Match(message);
}

// Filter files by extension
array<string> GetJsonFiles(array<string> allFiles) {
    array<string> result = new array<string>();
    URegexLikePattern rx = UCreateRegex("\\.json$", true);
    
    foreach (string file : allFiles) {
        if (rx.Match(file))
            result.Insert(file);
    }
    return result;
}

// Parse SteamID format
bool IsValidSteamID64(string id) {
    // SteamID64 is 17 digits starting with 7656
    URegexLikePattern rx = UCreateRegex("^7656\\d{13}$");
    return rx.Match(id);
}

// Match IP addresses
bool IsIPv4Address(string text) {
    URegexLikePattern rx = UCreateRegex("^\\d+\\.\\d+\\.\\d+\\.\\d+$");
    return rx.Match(text);
}

// Extract numbers from text
void PrintAllNumbers(string text) {
    URegexLikePattern rx = UCreateRegex("\\d+");
    Print("Found " + URegexCount("\\d+", text).ToString() + " numbers");
}
```

### Performance Notes

- **Compile once, match many**: Create `URegexLikePattern` once and reuse for multiple matches
- **Literal prefix optimization**: Patterns starting with literal characters use `IndexOf()` for fast rejection
- **Simple literal fast path**: Patterns with only literal characters use native string search
- **Use native methods for simple cases**: For startswith/endswith/contains, use native string methods
- **MAX_DEPTH = 512**: Recursion limit prevents stack overflow on pathological patterns
- **Character class optimization**: Classes without ranges use `IndexOf()` for O(n) matching

### Gotchas & Limitations

| Issue | Description |
|-------|-------------|
| Empty pattern | Matches everything (returns true immediately) |
| Pathological patterns | `.*.*.*a` can cause exponential backtracking |
| Unicode | Non-ASCII may not work correctly with character ranges |
| No lazy quantifiers | Only greedy `*`, `+`, `?` available |

## Tags
`utilities`, `strings`, `formatting`, `sanitization`, `validation`, `regex`, `reference`, `doc-usage`, `modder`
