/**
 * Universal Framework Constants
 *
 * Defines all constant values used throughout the Universal Framework including:
 * - Version information
 * - API status codes
 * - Database type identifiers
 * - Message queue types
 * - Update operation types
 * - Discord permissions
 * - TTS voice names and settings
 */

/**
 * Framework Version
 * Current version of the Universal Framework mod
 */
static const string UF_VERSION = "2.0.0";

/**
 * API Response Status Codes
 * These status codes are returned in callbacks to indicate the result of API operations
 */
static const int UF_SUCCESS = 200;          // Request succeeded
static const int UF_EMPTY = 204;            // Response was empty or query returned no results
static const int UF_NOTSETUP = 424;         // Discord account not linked (Discord requests only)
static const int UF_TIMEOUT = 408;          // Request timed out
static const int UF_CLIENTERROR = 400;      // Client-side error (bad request)
static const int UF_SERVERERROR = 500;      // Server-side error
static const int UF_ERROR = 418;            // Generic error
static const int UF_JSONERROR = 406;        // JSON parsing/conversion error
static const int UF_NOTFOUND = 404;         // Resource not found
static const int UF_TOOEARLY = 425;         // Request made too early (e.g., before initialization)
static const int UF_UNAUTHORIZED = 401;     // Authentication failed or expired

// AI Chat Handler Constants
static const int UF_AI_CHAT_MAX_POLL_TIME = 300; // Maximum polling duration in seconds (5 minutes)
static const int UF_AI_CHAT_MAX_RETRIES = 3;     // Maximum number of failed status check retries

// REST read operation timeout in seconds. RestApi is a singleton and this option is only
// applied by whichever code path creates it first, so every creation site must use this value.
static const int UF_REST_READ_TIMEOUT = 30;

// AI Chat Status Codes
static const int UF_AI_PENDING = 202;       // AI processing is in progress
static const int UF_AI_PROCESSING = 102;    // AI request is still being processed

/**
 * Legacy Database Status Codes (deprecated - use UF_ versions)
 */
static const int UF_DBSUCCESS = 200;
static const int UF_DBEMPTY = 204;
static const int UF_DBTIMEOUT = 408;
static const int UF_DBSERVERERROR = 500;


static const int UF_DBUNAUTHORIZED = 401;
static const int UF_DBERROR = 418;
static const int UF_DBTOOEARLY = 425;


/**
 * Database Type Identifiers
 * Used to specify which database collection to access via UF().db(type)
 */
static const int PLAYER_DB = 100;   // Player-specific data (client can only access their own data)
static const int OBJECT_DB = 101;   // Object/global data (accessible by all clients)

/**
 * Message Queue Types
 * Defines the order in which messages are processed from a queue
 */
static const string UF_QUEUE_FIFO = "FIFO";  // First In, First Out
static const string UF_QUEUE_LIFO = "LIFO";  // Last In, First Out

/**
 * UpdateOpts Class
 *
 * Defines the available database update operations for UF().db(OBJECT_DB).Update() calls.
 * These operations modify specific fields within a database document without
 * replacing the entire object.
 *
 * Operations:
 *   - SET: Sets the value of a field
 *   - PULL: Removes a specific value from an array
 *   - PUSH: Adds a value to an array
 *   - UNSET: Removes a field from the document
 *   - MUL: Multiplies a numeric field by the given value
 *   - RENAME: Renames a field
 *   - PULLALL: Empties an entire array
 *
 * Example:
 *   UF().db(OBJECT_DB).Update("MyMod", "player123", "coins", "100", UpdateOpts.SET);
 *   UF().db(OBJECT_DB).Update("MyMod", "player123", "items", "\"sword\"", UpdateOpts.PUSH);
 */
class UpdateOpts {
	static string SET = "set"; // `set` to set the value of an element
	static string PULL = "pull"; // `pull` to pull a value out of an array
	static string PUSH = "push"; // `push` to push a value into an array
	static string UNSET = "unset";// `unset` to remove an element from the database
	static string MUL = "mul"; // `mul` to mulitply an element by the value in the database
	static string RENAME = "rename";// `rename` to rename an element in the database
	static string PULLALL = "pullAll";// `pullAll` to empty an array
}


/**
 * DSPerms Class
 *
 * Defines Discord permission constants used for channel permission overwrites
 * when creating or editing Discord channels via UF().ds().ChannelCreate() and
 * UF().ds().ChannelEdit().
 *
 * Common Permissions:
 *   - VIEW_CHANNEL: Can see the channel
 *   - SEND_MESSAGES: Can send messages in text channels
 *   - CONNECT: Can join voice channels
 *   - SPEAK: Can speak in voice channels
 *   - ADD_REACTIONS: Can add reactions to messages
 *
 * Administrative Permissions (most require bot owner/admin):
 *   - ADMINISTRATOR: Has all permissions
 *   - MANAGE_CHANNELS: Can edit/delete channels
 *   - KICK_MEMBERS, BAN_MEMBERS: Moderation actions
 *   - MANAGE_ROLES: Can assign/remove roles
 *
 * Usage Example:
 *   autoptr UChannelOptions opts = new UChannelOptions();
 *   opts.AddPermission("roleId", DSPerms.VIEW_CHANNEL);
 *   opts.AddPermission("roleId", DSPerms.SEND_MESSAGES);
 */
class DSPerms {
	
	static string ADD_REACTIONS = "ADD_REACTIONS"; // (add new reactions to messages)
	static string VIEW_AUDIT_LOG = "VIEW_AUDIT_LOG";
	static string PRIORITY_SPEAKER = "PRIORITY_SPEAKER";
	static string STREAM = "STREAM";
	static string VIEW_CHANNEL = "VIEW_CHANNEL";
	static string SEND_MESSAGES = "SEND_MESSAGES";
	static string SEND_TTS_MESSAGES = "SEND_TTS_MESSAGES";
	static string MANAGE_MESSAGES = "MANAGE_MESSAGES"; // (delete messages and reactions)
	static string EMBED_LINKS = "EMBED_LINKS"; // (links posted will have a preview embedded)
	static string ATTACH_FILES = "ATTACH_FILES"; 
	static string READ_MESSAGE_HISTORY = "READ_MESSAGE_HISTORY"; // (view messages that were posted prior to opening Discord)
	static string MENTION_EVERYONE = "MENTION_EVERYONE";
	static string USE_EXTERNAL_EMOJIS = "USE_EXTERNAL_EMOJIS"; // (use emojis from different guilds)
	static string CONNECT = "CONNECT"; // (connect to a voice channel)
	static string USE_VAD = "USE_VAD"; // (use voice activity detection)
	static string SPEAK = "SPEAK"; // (speak in a voice channel)
	static string CREATE_INSTANT_INVITE = "CREATE_INSTANT_INVITE"; // (create invitations to the guild)
	
	
	//Since there is no functions to manage the discord these permission are kinda usless but keeping them encase something changes in the future
	static string ADMINISTRATOR = "ADMINISTRATOR";// (implicitly has all permissions, and bypasses all channel overwrites)
	static string KICK_MEMBERS = "KICK_MEMBERS";
	static string BAN_MEMBERS = "BAN_MEMBERS";
	static string MANAGE_CHANNELS = "MANAGE_CHANNELS"; //(edit and reorder channels)
	static string MANAGE_GUILD = "MANAGE_GUILD"; //  (edit the guild information, region, etc.)
	static string VIEW_GUILD_INSIGHTS = "VIEW_GUILD_INSIGHTS";
	static string MUTE_MEMBERS = "MUTE_MEMBERS"; //(mute members across all voice channels)
	static string DEAFEN_MEMBERS = "DEAFEN_MEMBERS"; //(deafen members across all voice channels)
	static string MOVE_MEMBERS = "MOVE_MEMBERS"; //(move members between voice channels)
	static string CHANGE_NICKNAME = "CHANGE_NICKNAME";
	static string MANAGE_NICKNAMES = "MANAGE_NICKNAMES"; //(change other members' nicknames)
	static string MANAGE_ROLES = "MANAGE_ROLES";
	static string MANAGE_WEBHOOKS = "MANAGE_WEBHOOKS";
	static string MANAGE_EMOJIS = "MANAGE_EMOJIS";	
}


/**
 * UTTSVoice Class
 *
 * Defines available voice names for Text-to-Speech (TTS) generation using OpenAI.
 * Each voice has distinct characteristics suitable for different use cases.
 *
 * Available Voices:
 *   - ALLOY: Neutral and balanced
 *   - ASH: Clear and articulate
 *   - BALLAD: Warm and expressive
 *   - CORAL: Friendly and engaging
 *   - ECHO: Deep and resonant
 *   - FABLE: Narrative and storytelling
 *   - ONYX: Deep and authoritative
 *   - NOVA: Bright and energetic
 *   - SAGE: Calm and wise
 *   - SHIMMER: Light and pleasant
 *
 * Usage Example:
 *   UF().api().GenerateTTS("Hello world", UTTSVoice.ALLOY, this, "OnTTSGenerated");
 */
class UTTSVoice {
	static const string ALLOY = "alloy";
	static const string ASH = "ash";
	static const string BALLAD = "ballad";
	static const string CORAL = "coral";
	static const string ECHO = "echo";
	static const string FABLE = "fable";
	static const string ONYX = "onyx";
	static const string NOVA = "nova"; 
	static const string SAGE = "sage"; 
	static const string SHIMMER = "shimmer";
	static const string VERSE = "verse";
}

/**
 * UTTSVisual Class
 *
 * Defines visual effects for TTS audio playback in-game.
 *
 * Options:
 *   - LINE: Display waveform visualization
 *   - NONE: No visual effects
 */
class UTTSVisual {
	static const string LINE = "line";
	static const string NONE = "none";
}


/**
 * UTTSPersonality Class
 *
 * Defines pre-configured personality/accent styles for Text-to-Speech generation.
 * Each personality includes specific delivery instructions for the TTS engine
 * to create immersive character voices.
 *
 * Available Personalities:
 *   - RAGED_SURVIVOR: Frenzied Russian-accented survivor with urgent delivery
 *   - (Add more as defined below...)
 *
 * Usage Example:
 *   UF().api().GenerateTTSWithPersonality("Get out of here!", UTTSPersonality.RAGED_SURVIVOR, UTTSVoice.ONYX, this, "OnTTS");
 */
class UTTSPersonality {
    static const string RAGED_SURVIVOR = "IMPORTANT: STRONG RUSSIAN ACCENT\nVoice: Ragged and explosive, each word a desperate cry.  \nTone: Frenzied and urgent.  \nDelivery: Rapid bursts with heavy static.  \nPhrasing: Abrupt commands.  \nFeatures: Intense static and a collapsing wasteland vibe."; // RAGED_SURVIVOR: Explosive urgency with a harsh Russian edge.

    static const string WOUNDED_VETERAN = "IMPORTANT: STRONG BRITISH RP ACCENT\nVoice: Gravelly and measured, echoing battle scars.  \nTone: Somber and reflective.  \nDelivery: Slow with deliberate pauses.  \nPhrasing: Formal military jargon.  \nFeatures: Distant gunfire and wind."; // WOUNDED_VETERAN: Reflective and measured with refined British clarity.

    static const string PARANOID_LONER = "IMPORTANT: STRONG NEW YORK ACCENT\nVoice: Hushed and jittery, clipped and alert.  \nTone: Suspicious and tense.  \nDelivery: Whispered with abrupt stops.  \nPhrasing: Short, clipped words.  \nFeatures: Subtle static and rustling sounds."; // PARANOID_LONER: Nervous and clipped with a strong New York bite.

    static const string SAVVY_SCAVENGER = "IMPORTANT: STRONG AUSTRALIAN ACCENT\nVoice: Rough and fast, full of streetwise banter.  \nTone: Wry and pragmatic.  \nDelivery: Informal and brisk.  \nPhrasing: Punchy slang.  \nFeatures: Urban decay ambience."; // SAVVY_SCAVENGER: Streetwise and brisk with a bold Australian twang.

    static const string DESPERATE_MEDIC = "IMPORTANT: STRONG MIDWESTERN AMERICAN ACCENT\nVoice: Gentle yet strained, focused on urgency.  \nTone: Empathetic and calm.  \nDelivery: Steady and clear.  \nPhrasing: Direct and instructional.  \nFeatures: Faint beeps and labored breathing."; // DESPERATE_MEDIC: Urgent and caring with a warm Midwestern drawl.

    static const string GRIZZLED_HUNTER = "IMPORTANT: STRONG SCOTTISH BROGUE\nVoice: Deep and rugged, echoing the wild.  \nTone: Deliberate and cautious.  \nDelivery: Slow and rhythmic.  \nPhrasing: Measured and earthy.  \nFeatures: Rustling leaves and distant animal calls."; // GRIZZLED_HUNTER: Rugged and measured with a robust Scottish lilt.

    static const string CYNICAL_OUTLAW = "IMPORTANT: STRONG TEXAS ACCENT\nVoice: Rough and clipped, with a dismissive sneer.  \nTone: Bitter and mocking.  \nDelivery: Casual and abrupt.  \nPhrasing: Snarky, terse remarks.  \nFeatures: Echoes of urban ruin."; // CYNICAL_OUTLAW: Hard-edged and biting with a strong Texan drawl.

    static const string FORMER_WARLORD = "IMPORTANT: STRONG GERMAN ACCENT\nVoice: Commanding and formal, with measured authority.  \nTone: Stern and introspective.  \nDelivery: Deliberate with clear pauses.  \nPhrasing: Formal and precise.  \nFeatures: Subtle battlefield echoes."; // FORMER_WARLORD: Authoritative and formal with a strong German tone.

    static const string JADED_OPPORTUNIST = "IMPORTANT: STRONG CALIFORNIAN ACCENT\nVoice: Cold and clipped, efficient and minimal.  \nTone: Detached and pragmatic.  \nDelivery: Rapid and businesslike.  \nPhrasing: Crisp, utilitarian statements.  \nFeatures: Urban decay ambience."; // JADED_OPPORTUNIST: Efficient and detached with a sleek Californian edge.

    static const string QUIET_OBSERVER = "IMPORTANT: STRONG IRISH ACCENT\nVoice: Soft and introspective, quiet and measured.  \nTone: Melancholic and reflective.  \nDelivery: Slow and deliberate.  \nPhrasing: Minimal, contemplative phrases.  \nFeatures: Gentle wind and echoing silence."; // QUIET_OBSERVER: Soft and reflective with a lyrical Irish lilt.
}


