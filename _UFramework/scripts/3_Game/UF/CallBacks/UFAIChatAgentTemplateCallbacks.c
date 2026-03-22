// ============ TEMPLATED CALLBACKS ============

class UAIChatAgentCreateCB<Class T> extends UFCallbackBase {

    void UAIChatAgentCreateCB(Class instance, string function, string oid = ""){
        // instance is stored in parent's Instance field
    }
    
    protected UAIChatAgent<T> GetAgent(){
        UAIChatAgent<T> agent;
        Class.CastTo(agent, Instance);
        return agent;
    }

    override void OnSuccess(string jsonData, int cid){
        UAIChatAgent<T> agent = GetAgent();
        if (!agent) return;
        autoptr UAIChatCreateResponse resp = new UAIChatCreateResponse;
        string error;
        JsonSerializer js = new JsonSerializer();
        if (js.ReadFromString(resp, jsonData, error) && resp.ChatId != ""){
            agent.OnSessionCreated(resp.ChatId);
        } else {
            agent.OnSessionCreateFailed("Failed to parse ChatId from response: " + error);
        }
    }

    override void OnError(int errorCode, int cid){
        UAIChatAgent<T> agent = GetAgent();
        if (!agent) return;
        agent.OnSessionCreateFailed("Error code: " + errorCode);
    }
}

class UAIChatAgentSendCB<Class T> extends UFCallbackBase {
    protected Class m_Handler;
    protected string m_HandlerFn;
    protected string m_PendingMessageId;
    protected int m_PollRetries;
    protected int m_MaxPollRetries;
    protected int m_ToolCallDepth;
    static const int MAX_TOOL_CALL_DEPTH = 10;

    void UAIChatAgentSendCB(Class instance, string function, string oid = ""){
        // instance is stored in parent's Instance field
        m_PendingMessageId = "";
        m_PollRetries = 0;
        m_MaxPollRetries = 90; // Default, will be overridden from agent
        m_ToolCallDepth = 0;
        UAIChatAgent<T> agent = GetAgent();
        if (agent) {
            m_MaxPollRetries = agent.GetPollTimeout();
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
    protected void CallHandlerDirect(int cid, int status){
        if (!m_Handler || m_HandlerFn == "") return;
        g_Game.GameScript.CallFunctionParams(m_Handler, m_HandlerFn, NULL, new Param4<int, int, string, T>(cid, status, "", NULL));
    }
    
    protected UAIChatAgent<T> GetAgent(){
        UAIChatAgent<T> agent;
        Class.CastTo(agent, Instance);
        return agent;
    }

    override void OnSuccess(string jsonData, int cid){
        UFLog.Debug("[UAIChatAgentSendCB<T>] OnSuccess - CID: " + cid + ", DataLen: " + jsonData.Length().ToString());
        UAIChatAgent<T> agent = GetAgent();
        if (!agent) {
            CallHandlerDirect(cid, UF_ERROR);
            return;
        }
        
        // First try to parse as tool call response
        autoptr UAIChatToolCallResponse toolResp = new UAIChatToolCallResponse;
        string error;
        JsonSerializer js = new JsonSerializer();
        
        if (js.ReadFromString(toolResp, jsonData, error) && toolResp && toolResp.Status == "ToolCall"){
            // Handle tool call
            UFLog.Debug("[UAIChatAgentSendCB<T>] Tool call detected: " + toolResp.ToolName + ", Depth: " + m_ToolCallDepth);
            m_ToolCallDepth++;
            if (m_ToolCallDepth > MAX_TOOL_CALL_DEPTH){
                UFLog.Debug("[UAIChatAgentSendCB<T>] Max tool call depth exceeded!");
                agent.OnMessageResponse(cid, UF_ERROR, NULL, "Maximum tool call depth reached", m_Handler, m_HandlerFn);
                return;
            }
            
            // Get param count for this tool and execute it
            int paramCount = agent.GetToolParamCount(toolResp.ToolName);
            UFLog.Debug("[UAIChatAgentSendCB<T>] Executing tool: " + toolResp.ToolName + ", ParamCount: " + paramCount);
            string toolResult = agent.ExecuteTool(toolResp.ToolName, toolResp.P1, toolResp.P2, toolResp.P3, toolResp.P4, toolResp.P5, paramCount);
            UFLog.Debug("[UAIChatAgentSendCB<T>] Tool result length: " + toolResult.Length().ToString());
            
            // Submit the result back to continue the conversation
            SubmitToolResultAndContinue(toolResp.MessageId, toolResp.ToolCallId, toolResult, cid);
            return;
        }
        
        // Not a tool call - parse as regular message response
        UFLog.Debug("[UAIChatAgentSendCB<T>] Parsing as regular message response");
        autoptr UAIChatMessageResponse resp = new UAIChatMessageResponse;
        if (!js.ReadFromString(resp, jsonData, error) || !resp){
            UFLog.Debug("[UAIChatAgentSendCB<T>] Failed to parse response: " + error);
            agent.OnMessageResponse(cid, UF_JSONERROR, NULL, "Failed to parse response", m_Handler, m_HandlerFn);
            return;
        }
        
        UFLog.Debug("[UAIChatAgentSendCB<T>] Response status: " + resp.Status + ", MessageId: " + resp.MessageId);
        // Handle different status responses
        if (resp.Status == "Success"){
            // Parse the Message field (AI's JSON response) into type T
            string aiMessage = resp.Message;
            autoptr T typedResponse;
            if (UJSONHandler<T>.FromString(aiMessage, typedResponse)){
                agent.OnMessageResponse(cid, UF_SUCCESS, typedResponse, aiMessage, m_Handler, m_HandlerFn);
            } else {
                Error2("[UF][UAIChatAgentSendCB<T>]", "Failed to parse AI response into type T: " + aiMessage);
                agent.OnMessageResponse(cid, UF_JSONERROR, NULL, aiMessage, m_Handler, m_HandlerFn);
            }
        } else if (resp.Status == "Pending" || resp.Status == "Wait"){
            // Start or continue polling
            // Only update MessageId if the response contains one - poll responses may not include it
            if (resp.MessageId != "") {
                m_PendingMessageId = resp.MessageId;
            }
            m_PollRetries++;
            UFLog.Debug("[UAIChatAgentSendCB<T>] " + resp.Status + " - MessageId: " + m_PendingMessageId + ", Retry: " + m_PollRetries + "/" + m_MaxPollRetries);
            if (m_PollRetries > m_MaxPollRetries){
                UFLog.Debug("[UAIChatAgentSendCB<T>] Max poll retries exceeded, timing out");
                agent.OnMessageResponse(cid, UF_TIMEOUT, NULL, "", m_Handler, m_HandlerFn);
                return;
            }
            // Create a NEW callback for polling IMMEDIATELY - don't use CallLater on 'this'
            // because 'this' will be deleted after OnSuccess returns (UNestedCallBack cleanup).
            // The new callback will schedule its own delayed poll.
            autoptr UAIChatAgentSendCB<T> pollCB = new UAIChatAgentSendCB<T>(agent, "");
            pollCB.Init(m_Handler, m_HandlerFn);
            pollCB.SetPollState(m_PendingMessageId, m_PollRetries, m_ToolCallDepth, m_MaxPollRetries);
            pollCB.ScheduleDelayedPoll(agent, cid);
        } else if (resp.Status == "NotFound"){
            UFLog.Debug("[UAIChatAgentSendCB<T>] NotFound status received");
            agent.OnMessageResponse(cid, UF_NOTFOUND, NULL, "", m_Handler, m_HandlerFn);
        } else if (resp.Status == "ToolCall"){
            // Should have been caught above, but handle edge case
            UFLog.Debug("[UAIChatAgentSendCB<T>] Unexpected ToolCall status in message response");
            agent.OnMessageResponse(cid, UF_ERROR, NULL, "Unexpected ToolCall status", m_Handler, m_HandlerFn);
        } else {
            // Error or unknown status
            UFLog.Debug("[UAIChatAgentSendCB<T>] Error or unknown status: " + resp.Status);
            agent.OnMessageResponse(cid, UF_ERROR, NULL, "", m_Handler, m_HandlerFn);
        }
    }
    
    protected void SubmitToolResultAndContinue(string messageId, string toolCallId, string result, int cid){
        UFLog.Debug("[UAIChatAgentSendCB<T>] SubmitToolResultAndContinue - MessageId: " + messageId + ", ToolCallId: " + toolCallId);
        UAIChatAgent<T> agent = GetAgent();
        if (!agent) {
            CallHandlerDirect(cid, UF_ERROR);
            return;
        }
        
        // Create a callback that continues with the same handler
        autoptr UAIChatAgentSendCB<T> continueCB = new UAIChatAgentSendCB<T>(agent, "");
        continueCB.Init(m_Handler, m_HandlerFn);
        continueCB.SetToolCallDepth(m_ToolCallDepth);
        
        // Submit the tool result
        UFAIChatEndpoint ai = UF().AI();
        ai.SubmitToolResult(messageId, toolCallId, result, continueCB);
    }
    
    void SetPollState(string messageId, int pollRetries, int toolCallDepth, int maxRetries = 90){
        m_PendingMessageId = messageId;
        m_PollRetries = pollRetries;
        m_ToolCallDepth = toolCallDepth;
        m_MaxPollRetries = maxRetries;
    }
    
    // Schedule a delayed poll using the agent to hold our reference
    void ScheduleDelayedPoll(UAIChatAgent<T> agent, int cid){
        UFLog.Debug("[UAIChatAgentSendCB<T>] ScheduleDelayedPoll - CID: " + cid + ", MessageId: " + m_PendingMessageId);
        // Store a reference to prevent garbage collection until CallLater fires
        agent.SetPendingPollCallback(this);
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(ExecutePoll, 1000, false, cid);
    }
    
    protected void ExecutePoll(int cid){
        UFLog.Debug("[UAIChatAgentSendCB<T>] ExecutePoll - CID: " + cid + ", MessageId: " + m_PendingMessageId);
        UAIChatAgent<T> agent = GetAgent();
        
        // Clear the agent's reference to us since we're about to make the API call
        if (agent) {
            agent.SetPendingPollCallback(NULL);
        }
        
        if (!agent) {
            UFLog.Debug("[UAIChatAgentSendCB<T>] ExecutePoll - Agent is null, failing safely");
            CallHandlerDirect(cid, UF_ERROR);
            return;
        }
        
        if (m_PendingMessageId == "") {
            UFLog.Debug("[UAIChatAgentSendCB<T>] ExecutePoll - MessageId is empty, failing safely");
            CallHandlerDirect(cid, UF_ERROR);
            return;
        }
        
        UFAIChatEndpoint ai = UF().AI();
        // 'this' will be wrapped in UNestedCallBack and deleted after the call completes
        // That's fine - OnSuccess will create a new callback for the next poll if needed
        ai.MessageStatus(m_PendingMessageId, this);
    }

    override void OnError(int errorCode, int cid){
        UFLog.Debug("[UAIChatAgentSendCB<T>] OnError - CID: " + cid + ", ErrorCode: " + errorCode);
        UAIChatAgent<T> agent = GetAgent();
        if (!agent) {
            // Agent gone but we still need to notify the handler
            CallHandlerDirect(cid, errorCode);
            return;
        }
        agent.OnMessageResponse(cid, errorCode, NULL, "", m_Handler, m_HandlerFn);
    }
}
