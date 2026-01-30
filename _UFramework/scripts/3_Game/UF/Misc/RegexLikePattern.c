/**
 * @file RegexLikePattern.c
 * @brief Regex-like pattern matcher optimized for DayZ Enforce Script.
 * 
 * Provides regex-style pattern matching without requiring external libraries.
 * Optimized to use Enforce Script's native string methods where possible.
 * 
 * @usage
 * void TestRegex() {
 *     URegexLikePattern rx = new URegexLikePattern("^Admin_[A-Za-z0-9_]+$");
 *     if (rx.IsValid() && rx.Match("Admin_John42"))
 *         Print("Match!");
 *     
 *     // Case-insensitive matching
 *     URegexLikePattern rx2 = new URegexLikePattern("hello", true);
 *     if (rx2.Match("HELLO WORLD"))
 *         Print("Found hello!");
 * }
 * 
 * @note Supported pattern features:
 *   ^                anchor at start of string
 *   $                anchor at end of string
 *   .                any single character
 *   \x               escape meta-char x (.,*,+,?,[,],^,$,\)
 *   \d               digit [0-9]
 *   \D               non-digit [^0-9]
 *   \w               word char [A-Za-z0-9_]
 *   \W               non-word char [^A-Za-z0-9_]
 *   \s               whitespace [ \t\n\r]
 *   \S               non-whitespace [^ \t\n\r]
 *   [abc]            char class (a or b or c)
 *   [a-z]            range in class
 *   [^a-z0-9]        negated class
 *   X*               zero or more of previous token (greedy)
 *   X+               one or more of previous token (greedy)
 *   X?               zero or one of previous token
 * 
 * @note NOT supported: groups ( ), alternation |, backreferences
 */

enum EUTokenType
{
    CHAR,          // literal
    DOT,           // '.'
    CLASS,         // [...]
    ANCHOR_START,  // ^
    ANCHOR_END,    // $
}

/**
 * @class URegexLikePattern
 * @brief Regex-like pattern matcher for DayZ Enforce Script.
 * 
 * Provides regex-style matching with common features:
 * - Anchors (^, $), wildcards (.), character classes ([abc], [a-z], [^0-9])
 * - Escape sequences (\d, \w, \s, \D, \W, \S)
 * - Quantifiers (*, +, ?)
 * 
 * @note Does NOT support: groups (), alternation |, backreferences
 * @note Optimized for literal prefix matching and simple patterns
 * @note Case-insensitive mode available via constructor
 */
class URegexLikePattern
{
    private const int REPEAT_ONCE     = 0;
    private const int REPEAT_OPTIONAL = 1; // '?'
    private const int REPEAT_STAR     = 2; // '*'
    private const int REPEAT_PLUS     = 3; // '+'

    // Parsed tokens
    private autoptr array<int>    m_Types;       // EUTokenType
    private autoptr array<string> m_Literals;    // CHAR literal
    private autoptr array<bool>   m_ClassNeg;    // CLASS negation
    private autoptr array<string> m_ClassSpecs;  // CLASS contents
    private autoptr array<int>    m_Repeat;      // REPEAT_*

    // Flags about pattern
    private bool   m_HasStartAnchor;
    private bool   m_HasEndAnchor;
    private bool   m_IsValid;
    private bool   m_CaseInsensitive;
    private string m_ErrorMessage;
    private string m_OriginalPattern;
    
    // Optimization: literal prefix for fast rejection
    private string m_LiteralPrefix;
    private bool   m_IsSimpleLiteral; // Pattern is just a literal string (no special chars)
    
    // Match result storage
    private int    m_MatchStart;
    private int    m_MatchEnd;

    // Backtracking / recursion safety
    private const int MAX_DEPTH = 512;
    
    // Built-in character class definitions
    private static const string CLASS_DIGIT     = "0123456789";
    private static const string CLASS_WORD      = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_";
    private static const string CLASS_SPACE     = " \t\n\r";

    // Constructor: parses the pattern once
    void URegexLikePattern(string pattern, bool caseInsensitive = false)
    {
        m_Types      = new array<int>();
        m_Literals   = new array<string>();
        m_ClassNeg   = new array<bool>();
        m_ClassSpecs = new array<string>();
        m_Repeat     = new array<int>();

        m_HasStartAnchor  = false;
        m_HasEndAnchor    = false;
        m_CaseInsensitive = caseInsensitive;
        m_OriginalPattern = pattern;
        m_LiteralPrefix   = "";
        m_IsSimpleLiteral = false;
        m_MatchStart      = -1;
        m_MatchEnd        = -1;
        
        m_IsValid = ParsePattern(pattern);
        
        if (m_IsValid)
            OptimizeLiteralPrefix();
    }

    bool IsValid()
    {
        return m_IsValid;
    }

    string GetError()
    {
        return m_ErrorMessage;
    }
    
    string GetPattern()
    {
        return m_OriginalPattern;
    }
    
    /**
     * Returns the start position of the last successful match.
     * Returns -1 if no match or Match() not called yet.
     */
    int GetMatchStart()
    {
        return m_MatchStart;
    }
    
    /**
     * Returns the end position (exclusive) of the last successful match.
     * Returns -1 if no match or Match() not called yet.
     */
    int GetMatchEnd()
    {
        return m_MatchEnd;
    }
    
    /**
     * Returns the matched substring from the last successful match.
     * Returns empty string if no match.
     */
    string GetMatchedString(string text)
    {
        if (m_MatchStart < 0 || m_MatchEnd < 0 || m_MatchStart >= m_MatchEnd)
            return "";
        return text.Substring(m_MatchStart, m_MatchEnd - m_MatchStart);
    }

    /**
     * Main match function.
     * If pattern starts with ^, anchors at start.
     * Otherwise searches for the pattern anywhere in the text.
     */
    bool Match(string text)
    {
        m_MatchStart = -1;
        m_MatchEnd   = -1;
        
        if (!m_IsValid)
            return false;
        
        string searchText = text;
        if (m_CaseInsensitive)
        {
            searchText = text;
            searchText.ToLower();
        }

        int tLen = searchText.Length();
        
        // Fast path: simple literal pattern - use native Contains/IndexOf
        if (m_IsSimpleLiteral && m_LiteralPrefix.Length() > 0)
        {
            int pos = searchText.IndexOf(m_LiteralPrefix);
            if (pos == -1)
                return false;
            
            // Check anchors
            if (m_HasStartAnchor && pos != 0)
                return false;
            if (m_HasEndAnchor && pos + m_LiteralPrefix.Length() != tLen)
                return false;
                
            m_MatchStart = pos;
            m_MatchEnd   = pos + m_LiteralPrefix.Length();
            return true;
        }
        
        // Fast rejection: if we have a literal prefix, check it exists
        if (m_LiteralPrefix.Length() > 0 && !m_HasStartAnchor)
        {
            if (searchText.IndexOf(m_LiteralPrefix) == -1)
                return false;
        }

        // If the first token is ANCHOR_START, we only try from position 0
        int matchLen = 0;
        if (m_Types.Count() > 0 && m_Types[0] == EUTokenType.ANCHOR_START)
        {
            if (MatchHere(0, searchText, 0, 0, matchLen))
            {
                m_MatchStart = 0;
                m_MatchEnd   = matchLen;
                return true;
            }
            return false;
        }

        // Otherwise try at any position (search)
        for (int start = 0; start <= tLen; start++)
        {
            matchLen = 0;
            if (MatchHere(0, searchText, start, 0, matchLen))
            {
                m_MatchStart = start;
                m_MatchEnd   = start + matchLen;
                return true;
            }
        }
        return false;
    }
    
    /**
     * Test if the pattern matches the entire string (implicit ^...$).
     */
    bool MatchFull(string text)
    {
        m_MatchStart = -1;
        m_MatchEnd   = -1;
        
        if (!m_IsValid)
            return false;
            
        string searchText = text;
        if (m_CaseInsensitive)
        {
            searchText = text;
            searchText.ToLower();
        }
        
        int matchLen = 0;
        if (MatchHere(0, searchText, 0, 0, matchLen))
        {
            if (matchLen == searchText.Length())
            {
                m_MatchStart = 0;
                m_MatchEnd   = matchLen;
                return true;
            }
        }
        return false;
    }
    
    /**
     * Count all non-overlapping matches in the text.
     */
    int CountMatches(string text)
    {
        if (!m_IsValid)
            return 0;
            
        string searchText = text;
        if (m_CaseInsensitive)
        {
            searchText = text;
            searchText.ToLower();
        }
        
        int count = 0;
        int tLen  = searchText.Length();
        int start = 0;
        int matchLen = 0;
        
        while (start <= tLen)
        {
            matchLen = 0;
            bool found = false;
            
            for (int pos = start; pos <= tLen; pos++)
            {
                if (MatchHere(0, searchText, pos, 0, matchLen))
                {
                    count++;
                    // Move past this match (at least 1 char to avoid infinite loop)
                    start = pos + matchLen;
                    if (matchLen == 0)
                        start = pos + 1;
                    found = true;
                    break;
                }
            }
            
            if (!found)
                break;
        }
        
        return count;
    }

    // ----------------- PARSING -----------------

    private void SetError(string msg)
    {
        m_ErrorMessage = msg;
        m_IsValid = false;
    }
    
    /**
     * Expands shorthand character classes (\d, \w, \s, etc.)
     */
    private string ExpandShorthandClass(string shorthand)
    {
        switch (shorthand)
        {
            case "d": return CLASS_DIGIT;
            case "D": return "^" + CLASS_DIGIT;  // negated
            case "w": return CLASS_WORD;
            case "W": return "^" + CLASS_WORD;   // negated
            case "s": return CLASS_SPACE;
            case "S": return "^" + CLASS_SPACE;  // negated
        }
        return "";
    }

    private bool ParsePattern(string pattern)
    {
        int i;
        int len = pattern.Length();
        bool lastWasToken = false;
        
        string workPattern = pattern;
        if (m_CaseInsensitive)
        {
            workPattern = pattern;
            workPattern.ToLower();
        }

        for (i = 0; i < len; i++)
        {
            string c = workPattern[i];

            // Anchors
            if (c == "^")
            {
                if (m_Types.Count() > 0)
                {
                    // '^' not at start: treat literal
                    PushToken(EUTokenType.CHAR, "^", "", false, REPEAT_ONCE);
                    lastWasToken = true;
                }
                else
                {
                    PushToken(EUTokenType.ANCHOR_START, "", "", false, REPEAT_ONCE);
                    m_HasStartAnchor = true;
                    lastWasToken = false;
                }
                continue;
            }

            if (c == "$")
            {
                if (i != len - 1)
                {
                    // '$' not at end: treat literal
                    PushToken(EUTokenType.CHAR, "$", "", false, REPEAT_ONCE);
                    lastWasToken = true;
                }
                else
                {
                    PushToken(EUTokenType.ANCHOR_END, "", "", false, REPEAT_ONCE);
                    m_HasEndAnchor = true;
                    lastWasToken = false;
                }
                continue;
            }

            // Character class [ ... ]
            if (c == "[")
            {
                int classStart = i + 1;
                int j = classStart;
                bool neg = false;

                if (j < len && workPattern[j] == "^")
                {
                    neg = true;
                    j++;
                    classStart = j; // Move past ^ so it's not included in spec
                }

                bool closed = false;
                for (; j < len; j++)
                {
                    if (workPattern[j] == "]")
                    {
                        closed = true;
                        break;
                    }
                }
                if (!closed)
                {
                    SetError("Unclosed '[' in pattern");
                    return false;
                }

                string inside = workPattern.Substring(classStart, j - classStart);
                if (inside.Length() == 0)
                {
                    // Empty [] or [^] – treat as literal
                    PushToken(EUTokenType.CHAR, "[", "", false, REPEAT_ONCE);
                    if (neg)
                        PushToken(EUTokenType.CHAR, "^", "", false, REPEAT_ONCE);
                    PushToken(EUTokenType.CHAR, "]", "", false, REPEAT_ONCE);
                    lastWasToken = true;
                    i = j;
                    continue;
                }

                PushToken(EUTokenType.CLASS, "", inside, neg, REPEAT_ONCE);
                lastWasToken = true;
                i = j;
                continue;
            }

            // Dot wildcard
            if (c == ".")
            {
                PushToken(EUTokenType.DOT, "", "", false, REPEAT_ONCE);
                lastWasToken = true;
                continue;
            }

            // Quantifiers '*', '+', and '?'
            if (c == "*" || c == "?" || c == "+")
            {
                if (!lastWasToken || m_Types.Count() == 0)
                {
                    // Leading or invalid quantifier: treat literally
                    PushToken(EUTokenType.CHAR, c, "", false, REPEAT_ONCE);
                    lastWasToken = true;
                    continue;
                }

                int idx = m_Types.Count() - 1;
                int tt  = m_Types[idx];
                if (tt == EUTokenType.ANCHOR_START || tt == EUTokenType.ANCHOR_END)
                {
                    // Can't quantify an anchor: treat quantifier literally
                    PushToken(EUTokenType.CHAR, c, "", false, REPEAT_ONCE);
                    lastWasToken = true;
                    continue;
                }

                if (c == "*")
                    m_Repeat.Set(idx, REPEAT_STAR);
                else if (c == "+")
                    m_Repeat.Set(idx, REPEAT_PLUS);
                else
                    m_Repeat.Set(idx, REPEAT_OPTIONAL);

                lastWasToken = false;
                continue;
            }

            // Escapes: \x => literal x or shorthand class
            if (c == "\\" && i + 1 < len)
            {
                // Use ORIGINAL pattern for escape char to preserve case of shorthand classes
                string next = pattern[i + 1];
                
                // Check for shorthand character classes (case-sensitive: d/D, w/W, s/S)
                string expanded = ExpandShorthandClass(next);
                if (expanded.Length() > 0)
                {
                    // Check if negated (uppercase shorthand)
                    bool isNegated = (expanded[0] == "^");
                    if (isNegated)
                        expanded = expanded.Substring(1, expanded.Length() - 1);
                    
                    PushToken(EUTokenType.CLASS, "", expanded, isNegated, REPEAT_ONCE);
                    lastWasToken = true;
                    i++;
                    continue;
                }
                
                // Regular escape - literal character (use lowercased version if case-insensitive)
                string escapedChar = workPattern[i + 1];
                PushToken(EUTokenType.CHAR, escapedChar, "", false, REPEAT_ONCE);
                lastWasToken = true;
                i++;
                continue;
            }

            // Default: literal char
            PushToken(EUTokenType.CHAR, c, "", false, REPEAT_ONCE);
            lastWasToken = true;
        }

        // Empty pattern is allowed (matches empty string)
        return true;
    }

    private void PushToken(int type, string lit, string classSpec, bool classNeg, int repeat)
    {
        m_Types.Insert(type);
        m_Literals.Insert(lit);
        m_ClassSpecs.Insert(classSpec);
        m_ClassNeg.Insert(classNeg);
        m_Repeat.Insert(repeat);
    }
    
    /**
     * Optimization: extract leading literal chars for fast rejection.
     * If the pattern starts with literal characters, we can use IndexOf()
     * to quickly reject strings that don't contain them.
     */
    private void OptimizeLiteralPrefix()
    {
        m_LiteralPrefix   = "";
        m_IsSimpleLiteral = true;
        
        int startIdx = 0;
        if (m_Types.Count() > 0 && m_Types[0] == EUTokenType.ANCHOR_START)
            startIdx = 1;
        
        for (int i = startIdx; i < m_Types.Count(); i++)
        {
            int tokenType = m_Types[i];
            int repeat    = m_Repeat[i];
            
            // Only simple literals with REPEAT_ONCE count for prefix
            if (tokenType == EUTokenType.CHAR && repeat == REPEAT_ONCE)
            {
                m_LiteralPrefix += m_Literals[i];
            }
            else if (tokenType == EUTokenType.ANCHOR_END)
            {
                // End anchor is ok, still simple literal
                continue;
            }
            else
            {
                // Any other token type breaks the literal prefix
                m_IsSimpleLiteral = false;
                break;
            }
        }
        
        // If we went through all tokens and only found literals, it's a simple pattern
        if (m_IsSimpleLiteral && m_LiteralPrefix.Length() == 0)
            m_IsSimpleLiteral = false;
    }

    // ----------------- MATCHING ENGINE -----------------

    /**
     * Core matching engine.
     * pi = pattern index, ti = text index, depth = recursion depth (safety)
     * matchLen is updated to track total characters matched
     */
    private bool MatchHere(int pi, string text, int ti, int depth, out int matchLen)
    {
        matchLen = 0;
        
        if (depth > MAX_DEPTH)
            return false; // safety against pathological patterns

        int pLen = m_Types.Count();
        int tLen = text.Length();

        // All tokens consumed
        if (pi >= pLen)
        {
            return true;
        }

        int tokenType = m_Types[pi];

        // End anchor
        if (tokenType == EUTokenType.ANCHOR_END)
        {
            return (ti == tLen);
        }
        
        // Start anchor (should only be at position 0, skip it)
        if (tokenType == EUTokenType.ANCHOR_START)
        {
            return MatchHere(pi + 1, text, ti, depth + 1, matchLen);
        }

        int    repeat   = m_Repeat[pi];
        string lit      = m_Literals[pi];
        string spec     = m_ClassSpecs[pi];
        bool   negClass = m_ClassNeg[pi];
        int    restLen  = 0;

        if (repeat == REPEAT_ONCE)
        {
            if (!SingleMatch(tokenType, lit, spec, negClass, text, ti))
                return false;
            restLen = 0;
            if (!MatchHere(pi + 1, text, ti + 1, depth + 1, restLen))
                return false;
            matchLen = 1 + restLen;
            return true;
        }

        if (repeat == REPEAT_OPTIONAL)
        {
            // Try with one match first
            if (SingleMatch(tokenType, lit, spec, negClass, text, ti))
            {
                restLen = 0;
                if (MatchHere(pi + 1, text, ti + 1, depth + 1, restLen))
                {
                    matchLen = 1 + restLen;
                    return true;
                }
            }
            // Or with zero matches
            restLen = 0;
            if (MatchHere(pi + 1, text, ti, depth + 1, restLen))
            {
                matchLen = restLen;
                return true;
            }
            return false;
        }

        // REPEAT_STAR (zero or more) or REPEAT_PLUS (one or more)
        int minMatches = 0;
        if (repeat == REPEAT_PLUS)
            minMatches = 1;
        
        // Greedy: consume as many as possible
        int consumed = 0;
        int maxPos = ti;
        
        while (maxPos < tLen && SingleMatch(tokenType, lit, spec, negClass, text, maxPos))
        {
            consumed++;
            maxPos++;
        }
        
        // Try matching rest of pattern, backing off one at a time (greedy backtracking)
        for (int k = consumed; k >= minMatches; k--)
        {
            restLen = 0;
            if (MatchHere(pi + 1, text, ti + k, depth + 1, restLen))
            {
                matchLen = k + restLen;
                return true;
            }
        }
        return false;
    }

    private bool SingleMatch(int tokenType, string lit, string classSpec, bool neg, string text, int ti)
    {
        int tLen = text.Length();
        if (ti >= tLen)
            return false;

        string ch = text[ti];

        switch (tokenType)
        {
            case EUTokenType.CHAR:
                return (ch == lit);

            case EUTokenType.DOT:
                return true;

            case EUTokenType.CLASS:
                return MatchClass(ch, classSpec, neg);

            case EUTokenType.ANCHOR_START:
            case EUTokenType.ANCHOR_END:
                return false; // anchors not consumed as characters
        }
        return false;
    }

    private bool MatchClass(string ch, string spec, bool neg)
    {
        int len    = spec.Length();
        int chCode = ch.ToAscii();
        bool ok    = false;

        // Fast path: use IndexOf for simple class without ranges
        if (spec.IndexOf("-") == -1)
        {
            ok = (spec.IndexOf(ch) != -1);
            if (neg)
                return !ok;
            return ok;
        }

        // Handle ranges
        for (int i = 0; i < len; i++)
        {
            string c = spec[i];

            // Range a-b (but not at start/end where '-' is literal)
            if (i + 2 < len && spec[i + 1] == "-")
            {
                string c2     = spec[i + 2];
                int startCode = c.ToAscii();
                int endCode   = c2.ToAscii();
                if (chCode >= startCode && chCode <= endCode)
                {
                    ok = true;
                    break;
                }
                i += 2;
                continue;
            }

            // Plain char match
            if (ch == c)
            {
                ok = true;
                break;
            }
        }

        if (neg)
            return !ok;
        return ok;
    }
}

// ----------------- Top-level helpers (with U prefix) -----------------

/**
 * Create a compiled regex pattern object.
 * 
 * @param pattern The regex pattern string.
 * @param caseInsensitive If true, matching ignores case.
 * @return A compiled URegexLikePattern object.
 */
URegexLikePattern UCreateRegex(string pattern, bool caseInsensitive = false)
{
    return new URegexLikePattern(pattern, caseInsensitive);
}

/**
 * Quick test if a pattern matches anywhere in the text.
 * Creates a temporary pattern object - for repeated matches, 
 * create the pattern once and reuse it.
 */
bool URegexMatch(string pattern, string text, bool caseInsensitive = false)
{
    URegexLikePattern rx = new URegexLikePattern(pattern, caseInsensitive);
    if (!rx.IsValid())
        return false;
    return rx.Match(text);
}

/**
 * Quick test if a pattern matches the entire text.
 */
bool URegexMatchFull(string pattern, string text, bool caseInsensitive = false)
{
    URegexLikePattern rx = new URegexLikePattern(pattern, caseInsensitive);
    if (!rx.IsValid())
        return false;
    return rx.MatchFull(text);
}

/**
 * Count occurrences of a pattern in text.
 */
int URegexCount(string pattern, string text, bool caseInsensitive = false)
{
    URegexLikePattern rx = new URegexLikePattern(pattern, caseInsensitive);
    if (!rx.IsValid())
        return 0;
    return rx.CountMatches(text);
}

// Test function demonstrating regex features
void TestRegexUsage()
{
    // Basic pattern with anchors
    URegexLikePattern rx1 = UCreateRegex("^Admin_[A-Za-z0-9_]+$");
    if (!rx1.IsValid())
    {
        Print("Pattern error: " + rx1.GetError());
        return;
    }

    if (rx1.Match("Admin_John42"))
        Print("Admin_John42 is valid admin name");

    // File extension matching
    URegexLikePattern rx2 = UCreateRegex(".*\\.p3d$");
    if (rx2.Match("house_small.p3d"))
        Print("Is a p3d file");

    // Search inside string (no ^)
    URegexLikePattern rx3 = UCreateRegex("Base_[0-9][0-9]");
    if (rx3.Match("This has Base_01 somewhere"))
        Print("Found Base_XX pattern inside string");
    
    // Case-insensitive matching
    URegexLikePattern rx4 = UCreateRegex("hello", true);
    if (rx4.Match("HELLO WORLD"))
        Print("Case-insensitive match works!");
    
    // Using shorthand classes
    URegexLikePattern rx5 = UCreateRegex("^\\d+$");  // All digits
    if (rx5.Match("12345"))
        Print("Is a number");
    
    URegexLikePattern rx6 = UCreateRegex("\\w+@\\w+\\.\\w+");  // Simple email
    if (rx6.Match("test@example.com"))
        Print("Looks like an email: " + rx6.GetMatchedString("test@example.com"));
    
    // Count matches
    int count = URegexCount("\\d+", "I have 3 apples and 42 oranges");
    Print("Found " + count.ToString() + " numbers");
}
