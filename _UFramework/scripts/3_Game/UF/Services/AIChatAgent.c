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
 * UAIChatToolDef - simple tool definition for AI chat agents.
 * Name = function name on the agent class (must return string)
 * Description = what the tool does (for AI)
 * Parameters = array of parameter names the tool accepts
 */
class UAIChatToolDef extends Managed {
    string Name;
    string Description;
    autoptr array<string> Parameters;

    void UAIChatToolDef(string name, string desc, array<string> params = NULL){
        Name = name;
        Description = desc;
        if (params){
            Parameters = new array<string>;
            Parameters.Copy(params);
        }
    }

    int ParamCount(){
        if (!Parameters) return 0;
        return Parameters.Count();
    }

    array<string> GetParamNames(){
        if (!Parameters) return new array<string>;
        return Parameters;
    }
}

/**
 * UFAIChatAgent - base class for per-mod AI chat agents.
 *
 * Modders extend this class and:
 *   1) Override SystemInstructions() to return their system prompt
 *   2) Override RegisterTools(out array<autoptr UAIChatToolDef> tools) to register tool functions
 *   3) Override OnToolCall() to dispatch tool calls to your functions that return string
 *   4) Call Chat(input, handler, handlerFn) to send messages with familiar callback pattern
 *
 * Example:
 *   class MyAI extends UFAIChatAgent {
 *       override string SystemInstructions(){ return "You are a helpful assistant."; }
 *       override void RegisterTools(out array<autoptr UAIChatToolDef> tools){
 *           tools.Insert(new UAIChatToolDef("Echo", "Echoes text back", {"text"}));
 *       }
 *       override string OnToolCall(string toolName, string p1, string p2, string p3, string p4, string p5){
 *           if (toolName == "Echo") return Echo(p1);
 *           return "";
 *       }
 *       string Echo(string text){ return "Echo: " + text; }
 *   }
 *
 *   autoptr MyAI ai = new MyAI();
 *   ai.Chat("Hello", this, "OnAIResponse");
 *
 *   void OnAIResponse(int cid, int status, string oid, string data){ ... }
 */
class UFAIChatAgent extends Managed {

    protected string m_ChatId;
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

    void UFAIChatAgent(){
        m_Ready = false;
        m_IncludeHistory = true;
        m_MaxHistory = 25;
        m_History = new array<autoptr UAIChatHistoryEntry>;
        m_StaticContext = new array<autoptr UAIChatContext>;
        m_Tools = new array<autoptr UAIChatToolDef>;

        // Let subclass register tools
        RegisterTools(m_Tools);
    }

    // ============ OVERRIDE HOOKS ============

    // Override to provide system instructions for this agent.
    string SystemInstructions(){ return ""; }

    // Override to register tool functions. Each tool name must match a function on this class that returns string.
    void RegisterTools(out array<autoptr UAIChatToolDef> tools){}

    // Override to dispatch tool calls to your functions. Return the result string.
    string OnToolCall(string toolName, string p1, string p2, string p3, string p4, string p5){
        // Default: no-op. Subclass should override and dispatch to appropriate function.
        return "";
    }

    // Override to provide dynamic context lines added to every message.
    array<string> ExtraContext(){ return NULL; }

    // Override to provide system context items (static knowledge, rules, etc).
    array<string> SystemContext(){ return NULL; }

    // Override to access or transform history before sending. Return the history entries.
    array<autoptr UAIChatHistoryEntry> GetHistory(){ return m_History; }

    // ============ CONFIGURATION ============

    void SetIncludeHistory(bool includeHistory, int maxHistory = 25){
        m_IncludeHistory = includeHistory;
        m_MaxHistory = maxHistory;
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
        if (!U().IsOpenAIEnabled()){
            Error2("[UF][AIChatAgent] Chat", "OpenAI service is not online");
            CallHandlerError(handler, handlerFn, -1, "OpenAI service is not online");
            return;
        }

        if (!m_Ready){
            // Need to create session first
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
        UFAIChatEndpoint ai = U().AI();
        int cid = ai.Create(SystemInstructions(), "string", "", "", m_MaxHistory, new UFAIChatAgentCreateCB(this, ""));
        if (cid == -1){
            Error2("[UF][AIChatAgent] CreateSession", "Failed to create AI chat session");
            CallHandlerError(m_PendingHandler, m_PendingHandlerFn, -1, "Failed to create session");
            ClearPending();
        }
    }

    void OnSessionCreated(string chatId){
        m_ChatId = chatId;
        m_Ready = true;

        if (m_PendingMessage != "" && m_PendingHandler){
            SendMessage(m_PendingMessage, m_PendingHandler, m_PendingHandlerFn);
            ClearPending();
        }
    }

    void OnSessionCreateFailed(string error){
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
        autoptr array<autoptr UAIChatContext> ctx = BuildContext();
        autoptr array<autoptr UAIToolDef> tools = BuildToolDefs();

        UFAIChatEndpoint ai = U().AI();
        autoptr UFAIChatAgentSendCB cb = new UFAIChatAgentSendCB(this, "");
        cb.Init(handler, handlerFn);
        int cid = ai.Send(m_ChatId, input, cb, ctx, tools);

        if (cid == -1){
            CallHandlerError(handler, handlerFn, -1, "Failed to send message");
            return;
        }

        if (m_IncludeHistory){
            m_History.Insert(new UAIChatHistoryEntry("user", input));
            TrimHistory();
        }
    }

    void OnMessageResponse(int cid, int status, string data, Class handler, string handlerFn){
        // TODO: Parse for tool calls and invoke them, then continue conversation
        // For now, just forward to handler
        if (m_IncludeHistory && data != ""){
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
            defs.Insert(new UAIToolDef(t.Name, t.Description, t.GetParamNames()));
        }
        if (defs.Count() == 0) return NULL;
        return defs;
    }

    protected void TrimHistory(){
        while (m_History.Count() > m_MaxHistory && m_MaxHistory > 0){
            m_History.RemoveOrdered(0);
        }
    }

    // Invoke a tool function by name with up to 5 string params. Returns the function's string result.
    string InvokeTool(string toolName, string p1 = "", string p2 = "", string p3 = "", string p4 = "", string p5 = ""){
        return OnToolCall(toolName, p1, p2, p3, p4, p5);
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
        if (!m_Agent) return;
        // Parse ChatId from response
        autoptr UAIChatCreateResponse resp = new UAIChatCreateResponse;
        string error;
        JsonSerializer js = new JsonSerializer();
        if (js.ReadFromString(resp, jsonData, error) && resp.ChatId != ""){
            m_Agent.OnSessionCreated(resp.ChatId);
        } else {
            m_Agent.OnSessionCreateFailed("Failed to parse ChatId from response: " + error);
        }
    }

    override void OnError(int errorCode, int cid){
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
    static const int MAX_POLL_RETRIES = 120; // ~2 minutes at 1 second intervals

    void UFAIChatAgentSendCB(Class instance, string function, string oid = ""){
        Class.CastTo(m_Agent, instance);
        m_PendingMessageId = "";
        m_PollRetries = 0;
    }
    
    void Init(Class handler, string handlerFn){
        m_Handler = handler;
        m_HandlerFn = handlerFn;
    }

    override void OnSuccess(string jsonData, int cid){
        if (!m_Agent) return;
        // Parse the message response
        autoptr UAIChatMessageResponse resp;
        string error;
        JsonSerializer js = new JsonSerializer();
        if (!js.ReadFromString(resp, jsonData, error) || !resp){
            m_Agent.OnMessageResponse(cid, UF_JSONERROR, "Failed to parse response: " + error, m_Handler, m_HandlerFn);
            return;
        }
        
        // Handle different status responses
        if (resp.Status == "Success"){
            m_Agent.OnMessageResponse(cid, UF_SUCCESS, resp.Message, m_Handler, m_HandlerFn);
        } else if (resp.Status == "Pending" || resp.Status == "Wait"){
            // Start or continue polling
            m_PendingMessageId = resp.MessageId;
            m_PollRetries++;
            if (m_PollRetries > MAX_POLL_RETRIES){
                m_Agent.OnMessageResponse(cid, UF_TIMEOUT, "AI response timed out", m_Handler, m_HandlerFn);
                return;
            }
            // Poll again after delay
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(PollMessageStatus, 1000, false, cid);
        } else if (resp.Status == "NotFound"){
            m_Agent.OnMessageResponse(cid, UF_NOTFOUND, "Message not found", m_Handler, m_HandlerFn);
        } else {
            // Error or unknown status
            m_Agent.OnMessageResponse(cid, UF_ERROR, "Status: " + resp.Status, m_Handler, m_HandlerFn);
        }
    }
    
    protected void PollMessageStatus(int cid){
        if (!m_Agent || m_PendingMessageId == "") return;
        UFAIChatEndpoint ai = U().AI();
        ai.MessageStatus(m_PendingMessageId, this);
    }

    override void OnError(int errorCode, int cid){
        if (!m_Agent) return;
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

    void UAIChatAgent(){
        m_Ready = false;
        m_IncludeHistory = true;
        m_MaxHistory = 25;
        m_History = new array<autoptr UAIChatHistoryEntry>;
        m_StaticContext = new array<autoptr UAIChatContext>;
        m_Tools = new array<autoptr UAIChatToolDef>;
        m_SchemaName = "";
        m_JsonSchema = "";

        // Let subclass register tools
        RegisterTools(m_Tools);
    }

    // ============ OVERRIDE HOOKS ============

    // Override to provide system instructions for this agent.
    string SystemInstructions(){ return ""; }

    // Override to register tool functions. Each tool name must match a function on this class that returns string.
    void RegisterTools(out array<autoptr UAIChatToolDef> tools){}

    // Override to dispatch tool calls to your functions. Return the result string.
    string OnToolCall(string toolName, string p1, string p2, string p3, string p4, string p5){
        return "";
    }

    // Override to provide dynamic context lines added to every message.
    array<string> ExtraContext(){ return NULL; }

    // Override to provide system context items (static knowledge, rules, etc).
    array<string> SystemContext(){ return NULL; }

    // Override to access or transform history before sending. Return the history entries.
    array<autoptr UAIChatHistoryEntry> GetHistory(){ return m_History; }

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
        UFAIChatEndpoint ai = U().AI();
        string schema = GetSchemaForAPI();
        // Use "JSON" response format with schema
        int cid = ai.Create(SystemInstructions(), "JSON", schema, "", m_MaxHistory, new UAIChatAgentCreateCB<T>(this, ""));
        if (cid == -1){
            Error2("[UF][UAIChatAgent<T>] CreateSession", "Failed to create AI chat session");
            CallHandlerError(m_PendingHandler, m_PendingHandlerFn, -1);
            ClearPending();
        }
    }

    void OnSessionCreated(string chatId){
        m_ChatId = chatId;
        m_Ready = true;

        if (m_PendingMessage != "" && m_PendingHandler){
            SendMessage(m_PendingMessage, m_PendingHandler, m_PendingHandlerFn);
            ClearPending();
        }
    }

    void OnSessionCreateFailed(string error){
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
        autoptr array<autoptr UAIChatContext> ctx = BuildContext();
        autoptr array<autoptr UAIToolDef> tools = BuildToolDefs();

        UFAIChatEndpoint ai = U().AI();
        autoptr UAIChatAgentSendCB<T> cb = new UAIChatAgentSendCB<T>(this, "");
        cb.Init(handler, handlerFn);
        int cid = ai.Send(m_ChatId, input, cb, ctx, tools);

        if (cid == -1){
            CallHandlerError(handler, handlerFn, -1);
            return;
        }

        if (m_IncludeHistory){
            m_History.Insert(new UAIChatHistoryEntry("user", input));
            TrimHistory();
        }
    }

    void OnMessageResponse(int cid, int status, T data, string rawJson, Class handler, string handlerFn){
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
            defs.Insert(new UAIToolDef(t.Name, t.Description, t.GetParamNames()));
        }
        if (defs.Count() == 0) return NULL;
        return defs;
    }

    protected void TrimHistory(){
        while (m_History.Count() > m_MaxHistory && m_MaxHistory > 0){
            m_History.RemoveOrdered(0);
        }
    }

    string InvokeTool(string toolName, string p1 = "", string p2 = "", string p3 = "", string p4 = "", string p5 = ""){
        return OnToolCall(toolName, p1, p2, p3, p4, p5);
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
    static const int MAX_POLL_RETRIES = 120; // ~2 minutes at 1 second intervals

    void UAIChatAgentSendCB(Class instance, string function, string oid = ""){
        // instance is stored in parent's Instance field
        m_PendingMessageId = "";
        m_PollRetries = 0;
    }
    
    void Init(Class handler, string handlerFn){
        m_Handler = handler;
        m_HandlerFn = handlerFn;
    }
    
    protected UAIChatAgent<T> GetAgent(){
        UAIChatAgent<T> agent;
        Class.CastTo(agent, Instance);
        return agent;
    }

    override void OnSuccess(string jsonData, int cid){
        UAIChatAgent<T> agent = GetAgent();
        if (!agent) return;
        // First parse the wrapper response
        autoptr UAIChatMessageResponse resp;
        string error;
        JsonSerializer js = new JsonSerializer();
        if (!js.ReadFromString(resp, jsonData, error) || !resp){
            agent.OnMessageResponse(cid, UF_JSONERROR, NULL, "", m_Handler, m_HandlerFn);
            return;
        }
        
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
            m_PendingMessageId = resp.MessageId;
            m_PollRetries++;
            if (m_PollRetries > MAX_POLL_RETRIES){
                agent.OnMessageResponse(cid, UF_TIMEOUT, NULL, "", m_Handler, m_HandlerFn);
                return;
            }
            // Poll again after delay
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(PollMessageStatus, 1000, false, cid);
        } else if (resp.Status == "NotFound"){
            agent.OnMessageResponse(cid, UF_NOTFOUND, NULL, "", m_Handler, m_HandlerFn);
        } else {
            // Error or unknown status
            agent.OnMessageResponse(cid, UF_ERROR, NULL, "", m_Handler, m_HandlerFn);
        }
    }
    
    protected void PollMessageStatus(int cid){
        UAIChatAgent<T> agent = GetAgent();
        if (!agent || m_PendingMessageId == "") return;
        UFAIChatEndpoint ai = U().AI();
        ai.MessageStatus(m_PendingMessageId, this);
    }

    override void OnError(int errorCode, int cid){
        UAIChatAgent<T> agent = GetAgent();
        if (!agent) return;
        agent.OnMessageResponse(cid, errorCode, NULL, "", m_Handler, m_HandlerFn);
    }
}
