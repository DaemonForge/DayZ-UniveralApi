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
        
        if (!UF().IsOpenAIEnabled()){
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
        UFAIChatEndpoint ai = UF().AI();
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

        UFAIChatEndpoint ai = UF().AI();
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



