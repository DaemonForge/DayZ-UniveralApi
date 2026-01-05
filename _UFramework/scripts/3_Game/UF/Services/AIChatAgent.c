/**
 * UAIChatHistoryEntry - stores a single message in chat history with role.
 */
class UAIChatHistoryEntry extends Managed {
    string Role;    // "user" or "assistant"
    string Message;

    void UAIChatHistoryEntry(string role, string message){
        Role = role;
        Message = message;
    }
}

/**
 * UAIChatToolParam - defines a single parameter with name and optional type.
 * Supported types: "string" (default), "int", "float", "bool", "vector"
 * 
 * For OpenAI JSON Schema:
 *   - "string" -> { type: "string" }
 *   - "int" -> { type: "integer" }
 *   - "float" -> { type: "number" }
 *   - "bool" -> { type: "boolean" }
 *   - "vector" -> { type: "string", description: "... as 'x y z'" } (sent as space-separated string)
 */
class UAIChatToolParam extends Managed {
    string Name;
    string Type;
    string Desc;
    
    void UAIChatToolParam(string name, string type = "string", string desc = ""){
        Name = name;
        Type = type;
        Desc = desc;
    }
}

/**
 * UAIChatToolDef - tool definition for AI chat agents.
 * Name = function name on the agent class (must return string)
 * Description = what the tool does (for AI)
 * Parameters = array of parameter names/types the tool accepts
 *
 * Simple usage (all strings):
 *   new UAIChatToolDef("GetHealth", "Get player health", {"playerName"})
 *
 * Typed usage:
 *   autoptr array<autoptr UAIChatToolParam> params = new array<autoptr UAIChatToolParam>;
 *   params.Insert(new UAIChatToolParam("playerName", "string"));
 *   params.Insert(new UAIChatToolParam("amount", "int"));
 *   new UAIChatToolDef("SetHealth", "Set player health", params)
 */
class UAIChatToolDef extends Managed {
    string Name;
    string Description;
    autoptr array<string> Parameters;
    autoptr array<string> ParameterTypes;
    autoptr array<string> ParameterDescs;

    // Simple constructor - all params are strings
    void UAIChatToolDef(string name, string desc, array<string> params = NULL){
        Name = name;
        Description = desc;
        ParameterTypes = new array<string>;
        ParameterDescs = new array<string>;
        if (params){
            Parameters = new array<string>;
            Parameters.Copy(params);
            for (int i = 0; i < params.Count(); i++){
                ParameterTypes.Insert("string");
                ParameterDescs.Insert("");
            }
        }
    }
    
    // Typed constructor - specify types per parameter
    static UAIChatToolDef CreateTyped(string name, string desc, array<autoptr UAIChatToolParam> params){
        autoptr UAIChatToolDef def = new UAIChatToolDef(name, desc, NULL);
        if (params){
            def.Parameters = new array<string>;
            def.ParameterTypes = new array<string>;
            def.ParameterDescs = new array<string>;
            foreach (autoptr UAIChatToolParam p : params){
                if (p){
                    def.Parameters.Insert(p.Name);
                    def.ParameterTypes.Insert(p.Type);
                    def.ParameterDescs.Insert(p.Desc);
                }
            }
        }
        return def;
    }

    int ParamCount(){
        if (!Parameters) return 0;
        return Parameters.Count();
    }

    array<string> GetParamNames(){
        if (!Parameters) return new array<string>;
        return Parameters;
    }
    
    array<string> GetParamTypes(){
        if (!ParameterTypes) return new array<string>;
        return ParameterTypes;
    }
    
    array<string> GetParamDescs(){
        if (!ParameterDescs) return new array<string>;
        return ParameterDescs;
    }
}

/**
 * UAIChatToolParams - helper class for parsing tool parameters.
 * Use these static methods to convert string parameters to proper types.
 *
 * Example:
 *   string Teleport(string playerName, string position){
 *       vector pos = UAIChatToolParams.Vec(position);  // "123.5 51.0 45.2" -> vector
 *       return "Done";
 *   }
 */
class UAIChatToolParams {
    // Parse string to int (returns defaultVal on empty/invalid)
    static int Int(string val, int defaultVal = 0){
        if (val == "") return defaultVal;
        return val.ToInt();
    }
    
    // Parse string to float (returns defaultVal on empty/invalid)
    static float Float(string val, float defaultVal = 0.0){
        if (val == "") return defaultVal;
        return val.ToFloat();
    }
    
    // Parse string to bool ("true", "1", "yes" = true)
    static bool Bool(string val, bool defaultVal = false){
        if (val == "") return defaultVal;
        string lower = val;
        lower.ToLower();
        return lower == "true" || lower == "1" || lower == "yes";
    }
    
    // Parse "x y z" string to vector
    static vector Vec(string val){
        if (val == "") return vector.Zero;
        return val.ToVector();
    }
    
    // Parse 3 separate strings to vector
    static vector Vec3(string x, string y, string z){
        return Vector(x.ToFloat(), y.ToFloat(), z.ToFloat());
    }
}

/**
 * UFAIChatAgent - base class for per-mod AI chat agents.
 *
 * Modders extend this class and:
 *   1) Override SystemInstructions() to return their system prompt
 *   2) Override RegisterTools(out array<autoptr UAIChatToolDef> tools) to register tool functions
 *   3) Define methods matching your tool names (framework calls them automatically)
 *   4) Call Chat(input, handler, handlerFn) to send messages with familiar callback pattern
 *
 * Basic Example (string params):
 *   class MyAI extends UFAIChatAgent {
 *       override string SystemInstructions(){ return "You are a helpful assistant."; }
 *       override void RegisterTools(out array<autoptr UAIChatToolDef> tools){
 *           tools.Insert(new UAIChatToolDef("Echo", "Echoes text back", {"text"}));
 *       }
 *       string Echo(string text){ return "Echo: " + text; }  // Called automatically!
 *   }
 *
 * Typed Example (int, float, vector, etc.):
 *   override void RegisterTools(out array<autoptr UAIChatToolDef> tools){
 *       // Create typed parameters
 *       autoptr array<autoptr UAIChatToolParam> teleportParams = new array<autoptr UAIChatToolParam>;
 *       teleportParams.Insert(new UAIChatToolParam("playerName", "string", "Name of the player"));
 *       teleportParams.Insert(new UAIChatToolParam("position", "vector", "Target position"));
 *       tools.Insert(UAIChatToolDef.CreateTyped("Teleport", "Teleport a player", teleportParams));
 *   }
 *   
 *   string Teleport(string playerName, string position){
 *       vector pos = UAIChatToolParams.Vec(position);  // "123.5 51.0 45.2" -> vector
 *       // ... teleport logic ...
 *       return "Teleported " + playerName + " to " + pos.ToString();
 *   }
 *
 * Supported parameter types: "string", "int", "float", "bool", "vector"
 *
 * Usage:
 *   autoptr MyAI ai = new MyAI();
 *   ai.Chat("Hello", this, "OnAIResponse");
 *
 *   void OnAIResponse(int cid, int status, string oid, string data){ ... }
 */
class UFAIChatAgent extends Managed {

    protected string m_ChatId;
    protected string m_KBId;
    protected bool m_Ready;
    protected bool m_IncludeHistory;
    protected int m_MaxHistory;
    protected autoptr array<autoptr UAIChatHistoryEntry> m_History;
    protected autoptr array<autoptr UAIChatContext> m_StaticContext;
    protected autoptr array<autoptr UAIChatToolDef> m_Tools;

    // Pending chat request while session is being created
    protected string m_PendingMessage;
    protected Class m_PendingHandler;
    protected string m_PendingHandlerFn;
    
    // Holds the current polling callback to prevent garbage collection during CallLater delay
    protected autoptr UFCallbackBase m_PendingPollCallback;
    
    // Configurable poll timeout in seconds (default 90)
    protected int m_PollTimeout;

    void UFAIChatAgent(){
        m_Ready = false;
        m_IncludeHistory = true;
        m_MaxHistory = 25;
        m_KBId = "";
        m_PollTimeout = 90;
        m_History = new array<autoptr UAIChatHistoryEntry>;
        m_StaticContext = new array<autoptr UAIChatContext>;
        m_Tools = new array<autoptr UAIChatToolDef>;

        // Let subclass register tools
        RegisterTools(m_Tools);
    }
    
    // Called by polling callbacks to keep themselves alive during CallLater delay
    void SetPendingPollCallback(UFCallbackBase cb){
        m_PendingPollCallback = cb;
    }
    
    // Set the polling timeout in seconds (default 90). Call before Chat() or override in subclass.
    void SetPollTimeout(int seconds){
        m_PollTimeout = Math.Max(10, seconds); // Minimum 10 seconds
    }
    
    // Get the current poll timeout
    int GetPollTimeout(){
        return m_PollTimeout;
    }

    // ============ OVERRIDE HOOKS ============

    // Override to provide system instructions for this agent.
    string SystemInstructions(){ return ""; }

    // Override to register tool functions. Each tool name should match a method on your class.
    // Example: RegisterTools adds "GetPlayerHealth" -> you define: string GetPlayerHealth(string playerName)
    void RegisterTools(out array<autoptr UAIChatToolDef> tools){}

    // Override to provide dynamic context lines added to every message.
    array<string> ExtraContext(){ return NULL; }

    // Override to provide system context items (static knowledge, rules, etc).
    array<string> SystemContext(){ return NULL; }

    // Override to access or transform history before sending. Return the history entries.
    array<autoptr UAIChatHistoryEntry> GetHistory(){ return m_History; }

    // Override to specify the AI model to use. Return empty string for default (gpt-4o-mini).
    // Available models: "gpt-4o", "gpt-4o-mini", "gpt-4-turbo", "gpt-3.5-turbo", "o1", "o1-mini", "o3-mini"
    string GetModel(){ return ""; }
    
    // ============ TOOL DISPATCH ============
    
    /**
     * Executes a tool by calling the method with the matching name.
     * The framework calls this automatically - you just define methods matching your tool names.
     * 
     * Example: If you register "GetPlayerHealth", define:
     *   string GetPlayerHealth(string playerName) { return "Health is 100%"; }
     * 
     * Supports 0-5 parameters. All parameters are strings.
     */
    string ExecuteTool(string toolName, string p1, string p2, string p3, string p4, string p5, int paramCount){
        string result = "";
        bool success = false;
        
        // Try to call the method by name with the appropriate number of parameters
        switch (paramCount) {
            case 0:
                success = GetGame().GameScript.CallFunctionParams(this, toolName, result, NULL);
                break;
            case 1:
                success = GetGame().GameScript.CallFunctionParams(this, toolName, result, new Param1<string>(p1));
                break;
            case 2:
                success = GetGame().GameScript.CallFunctionParams(this, toolName, result, new Param2<string, string>(p1, p2));
                break;
            case 3:
                success = GetGame().GameScript.CallFunctionParams(this, toolName, result, new Param3<string, string, string>(p1, p2, p3));
                break;
            case 4:
                success = GetGame().GameScript.CallFunctionParams(this, toolName, result, new Param4<string, string, string, string>(p1, p2, p3, p4));
                break;
            case 5:
                success = GetGame().GameScript.CallFunctionParams(this, toolName, result, new Param5<string, string, string, string, string>(p1, p2, p3, p4, p5));
                break;
        }
        
        if (!success) {
            Error2("[UF][AIChatAgent] ExecuteTool", "Method '" + toolName + "' not found or failed. Make sure you defined: string " + toolName + "(...)");
            return "Error: Tool '" + toolName + "' not implemented";
        }
        
        return result;
    }
    
    /**
     * Gets the parameter count for a registered tool.
     */
    int GetToolParamCount(string toolName){
        foreach (autoptr UAIChatToolDef tool : m_Tools){
            if (tool && tool.Name == toolName){
                return tool.ParamCount();
            }
        }
        return 0;
    }

    // ============ CONFIGURATION ============

    void SetIncludeHistory(bool includeHistory, int maxHistory = 25){
        m_IncludeHistory = includeHistory;
        m_MaxHistory = maxHistory;
    }

    /**
     * Sets the Knowledge Base ID for this agent.
     * When set, the agent will automatically have access to a KB search tool
     * that allows it to query the knowledge base for relevant information.
     * The KB must be created and configured on the service side.
     * @param kbId - The Knowledge Base ID to use
     */
    void SetKBId(string kbId){
        m_KBId = kbId;
    }

    /**
     * Gets the currently configured Knowledge Base ID.
     * @return The KB ID, or empty string if not set
     */
    string GetKBId(){
        return m_KBId;
    }

    void AddStaticContext(string description, array<string> items){
        if (description == "" || !items) return;
        autoptr UAIChatContext ctx = new UAIChatContext(description);
        foreach (string it : items){
            if (it != "") ctx.AddContext(it);
        }
        m_StaticContext.Insert(ctx);
    }

    // ============ MAIN API ============

    /**
     * Send a chat message. Callback signature: void handler(int cid, int status, string oid, string data)
     * If the session is not yet created, it will be created automatically.
     */
    void Chat(string input, Class handler, string handlerFn){
        UFLog.Debug("[AIChatAgent] Chat - Input length: " + input.Length().ToString() + ", Ready: " + m_Ready.ToString() + ", KBId: " + m_KBId);
        
        if (!U().IsOpenAIEnabled()){
            Error2("[UF][AIChatAgent] Chat", "OpenAI service is not online");
            CallHandlerError(handler, handlerFn, -1, "OpenAI service is not online");
            return;
        }

        if (!m_Ready){
            // Need to create session first
            UFLog.Debug("[AIChatAgent] Chat - Session not ready, creating...");
            m_PendingMessage = input;
            m_PendingHandler = handler;
            m_PendingHandlerFn = handlerFn;
            CreateSession();
            return;
        }

        SendMessage(input, handler, handlerFn);
    }

    // ============ INTERNAL ============

    protected void CreateSession(){
        UFLog.Debug("[AIChatAgent] CreateSession - Starting, KBId: " + m_KBId);
        UFAIChatEndpoint ai = U().AI();
        int cid = ai.Create(SystemInstructions(), "string", "", GetModel(), m_MaxHistory, new UFAIChatAgentCreateCB(this, ""), m_KBId);
        if (cid == -1){
            Error2("[UF][AIChatAgent] CreateSession", "Failed to create AI chat session");
            CallHandlerError(m_PendingHandler, m_PendingHandlerFn, -1, "Failed to create session");
            ClearPending();
        } else {
            UFLog.Debug("[AIChatAgent] CreateSession - Request sent, CID: " + cid);
        }
    }

    void OnSessionCreated(string chatId){
        string hasPending = "no";
        if (m_PendingMessage != "") hasPending = "yes";
        UFLog.Debug("[AIChatAgent] OnSessionCreated - ChatId: " + chatId + ", PendingMessage: " + hasPending);
        m_ChatId = chatId;
        m_Ready = true;

        if (m_PendingMessage != "" && m_PendingHandler){
            UFLog.Debug("[AIChatAgent] OnSessionCreated - Sending pending message");
            SendMessage(m_PendingMessage, m_PendingHandler, m_PendingHandlerFn);
            ClearPending();
        }
    }

    void OnSessionCreateFailed(string error){
        UFLog.Debug("[AIChatAgent] OnSessionCreateFailed - Error: " + error);
        Error2("[UF][AIChatAgent] OnSessionCreateFailed", error);
        CallHandlerError(m_PendingHandler, m_PendingHandlerFn, -1, error);
        ClearPending();
    }

    protected void ClearPending(){
        m_PendingMessage = "";
        m_PendingHandler = NULL;
        m_PendingHandlerFn = "";
    }

    protected void SendMessage(string input, Class handler, string handlerFn){
        UFLog.Debug("[AIChatAgent] SendMessage - ChatId: " + m_ChatId + ", InputLen: " + input.Length().ToString());
        
        autoptr array<autoptr UAIChatContext> ctx = BuildContext();
        autoptr array<autoptr UAIToolDef> tools = BuildToolDefs();
        
        if (ctx) {
            UFLog.Debug("[AIChatAgent] SendMessage - Context blocks: " + ctx.Count().ToString());
        }
        if (tools) {
            UFLog.Debug("[AIChatAgent] SendMessage - Tools: " + tools.Count().ToString());
        }

        UFAIChatEndpoint ai = U().AI();
        autoptr UFAIChatAgentSendCB cb = new UFAIChatAgentSendCB(this, "");
        cb.Init(handler, handlerFn);
        int cid = ai.Send(m_ChatId, input, cb, ctx, tools);

        if (cid == -1){
            CallHandlerError(handler, handlerFn, -1, "Failed to send message");
            return;
        }
        
        UFLog.Debug("[AIChatAgent] SendMessage - Sent, CID: " + cid);

        if (m_IncludeHistory){
            m_History.Insert(new UAIChatHistoryEntry("user", input));
            TrimHistory();
        }
    }

    void OnMessageResponse(int cid, int status, string data, Class handler, string handlerFn){
        // Tool calls are now handled by native OpenAI function calling in the callback.
        // This method only receives final responses.
        UFLog.Debug("[AIChatAgent] OnMessageResponse - CID: " + cid + ", Status: " + status + ", DataLen: " + data.Length().ToString());
        
        if (m_IncludeHistory && data != "" && status == UF_SUCCESS){
            m_History.Insert(new UAIChatHistoryEntry("assistant", data));
            TrimHistory();
        }
        CallHandler(handler, handlerFn, cid, status, m_ChatId, data);
    }

    protected autoptr array<autoptr UAIChatContext> BuildContext(){
        autoptr array<autoptr UAIChatContext> ctx = new array<autoptr UAIChatContext>;

        // System context from override
        array<string> sysCtx = SystemContext();
        if (sysCtx && sysCtx.Count() > 0){
            autoptr UAIChatContext sys = new UAIChatContext("system");
            foreach (string s : sysCtx){
                if (s != "") sys.AddContext(s);
            }
            ctx.Insert(sys);
        }

        // Static context blocks
        foreach (autoptr UAIChatContext c : m_StaticContext){
            ctx.Insert(c);
        }

        // Extra dynamic context from override
        array<string> extra = ExtraContext();
        if (extra && extra.Count() > 0){
            autoptr UAIChatContext ex = new UAIChatContext("extra");
            foreach (string e : extra){
                if (e != "") ex.AddContext(e);
            }
            ctx.Insert(ex);
        }

        // Chat history with roles
        array<autoptr UAIChatHistoryEntry> history = GetHistory();
        if (m_IncludeHistory && history && history.Count() > 0){
            autoptr UAIChatContext hist = new UAIChatContext("conversation_history");
            foreach (autoptr UAIChatHistoryEntry h : history){
                if (h) hist.AddContext(h.Role + ": " + h.Message);
            }
            ctx.Insert(hist);
        }

        if (ctx.Count() == 0) return NULL;
        return ctx;
    }

    protected autoptr array<autoptr UAIToolDef> BuildToolDefs(){
        if (!m_Tools || m_Tools.Count() == 0) return NULL;
        autoptr array<autoptr UAIToolDef> defs = new array<autoptr UAIToolDef>;
        foreach (autoptr UAIChatToolDef t : m_Tools){
            if (!t || t.Name == "") continue;
            autoptr UAIToolDef toolDef = new UAIToolDef(t.Name, t.Description, t.GetParamNames(), t.GetParamTypes(), t.GetParamDescs());
            defs.Insert(toolDef);
        }
        if (defs.Count() == 0) return NULL;
        return defs;
    }

    protected void TrimHistory(){
        while (m_History.Count() > m_MaxHistory && m_MaxHistory > 0){
            m_History.RemoveOrdered(0);
        }
    }

    // ============ CALLBACK HELPERS ============

    protected void CallHandler(Class handler, string handlerFn, int cid, int status, string oid, string data){
        if (!handler || handlerFn == "") return;
        g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallByName(handler, handlerFn, new Param4<int, int, string, string>(cid, status, oid, data));
    }

    protected void CallHandlerError(Class handler, string handlerFn, int cid, string error){
        CallHandler(handler, handlerFn, cid, -1, "", error);
    }
}

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

// ============================================================================
// TEMPLATED AI CHAT AGENT - Returns typed objects instead of strings
// ============================================================================

/**
 * UAIChatAgent<T> - Template AI chat agent that returns typed objects.
 *
 * Similar to UDBHandler<T>, this class allows you to define an AI agent that
 * returns structured data parsed into your class type T.
 *
 * IMPORTANT: You MUST call SetSchema() before calling Chat() to provide a valid
 * JSON Schema that defines the expected response format.
 *
 * Usage:
 *   // Define your response class
 *   class MyAIResponse {
 *       string Action;
 *       int Priority;
 *       array<string> Items;
 *   }
 *
 *   // Create a typed agent
 *   class MyTypedAI extends UAIChatAgent<MyAIResponse> {
 *       override string SystemInstructions(){ 
 *           return "You are a helpful assistant that responds with structured data."; 
 *       }
 *   }
 *
 *   // Use it - MUST set schema first!
 *   // NOTE: OpenAI strict mode requires "additionalProperties": false
 *   autoptr MyTypedAI ai = new MyTypedAI();
 *   ai.SetSchema("MyAIResponse", "{\"type\":\"object\",\"properties\":{\"Action\":{\"type\":\"string\"},\"Priority\":{\"type\":\"integer\"},\"Items\":{\"type\":\"array\",\"items\":{\"type\":\"string\"}}},\"required\":[\"Action\",\"Priority\",\"Items\"],\"additionalProperties\":false}");
 *   ai.Chat("What should I do?", this, "OnAIResponse");
 *
 *   // Callback receives typed object
 *   void OnAIResponse(int cid, int status, string oid, MyAIResponse data){
 *       if (status == UF_SUCCESS && data){
 *           Print("Action: " + data.Action);
 *           Print("Priority: " + data.Priority);
 *       }
 *   }
 */
class UAIChatAgent<Class T> extends Managed {

    protected string m_ChatId;
    protected string m_KBId;
    protected bool m_Ready;
    protected bool m_IncludeHistory;
    protected int m_MaxHistory;
    protected autoptr array<autoptr UAIChatHistoryEntry> m_History;
    protected autoptr array<autoptr UAIChatContext> m_StaticContext;
    protected autoptr array<autoptr UAIChatToolDef> m_Tools;
    protected string m_SchemaName;
    protected string m_JsonSchema;

    // Pending chat request while session is being created
    protected string m_PendingMessage;
    protected Class m_PendingHandler;
    protected string m_PendingHandlerFn;
    
    // Holds the current polling callback to prevent garbage collection during CallLater delay
    protected autoptr UFCallbackBase m_PendingPollCallback;
    
    // Configurable poll timeout in seconds (default 90)
    protected int m_PollTimeout;

    void UAIChatAgent(){
        m_Ready = false;
        m_IncludeHistory = true;
        m_MaxHistory = 25;
        m_KBId = "";
        m_PollTimeout = 90;
        m_History = new array<autoptr UAIChatHistoryEntry>;
        m_StaticContext = new array<autoptr UAIChatContext>;
        m_Tools = new array<autoptr UAIChatToolDef>;
        m_SchemaName = "";
        m_JsonSchema = "";

        // Let subclass register tools
        RegisterTools(m_Tools);
    }
    
    // Called by polling callbacks to keep themselves alive during CallLater delay
    void SetPendingPollCallback(UFCallbackBase cb){
        m_PendingPollCallback = cb;
    }
    
    // Set the polling timeout in seconds (default 90). Override in subclass or call before Chat().
    void SetPollTimeout(int seconds){
        m_PollTimeout = Math.Max(10, seconds); // Minimum 10 seconds
    }
    
    // Get the current poll timeout
    int GetPollTimeout(){
        return m_PollTimeout;
    }

    // ============ OVERRIDE HOOKS ============

    // Override to provide system instructions for this agent.
    string SystemInstructions(){ return ""; }

    // Override to register tool functions. Each tool name should match a method on your class.
    // Example: RegisterTools adds "GetPlayerHealth" -> you define: string GetPlayerHealth(string playerName)
    void RegisterTools(out array<autoptr UAIChatToolDef> tools){}

    // Override to provide dynamic context lines added to every message.
    array<string> ExtraContext(){ return NULL; }

    // Override to provide system context items (static knowledge, rules, etc).
    array<string> SystemContext(){ return NULL; }

    // Override to access or transform history before sending. Return the history entries.
    array<autoptr UAIChatHistoryEntry> GetHistory(){ return m_History; }

    // Override to specify the AI model to use. Return empty string for default (gpt-4o-mini).
    // Available models: "gpt-4o", "gpt-4o-mini", "gpt-4-turbo", "gpt-3.5-turbo", "o1", "o1-mini", "o3-mini"
    string GetModel(){ return ""; }
    
    // ============ TOOL DISPATCH ============
    
    /**
     * Executes a tool by calling the method with the matching name.
     * The framework calls this automatically - you just define methods matching your tool names.
     */
    string ExecuteTool(string toolName, string p1, string p2, string p3, string p4, string p5, int paramCount){
        string result = "";
        bool success = false;
        
        switch (paramCount) {
            case 0:
                success = GetGame().GameScript.CallFunctionParams(this, toolName, result, NULL);
                break;
            case 1:
                success = GetGame().GameScript.CallFunctionParams(this, toolName, result, new Param1<string>(p1));
                break;
            case 2:
                success = GetGame().GameScript.CallFunctionParams(this, toolName, result, new Param2<string, string>(p1, p2));
                break;
            case 3:
                success = GetGame().GameScript.CallFunctionParams(this, toolName, result, new Param3<string, string, string>(p1, p2, p3));
                break;
            case 4:
                success = GetGame().GameScript.CallFunctionParams(this, toolName, result, new Param4<string, string, string, string>(p1, p2, p3, p4));
                break;
            case 5:
                success = GetGame().GameScript.CallFunctionParams(this, toolName, result, new Param5<string, string, string, string, string>(p1, p2, p3, p4, p5));
                break;
        }
        
        if (!success) {
            Error2("[UF][UAIChatAgent<T>] ExecuteTool", "Method '" + toolName + "' not found. Define: string " + toolName + "(...)");
            return "Error: Tool '" + toolName + "' not implemented";
        }
        
        return result;
    }
    
    int GetToolParamCount(string toolName){
        foreach (autoptr UAIChatToolDef tool : m_Tools){
            if (tool && tool.Name == toolName){
                return tool.ParamCount();
            }
        }
        return 0;
    }

    // ============ CONFIGURATION ============

    /**
     * SetSchema - REQUIRED: Set the JSON schema for the AI response format.
     * @param name - The schema name (e.g. "MyResponse"). Used by OpenAI for structured output.
     * @param jsonSchemaString - A valid JSON Schema string defining the response structure.
     *
     * Example JSON Schema for a response with Action, Priority, Items:
     *   {"type":"object","properties":{"Action":{"type":"string"},"Priority":{"type":"integer"},"Items":{"type":"array","items":{"type":"string"}}},"required":["Action","Priority","Items"],"additionalProperties":false}
     */
    void SetSchema(string name, string jsonSchemaString){
        m_SchemaName = name;
        m_JsonSchema = jsonSchemaString;
    }

    // Check if schema has been set
    bool HasSchema(){
        return m_SchemaName != "" && m_JsonSchema != "";
    }

    // Get the full schema object as JSON string for the API (includes name, schema, strict)
    protected string GetSchemaForAPI(){
        // Format: {"name":"<name>","schema":<schema>,"strict":true}
        return "{\"name\":\"" + m_SchemaName + "\",\"schema\":" + m_JsonSchema + ",\"strict\":true}";
    }

    void SetIncludeHistory(bool includeHistory, int maxHistory = 25){
        m_IncludeHistory = includeHistory;
        m_MaxHistory = maxHistory;
    }

    /**
     * Sets the Knowledge Base ID for this agent.
     * When set, the agent will automatically have access to a KB search tool
     * that allows it to query the knowledge base for relevant information.
     * The KB must be created and configured on the service side.
     * @param kbId - The Knowledge Base ID to use
     */
    void SetKBId(string kbId){
        m_KBId = kbId;
        UFLog.Debug("[UAIChatAgent<T>] SetKBId: " + kbId);
    }

    /**
     * Gets the currently configured Knowledge Base ID.
     * @return The KB ID, or empty string if not set
     */
    string GetKBId(){
        return m_KBId;
    }

    void AddStaticContext(string description, array<string> items){
        if (description == "" || !items) return;
        autoptr UAIChatContext ctx = new UAIChatContext(description);
        foreach (string it : items){
            if (it != "") ctx.AddContext(it);
        }
        m_StaticContext.Insert(ctx);
    }

    // ============ MAIN API ============

    /**
     * Send a chat message. Callback signature: void handler(int cid, int status, string oid, T data)
     * If the session is not yet created, it will be created automatically.
     */
    void Chat(string input, Class handler, string handlerFn){
        UFLog.Debug("[UAIChatAgent<T>] Chat - InputLen: " + input.Length().ToString() + ", Ready: " + m_Ready.ToString() + ", KBId: " + m_KBId);
        
        if (!HasSchema()){
            Error2("[UF][UAIChatAgent<T>] Chat", "Schema not set. Call SetSchema() before Chat()");
            CallHandlerError(handler, handlerFn, -1);
            return;
        }

        if (!U().IsOpenAIEnabled()){
            Error2("[UF][UAIChatAgent<T>] Chat", "OpenAI service is not online");
            CallHandlerError(handler, handlerFn, -1);
            return;
        }

        if (!m_Ready){
            // Need to create session first
            UFLog.Debug("[UAIChatAgent<T>] Chat - Session not ready, creating...");
            m_PendingMessage = input;
            m_PendingHandler = handler;
            m_PendingHandlerFn = handlerFn;
            CreateSession();
            return;
        }

        SendMessage(input, handler, handlerFn);
    }

    // ============ INTERNAL ============

    protected void CreateSession(){
        UFLog.Debug("[UAIChatAgent<T>] CreateSession - KBId: " + m_KBId);
        UFAIChatEndpoint ai = U().AI();
        string schema = GetSchemaForAPI();
        // Use "JSON" response format with schema
        int cid = ai.Create(SystemInstructions(), "JSON", schema, GetModel(), m_MaxHistory, new UAIChatAgentCreateCB<T>(this, ""), m_KBId);
        if (cid == -1){
            Error2("[UF][UAIChatAgent<T>] CreateSession", "Failed to create AI chat session");
            CallHandlerError(m_PendingHandler, m_PendingHandlerFn, -1);
            ClearPending();
        } else {
            UFLog.Debug("[UAIChatAgent<T>] CreateSession - Request sent, CID: " + cid);
        }
    }

    void OnSessionCreated(string chatId){
        m_ChatId = chatId;
        m_Ready = true;
        string hasPending = "no";
        if (m_PendingMessage != "") hasPending = "yes";
        UFLog.Debug("[UAIChatAgent<T>] OnSessionCreated - ChatId: " + chatId + ", KBId: " + m_KBId + ", PendingMessage: " + hasPending);

        if (m_PendingMessage != "" && m_PendingHandler){
            UFLog.Debug("[UAIChatAgent<T>] OnSessionCreated - Sending pending message");
            SendMessage(m_PendingMessage, m_PendingHandler, m_PendingHandlerFn);
            ClearPending();
        }
    }

    void OnSessionCreateFailed(string error){
        UFLog.Debug("[UAIChatAgent<T>] OnSessionCreateFailed - Error: " + error);
        Error2("[UF][UAIChatAgent<T>] OnSessionCreateFailed", error);
        CallHandlerError(m_PendingHandler, m_PendingHandlerFn, -1);
        ClearPending();
    }

    protected void ClearPending(){
        m_PendingMessage = "";
        m_PendingHandler = NULL;
        m_PendingHandlerFn = "";
    }

    protected void SendMessage(string input, Class handler, string handlerFn){
        UFLog.Debug("[UAIChatAgent<T>] SendMessage - ChatId: " + m_ChatId + ", InputLen: " + input.Length().ToString());
        
        autoptr array<autoptr UAIChatContext> ctx = BuildContext();
        autoptr array<autoptr UAIToolDef> tools = BuildToolDefs();
        
        if (ctx) {
            UFLog.Debug("[UAIChatAgent<T>] SendMessage - Context blocks: " + ctx.Count().ToString());
        }
        if (tools) {
            UFLog.Debug("[UAIChatAgent<T>] SendMessage - Tools: " + tools.Count().ToString());
        }

        UFAIChatEndpoint ai = U().AI();
        autoptr UAIChatAgentSendCB<T> cb = new UAIChatAgentSendCB<T>(this, "");
        cb.Init(handler, handlerFn);
        int cid = ai.Send(m_ChatId, input, cb, ctx, tools);

        if (cid == -1){
            CallHandlerError(handler, handlerFn, -1);
            return;
        }
        
        UFLog.Debug("[UAIChatAgent<T>] SendMessage - Sent, CID: " + cid);

        if (m_IncludeHistory){
            m_History.Insert(new UAIChatHistoryEntry("user", input));
            TrimHistory();
        }
    }

    void OnMessageResponse(int cid, int status, T data, string rawJson, Class handler, string handlerFn){
        UFLog.Debug("[UAIChatAgent<T>] OnMessageResponse - CID: " + cid + ", Status: " + status + ", RawLen: " + rawJson.Length().ToString());
        
        // Store raw JSON in history for context
        if (m_IncludeHistory && rawJson != ""){
            m_History.Insert(new UAIChatHistoryEntry("assistant", rawJson));
            TrimHistory();
        }
        CallHandler(handler, handlerFn, cid, status, m_ChatId, data);
    }

    protected autoptr array<autoptr UAIChatContext> BuildContext(){
        autoptr array<autoptr UAIChatContext> ctx = new array<autoptr UAIChatContext>;

        // System context from override
        array<string> sysCtx = SystemContext();
        if (sysCtx && sysCtx.Count() > 0){
            autoptr UAIChatContext sys = new UAIChatContext("system");
            foreach (string s : sysCtx){
                if (s != "") sys.AddContext(s);
            }
            ctx.Insert(sys);
        }

        // Static context blocks
        foreach (autoptr UAIChatContext c : m_StaticContext){
            ctx.Insert(c);
        }

        // Extra dynamic context from override
        array<string> extra = ExtraContext();
        if (extra && extra.Count() > 0){
            autoptr UAIChatContext ex = new UAIChatContext("extra");
            foreach (string e : extra){
                if (e != "") ex.AddContext(e);
            }
            ctx.Insert(ex);
        }

        // Chat history with roles
        array<autoptr UAIChatHistoryEntry> history = GetHistory();
        if (m_IncludeHistory && history && history.Count() > 0){
            autoptr UAIChatContext hist = new UAIChatContext("conversation_history");
            foreach (autoptr UAIChatHistoryEntry h : history){
                if (h) hist.AddContext(h.Role + ": " + h.Message);
            }
            ctx.Insert(hist);
        }

        if (ctx.Count() == 0) return NULL;
        return ctx;
    }

    protected autoptr array<autoptr UAIToolDef> BuildToolDefs(){
        if (!m_Tools || m_Tools.Count() == 0) return NULL;
        autoptr array<autoptr UAIToolDef> defs = new array<autoptr UAIToolDef>;
        foreach (autoptr UAIChatToolDef t : m_Tools){
            if (!t || t.Name == "") continue;
            autoptr UAIToolDef toolDef = new UAIToolDef(t.Name, t.Description, t.GetParamNames(), t.GetParamTypes(), t.GetParamDescs());
            defs.Insert(toolDef);
        }
        if (defs.Count() == 0) return NULL;
        return defs;
    }

    protected void TrimHistory(){
        while (m_History.Count() > m_MaxHistory && m_MaxHistory > 0){
            m_History.RemoveOrdered(0);
        }
    }

    // ============ CALLBACK HELPERS ============

    protected void CallHandler(Class handler, string handlerFn, int cid, int status, string oid, T data){
        if (!handler || handlerFn == "") return;
        g_Game.GameScript.CallFunctionParams(handler, handlerFn, NULL, new Param4<int, int, string, T>(cid, status, oid, data));
    }

    protected void CallHandlerError(Class handler, string handlerFn, int cid){
        CallHandler(handler, handlerFn, cid, -1, "", NULL);
    }
}

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
        UFAIChatEndpoint ai = U().AI();
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
        
        UFAIChatEndpoint ai = U().AI();
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
