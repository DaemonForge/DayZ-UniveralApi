/**
 * File: UFMsgEndpoint.c
 * Description: The class for calling against the Messages Queuing Endpoint.
 * 
 * This endpoint provides a message queue system where:
 * - Messages can be written by server or players (if allowed)
 * - Each reader maintains their own "last read" pointer
 * - Queues can be FIFO or LIFO ordered
 * - Queue reset marks all existing messages as "read" for all readers
 */

class UFMsgEndpoint extends UFBaseEndpoint {
    
	override protected string EndpointBaseUrl(){
		return UFConfig().GetBaseURL() + "Messages/";
	}
	
	/**
	 * Read all unread messages from a queue (no limit)
	 * @param mod The mod identifier
	 * @param queue The queue identifier
	 * @param cb Callback to receive the messages
	 * @return Call ID or -1 on error
	 */
	int Read(string mod, string queue, UFCallbackBase cb){
		return this.Read(mod, queue, -1, cb);
	}
	
	/**
	 * Read messages from a queue with a limit
	 * @param mod The mod identifier
	 * @param queue The queue identifier
	 * @param limit Maximum number of messages to read (-1 for all, 0 to update pointer only)
	 * @param cb Callback to receive the messages
	 * @return Call ID or -1 on error
	 */
	int Read(string mod, string queue, int limit, UFCallbackBase cb){
		if (mod == "" || queue == ""){
			Error2("[UF] Message Queue Read", "mod and queue must be valid strings");
			return -1;
		}
		if (!cb){
			Error2("[UF] Message Queue Read", "Callback is NULL");
			return -1;
		}
		
		int cid = -1;	
		string endpoint = "Read/" + mod + "/" + queue;
		autoptr UMsgReadObj obj = new UMsgReadObj(limit, false);
		Post(endpoint, obj.ToJson(), U().RegisterCall(new UNestedCallBack(cb), cid));
		
		if (cid == -1){
			Error2("[UF] Message Queue Read", "Error registering callback");
		}
		return cid;
	}
	
	/**
	 * Read the latest N messages from a queue, skipping older unread messages
	 * This is useful when you want the most recent messages and don't care about older ones.
	 * Older messages are marked as read (pointer is updated to skip them).
	 * @param mod The mod identifier
	 * @param queue The queue identifier
	 * @param limit Maximum number of latest messages to read
	 * @param cb Callback to receive the messages
	 * @return Call ID or -1 on error
	 */
	int ReadLatest(string mod, string queue, int limit, UFCallbackBase cb){
		if (mod == "" || queue == ""){
			Error2("[UF] Message Queue ReadLatest", "mod and queue must be valid strings");
			return -1;
		}
		if (!cb){
			Error2("[UF] Message Queue ReadLatest", "Callback is NULL");
			return -1;
		}
		if (limit <= 0){
			Error2("[UF] Message Queue ReadLatest", "limit must be greater than 0");
			return -1;
		}
		
		// Safety check: ensure framework is ready
		UFramework uf = U();
		if (!uf){
			UFLog.Err("[UFMsgEndpoint::ReadLatest] U() returned NULL - framework not ready");
			return -1;
		}
		if (!UFConfig()){
			UFLog.Err("[UFMsgEndpoint::ReadLatest] UFConfig() is NULL - config not loaded");
			return -1;
		}
		
		int cid = -1;	
		string endpoint = "Read/" + mod + "/" + queue;
		autoptr UMsgReadObj obj = new UMsgReadObj(limit, true);
		
		RestCallback regCb = uf.RegisterCall(new UNestedCallBack(cb), cid);
		if (!regCb){
			UFLog.Err("[UFMsgEndpoint::ReadLatest] RegisterCall returned NULL");
			return -1;
		}
		Post(endpoint, obj.ToJson(), regCb);
		
		if (cid == -1){
			Error2("[UF] Message Queue ReadLatest", "Error registering callback");
		}
		return cid;
	}
	
	/**
	 * Write a message to a queue using a typed message object
	 * @param mod The mod identifier
	 * @param queue The queue identifier
	 * @param msg The message object (will be serialized to JSON)
	 * @param cb Optional callback for write confirmation
	 * @return Call ID or -1 on error
	 */
	int Write(string mod, string queue, UMessageBase msg, UFCallbackBase cb = NULL){
		if (!msg){
			Error2("[UF] Message Queue Write", "Message object is NULL");
			return -1;
		}
		return Write(mod, queue, msg.ToJson(), cb);
	}
	
	/**
	 * Write a raw JSON message to a queue
	 * @param mod The mod identifier
	 * @param queue The queue identifier
	 * @param jsonString The JSON message content (should be wrapped in {"Message": ...})
	 * @param cb Optional callback for write confirmation
	 * @return Call ID or -1 on error
	 */
	int Write(string mod, string queue, string jsonString, UFCallbackBase cb = NULL){
		if (mod == "" || queue == ""){
			Error2("[UF] Message Queue Write", "mod and queue must be valid strings");
			return -1;
		}
		if (jsonString == ""){
			Error2("[UF] Message Queue Write", "jsonString must not be empty");
			return -1;
		}
		
		string endpoint = "Write/" + mod + "/" + queue;
		int cid = -1;
		if (cb){
			Post(endpoint, jsonString, U().RegisterCall(new UNestedCallBack(cb), cid));
		} else {
			Post(endpoint, jsonString, U().RegisterCall(new USilentCallBack(), cid));
		}
		
		if (cid == -1){
			Error2("[UF] Message Queue Write", "Error registering callback");
		}
		return cid;
	}
	
	/**
	 * Reset a queue - marks all existing messages as read for all readers
	 * @param mod The mod identifier
	 * @param queue The queue identifier
	 * @param cb Optional callback for reset confirmation
	 * @return Call ID or -1 on error
	 */
	int Reset(string mod, string queue, UFCallbackBase cb = NULL){
		if (mod == "" || queue == ""){
			Error2("[UF] Message Queue Reset", "mod and queue must be valid strings");
			return -1;
		}
		
		string endpoint = "Reset/" + mod + "/" + queue;
		int cid = -1;
		if (cb){
			Post(endpoint, "{}", U().RegisterCall(new UNestedCallBack(cb), cid));
		} else {
			Post(endpoint, "{}", U().RegisterCall(new USilentCallBack(), cid));
		}
		
		if (cid == -1){
			Error2("[UF] Message Queue Reset", "Error registering callback");
		}
		return cid;
	}
	
	/**
	 * Set queue metadata (order, player write permissions)
	 * @param mod The mod identifier
	 * @param queue The queue identifier
	 * @param obj The metadata configuration
	 * @param cb Optional callback for confirmation
	 * @return Call ID or -1 on error
	 */
	int SetMeta(string mod, string queue, UQueueMeta obj, UFCallbackBase cb = NULL){
		if (mod == "" || queue == ""){
			Error2("[UF] Message Queue SetMeta", "mod and queue must be valid strings");
			return -1;
		}
		if (!obj){
			Error2("[UF] Message Queue SetMeta", "UQueueMeta object is NULL");
			return -1;
		}
		
		string endpoint = "Meta/" + mod + "/" + queue;
		int cid = -1;
		if (cb){
			Post(endpoint, obj.ToJson(), U().RegisterCall(new UNestedCallBack(cb), cid));
		} else {
			Post(endpoint, obj.ToJson(), U().RegisterCall(new USilentCallBack(), cid));
		}
		
		if (cid == -1){
			Error2("[UF] Message Queue SetMeta", "Error registering callback");
		}
		return cid;
	}
	
	/**
	 * Purge old messages from a queue (server only)
	 * @param mod The mod identifier
	 * @param queue The queue identifier
	 * @param olderThanDays Delete messages older than this many days (default 30)
	 * @param cb Optional callback for confirmation
	 * @return Call ID or -1 on error
	 */
	int Purge(string mod, string queue, int olderThanDays = 30, UFCallbackBase cb = NULL){
		if (mod == "" || queue == ""){
			Error2("[UF] Message Queue Purge", "mod and queue must be valid strings");
			return -1;
		}
		
		string endpoint = "Purge/" + mod + "/" + queue;
		string body = "{\"OlderThanDays\":" + olderThanDays.ToString() + "}";
		int cid = -1;
		if (cb){
			Post(endpoint, body, U().RegisterCall(new UNestedCallBack(cb), cid));
		} else {
			Post(endpoint, body, U().RegisterCall(new USilentCallBack(), cid));
		}
		
		if (cid == -1){
			Error2("[UF] Message Queue Purge", "Error registering callback");
		}
		return cid;
	}
}
