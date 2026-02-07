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
