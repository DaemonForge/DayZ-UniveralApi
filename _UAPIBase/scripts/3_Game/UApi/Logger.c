static const int LOG_ERROR = 0;
static const int LOG_VERBOSE = 1;
static const int LOG_INFO = 2;
static const int LOG_DEBUG = 3;

class UApiLog extends ULoggerBase {
	protected static ref ULoggerBaseInstance m_ULoggerBaseInstance;
	override static void CreateInstance(){
		m_type = "UAPI";
		m_ULoggerBaseInstance = new ULoggerBaseInstance("UAPI");
	}
	override static ULoggerBaseInstance GetInstance(){
		if (!m_ULoggerBaseInstance){CreateInstance();}
		return m_ULoggerBaseInstance;
	}
}

class ULoggerBase extends Managed {
	protected static string m_type = "";
	protected static ref ULoggerBaseInstance m_LoggerBaseInstance;
	
	static void CreateInstance(){
		m_LoggerBaseInstance = new ULoggerBaseInstance(m_type);
	}
	
	static ULoggerBaseInstance GetInstance(){
		if (!m_LoggerBaseInstance){CreateInstance();}
		return m_LoggerBaseInstance;
	}
	
	static void Log(string text, int level = 1) {
		GetInstance().DoLog(text,level);
	}
	
	static void Info(string text){
		GetInstance().DoLog(text, LOG_INFO);
	}
	
	static void Debug(string text){
		GetInstance().DoLog(text, LOG_DEBUG);
	}

	static void Err(string text){
		Error2("[" + m_type + "] Error", text);
		GetInstance().DoLog(text, LOG_ERROR);
	}
	
	static void SetLogLevels(int level, int apiLevel = -99){
		if (apiLevel == -99){
			apiLevel = level;
		}
		GetInstance().SetLogLevel(level);
		GetInstance().SetApiLogLevel(apiLevel);
	}
	
};
class ULoggerBaseInstance extends Managed {
	
	protected int				m_LogLevel	= 3;
	protected int				m_LogToApiLevel = 3;
	protected bool 			m_isInit = false;
	
	protected static string LogDir = "$profile:";
	protected string m_LogType = "";
	protected FileHandle		m_FileHandle;
	
	void ULoggerBaseInstance(string logType, int level = 4) {	
		m_LogLevel = level;	
		m_LogType = logType;
		if ( !GetGame().IsServer() || GetGame().IsClient() ){
			return;	
		}
		m_FileHandle = CreateFile(LogDir + m_LogType + "_" + GetDateStampFile() + ".log");
		if (m_FileHandle != 0){
			m_isInit = true;
		}
	}
	
	void ~ULoggerBaseInstance() {
		if ( m_isInit ) {
			CloseFile(m_FileHandle);
		}
	}
	
	void SetLogLevel(int level){
		m_LogLevel = level;
	}
	
	void SetApiLogLevel(int level){
		m_LogToApiLevel = level;
	}
	
	protected FileHandle CreateFile(string path) {
		if ( !GetGame().IsServer() || GetGame().IsClient() ){
			return null;	
		}
		
		FileHandle fHandle = OpenFile(path, FileMode.WRITE);
		if (fHandle != 0) {
			FPrintln(fHandle, "MapLink Log Started: " + GetDateStamp() + " " + GetTimeStamp() );
			return fHandle;
		}
		Error2("[MapLink] Error", "Unable to create" + path + " file in Profile.");
		return fHandle;
	}
	
	protected static string GetDateStamp() {
		int yr, mth, day;
		GetYearMonthDay(yr, mth, day);
		string sday = day.ToString();
		if (sday.Length() == 1){
			sday = "0" + sday;
		}
		
		string smth = mth.ToString();
		if (smth.Length() == 1){
			smth = "0" + mth.ToString();
		}
		
		return yr.ToString() + "-" + smth + "-" + sday;
	}
	
	protected static string GetDateStampFile() {
		int hr, min, sec;
		GetHourMinuteSecond(hr, min, sec);
		
		string ssec = sec.ToString();
		if (ssec.Length() == 1){
			ssec = "0" + ssec;
		}
		string smin = min.ToString();
		if (smin.Length() == 1){
			smin = "0" + smin;
		}
		string shr = hr.ToString();
		if (shr.Length() == 1) {
			shr = "0" + shr;
		}
		
		return  GetDateStamp() + "_" + shr + "-" + smin + "-" + ssec;
	}
	
	protected static string GetTimeStamp() {
		int hr, min, sec;
		GetHourMinuteSecond(hr, min, sec);
		
		string ssec = sec.ToString();
		if (ssec.Length() == 1){
			ssec = "0" + ssec;
		}
		string smin = min.ToString();
		if (smin.Length() == 1){
			smin = "0" + smin;
		}
		string shr = hr.ToString();
		if (shr.Length() == 1) {
			shr = "0" + shr;
		}
		
		return  shr + ":" + smin + ":" + ssec;
	}
	
	
	void DoLog(string text, int level = 1)
	{	
		if (level == 2 && m_LogLevel >= level) {
			GetGame().AdminLog("[" + m_LogType + "]" + GetTag(level) + text);
		}
		if (m_isInit && m_LogLevel >= level){
			//Print("[MapLink] " + GetTag(level) + GetTimeStamp() + " | " + text);
			string towrite = GetTag(level)  + GetTimeStamp() + " | " + " " + text;
			FPrintln(m_FileHandle, towrite);
		} else if (m_LogLevel >= level) {
			Print("[" + m_LogType + "]" + GetTag(level) + " " + text);
		}
		if (m_LogToApiLevel >= level){
			SendToApi(GetJsonObject(m_LogType,text, level));
		}
	}
	
	protected void SendToApi(string jsonString){
		
	}
	
	protected static string GetTag(int level){
		switch ( level ) {
			case LOG_ERROR:
				return "[ERROR] ";
				break;
			case LOG_VERBOSE:
				return "[VERBOSE] ";
				break;
			case LOG_DEBUG:
				return "[DEBUG] ";
				break;
			case LOG_INFO:
				return "[INFO] ";
				break;
			default:
				return "[INFO] ";
				break;
		}
		return "[NULL] ";
	}
	
	static string GetJsonObject(string type, string text, int level) {
		string sLevel = "INFO";
		
		switch ( level ) {
			case LOG_ERROR:
				sLevel = "ERROR";
				break;
			case LOG_VERBOSE:
				sLevel =  "VERBOSE";
				break;
			case LOG_DEBUG:
				sLevel =  "DEBUG";
				break;
			case LOG_INFO:
				sLevel =  "INFO";
				break;
			default:
				sLevel =  "INFO";
				break;
		}
		autoptr ULoggerObject obj = new ULoggerObject( type, text, sLevel);
		return obj.ToJson();
	}
}

class ULoggerObject extends UApiObject_Base {
	
	string Type;
	string Message;
	string Level;
	
	void ULoggerObject(string type, string text, string level){
		Type = type;
		Message = text;
		Level = level;
		
	}
	
	override string ToJson(){
		string jsonString = JsonFileLoader<ULoggerObject>.JsonMakeData(this);
		return jsonString;
	}
	
}