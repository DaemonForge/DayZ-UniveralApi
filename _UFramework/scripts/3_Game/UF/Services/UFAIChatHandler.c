/**
 * File: UFAIChatHandler.c
 * Description: Service class for managing AI Chat interactions
 */

/**
 * Template version of AI Chat Handler for typed responses
 * Requires JSON schema to properly parse responses into objects
 */
class UAIChatHandler<Class T> extends UAIChatHandlerBase 
{
	protected string m_JsonSchema;
	
	/**
	 * Constructor for UAIChatHandler with existing chat ID
	 * @param chatId Existing chat ID to connect to
	 * @param obj Instance to receive callbacks
	 * @param funcName Function name to call on response
	 * @param jsonSchema JSON schema for parsing responses (only required for server-side creation)
	 */
	void UAIChatHandler(string chatId, Class obj, string funcName, string jsonSchema = "")
	{
		UFLog.Debug("[UAIChatHandler<T>] Created with existing ChatId: " + chatId);
		m_ChatId = chatId;
		m_JsonSchema = jsonSchema;
		Class.CastTo(m_obj, obj);
		m_funcName = funcName;
	}
	
	/**
	 * Constructor for creating a new chat with typed responses
	 * @param systemMessage Initial system prompt for the AI
	 * @param obj Instance to receive callbacks
	 * @param funcName Function name to call on response 
	 * @param jsonSchema JSON schema for parsing responses (required)
	 * @param model Optional AI model to use
	 * @param maxHistory Optional maximum history to keep
	 */
	void UAIChatHandler(string systemMessage, Class obj, string funcName, string jsonSchema, string model = "", int maxHistory = -1)
	{
		UFLog.Debug("[UAIChatHandler<T>] Creating new chat session, Model: " + model + ", MaxHistory: " + maxHistory);
		
		if (jsonSchema == "") {
			Error("[UF] [UAIChatHandler] JSON schema is required for typed responses");
			return;
		}
		
		m_IsCreating = true;
		m_JsonSchema = jsonSchema;
		Class.CastTo(m_obj, obj);
		m_funcName = funcName;
		
		UFLog.Debug("[UAIChatHandler<T>] Calling Create endpoint, SchemaLen: " + jsonSchema.Length().ToString());
		// Create the chat with JSON response format
		m_LastCallId = U().AI().Create(systemMessage, "JSON", jsonSchema, model, maxHistory, new UFCallback<UAIChatCreateResponse>(this, "OnChatCreated"));
		UFLog.Debug("[UAIChatHandler<T>] Create request sent, CID: " + m_LastCallId);
	}
	
	/**
	 * Send a message and process the typed response
	 * @param message Message content to send
	 * @param context Optional context information
	 * @return Call ID or -1 on error
	 */
	override int SendMessage(string message, array<autoptr UAIChatContext> context = NULL)
	{
		if (m_ChatId == "") {
			if (m_IsCreating) {
				// Chat is being created, queue the message
				UFLog.Debug("[UAIChatHandler] Chat is being created, queuing message: " + message);
				QueueMessage(message, context);
				return 0;
			} else {
				Error("[UF] [UAIChatHandler] Cannot send message, no chat ID set");
				return -1;
			}
		}
		
		// Check if we need to queue this message
		if (QueueMessageIfNeeded(message, context)) {
			return 0; // Return dummy call ID for queued message
		}
		
		// Send the message immediately
		m_LastCallId = U().AI().Send(m_ChatId, message, new UFAIMessageCallback<T>(this, "OnMessageResponse"), context);
		
		// Start polling timer for this message
		if (m_PollingEnabled) {
			m_PendingMessageStartTime = g_Game.GetTime() / 1000;
		}
		
		return m_LastCallId;
	}
	
	/**
	 * Process message response callback
	 * @param status Response status code
	 * @param response The response object
	 * @param callId The call ID (optional)
	 */
	override void OnMessageResponse(int status, UAIChatMessageResponse response, int callId = -1)
	{
		UFLog.Debug("[UAIChatHandler<T>] OnMessageResponse - Status: " + status + ", ChatId: " + m_ChatId);
		
		if (status != UF_SUCCESS) {
			Error2("[UF] [UAIChatHandler] Message response error", "Status: " + status);
			g_Game.GameScript.CallFunctionParams(m_obj, m_funcName, NULL, 
				new Param4<int, int, string, T>(callId, status, m_ChatId, null));
				
			// Move to the next message in queue if this one failed
			m_PendingMessageId = "";
			m_PendingMessageRetries = 0;
			ProcessMessageQueue();
			return;
		}
		
		int ufStatus = UF_SUCCESS;
		if (response && response.Status) {
			switch (response.Status) {
				case "NotFound":
					ufStatus = UF_NOTFOUND;
					break;
				case "Error":
					ufStatus = UF_ERROR;
					break;
				case "Pending":
					ufStatus = UF_AI_PENDING;
					break;
				case "Wait":
					ufStatus = UF_AI_PROCESSING;
					break;
				case "Invalid":
					ufStatus = UF_CLIENTERROR;
					break;
				case "Timeout":
					ufStatus = UF_TIMEOUT;
					break;
			}
		}
		
		if (response && response.Status == "Success") {
			string jsonMessage = response.GetMessage();
			UFLog.Debug("[UAIChatHandler<T>] Response Success - Parsing JSON, Length: " + jsonMessage.Length().ToString());
			T typedResponse;
			
			// Parse JSON to the typed object
			if (jsonMessage != "") {
				string error;
				JsonSerializer js = new JsonSerializer();
				bool success = js.ReadFromString(typedResponse, jsonMessage, error);
				
				if (!success || error != "") {
					Error2("[UF] [UAIChatHandler] Failed to parse JSON response", error);
					UFLog.Debug("[UAIChatHandler<T>] JSON Parse Error: " + error + ", Raw: " + jsonMessage.Substring(0, Math.Min(200, jsonMessage.Length())));
					g_Game.GameScript.CallFunctionParams(m_obj, m_funcName, NULL, 
						new Param4<int, int, string, T>(callId, UF_JSONERROR, m_ChatId, null));
					
					// Move to the next message even if parsing failed
					m_PendingMessageId = "";
					m_PendingMessageRetries = 0;
					ProcessMessageQueue();
					return;
				}
				UFLog.Debug("[UAIChatHandler<T>] JSON parsed successfully");
			}
			
			// Message completed successfully
			m_PendingMessageId = "";
			m_PendingMessageRetries = 0;
			
			UFLog.Debug("[UAIChatHandler<T>] Calling user callback: " + m_funcName);
			// Call the callback with the parsed object
			g_Game.GameScript.CallFunctionParams(m_obj, m_funcName, NULL, 
				new Param4<int, int, string, T>(callId, ufStatus, m_ChatId, typedResponse));
				
			// Process next message in queue
			ProcessMessageQueue();
		} else if (response && response.Status == "Pending" && m_PollingEnabled) {
			// Start polling for a pending message
			UFLog.Debug("[UAIChatHandler<T>] Response Pending - Starting poll, MessageId: " + response.MessageId);
			m_PendingMessageId = response.MessageId;
			m_PendingMessageStartTime = g_Game.GetTime() / 1000;
			m_PendingMessageRetries = 0;
			EnsurePollingStarted();
		} else {
			// Status is Error or other, just pass along the status
			g_Game.GameScript.CallFunctionParams(m_obj, m_funcName, NULL, 
				new Param4<int, int, string, T>(callId, ufStatus, m_ChatId, null));
				
			// Move to the next message in queue
			m_PendingMessageId = "";
			m_PendingMessageRetries = 0;
			ProcessMessageQueue();
		}
	}
}

/**
 * String version of AI Chat Handler for simpler text responses
 * Does not require JSON schema
 */
class UStringAIChatHandler extends UAIChatHandlerBase
{
	/**
	 * Constructor for UStringAIChatHandler with existing chat ID
	 * @param chatId Existing chat ID to connect to
	 * @param obj Instance to receive callbacks
	 * @param funcName Function name to call on response
	 */
	void UStringAIChatHandler(string chatId, Class obj, string funcName)
	{
		UFLog.Debug("[UStringAIChatHandler] Created with existing ChatId: " + chatId);
		m_ChatId = chatId;
		Class.CastTo(m_obj, obj);
		m_funcName = funcName;
	}
	
	/**
	 * Constructor for creating a new chat with string responses
	 * @param systemMessage Initial system prompt for the AI
	 * @param obj Instance to receive callbacks
	 * @param funcName Function name to call on response
	 * @param model Optional AI model to use
	 * @param maxHistory Optional maximum history to keep
	 */
	void UStringAIChatHandler(string systemMessage, Class obj, string funcName, string model = "", int maxHistory = -1)
	{
		UFLog.Debug("[UStringAIChatHandler] Creating new chat session, Model: " + model + ", MaxHistory: " + maxHistory);
		m_IsCreating = true;
		Class.CastTo(m_obj, obj);
		m_funcName = funcName;
		
		UFLog.Debug("[UStringAIChatHandler] Calling Create endpoint");
		// Create the chat with string response format
		m_LastCallId = U().AI().Create(systemMessage, "string", "", model, maxHistory, 
			new UFCallback<StatusObject>(this, "OnChatCreated"));
		UFLog.Debug("[UStringAIChatHandler] Create request sent, CID: " + m_LastCallId);
	}
	
	/**
	 * Send a message and process the string response
	 * @param message Message content to send
	 * @param context Optional context information
	 * @return Call ID or -1 on error
	 */
	override int SendMessage(string message, array<autoptr UAIChatContext> context = NULL)
	{
		if (m_ChatId == "") {
			if (m_IsCreating) {
				// Chat is being created, queue the message
				UFLog.Debug("[UStringAIChatHandler] Chat is being created, queuing message: " + message);
				QueueMessage(message, context);
				return 0;
			} else {
				Error("[UF] [UStringAIChatHandler] Cannot send message, no chat ID set");
				return -1;
			}
		}
		
		// Check if we need to queue this message
		if (QueueMessageIfNeeded(message, context)) {
			return 0; // Return dummy call ID for queued message
		}
		
		// Send the message immediately
		m_LastCallId = U().AI().Send(m_ChatId, message, new UFAIMessageCallback<string>(this, "OnMessageResponse"), context);
		
		// Start polling timer for this message
		if (m_PollingEnabled) {
			m_PendingMessageStartTime = g_Game.GetTime() / 1000;
		}
		
		return m_LastCallId;
	}
	
	/**
	 * Process message response callback
	 * @param status Response status code
	 * @param response The response object
	 */
	override void OnMessageResponse(int status, UAIChatMessageResponse response, int callId = -1)
	{
		UFLog.Debug("[UStringAIChatHandler] OnMessageResponse - Status: " + status + ", ChatId: " + m_ChatId);
		
		if (status != UF_SUCCESS) {
			Error2("[UF] [UStringAIChatHandler] Message response error", "Status: " + status);
			g_Game.GameScript.CallFunctionParams(m_obj, m_funcName, NULL, 
				new Param4<int, int, string, string>(callId, status, m_ChatId, ""));
				
			// Move to the next message in queue if this one failed
			m_PendingMessageId = "";
			m_PendingMessageRetries = 0;
			ProcessMessageQueue();
			return;
		}
		
		int ufStatus = UF_SUCCESS;
		if (response && response.Status) {
			switch (response.Status) {
				case "NotFound":
					ufStatus = UF_NOTFOUND;
					break;
				case "Error":
					ufStatus = UF_ERROR;
					break;
				case "Pending":
					ufStatus = UF_AI_PENDING;
					break;
				case "Wait":
					ufStatus = UF_AI_PROCESSING;
					break;
				case "Invalid":
					ufStatus = UF_CLIENTERROR;
					break;
				case "Timeout":
					ufStatus = UF_TIMEOUT;
					break;
			}
		}
		
		if (response && response.Status == "Pending" && m_PollingEnabled) {
			// Start polling for a pending message
			UFLog.Debug("[UStringAIChatHandler] Response Pending - Starting poll, MessageId: " + response.MessageId);
			m_PendingMessageId = response.MessageId;
			m_PendingMessageStartTime = g_Game.GetTime() / 1000;
			m_PendingMessageRetries = 0;
			EnsurePollingStarted();
			return;
		}
		
		// Message completed successfully or failed with an error
		if (response) {
			UFLog.Debug("[UStringAIChatHandler] Response Status: " + response.Status + ", MsgLen: " + response.GetMessage().Length().ToString());
		}
		m_PendingMessageId = "";
		m_PendingMessageRetries = 0;
		
		if (response && response.Status == "Success"){
			UFLog.Debug("[UStringAIChatHandler] Success - Calling user callback: " + m_funcName);
		// Pass the status and message string to the callback
			g_Game.GameScript.CallFunctionParams(m_obj, m_funcName, NULL, new Param4<int, int, string, string>(callId, ufStatus, m_ChatId, response.GetMessage() ));
		}
		// Process next message in queue
		ProcessMessageQueue();
	}
}

/**
 * Base class for AI Chat Handler with common functionality
 */
class UAIChatHandlerBase extends Managed
{
	protected string m_ChatId;
	protected Class m_obj;
	protected string m_funcName;
	protected int m_LastCallId = -1;
	protected string m_PendingMessageId;
	protected string m_PendingSummaryId;
	protected bool m_PollingEnabled = false;
	protected int m_PollingFrequency = 1;
	protected bool m_IsCreating = false;
	protected string m_CreateCallbackFunc;
	protected bool m_IsPollingActive = false;
	protected int m_PendingMessageStartTime = 0;
	protected int m_PendingSummaryStartTime = 0;
	protected int m_PendingMessageRetries = 0;
	protected int m_PendingSummaryRetries = 0;
	
	// Message queue system
	protected autoptr array<autoptr UAIChatQueuedMessage> m_MessageQueue;
	protected bool m_IsProcessingQueue = false;
	
	/**
	 * Set polling configuration
	 * @param enabled Whether polling is enabled
	 * @param frequency How often to poll (in seconds)
	 */
	void SetPolling(bool enabled, int frequency = 1)
	{
		m_PollingEnabled = enabled;
		m_PollingFrequency = Math.Max(1, frequency);
		
		if (!m_PollingEnabled) {
			StopPolling();
		} else if ((m_PendingMessageId != "" || m_PendingSummaryId != "") && !m_IsPollingActive) {
			StartPolling();
		}
	}
	
	/**
	 * Directly queue a message (used for messages before chat is created)
	 */
	protected void QueueMessage(string message, array<autoptr UAIChatContext> context = NULL)
	{
		// Initialize queue if needed
		if (!m_MessageQueue) {
			m_MessageQueue = new array<autoptr UAIChatQueuedMessage>;
		}
		
		// Create and add queued message object
		UAIChatQueuedMessage queuedMsg = new UAIChatQueuedMessage(message, context);
		m_MessageQueue.Insert(queuedMsg);
	}
	
	/**
	 * Adds a message to the queue if there's already a pending message
	 * @param message Message to queue
	 * @param context Optional context for the message
	 * @return true if message was queued, false if sent immediately
	 */
	protected bool QueueMessageIfNeeded(string message, array<autoptr UAIChatContext> context = NULL)
	{
		// Initialize queue if needed
		if (!m_MessageQueue) {
			m_MessageQueue = new array<autoptr UAIChatQueuedMessage>;
		}
		
		// If we're busy with another message, queue this one
		if (m_PendingMessageId != "" || m_IsProcessingQueue || m_IsCreating) {
			UFLog.Debug("[UAIChatHandlerBase] Queuing message: " + message);
			
			// Create and add queued message object
			UAIChatQueuedMessage queuedMsg = new UAIChatQueuedMessage(message, context);
			m_MessageQueue.Insert(queuedMsg);
			
			return true; // Message was queued
		}
		
		return false; // No need to queue, can send immediately
	}
	
	/**
	 * Process the next message in queue if any
	 */
	protected void ProcessMessageQueue()
	{
		int queueCount = 0;
		if (m_MessageQueue) queueCount = m_MessageQueue.Count();
		UFLog.Debug("[UAIChatHandlerBase] ProcessMessageQueue - QueueCount: " + queueCount + ", IsCreating: " + m_IsCreating + ", PendingMsgId: " + m_PendingMessageId);
		// Skip if no queue or it's empty
		if (!m_MessageQueue || m_MessageQueue.Count() == 0) {
			m_IsProcessingQueue = false;
			UFLog.Debug("[UAIChatHandlerBase] ProcessMessageQueue - Queue empty, stopping processing");
			
			// Stop polling if no more pending operations
			if (m_PendingMessageId == "" && m_PendingSummaryId == "") {
				StopPolling();
			}
			return;
		}
		
		// Skip if we're still waiting for a response or chat is still being created
		if (m_PendingMessageId != "" || m_IsCreating) {
			return;
		}
		
		m_IsProcessingQueue = true;
		
		// Get the next message from the queue
		UAIChatQueuedMessage nextMsg = m_MessageQueue[0];
		m_MessageQueue.RemoveOrdered(0);
		
		// Send the message
		UFLog.Debug("[UAIChatHandlerBase] Processing queued message: " + nextMsg.message);
		SendMessage(nextMsg.message, nextMsg.context);
	}
	
	/**
	 * Make sure polling is started if needed but not already running
	 */
	protected void EnsurePollingStarted()
	{
		if (m_PollingEnabled && !m_IsPollingActive) {
			StartPolling();
		}
	}
	
	/**
	 * Start polling for pending operations
	 */
	protected void StartPolling()
	{
		if (!m_PollingEnabled || m_IsPollingActive) return;
		
		// Register with the cron system to poll periodically
		UFLog.Debug("[UAIChatHandlerBase] Starting polling with frequency: " + m_PollingFrequency);
		U().Cron().runEndless(m_PollingFrequency, this, "PollPendingOperations", NULL);
		m_IsPollingActive = true;
	}
	
	/**
	 * Stop polling for pending operations
	 */
	protected void StopPolling()
	{
		if (!m_IsPollingActive) return;
		
		UFLog.Debug("[UAIChatHandlerBase] Stopping polling");
		U().Cron().Remove(this, "PollPendingOperations");
		m_IsPollingActive = false;
	}
	
	/**
	 * Poll for any pending operations
	 */
	void PollPendingOperations()
	{
		int queueCount = 0;
		if (m_MessageQueue) queueCount = m_MessageQueue.Count();
		UFLog.Debug("[UAIChatHandlerBase] PollPendingOperations - PendingMsgId: " + m_PendingMessageId + ", PendingSummaryId: " + m_PendingSummaryId + ", QueueCount: " + queueCount);
		// Check for messages that exceed the timeout limit
		int currentTime = g_Game.GetTime() / 1000;
		
		// Check pending message timeout
		if (m_PendingMessageId != "") {
			int messageElapsedTime = currentTime - m_PendingMessageStartTime;
			if (messageElapsedTime > UF_AI_CHAT_MAX_POLL_TIME) {
				// Message timed out
				Error2("[UF] [UAIChatHandlerBase] Message timed out after", "" + messageElapsedTime + " seconds");
				
				// Treat as an error and move on
				m_PendingMessageId = "";
				m_PendingMessageRetries = 0;
				
				// Notify the callback of timeout
				g_Game.GameScript.CallFunctionParams(m_obj, m_funcName, NULL, 
					new Param4<int, int, string, string>(m_LastCallId, UF_TIMEOUT, m_ChatId, "Operation timed out after " + messageElapsedTime + " seconds"));
				
				// Process next message in queue
				ProcessMessageQueue();
				return;
			}
			
			CheckPendingMessage();
		}
		
		// Check pending summary timeout
		if (m_PendingSummaryId != "") {
			int summaryElapsedTime = currentTime - m_PendingSummaryStartTime;
			if (summaryElapsedTime > UF_AI_CHAT_MAX_POLL_TIME) {
				// Summary timed out
				Error2("[UF] [UAIChatHandlerBase] Summary timed out after", "" + summaryElapsedTime + " seconds");
				
				// Treat as an error and move on
				m_PendingSummaryId = "";
				m_PendingSummaryRetries = 0;
				
				// Notify the callback of timeout
				g_Game.GameScript.CallFunctionParams(m_obj, m_funcName, NULL, 
					new Param4<int, int, string, string>(m_LastCallId, UF_TIMEOUT, m_ChatId, "Summary operation timed out after " + summaryElapsedTime + " seconds"));
				return;
			}
			
			CheckPendingSummary();
		}
		
		// Stop polling if nothing left to check
		if (m_PendingMessageId == "" && m_PendingSummaryId == "" && (!m_MessageQueue || m_MessageQueue.Count() == 0)) {
			StopPolling();
		}
	}
	
	/**
	 * Check the status of a pending message
	 */
	protected void CheckPendingMessage()
	{
		if (m_PendingMessageId == "") return;
		
		UFLog.Debug("[UAIChatHandlerBase] CheckPendingMessage - MessageId: " + m_PendingMessageId + ", Elapsed: " + ((g_Game.GetTime() / 1000) - m_PendingMessageStartTime) + "s");
		m_LastCallId = U().AI().MessageStatus(m_PendingMessageId, 
			new UFCallback<UAIChatMessageResponse>(this, "OnMessageStatusUpdate"));
	}
	
	/**
	 * Setup notification for when chat is created 
	 * @param callbackFunc Function to call on chat creation completion
	 */
	void NotifyOnCreated(string callbackFunc = "")
	{
		m_CreateCallbackFunc = callbackFunc;
	}
	
	/**
	 * Check message status update
	 * @param status The status code
	 * @param response The status response
	 */
	void OnMessageStatusUpdate(int status, UAIChatMessageResponse response)
	{
		UFLog.Debug("[UAIChatHandlerBase] OnMessageStatusUpdate - Status: " + status + ", PendingId: " + m_PendingMessageId + ", Retries: " + m_PendingMessageRetries);
		
		if (status != UF_SUCCESS) {
			Error2("[UF] [UAIChatHandlerBase] Message status error", "Status: " + status);
			
			// Increment retry counter
			m_PendingMessageRetries++;
			UFLog.Debug("[UAIChatHandlerBase] Status check failed, retry: " + m_PendingMessageRetries + "/" + UF_AI_CHAT_MAX_RETRIES);
			
			// If we've reached the retry limit, fail the message
			if (m_PendingMessageRetries >= UF_AI_CHAT_MAX_RETRIES) {
				Error2("[UF] [UAIChatHandlerBase] Message status check failed after", "" + m_PendingMessageRetries + " retries");
				
				// Clear pending state and move on
				m_PendingMessageId = "";
				m_PendingMessageRetries = 0;
				
				// Notify the callback of network error
				g_Game.GameScript.CallFunctionParams(m_obj, m_funcName, NULL, 
					new Param4<int, int, string, string>(m_LastCallId, UF_ERROR, m_ChatId, "Network error after " + m_PendingMessageRetries + " retries"));
				
				// Process next message in queue
				ProcessMessageQueue();
			}
			
			return;
		}
		
		// Reset retry counter on successful status check
		m_PendingMessageRetries = 0;
		
		if (response.Status != "Pending") {
			// Message processing finished
			string oldMessageId = m_PendingMessageId;
			m_PendingMessageId = "";
			
			// Forward the final status to the message handler
			OnMessageResponse(status, response, m_LastCallId);
		}
	}
	
	/**
	 * Check the status of a pending summary
	 */
	int CheckPendingSummary()
	{
		if (m_PendingSummaryId == "") return -1;
		
		m_LastCallId = U().AI().SummaryStatus(m_PendingSummaryId, 
			new UFCallback<UAIChatSummaryResponse>(this, "OnSummaryStatusUpdate"));
		return m_LastCallId;
	}
	
	/**
	 * Handle summary status update
	 * @param status The status code
	 * @param response The summary response
	 */
	void OnSummaryStatusUpdate(int status, UAIChatSummaryResponse response)
	{
		if (status != UF_SUCCESS) {
			Error2("[UF] [UAIChatHandlerBase] Summary status error", "Status: " + status);
			
			// Increment retry counter
			m_PendingSummaryRetries++;
			
			// If we've reached the retry limit, fail the summary
			if (m_PendingSummaryRetries >= UF_AI_CHAT_MAX_RETRIES) {
				Error2("[UF] [UAIChatHandlerBase] Summary status check failed after", "" + m_PendingSummaryRetries + " retries");
				
				// Clear pending state and move on
				m_PendingSummaryId = "";
				m_PendingSummaryRetries = 0;
				
				// Notify the callback of network error
				g_Game.GameScript.CallFunctionParams(m_obj, m_funcName, NULL, 
					new Param4<int, int, string, string>(m_LastCallId, UF_ERROR, m_ChatId, "Network error after " + m_PendingSummaryRetries + " retries"));
			}
			
			return;
		}
		
		// Reset retry counter on successful status check
		m_PendingSummaryRetries = 0;
		
		if (response.Status != "Pending") {
			// Summary generation finished
			m_PendingSummaryId = "";
			
			// If no other pending operations, stop polling
			if (m_PendingMessageId == "" && (!m_MessageQueue || m_MessageQueue.Count() == 0)) {
				StopPolling();
			}
			
			int ufStatus = UF_SUCCESS;
			if (response.Status) {
				switch (response.Status) {
					case "Error":
						ufStatus = UF_ERROR;
						break;
					case "Invalid":
						ufStatus = UF_CLIENTERROR;
						break;
				}
			}
			
			// Call back with the summary
			g_Game.GameScript.CallFunctionParams(m_obj, m_funcName, NULL, new Param4<int, int, string, string>(m_LastCallId, ufStatus, m_ChatId, response.GetSummary()));
		}
	}
	
	/**
	 * Explicitly check a specific message status
	 * @param messageId The message ID to check
	 * @return Call ID or -1 on error
	 */
	int CheckMessageStatus(string messageId)
	{
		if (messageId == "") {
			Error("[UF] [UAIChatHandlerBase] Cannot check message status, no message ID provided");
			return -1;
		}
		
		m_LastCallId = U().AI().MessageStatus(messageId, 
			new UFCallback<UAIChatMessageResponse>(this, "OnMessageStatusUpdate"));
		return m_LastCallId;
	}
	
	/**
	 * Explicitly check a specific summary status
	 * @param summaryId The summary ID to check
	 * @return Call ID or -1 on error
	 */
	int CheckSummaryStatus(string summaryId)
	{
		if (summaryId == "") {
			Error("[UF] [UAIChatHandlerBase] Cannot check summary status, no summary ID provided");
			return -1;
		}
		
		m_LastCallId = U().AI().SummaryStatus(summaryId, new UFCallback<UAIChatSummaryResponse>(this, "OnSummaryStatusUpdate"));
		return m_LastCallId;
	}
	
	/**
	 * Handle chat creation callback
	 * @param status Response status code
	 * @param response The response object
	 */
	void OnChatCreated(int status, UAIChatCreateResponse response)
	{
		UFLog.Debug("[UAIChatHandlerBase] OnChatCreated - Status: " + status + ", ResponseStatus: " + response.Status);
		m_IsCreating = false;
		
		if (status != UF_SUCCESS || response.Status != "Success") {
			Error2("[UF] [UAIChatHandlerBase] Failed to create chat", "Status: " + status);
			UFLog.Debug("[UAIChatHandlerBase] Chat creation failed - Status: " + status + ", ResponseStatus: " + response.Status + ", Error: " + response.Error);
			
			// Clear all queued messages since the chat creation failed
			if (m_MessageQueue) {
				UFLog.Debug("[UAIChatHandlerBase] Clearing " + m_MessageQueue.Count() + " queued messages due to creation failure");
				m_MessageQueue.Clear();
			}
			
			// Notify client about creation failure if callback is set
			if (m_CreateCallbackFunc != "") {
				g_Game.GameScript.CallFunctionParams(m_obj, m_CreateCallbackFunc, NULL, new Param4<int, int, string, bool>(m_LastCallId, status, "", false));
			}
			return;
		}
		
		// Store the chat ID for future operations
		m_ChatId = response.ChatId;
		UFLog.Debug("[UAIChatHandlerBase] Chat created with ID: " + m_ChatId);
		
		// Notify client about creation success if callback is set
		if (m_CreateCallbackFunc != "") {
			g_Game.GameScript.CallFunctionParams(m_obj, m_CreateCallbackFunc, NULL, new Param4<int, int, string, bool>(m_LastCallId, UF_SUCCESS, m_ChatId, true));
		}
		
		// Process any messages that were queued before the chat was created
		if (m_MessageQueue && m_MessageQueue.Count() > 0) {
			ProcessMessageQueue();
		}
	}
	
	/**
	 * Send a message in this chat
	 * Must be implemented by derived classes
	 * @param message Message content to send
	 * @param context Optional context information
	 * @return Call ID or -1 on error
	 */
	int SendMessage(string message, array<autoptr UAIChatContext> context = NULL)
	{
		Error("[UF] [UAIChatHandlerBase] SendMessage not implemented in base class");
		return -1;
	}
	
	/**
	 * Reset the chat (clear history)
	 * @return Call ID or -1 on error
	 */
	int Reset()
	{
		if (m_ChatId == "") {
			Error("[UF] [UAIChatHandlerBase] Cannot reset, no chat ID set");
			return -1;
		}
		
		// Clear any pending messages and state
		if (m_MessageQueue) {
			m_MessageQueue.Clear();
		}
		
		m_PendingMessageId = "";
		m_PendingSummaryId = "";
		m_IsProcessingQueue = false;
		
		StopPolling();
		
		m_LastCallId = U().AI().Reset(m_ChatId);
		return m_LastCallId;
	}
	
	/**
	 * Delete the chat entirely
	 * @return Call ID or -1 on error
	 */
	int Delete()
	{
		if (m_ChatId == "") {
			Error("[UF] [UAIChatHandlerBase] Cannot delete, no chat ID set");
			return -1;
		}
		
		// Clear any pending messages and state
		if (m_MessageQueue) {
			m_MessageQueue.Clear();
		}
		
		m_PendingMessageId = "";
		m_PendingSummaryId = "";
		m_IsProcessingQueue = false;
		
		StopPolling();
		
		m_LastCallId = U().AI().Delete(m_ChatId);
		return m_LastCallId;
	}
	
	/**
	 * Generate a summary of the chat
	 * @return Call ID or -1 on error
	 */
	int Summarize(UFCallbackBase callback = NULL)
	{
		if (m_ChatId == "") {
			Error("[UF] [UAIChatHandlerBase] Cannot summarize, no chat ID set");
			return -1;
		}
		
		if (callback) {
			// External callback provided, just pass it through
			m_LastCallId = U().AI().Summarize(m_ChatId, callback);
		} else {
			// Use internal callback and polling
			m_LastCallId = U().AI().Summarize(m_ChatId, 
				new UFCallback<UAIChatSummaryResponse>(this, "OnSummarizeResponse"));
		}
		
		return m_LastCallId;
	}
	
	/**
	 * Process summarize response
	 */
	void OnSummarizeResponse(int status, UAIChatSummaryResponse response)
	{
		if (status != UF_SUCCESS) {
			Error2("[UF] [UAIChatHandlerBase] Summarize error", "Status: " + status);
			g_Game.GameScript.CallFunctionParams(m_obj, m_funcName, NULL, 
				new Param4<int, int, string, string>(m_LastCallId, status, m_ChatId, ""));
			return;
		}
		
		int ufStatus = UF_SUCCESS;
		if (response && response.Status) {
			switch (response.Status) {
				case "Error":
					ufStatus = UF_ERROR;
					break;
				case "Pending":
					ufStatus = UF_AI_PENDING;
					break;
				case "Wait":
					ufStatus = UF_AI_PROCESSING;
					break;
				case "Invalid":
					ufStatus = UF_CLIENTERROR;
					break;
				case "Timeout":
					ufStatus = UF_TIMEOUT;
					break;
			}
		}
		
		if (response && response.Status == "Pending" && m_PollingEnabled) {
			// Store the summary ID and start polling
			m_PendingSummaryId = response.SummaryId;
			m_PendingSummaryStartTime = g_Game.GetTime() / 1000;
			m_PendingSummaryRetries = 0;
			EnsurePollingStarted();
		} else if (response && response.Status == "Success") {
			// Success or error, just pass it through
			g_Game.GameScript.CallFunctionParams(m_obj, m_funcName, NULL, new Param4<int, int, string, string>(m_LastCallId, ufStatus, m_ChatId, response.GetSummary()));
		}
	}
	
	/**
	 * Get the chat history
	 * @return Call ID or -1 on error
	 */
	int GetHistory(UFCallbackBase callback)
	{
		if (m_ChatId == "") {
			Error("[UF] [UAIChatHandlerBase] Cannot get history, no chat ID set");
			return -1;
		}
		
		m_LastCallId = U().AI().Read(m_ChatId, callback);
		return m_LastCallId;
	}
	
	/**
	 * Get the chat ID
	 * @return The chat ID
	 */
	string GetChatId()
	{
		return m_ChatId;
	}
	
	/**
	 * Get the number of messages in the queue
	 * @return Number of messages waiting to be sent
	 */
	int GetQueueCount()
	{
		if (!m_MessageQueue) {
			return 0;
		}
		return m_MessageQueue.Count();
	}
	
	/**
	 * Cancel the last call and stop polling
	 */
	void Cancel()
	{
		if (m_LastCallId > 0) {
			U().RequestCallCancel(m_LastCallId);
			m_LastCallId = -1;
		}
		
		StopPolling();
	}
	
	/**
	 * Clear the message queue without sending any messages
	 */
	void ClearQueue()
	{
		if (m_MessageQueue) {
			m_MessageQueue.Clear();
		}
		m_IsProcessingQueue = false;
	}
	
	/**
	 * Check if this chat is currently busy with an async operation
	 * @return True if busy, false otherwise
	 */
	bool IsBusy()
	{
		return m_IsCreating || m_PendingMessageId != "" || m_PendingSummaryId != "" || m_IsProcessingQueue || (m_MessageQueue && m_MessageQueue.Count() > 0);
	}
	
	/**
	 * Abstract message response handler
	 * Must be implemented by derived classes
	 * @param status Response status code
	 * @param response The response object 
	 * @param callId The call ID (optional)
	 */
	void OnMessageResponse(int status, UAIChatMessageResponse response, int callId = -1)
	{
		// To be implemented by derived classes
	}
	
	/**
	 * Destructor to clean up
	 */
	void ~UAIChatHandlerBase()
	{
		// Clear pending states
		m_PendingMessageId = "";
		m_PendingSummaryId = "";
		
		// Stop all polling
		StopPolling();
		
		// Cancel any pending requests
		if (m_LastCallId > 0) {
			U().RequestCallCancel(m_LastCallId);
			m_LastCallId = -1;
		}
		
		// Clear the message queue
		if (m_MessageQueue) {
			m_MessageQueue.Clear();
			m_MessageQueue = NULL;
		}
	}
}

/**
 * Callback class for AI chat message responses
 */
class UFAIMessageCallback<Class T> extends UFCallbackBase
{
	
	
	override void OnSuccess(string jsonData, int cid)
	{
		autoptr UAIChatMessageResponse response;
		string error;
		
		JsonSerializer js = new JsonSerializer();
		bool success = js.ReadFromString(response, jsonData, error);
		int rstatus = UF_JSONERROR;
		if (!success || error != "") {
			Error2("[UF] [UFAIMessageCallback] Failed to parse response", error);
			g_Game.GameScript.CallFunctionParams(Instance, Function, NULL, new Param4<int, int, string, UAIChatMessageResponse>(cid, UF_JSONERROR, OID, null));
			return;
		}
			rstatus = UF_SUCCESS;
			StatusObject sobj;
			if (Class.CastTo(sobj, response)){
				switch (sobj.Status) {
					case "NotFound":
						rstatus = UF_NOTFOUND;
						break;
					case "Empty":
						rstatus = UF_EMPTY;
						break;
					case "Error":
						rstatus = UF_ERROR;
						break;
					case "NoPerms":
						rstatus = UF_UNAUTHORIZED;
						break;
					case "NoAuth":
						rstatus = UF_UNAUTHORIZED;
						break;
					case "InvalidAuth":
						rstatus = UF_UNAUTHORIZED;
						break;
					case "NotSetup":
						rstatus = UF_NOTSETUP;
						break;
				}
			}
		
		g_Game.GameScript.CallFunctionParams(Instance, Function, NULL, new Param4<int, int, string, UAIChatMessageResponse>(cid, rstatus, response.GetMessageId(), response));
	}
	
	override void OnError(int errorCode, int cid)
	{
		Error2("[UF] [UFAIMessageCallback] Error", "Code: " + errorCode);
		g_Game.GameScript.CallFunctionParams(Instance, Function, NULL, new Param4<int, int, string, UAIChatMessageResponse>(cid, errorCode, OID, null));
	}
} 