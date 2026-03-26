/**
 * Typed Queue Handler - automatically polls and delivers typed messages
 * @tparam T The type of messages expected from the queue
 * 
 * Usage:
 * 1. Create handler with mod, queue, callback object, and callback function
 * 2. Handler will poll at specified frequency
 * 3. Callback receives each message individually
 */
class UQueueHandler<Class T> extends UQueueHandlerBase 
{
	/**
	 * Reads unread messages from the queue
	 * @return int Call ID
	 */
	override int Read(){
		return UF().Msg().Read(m_mod, m_queue, new UFMsgCallback<T>(this, "readCB", m_queue));
	}
	
	/**
	 * Reads up to N unread messages from the queue
	 * @param limit Maximum number of messages to read
	 * @return int Call ID
	 */
	int Read(int limit){
		return UF().Msg().Read(m_mod, m_queue, limit, new UFMsgCallback<T>(this, "readCB", m_queue));
	}
	
	/**
	 * Read the latest N messages, skipping any older unread messages
	 * This marks all older messages as read, then returns the N most recent
	 * Useful for catching up on a backlog while keeping only recent context
	 * @param limit Maximum number of recent messages to return
	 * @return Call ID or -1 on error
	 */
	int ReadLatest(int limit){
		return UF().Msg().ReadLatest(m_mod, m_queue, limit, new UFMsgCallback<T>(this, "readCB", m_queue));
	}
	
	/**
	 * Write a typed message to the queue
	 * @param message The message object to send
	 * @return Call ID or -1 on error
	 */
	int Write(T message){
		if (!message){
			Error2("[UF] UQueueHandler", "Cannot write NULL message");
			return -1;
		}
		autoptr UMessage<T> msg = new UMessage<T>(message);
		string txt = msg.ToJson();
		return UF().Msg().Write(m_mod, m_queue, txt);
	}
 	
	/**
	 * Internal callback - delivers each message to the registered callback
	 */
	void readCB(int cid, int status, string oid, array<autoptr T> messages){
		// Mark that read has completed
		m_ReadInProgress = false;
		m_LastReadCall = -1;
		
		if (status == UF_SUCCESS && messages){
			foreach(T message : messages){
				if (message && GetInstance()){
					g_Game.GameScript.CallFunctionParams(GetInstance(), GetFuncName(), NULL, new Param4<int, int, string, T>(cid, status, oid, message));
				}
			}
		} else if (status != UF_EMPTY && status != UF_SUCCESS){
			// Log non-success statuses (but not as crash-log errors)
			// CLIENT_ERROR (400) is normal for things like non-existent queues
			if (status == UF_SERVERERROR || status == UF_TIMEOUT){
				UFLog.Info("UQueueHandler<" /*+ T.StaticType().ToString() */ + "> Read status: " + UUtil.StatusToString(status));
			} else {
				UFLog.Debug("UQueueHandler<" /*+ T.StaticType().ToString() */ + "> Read status: " + UUtil.StatusToString(status));
			}
		}
	} 
}


/**
 * String Queue Handler - automatically polls and delivers string messages
 * 
 * Usage:
 * 1. Create handler with mod, queue, callback object, and callback function
 * 2. Handler will poll at specified frequency
 * 3. Callback receives each message individually
 */
class UStringQueueHandler extends UQueueHandlerBase 
{
	/**
	 * Reads unread string messages from the queue
	 * @return int Call ID
	 */
	override int Read(){
		return UF().Msg().Read(m_mod, m_queue, new UFMsgStringCallback(this, "readCB", m_queue));
	}
	
	/**
	 * Reads up to N unread string messages from the queue
	 * @param limit Maximum number of messages to read
	 * @return int Call ID
	 */
	int Read(int limit){
		return UF().Msg().Read(m_mod, m_queue, limit, new UFMsgStringCallback(this, "readCB", m_queue));
	}
	
	/**
	 * Read the latest N messages, skipping any older unread messages
	 * This marks all older messages as read, then returns the N most recent
	 * Useful for catching up on a backlog while keeping only recent context
	 * @param limit Maximum number of recent messages to return
	 * @return Call ID or -1 on error
	 */
	int ReadLatest(int limit){
		return UF().Msg().ReadLatest(m_mod, m_queue, limit, new UFMsgStringCallback(this, "readCB", m_queue));
	}
	
	/**
	 * Write a string message to the queue
	 * @param message The string message to send
	 * @return Call ID or -1 on error
	 */
 	int Write(string message){
		if (message == "") {
			Error2("[UF] UStringQueueHandler", "Cannot write empty message");
			return -1;
		}
		autoptr UStringMessage msg = new UStringMessage(message);
		string txt = msg.ToJson();
		return UF().Msg().Write(m_mod, m_queue, txt);
	}
	
	/**
	 * Internal callback - delivers each message to the registered callback
	 */
	void readCB(int cid, int status, string oid, TStringArray messages){
		// Mark that read has completed
		m_ReadInProgress = false;
		m_LastReadCall = -1;
		
		if (status == UF_SUCCESS && messages){
			foreach(string message : messages){
				if (message != "" && GetInstance()){
					g_Game.GameScript.CallFunctionParams(GetInstance(), GetFuncName(), NULL, new Param4<int, int, string, string>(cid, status, oid, message));
				}
			}
		} else if (status != UF_EMPTY && status != UF_SUCCESS){
			// Log non-success statuses (but not as crash-log errors)
			// CLIENT_ERROR (400) is normal for things like non-existent queues
			if (status == UF_SERVERERROR || status == UF_TIMEOUT){
				UFLog.Info("UStringQueueHandler Read status: " + UUtil.StatusToString(status));
			} else {
				UFLog.Debug("UStringQueueHandler Read status: " + UUtil.StatusToString(status));
			}
		}
	}
}


/**
 * Base class for queue handlers
 * Provides automatic polling, cleanup, and common functionality
 */
class UQueueHandlerBase extends Managed 
{
	protected string m_mod;
	protected string m_queue;
	protected Class m_obj;
	protected string m_funcName;
	protected int m_limit;
	protected int m_LastReadCall = -1;
	protected int m_LastWriteCall = -1;
	protected int m_PollingFrequency = 3;
	protected bool m_ReadInProgress = false;
	protected bool m_IsDestroying = false;
    
	/**
	 * Create a queue handler for reading and writing
	 * @param mod The mod identifier
	 * @param queue The queue identifier
	 * @param obj The object to call back on
	 * @param funcName The function to call with each message
	 * @param meta Optional queue metadata to set on creation
	 * @param limit Maximum messages per read (-1 for all)
	 * @param pollingFrequency How often to poll in seconds (default 3)
	 */
    void UQueueHandlerBase(string mod, string queue, Class obj, string funcName, UQueueMeta meta = NULL, int limit = -1, int pollingFrequency = 3)
    {
		m_mod = mod;
		m_queue = queue;
		Class.CastTo(m_obj, obj);
		m_funcName = funcName;
		m_limit = limit;
		m_PollingFrequency = pollingFrequency;
		m_IsDestroying = false;
		Init(meta);
    }
	
	/**
	 * Create a queue handler for writing only (no polling)
	 * @param mod The mod identifier
	 * @param queue The queue identifier
	 * @param meta Optional queue metadata to set on creation
	 */
    void UQueueHandlerBase(string mod, string queue, UQueueMeta meta)
    {
		m_mod = mod;
		m_queue = queue;
		m_PollingFrequency = -1;
		m_IsDestroying = false;
		Init(meta);
    }
			
	protected void Init(UQueueMeta meta)
	{
		// Only start polling if we have a valid callback setup
		if (m_PollingFrequency > 0 && m_obj && m_funcName != ""){
			UF().Cron().runEndless(m_PollingFrequency, this, "CheckQueue", NULL);
		}
		
		// Only set meta from server
		if (meta && g_Game.IsDedicatedServer()){
			m_LastWriteCall = UF().Msg().SetMeta(m_mod, m_queue, meta);
		} 
	}

	/**
	 * Destructor - cleans up cron job and pending callbacks
	 */
    void ~UQueueHandlerBase()
    {
		m_IsDestroying = true;
		
		// Remove from cron scheduler
		UF().Cron().Remove(this, "CheckQueue");
		
		// Cancel any pending calls
		if (m_LastReadCall > 0){
			Cancel(m_LastReadCall);
		}
		if (m_LastWriteCall > 0){
			Cancel(m_LastWriteCall);
		}
		
		m_obj = NULL;
    }

	/**
	 * Called by cron scheduler to check for new messages
	 * Prevents overlapping reads
	 */
	void CheckQueue(){
		// Don't start new read if one is in progress or we're destroying
		if (m_ReadInProgress || m_IsDestroying){
			return;
		}
		
		m_ReadInProgress = true;
		m_LastReadCall = Read();
		
		// If read failed to start, mark as not in progress
		if (m_LastReadCall < 0){
			m_ReadInProgress = false;
		}
	}
	
	/**
	 * Write raw JSON to the queue
	 * @param sText The JSON string (should contain {"Message": ...})
	 * @return Call ID or -1 on error
	 */
	int jsonWrite(string sText){
		return UF().Msg().Write(m_mod, m_queue, sText);
	}
	
	/**
	 * Override in subclass to perform the read operation
	 */
	int Read(){
		Error2("[UF] UFQueueHandlerBase", "Using unimplemented Read");
		return -1;
	}
	
	/**
	 * Reset the queue - marks all existing messages as read
	 * @return Call ID or -1 on error
	 */
	int Reset(){
		return UF().Msg().Reset(m_mod, m_queue);
	}
	
	/**
	 * Purge old messages from the queue
	 * @param olderThanDays Delete messages older than this many days
	 * @return Call ID or -1 on error
	 */
	int Purge(int olderThanDays = 30){
		return UF().Msg().Purge(m_mod, m_queue, olderThanDays);
	}
	
	/**
	 * Cancel a pending callback to prevent access violations
	 * @param cid The call ID to cancel
	 */
	void Cancel(int cid){
		UF().RequestCallCancel(cid);
	}
 
	/**
	 * Returns the callback instance
	 * @return Class The instance receiving callbacks
	 */
	Class GetInstance(){
		return m_obj;
	}
	
	/**
	 * Returns the callback function name
	 * @return string The function name
	 */
	string GetFuncName(){
		return m_funcName;
	}
	
	/**
	 * Returns the mod identifier
	 * @return string The mod ID
	 */
	string GetMod(){
		return m_mod;
	}
	
	/**
	 * Returns the queue identifier
	 * @return string The queue ID
	 */
	string GetQueue(){
		return m_queue;
	}
	
	/**
	 * Check if the handler is actively reading
	 */
	bool IsReadInProgress(){
		return m_ReadInProgress;
	}
}
