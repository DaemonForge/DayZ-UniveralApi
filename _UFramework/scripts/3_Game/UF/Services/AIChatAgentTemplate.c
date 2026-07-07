
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
    protected autoptr array<string> m_AllowedPlayers;
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

    // Override (or use SetAllowedPlayers) to restrict which players can access this chat session.
    // Accepts DayZ GUIDs or SteamID64s - the service normalizes SteamIDs to GUIDs.
    // Return NULL or an empty array for a public session (default).
    array<string> AllowedPlayers(){ return m_AllowedPlayers; }

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
                success = g_Game.GameScript.CallFunctionParams(this, toolName, result, NULL); 
                break;
            case 1:
                success = g_Game.GameScript.CallFunctionParams(this, toolName, result, new Param1<string>(p1));
                break;
            case 2:
                success = g_Game.GameScript.CallFunctionParams(this, toolName, result, new Param2<string, string>(p1, p2));
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

    /**
     * Restricts this agent's chat session to specific players.
     * Accepts DayZ GUIDs (player.GetIdentity().GetId()) or SteamID64s - mixed freely,
     * the service normalizes SteamIDs to GUIDs. NULL or empty = public session (default).
     * Call before Chat(); if the session already exists it is updated live.
     */
    void SetAllowedPlayers(array<string> players){
        m_AllowedPlayers = players;
        if (m_Ready && m_ChatId != ""){
            UF().AI().SetAccess(m_ChatId, players);
        }
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

        if (!UF().IsOpenAIEnabled()){
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
        UFAIChatEndpoint ai = UF().AI();
        string schema = GetSchemaForAPI();
        // Use "JSON" response format with schema
        int cid = ai.Create(SystemInstructions(), "JSON", schema, GetModel(), m_MaxHistory, new UAIChatAgentCreateCB<T>(this, ""), m_KBId, AllowedPlayers());
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

        UFAIChatEndpoint ai = UF().AI();
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