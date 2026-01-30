/**
 * @class UUtilBase
 * @brief Base utility class providing emoji handling and date/time helpers.
 * 
 * Provides foundational utilities for UUtil including:
 * - Emoji-to-ASCII mapping and replacement (200+ emojis supported)
 * - Date/time formatting and padding helpers
 * - Leap year calculations
 * - Protected helper methods for string formatting
 * 
 * @note This is the base class for UUtil - most methods are protected/static.
 * @note Emoji maps use lazy initialization for performance.
 */
class UUtilBase extends Managed
{
	// ============================================
	// DATE/TIME CONSTANTS AND HELPERS
	// ============================================
	
	/** Unix epoch start year */
	protected static int UnixStartYear = 1970;
	
	/** Days in each month (non-leap year) */
	protected static int DaysInMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	
	/**
	 * IsLeapYear
	 * ----------
	 * Determines if a given year is a leap year.
	 *
	 * @param year The year to check.
	 * @return bool True if the year is a leap year, false otherwise.
	 */
	protected static bool IsLeapYear(int year)
	{
		if (year % 4 == 0)
		{
			if (year % 100 == 0)
			{
				if (year % 400 == 0)
				{
					return true;
				}
				return false;
			}
			return true;
		}
		return false;
	}
	
	/**
	 * GetDaysInMonth
	 * --------------
	 * Returns the number of days in a specific month, accounting for leap years.
	 *
	 * @param month The month (1-12).
	 * @param year The year (for leap year calculation).
	 * @return int The number of days in that month.
	 */
	protected static int GetDaysInMonth(int month, int year)
	{
		if (month < 1 || month > 12)
			return 0;
		
		int days = DaysInMonth[month - 1];
		
		// February in a leap year
		if (month == 2 && IsLeapYear(year))
			return 29;
		
		return days;
	}
	
	/**
	 * GetDaysInYear
	 * -------------
	 * Returns the number of days in a specific year.
	 *
	 * @param year The year to check.
	 * @return int 366 for leap years, 365 otherwise.
	 */
	protected static int GetDaysInYear(int year)
	{
		if (IsLeapYear(year))
			return 366;
		return 365;
	}
	
	// ============================================
	// STRING FORMATTING HELPERS
	// ============================================
	
	/**
	 * PadZero
	 * -------
	 * Pads a single-digit number with a leading zero.
	 *
	 * @param value The integer value to pad.
	 * @return string The value as a string, padded with leading zero if needed.
	 */
	protected static string PadZero(int value)
	{
		if (value < 10 && value >= 0)
			return "0" + value.ToString();
		return value.ToString();
	}
	
	/**
	 * PadZeroN
	 * --------
	 * Pads a number to a specific width with leading zeros.
	 *
	 * @param value The integer value to pad.
	 * @param width The minimum width of the output string.
	 * @return string The value as a string, padded to the specified width.
	 */
	protected static string PadZeroN(int value, int width)
	{
		string result = value.ToString();
		while (result.Length() < width)
		{
			result = "0" + result;
		}
		return result;
	}
	
	// ============================================
	// EMOJI HANDLING
	// ============================================
	// Static maps for emoji handling - initialized on first use
	protected static ref map<string, string> s_EmojiToASCII;
	protected static ref array<string> s_KnownEmojis;
	protected static bool s_MapsInitialized = false;
	
	/**
	 * EnsureEmojiMapsInitialized
	 * --------------------------
	 * Initializes the emoji lookup maps if not already done.
	 * Uses lazy initialization to avoid startup overhead.
	 */
	protected static void EnsureEmojiMapsInitialized()
	{
		if (s_MapsInitialized)
			return;
		
		s_EmojiToASCII = new map<string, string>();
		s_KnownEmojis = new array<string>();
		
		// Build the emoji mappings
		InitializeEmojiMappings();
		
		s_MapsInitialized = true;
	}
	
	/**
	 * InitializeEmojiMappings
	 * -----------------------
	 * Populates the emoji to ASCII mapping table.
	 * Called once during lazy initialization.
	 */
	protected static void InitializeEmojiMappings()
	{
		// Smileys & Emotion - Happy/Positive
		AddEmojiMapping("😀", ":D");
		AddEmojiMapping("😃", ":D");
		AddEmojiMapping("😄", ":D");
		AddEmojiMapping("😁", ":D");
		AddEmojiMapping("😆", "XD");
		AddEmojiMapping("😅", ":')");
		AddEmojiMapping("🤣", "XD");
		AddEmojiMapping("😂", "XD");
		AddEmojiMapping("🙂", ":)");
		AddEmojiMapping("🙃", "(:");
		AddEmojiMapping("😉", ";)");
		AddEmojiMapping("😊", ":)");
		AddEmojiMapping("😇", "O:)");
		AddEmojiMapping("🥰", ":)");
		AddEmojiMapping("😍", "<3_<3");
		AddEmojiMapping("🤩", "*_*");
		AddEmojiMapping("😘", ":*");
		AddEmojiMapping("😗", ":*");
		AddEmojiMapping("☺", ":)");
		AddEmojiMapping("☺️", ":)");
		AddEmojiMapping("😚", ":*");
		AddEmojiMapping("😙", ":*");
		AddEmojiMapping("🥲", ":'-)");
		
		// Smileys - Tongue/Playful
		AddEmojiMapping("😋", ":P");
		AddEmojiMapping("😛", ":P");
		AddEmojiMapping("😜", ";P");
		AddEmojiMapping("🤪", ";P");
		AddEmojiMapping("😝", "XP");
		AddEmojiMapping("🤑", "$_$");
		
		// Smileys - Neutral/Skeptical
		AddEmojiMapping("🤗", "(^_^)");
		AddEmojiMapping("🤭", ":x");
		AddEmojiMapping("🤫", "shh");
		AddEmojiMapping("🤔", ":/");
		AddEmojiMapping("🤐", ":x");
		AddEmojiMapping("🤨", "o_O");
		AddEmojiMapping("😐", "-_-");
		AddEmojiMapping("😑", "-_-");
		AddEmojiMapping("😶", "...");
		AddEmojiMapping("😏", ";)");
		AddEmojiMapping("😒", "-_-");
		AddEmojiMapping("🙄", "-_-");
		AddEmojiMapping("😬", ":S");
		AddEmojiMapping("🤥", ":>");
		
		// Smileys - Sleepy/Unwell
		AddEmojiMapping("😌", "-_-");
		AddEmojiMapping("😔", ":(");
		AddEmojiMapping("😪", ":'(");
		AddEmojiMapping("🤤", ":P~");
		AddEmojiMapping("😴", "zzZ");
		AddEmojiMapping("😷", ":X");
		AddEmojiMapping("🤒", ":X");
		AddEmojiMapping("🤕", ":X");
		AddEmojiMapping("🤢", ":X");
		AddEmojiMapping("🤮", ":X");
		AddEmojiMapping("🤧", ":X");
		AddEmojiMapping("🥵", ">_<");
		AddEmojiMapping("🥶", "._.");
		AddEmojiMapping("🥴", "@_@");
		AddEmojiMapping("😵", "x_x");
		AddEmojiMapping("🤯", "O_O");
		
		// Smileys - Additional faces
		AddEmojiMapping("🤠", "cowboy");
		AddEmojiMapping("😎", "B)");
		AddEmojiMapping("🤓", "nerd");
		AddEmojiMapping("🧐", "hmm");
		AddEmojiMapping("😕", ":/");
		AddEmojiMapping("😟", ":(");
		AddEmojiMapping("🙁", ":(");
		AddEmojiMapping("😮", ":O");
		AddEmojiMapping("😯", ":O");
		AddEmojiMapping("😲", ":O");
		
		// Smileys - Surprised/Shocked
		AddEmojiMapping("😳", "O_O");
		AddEmojiMapping("🥺", ";_;");
		AddEmojiMapping("😦", ":(");
		AddEmojiMapping("😧", "D:");
		AddEmojiMapping("😨", "D:");
		AddEmojiMapping("😰", "D:");
		AddEmojiMapping("😥", ":'(");
		AddEmojiMapping("😢", ":'(");
		AddEmojiMapping("😭", "T_T");
		AddEmojiMapping("😱", "D:");
		AddEmojiMapping("😖", ">_<");
		AddEmojiMapping("😣", ">_<");
		AddEmojiMapping("😞", ":(");
		AddEmojiMapping("😓", "-_-'");
		AddEmojiMapping("😩", ":(");
		AddEmojiMapping("😫", ">_<");
		AddEmojiMapping("🥱", "-o-");
		
		// Smileys - Negative/Angry
		AddEmojiMapping("😤", ">:(");
		AddEmojiMapping("😡", ">:(");
		AddEmojiMapping("😠", ">:(");
		AddEmojiMapping("🤬", "@#$%!");
		AddEmojiMapping("😈", ">:)");
		AddEmojiMapping("👿", ">:)");
		AddEmojiMapping("💀", "x_x");
		AddEmojiMapping("☠", "x_x");
		AddEmojiMapping("☠️", "x_x");
		AddEmojiMapping("💩", "poo");
		
		// Smileys - Misc faces
		AddEmojiMapping("🤡", ":o)");
		AddEmojiMapping("👹", ">:O");
		AddEmojiMapping("👺", ">:O");
		AddEmojiMapping("👻", ":O");
		AddEmojiMapping("👽", "@_@");
		AddEmojiMapping("👾", "[o_o]");
		AddEmojiMapping("🤖", "[o_o]");
		
		// Gestures - Hands
		AddEmojiMapping("👋", "o/");
		AddEmojiMapping("🤚", "o/");
		AddEmojiMapping("🖐", "o/");
		AddEmojiMapping("🖐️", "o/");
		AddEmojiMapping("✋", "o/");
		AddEmojiMapping("🖖", "\\\\//");
		AddEmojiMapping("👌", "OK");
		AddEmojiMapping("🤌", "*chef*");
		AddEmojiMapping("🤏", "...");
		AddEmojiMapping("✌", "V");
		AddEmojiMapping("✌️", "V");
		AddEmojiMapping("🤞", "X");
		AddEmojiMapping("🤟", "\\m/");
		AddEmojiMapping("🤘", "\\m/");
		AddEmojiMapping("🤙", "call");
		AddEmojiMapping("👈", "<-");
		AddEmojiMapping("👉", "->");
		AddEmojiMapping("👆", "^");
		AddEmojiMapping("🖕", "!@#$");
		AddEmojiMapping("👇", "v");
		AddEmojiMapping("☝", "^");
		AddEmojiMapping("☝️", "^");
		AddEmojiMapping("👍", "+1");
		AddEmojiMapping("👎", "-1");
		AddEmojiMapping("✊", "fist");
		AddEmojiMapping("👊", "fist");
		AddEmojiMapping("🤛", "fist");
		AddEmojiMapping("🤜", "fist");
		AddEmojiMapping("👏", "*clap*");
		AddEmojiMapping("🙌", "\\o/");
		AddEmojiMapping("👐", "\\o/");
		AddEmojiMapping("🤲", "\\o/");
		AddEmojiMapping("🤝", "handshake");
		AddEmojiMapping("🙏", "pray");
		AddEmojiMapping("✍", "write");
		AddEmojiMapping("✍️", "write");
		AddEmojiMapping("💪", "flex");
		AddEmojiMapping("🦾", "flex");
		
		// Hearts & Love
		AddEmojiMapping("❤", "<3");
		AddEmojiMapping("❤️", "<3");
		AddEmojiMapping("🧡", "<3");
		AddEmojiMapping("💛", "<3");
		AddEmojiMapping("💚", "<3");
		AddEmojiMapping("💙", "<3");
		AddEmojiMapping("💜", "<3");
		AddEmojiMapping("🖤", "<3");
		AddEmojiMapping("🤍", "<3");
		AddEmojiMapping("🤎", "<3");
		AddEmojiMapping("💔", "</3");
		AddEmojiMapping("❣", "<3");
		AddEmojiMapping("❣️", "<3");
		AddEmojiMapping("💕", "<3<3");
		AddEmojiMapping("💞", "<3<3");
		AddEmojiMapping("💓", "<3");
		AddEmojiMapping("💗", "<3");
		AddEmojiMapping("💖", "<3*");
		AddEmojiMapping("💘", "<3--");
		AddEmojiMapping("💝", "<3");
		AddEmojiMapping("💟", "<3");
		
		// Misc symbols
		AddEmojiMapping("⭐", "*");
		AddEmojiMapping("🌟", "*");
		AddEmojiMapping("✨", "*");
		AddEmojiMapping("💫", "*");
		AddEmojiMapping("🔥", "fire");
		AddEmojiMapping("💯", "100");
		AddEmojiMapping("💢", "X");
		AddEmojiMapping("💥", "boom");
		AddEmojiMapping("💦", "sweat");
		AddEmojiMapping("💨", "poof");
		AddEmojiMapping("🕳", "O");
		AddEmojiMapping("🕳️", "O");
		AddEmojiMapping("💣", "bomb");
		AddEmojiMapping("💬", "...");
		AddEmojiMapping("👁‍🗨", "o_o");
		AddEmojiMapping("👁️‍🗨️", "o_o");
		AddEmojiMapping("🗨", "...");
		AddEmojiMapping("🗨️", "...");
		AddEmojiMapping("🗯", "!!!");
		AddEmojiMapping("🗯️", "!!!");
		AddEmojiMapping("💭", "...");
		AddEmojiMapping("💤", "zzZ");
		
		// Common text symbols
		AddEmojiMapping("→", "->");
		AddEmojiMapping("←", "<-");
		AddEmojiMapping("↑", "^");
		AddEmojiMapping("↓", "v");
		AddEmojiMapping("…", "...");
		AddEmojiMapping("—", "-");
		AddEmojiMapping("–", "-");
		AddEmojiMapping("'", "'");
		AddEmojiMapping("'", "'");
		AddEmojiMapping("•", "*");
		AddEmojiMapping("·", "*");
		AddEmojiMapping("©", "(c)");
		AddEmojiMapping("®", "(R)");
		AddEmojiMapping("™", "(TM)");
		AddEmojiMapping("°", "deg");
		AddEmojiMapping("±", "+/-");
		AddEmojiMapping("×", "x");
		AddEmojiMapping("÷", "/");
		AddEmojiMapping("≈", "~");
		AddEmojiMapping("≠", "!=");
		AddEmojiMapping("≤", "<=");
		AddEmojiMapping("≥", ">=");
		AddEmojiMapping("∞", "inf");
		
		// Check/X marks
		AddEmojiMapping("✓", "check");
		AddEmojiMapping("✔", "check");
		AddEmojiMapping("✔️", "check");
		AddEmojiMapping("✗", "X");
		AddEmojiMapping("✘", "X");
		AddEmojiMapping("❌", "X");
		AddEmojiMapping("❎", "X");
		AddEmojiMapping("✅", "check");
		
		// Info/Warning icons
		AddEmojiMapping("ℹ", "(i)");
		AddEmojiMapping("ℹ️", "(i)");
		AddEmojiMapping("⚠", "(!!)");
		AddEmojiMapping("⚠️", "(!!)");
		AddEmojiMapping("🚫", "NO");
		AddEmojiMapping("⛔", "STOP");
		
		// Colored circles
		AddEmojiMapping("🔴", "[O]");
		AddEmojiMapping("🟠", "[O]");
		AddEmojiMapping("🟡", "[O]");
		AddEmojiMapping("🟢", "[O]");
		AddEmojiMapping("🔵", "[O]");
		AddEmojiMapping("🟣", "[O]");
		AddEmojiMapping("⚫", "[O]");
		AddEmojiMapping("⚪", "[O]");
		AddEmojiMapping("🟤", "[O]");
		
		// Arrows
		AddEmojiMapping("➡️", "->");
		AddEmojiMapping("⬅️", "<-");
		AddEmojiMapping("⬆️", "^");
		AddEmojiMapping("⬇️", "v");
		AddEmojiMapping("↗️", "/^");
		AddEmojiMapping("↘️", "\\v");
		AddEmojiMapping("↙️", "/v");
		AddEmojiMapping("↖️", "\\^");
		AddEmojiMapping("↕️", "^v");
		AddEmojiMapping("↔️", "<->");
		AddEmojiMapping("🔄", "loop");
		AddEmojiMapping("🔃", "loop");
		
		// Celebrations/Objects
		AddEmojiMapping("🎉", "*party*");
		AddEmojiMapping("🎊", "*party*");
		AddEmojiMapping("🎈", "balloon");
		AddEmojiMapping("🎁", "gift");
		AddEmojiMapping("🏆", "trophy");
		AddEmojiMapping("🥇", "1st");
		AddEmojiMapping("🥈", "2nd");
		AddEmojiMapping("🥉", "3rd");
		AddEmojiMapping("📢", "announce");
		AddEmojiMapping("📣", "announce");
		AddEmojiMapping("🔔", "bell");
		AddEmojiMapping("🔕", "mute");
		
		// Questions/Exclamations
		AddEmojiMapping("❓", "?");
		AddEmojiMapping("❔", "?");
		AddEmojiMapping("❗", "!");
		AddEmojiMapping("❕", "!");
	}
	
	/**
	 * AddEmojiMapping
	 * ---------------
	 * Helper to add an emoji mapping and track known emojis.
	 */
	protected static void AddEmojiMapping(string emoji, string ascii)
	{
		s_EmojiToASCII.Set(emoji, ascii);
		s_KnownEmojis.Insert(emoji);
	}
	
	/**
	 * GetEmojiMap
	 * -----------
	 * Returns the emoji to ASCII mapping table.
	 * Initializes the maps if not already done.
	 */
	static map<string, string> GetEmojiMap()
	{
		EnsureEmojiMapsInitialized();
		return s_EmojiToASCII;
	}
	
	/**
	 * GetKnownEmojis
	 * --------------
	 * Returns the list of all known emojis.
	 * Initializes the maps if not already done.
	 */
	static array<string> GetKnownEmojis()
	{
		EnsureEmojiMapsInitialized();
		return s_KnownEmojis;
	}
	
	/**
	 * Replaces all emojis in text with ASCII equivalents.
	 *
	 * @param input String containing emojis.
	 * @return String with emojis replaced (e.g., "😀" -> ":D", "❤️" -> "<3").
	 * 
	 * @note Supports 200+ common emojis.
	 * @note Uses centralized emoji map for maintainability.
	 * 
	 * @usage
	 * string clean = UUtil::ApplyEmojiReplacements("Hello 😀❤️"); // "Hello :D<3"
	 */
	static string ApplyEmojiReplacements(string input)
	{
		if (input == "" || input.Length() == 0)
			return input;
		
		EnsureEmojiMapsInitialized();
		
		string result = input;
		
		// Iterate through all emoji mappings and apply replacements
		// Note: Replace() modifies string in-place and returns count of replacements
		for (int i = 0; i < s_KnownEmojis.Count(); i++)
		{
			string emoji = s_KnownEmojis[i];
			string ascii = s_EmojiToASCII.Get(emoji);
			result.Replace(emoji, ascii);
		}
		
		return result;
	}
	
	/**
	 * Removes all known emojis from text without replacement.
	 *
	 * @param input String to strip emojis from.
	 * @return String with all emojis removed.
	 * 
	 * @note More aggressive than ApplyEmojiReplacements - completely removes emojis.
	 * 
	 * @usage
	 * string clean = UUtil::StripKnownEmojis("Hello 😀❤️"); // "Hello "
	 */
	static string StripKnownEmojis(string input)
	{
		if (input == "" || input.Length() == 0)
			return input;
		
		EnsureEmojiMapsInitialized();
		
		string result = input;
		
		// Remove all known emojis
		// Note: Replace() modifies string in-place and returns count of replacements
		for (int i = 0; i < s_KnownEmojis.Count(); i++)
		{
			result.Replace(s_KnownEmojis[i], "");
		}
		
		return result;
	}
}
