// File: Scripts/4_World/YourMod/URegexLikePattern.c
//
// Robust regex-like pattern matcher for DayZ Enforce Script.
//
// Usage example:
//
//     void TestRegex()
//     {
//         URegexLikePattern rx = new URegexLikePattern("^Admin_[A-Za-z0-9_]*$");
//         if (rx.IsValid() && rx.Match("Admin_John42"))
//             Print("Match!");
//     }
//
// Supported pattern features:
//   ^                anchor at start of string
//   $                anchor at end of string
//   .                any single character
//   \x               escape meta-char x (.,*,?,[,],^,$,\)
//   [abc]            char class (a or b or c)
//   [a-z]            range in class
//   [^a-z0-9]        negated class
//   X*               zero or more of previous token (greedy)
//   X?               zero or one of previous token
//
// No groups ( ), no alternation (|), no +.
// You can emulate '+' as 'XX*' if you really need it.

enum EUTokenType
{
        CHAR,          // literal
        DOT,           // '.'
        CLASS,         // [...]
        ANCHOR_START,  // ^
        ANCHOR_END,    // $
}

class URegexLikePattern
{
    private const int REPEAT_ONCE      = 0;
    private const int REPEAT_OPTIONAL  = 1; // '?'
    private const int REPEAT_STAR      = 2; // '*'


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
    private string m_ErrorMessage;

    // Backtracking / recursion safety
    private const int MAX_DEPTH = 512;

    // Constructor: parses the pattern once
    void URegexLikePattern(string pattern)
    {
        m_Types      = new array<int>;
        m_Literals   = new array<string>;
        m_ClassNeg   = new array<bool>;
        m_ClassSpecs = new array<string>;
        m_Repeat     = new array<int>;

        m_HasStartAnchor = false;
        m_HasEndAnchor   = false;
        m_IsValid        = ParsePattern(pattern);
    }

    bool IsValid()
    {
        return m_IsValid;
    }

    string GetError()
    {
        return m_ErrorMessage;
    }

    // Public match:
    // - If pattern starts with ^, we anchor at start.
    // - Otherwise we search for the pattern anywhere in the text.
    bool Match(string text)
    {
        if (!m_IsValid)
            return false;

        int tLen = text.Length();

        // If the first token is ANCHOR_START, we only try from 0
        if (m_Types.Count() > 0 && m_Types.Get(0) == EUTokenType.ANCHOR_START)
        {
            return MatchHere(0, text, 0, 0);
        }

        // Otherwise try at any position (search)
        int start;
        for (start = 0; start <= tLen; start++)
        {
            if (MatchHere(0, text, start, 0))
                return true;
        }
        return false;
    }

    // ----------------- PARSING -----------------

    private void SetError(string msg)
    {
        m_ErrorMessage = msg;
        m_IsValid = false;
    }

    private bool ParsePattern(string pattern)
    {
        int i;
        int len = pattern.Length();
        bool lastWasToken = false;

        for (i = 0; i < len; i++)
        {
            string c = pattern[i];

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
                int start = i + 1;
                int j     = start;
                bool neg  = false;

                if (j < len && pattern[j] == "^")
                {
                    neg = true;
                    j++;
                }

                bool closed = false;
                for (; j < len; j++)
                {
                    if (pattern[j] == "]")
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

                string inside = pattern.Substring(start, j - start);
                if (inside.Length() == 0)
                {
                    // Empty [] – treat as literal "[]"
                    PushToken(EUTokenType.CHAR, "[", "", false, REPEAT_ONCE);
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

            // Quantifiers '*' and '?'
            if (c == "*" || c == "?")
            {
                if (!lastWasToken || m_Types.Count() == 0)
                {
                    // Leading or invalid quantifier: treat literally
                    PushToken(EUTokenType.CHAR, c, "", false, REPEAT_ONCE);
                    lastWasToken = true;
                    continue;
                }

                int idx = m_Types.Count() - 1;
                int tt  = m_Types.Get(idx);
                if (tt == EUTokenType.ANCHOR_START || tt == EUTokenType.ANCHOR_END)
                {
                    // Can't quantify an anchor: treat quantifier literally
                    PushToken(EUTokenType.CHAR, c, "", false, REPEAT_ONCE);
                    lastWasToken = true;
                    continue;
                }

                if (c == "*")
                    m_Repeat.Set(idx, REPEAT_STAR);
                else
                    m_Repeat.Set(idx, REPEAT_OPTIONAL);

                lastWasToken = false;
                continue;
            }

            // Escapes: \x => literal x
            if (c == "\\" && i + 1 < len)
            {
                string next = pattern[i + 1];
                PushToken(EUTokenType.CHAR, next, "", false, REPEAT_ONCE);
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

    // ----------------- MATCHING ENGINE -----------------

    // pi = pattern index, ti = text index, depth = recursion depth (for safety)
    private bool MatchHere(int pi, string text, int ti, int depth)
    {
        if (depth > MAX_DEPTH)
            return false; // safety against pathological patterns

        int pLen = m_Types.Count();
        int tLen = text.Length();

        // All tokens consumed
        if (pi >= pLen)
        {
            // If last token was '$', it was handled already
            return true;
        }

        int tokenType = m_Types.Get(pi);

        // End anchor
        if (tokenType == EUTokenType.ANCHOR_END)
        {
            return (ti == tLen);
        }

        int    repeat    = m_Repeat.Get(pi);
        string lit       = m_Literals.Get(pi);
        string spec      = m_ClassSpecs.Get(pi);
        bool   negClass  = m_ClassNeg.Get(pi);

        if (repeat == REPEAT_ONCE)
        {
            if (!SingleMatch(tokenType, lit, spec, negClass, text, ti))
                return false;
            return MatchHere(pi + 1, text, ti + 1, depth + 1);
        }

        if (repeat == REPEAT_OPTIONAL)
        {
            // Try with one
            if (SingleMatch(tokenType, lit, spec, negClass, text, ti))
            {
                if (MatchHere(pi + 1, text, ti + 1, depth + 1))
                    return true;
            }
            // Or with zero
            return MatchHere(pi + 1, text, ti, depth + 1);
        }

        // REPEAT_STAR
        int maxConsume = ti;

        while (SingleMatch(tokenType, lit, spec, negClass, text, maxConsume))
        {
            maxConsume++;
            if (maxConsume > tLen)
                break;
        }

        int k;
        for (k = maxConsume; k >= ti; k--)
        {
            if (MatchHere(pi + 1, text, k, depth + 1))
                return true;
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
                return false; // not consumed here
        }
        return false;
    }

    private bool MatchClass(string ch, string spec, bool neg)
    {
        int i;
        int len    = spec.Length();
        int chCode = ch.ToAscii();
        bool ok    = false;

        for (i = 0; i < len; i++)
        {
            string c = spec[i];

            // range a-b
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

            // plain char
            if (ch == c)
            {
                ok = true;
                break;
            }
        }

        if (neg)
        {
            return !ok;
        }
        return ok;
    }
}

// ----------------- Top-level helpers (with U prefix) -----------------

// Simple factory: create a compiled pattern object
URegexLikePattern UCreateRegex(string pattern)
{
    return new URegexLikePattern(pattern);
}

// Example of "function that returns a function"-style helper:
// here we just return the matcher object, which exposes .Match().
URegexLikePattern UFunctionThatReturnsMatcher(string pattern)
{
    return new URegexLikePattern(pattern);
}

void TestRegexUsage()
{
    URegexLikePattern rx1 = UCreateRegex("^Admin_[A-Za-z0-9_]*$");
    if (!rx1.IsValid())
    {
        Print("Pattern error: " + rx1.GetError());
        return;
    }

    if (rx1.Match("Admin_John42"))
        Print("Admin_John42 is valid admin name");

    URegexLikePattern rx2 = UCreateRegex(".*\\.p3d$");
    if (rx2.Match("house_small.p3d"))
        Print("Is a p3d file");

    // Search inside string (no ^)
    URegexLikePattern rx3 = UCreateRegex("Base_[0-9][0-9]");
    if (rx3.Match("This has Base_01 somewhere"))
        Print("Found Base_XX pattern inside string");
}
