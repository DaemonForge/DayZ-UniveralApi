
// ============ INTERNAL CALLBACKS ============

class UFAIChatAgentCreateCB extends UFCallbackBase {
    protected autoptr UFAIChatAgent m_Agent;

    void UFAIChatAgentCreateCB(Class instance, string function, string oid = ""){
        Class.CastTo(m_Agent, instance);
    }

    override void OnSuccess(string jsonData, int cid){
        UFLog.Debug("[UFAIChatAgentCreateCB] OnSuccess - CID: " + cid + ", DataLen: " + jsonData.Length().ToString());
        if (!m_Agent) return;
        // Parse ChatId from response
        autoptr UAIChatCreateResponse resp = new UAIChatCreateResponse;
        string error;
        JsonSerializer js = new JsonSerializer();
        if (js.ReadFromString(resp, jsonData, error) && resp.ChatId != ""){
            UFLog.Debug("[UFAIChatAgentCreateCB] Session created with ChatId: " + resp.ChatId);
            m_Agent.OnSessionCreated(resp.ChatId);
        } else {
            UFLog.Debug("[UFAIChatAgentCreateCB] Failed to parse ChatId: " + error);
            m_Agent.OnSessionCreateFailed("Failed to parse ChatId from response: " + error);
        }
    }

    override void OnError(int errorCode, int cid){
        UFLog.Debug("[UFAIChatAgentCreateCB] OnError - CID: " + cid + ", ErrorCode: " + errorCode);
        if (!m_Agent) return;
        m_Agent.OnSessionCreateFailed("Error code: " + errorCode);
    }
}

class UFAIChatAgentSendCB extends UFCallbackBase {
    protected autoptr UFAIChatAgent m_Agent;
    protected Class m_Handler;
    protected string m_HandlerFn;
    protected string m_PendingMessageId;
    protected int m_PollRetries;
    protected int m_MaxPollRetries;
    protected int m_ToolCallDepth;
    static const int MAX_TOOL_CALL_DEPTH = 10;

    void UFAIChatAgentSendCB(Class instance, string function, string oid = ""){
        Class.CastTo(m_Agent, instance);
        m_PendingMessageId = "";
        m_PollRetries = 0;
        m_MaxPollRetries = 90; // Default, will be overridden from agent
        m_ToolCallDepth = 0;
        if (m_Agent) {
            m_MaxPollRetries = m_Agent.GetPollTimeout();
        }
    }
    
    void Init(Class handler, string handlerFn){
        m_Handler = handler;
        m_HandlerFn = handlerFn;
    }
    
    void SetToolCallDepth(int depth){
        m_ToolCallDepth = depth;
    }
    
    // Direct callback to user when agent is unavailable - ensures callback always fires
    protected void CallHandlerDirect(int cid, int status, string data){
        if (!m_Handler || m_HandlerFn == "") return;
        g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallByName(m_Handler, m_HandlerFn, new Param4<int, int, string, string>(cid, status, "", data));
    }

    override void OnSuccess(string jsonData, int cid){
        UFLog.Debug("[UFAIChatAgentSendCB] OnSuccess - CID: " + cid + ", DataLen: " + jsonData.Length().ToString());
        if (!m_Agent) {
            CallHandlerDirect(cid, UF_ERROR, "Agent no longer available");
            return;
        }
        
        // First try to parse as tool call response
        autoptr UAIChatToolCallResponse toolResp = new UAIChatToolCallResponse;
        string error;
        JsonSerializer js = new JsonSerializer();
        
        if (js.ReadFromString(toolResp, jsonData, error) && toolResp && toolResp.Status == "ToolCall"){
            // Handle tool call
            UFLog.Debug("[UFAIChatAgentSendCB] Tool call received: " + toolResp.ToolName + ", Depth: " + m_ToolCallDepth);
            m_ToolCallDepth++;
            if (m_ToolCallDepth > MAX_TOOL_CALL_DEPTH){
                UFLog.Debug("[UFAIChatAgentSendCB] Max tool call depth exceeded!");
                m_Agent.OnMessageResponse(cid, UF_ERROR, "Maximum tool call depth reached", m_Handler, m_HandlerFn);
                return;
            }
            
            // Get param count for this tool and execute it
            int paramCount = m_Agent.GetToolParamCount(toolResp.ToolName);
            UFLog.Debug("[UFAIChatAgentSendCB] Executing tool: " + toolResp.ToolName + ", ParamCount: " + paramCount);
            string toolResult = m_Agent.ExecuteTool(toolResp.ToolName, toolResp.P1, toolResp.P2, toolResp.P3, toolResp.P4, toolResp.P5, paramCount);
            UFLog.Debug("[UFAIChatAgentSendCB] Tool result length: " + toolResult.Length().ToString());
            
            // Submit the result back to continue the conversation
            SubmitToolResultAndContinue(toolResp.MessageId, toolResp.ToolCallId, toolResult, cid);
            return;
        }
        
        // Not a tool call - parse as regular message response
        UFLog.Debug("[UFAIChatAgentSendCB] Parsing as regular message response");
        autoptr UAIChatMessageResponse resp = new UAIChatMessageResponse;
        if (!js.ReadFromString(resp, jsonData, error) || !resp){
            UFLog.Debug("[UFAIChatAgentSendCB] Failed to parse response: " + error);
            m_Agent.OnMessageResponse(cid, UF_JSONERROR, "Failed to parse response: " + error, m_Handler, m_HandlerFn);
            return;
        }
        
        // Handle different status responses
        if (resp.Status == "Success"){
            UFLog.Debug("[UFAIChatAgentSendCB] Success - Message length: " + resp.Message.Length().ToString());
            m_Agent.OnMessageResponse(cid, UF_SUCCESS, resp.Message, m_Handler, m_HandlerFn);
        } else if (resp.Status == "Pending" || resp.Status == "Wait"){
            // Start or continue polling
            // Only update MessageId if the response contains one - poll responses may not include it
            if (resp.MessageId != "") {
                m_PendingMessageId = resp.MessageId;
            }
            m_PollRetries++;
            UFLog.Debug("[UFAIChatAgentSendCB] " + resp.Status + " - MessageId: " + m_PendingMessageId + ", Retry: " + m_PollRetries + "/" + m_MaxPollRetries);
            if (m_PollRetries > m_MaxPollRetries){
                UFLog.Debug("[UFAIChatAgentSendCB] Max poll retries exceeded, timing out");
                m_Agent.OnMessageResponse(cid, UF_TIMEOUT, "AI response timed out", m_Handler, m_HandlerFn);
                return;
            }
            // Create a NEW callback for polling IMMEDIATELY - don't use CallLater on 'this'
            // because 'this' will be deleted after OnSuccess returns (UNestedCallBack cleanup).
            // The new callback will schedule its own delayed poll.
            autoptr UFAIChatAgentSendCB pollCB = new UFAIChatAgentSendCB(m_Agent, "");
            pollCB.Init(m_Handler, m_HandlerFn);
            pollCB.SetPollState(m_PendingMessageId, m_PollRetries, m_ToolCallDepth, m_MaxPollRetries);
            pollCB.ScheduleDelayedPoll(cid);
        } else if (resp.Status == "NotFound"){
            UFLog.Debug("[UFAIChatAgentSendCB] NotFound status received");
            m_Agent.OnMessageResponse(cid, UF_NOTFOUND, "Message not found", m_Handler, m_HandlerFn);
        } else if (resp.Status == "ToolCall"){
            // Also handle ToolCall status from regular response (shouldn't happen but handle it)
            UFLog.Debug("[UFAIChatAgentSendCB] Unexpected ToolCall status in message response");
            m_Agent.OnMessageResponse(cid, UF_ERROR, "Unexpected ToolCall status in message response", m_Handler, m_HandlerFn);
        } else {
            // Error or unknown status
            UFLog.Debug("[UFAIChatAgentSendCB] Error or unknown status: " + resp.Status);
            m_Agent.OnMessageResponse(cid, UF_ERROR, "Status: " + resp.Status, m_Handler, m_HandlerFn);
        }
    }
    
    protected void SubmitToolResultAndContinue(string messageId, string toolCallId, string result, int cid){
        UFLog.Debug("[UFAIChatAgentSendCB] SubmitToolResultAndContinue - MessageId: " + messageId + ", ToolCallId: " + toolCallId);
        if (!m_Agent) {
            CallHandlerDirect(cid, UF_ERROR, "Agent no longer available");
            return;
        }
        
        // Create a callback that continues with the same handler
        autoptr UFAIChatAgentSendCB continueCB = new UFAIChatAgentSendCB(m_Agent, "");
        continueCB.Init(m_Handler, m_HandlerFn);
        continueCB.SetToolCallDepth(m_ToolCallDepth);
        
        // Submit the tool result
        UFAIChatEndpoint ai = U().AI();
        ai.SubmitToolResult(messageId, toolCallId, result, continueCB);
    }
    
    void SetPollState(string messageId, int pollRetries, int toolCallDepth, int maxRetries = 90){
        m_PendingMessageId = messageId;
        m_PollRetries = pollRetries;
        m_ToolCallDepth = toolCallDepth;
        m_MaxPollRetries = maxRetries;
    }
    
    // Schedule a delayed poll using a static reference to prevent premature deletion
    void ScheduleDelayedPoll(int cid){
        UFLog.Debug("[UFAIChatAgentSendCB] ScheduleDelayedPoll - CID: " + cid + ", MessageId: " + m_PendingMessageId);
        // Store a reference to prevent garbage collection until CallLater fires
        // The agent keeps us alive by holding the pending poll reference
        m_Agent.SetPendingPollCallback(this);
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(ExecutePoll, 1000, false, cid);
    }
    
    protected void ExecutePoll(int cid){
        UFLog.Debug("[UFAIChatAgentSendCB] ExecutePoll - CID: " + cid + ", MessageId: " + m_PendingMessageId);
        // Clear the agent's reference to us since we're about to make the API call
        if (m_Agent) {
            m_Agent.SetPendingPollCallback(NULL);
        }
        
        if (!m_Agent) {
            UFLog.Debug("[UFAIChatAgentSendCB] ExecutePoll - Agent is null, failing safely");
            CallHandlerDirect(cid, UF_ERROR, "Agent no longer available");
            return;
        }
        
        if (m_PendingMessageId == "") {
            UFLog.Debug("[UFAIChatAgentSendCB] ExecutePoll - MessageId is empty, failing safely");
            CallHandlerDirect(cid, UF_ERROR, "No message ID to poll");
            return;
        }
        
        UFAIChatEndpoint ai = U().AI();
        // 'this' will be wrapped in UNestedCallBack and deleted after the call completes
        // That's fine - OnSuccess will create a new callback for the next poll if needed
        ai.MessageStatus(m_PendingMessageId, this);
    }

    override void OnError(int errorCode, int cid){
        UFLog.Debug("[UFAIChatAgentSendCB] OnError - CID: " + cid + ", ErrorCode: " + errorCode);
        if (!m_Agent) {
            // Agent gone but we still need to notify the handler
            CallHandlerDirect(cid, errorCode, "Error code: " + errorCode);
            return;
        }
        m_Agent.OnMessageResponse(cid, errorCode, "Error code: " + errorCode, m_Handler, m_HandlerFn);
    }
}