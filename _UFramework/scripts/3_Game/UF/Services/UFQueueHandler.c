class UQueueHandler<Class T> extends UQueueHandlerBase 
{
	override int Read(){
		return U().Msg().Read(m_mod,m_queue, new UFMsgCallback<T>(this, "readCB", m_queue));
	}
	
	int Read(int limit){
		return U().Msg().Read(m_mod,m_queue, limit, new UFMsgCallback<T>(this, "readCB", m_queue));
	}
	
	int Write(T message){
		
		autoptr UMessage<T> msg = new UMessage<T>(message);
		string txt = msg.ToJson();
		delete msg;
		return U().Msg().Write(m_mod, m_queue, txt);
	}
 	
	void readCB(int cid, int status, string oid, array<T> messages){
		if (status == UF_SUCCESS){
			foreach(T message : messages){
				g_Game.GameScript.CallFunctionParams(GetInstance(), GetFuncName(), NULL, new Param4<int, int, string, T>(cid, status, oid, message));
			}
		}
	} 
}


class UStringQueueHandler extends UQueueHandlerBase 
{
	
	override int Read(){
		return U().Msg().Read(m_mod, m_queue, new UFMsgStringCallback(this, "readCB", m_queue));
	}
	
	int Read(int limit){
		return U().Msg().Read(m_mod, m_queue, limit, new UFMsgStringCallback(this, "readCB", m_queue));
	}
	
	
 	int Write(string message){
		autoptr UStringMessage msg = new UStringMessage(message);
		string txt = msg.ToJson();
		delete msg;
		return U().Msg().Write(m_mod,m_queue ,txt);
	}
	
	void readCB(int cid, int status, string oid, TStringArray messages){
		if (status == UF_SUCCESS){
			foreach(string message : messages){
				g_Game.GameScript.CallFunctionParams(GetInstance(), GetFuncName(), NULL, new Param4<int, int, string, string>(cid, status, oid, message));
			}
		}
	}
}



class UQueueHandlerBase extends Managed 
{
	
	protected string m_mod;
	protected string m_queue;
	protected Class m_obj;
	protected string m_funcName;
	protected int m_limit;
	protected int m_LastReadCall = -1;
	protected int m_LastWriteCall = -1;
	protected int m_PolingFrequency = 3;
    
    void UQueueHandlerBase(string mod, string queue, Class obj, string funcName, UQueueMeta meta = NULL, int limit = -1, int polingFrequency = 3 )
    {
        // Initialization code
		m_mod = mod;
		m_queue = queue;
		Class.CastTo(m_obj, obj);
		m_funcName = funcName;
		m_limit = limit;
		m_PolingFrequency = polingFrequency;
		Init( meta );
    }
		
	//If you are only initing for writing
    void UQueueHandlerBase(string mod, string queue, UQueueMeta meta )
    {
        // Initialization code
		m_mod = mod;
		m_queue = queue;
		m_PolingFrequency = -1;
		Init( meta );
    }
			
	protected void Init(UQueueMeta meta)
	{
		if (m_PolingFrequency > 0) U().Cron().runEndless(m_PolingFrequency, this, "CheckQueue", NULL);
		if (meta && g_Game.IsDedicatedServer()){
			m_LastWriteCall = U().Msg().SetMeta(m_mod,m_queue,meta);
		}
	}

    // Destructor: Called when the instance is destroyed (if needed)
    void ~UQueueHandlerBase()
    {
		U().Cron().Remove(this, "CheckQueue");
		if (m_LastReadCall > 0) Cancel(m_LastReadCall);
		if (m_LastWriteCall > 0) Cancel(m_LastWriteCall);
    }

	void CheckQueue(){
		m_LastReadCall = Read();
	}
	
	int jsonWrite(string sText){
		return U().Msg().Write(m_mod,m_queue,sText);
	}
	
	protected int Read(){
		Error2("[UF] UFQueueHandlerBase", "Using unimplemented Read");
		return -1;
	}
	
	int Reset(){
		return U().Msg().Reset(m_mod,m_queue);
	}
	
	/* 
		Call Cancel
		
		This allows you to cancel a call back to prevent access violations 
	*/
	void Cancel(int cid){
		U().RequestCallCancel(cid);
	}
 
	Class GetInstance(){
		return m_obj;
	}
	
	string GetFuncName(){
		return m_funcName;
	}
}
