ref UFVideoPlayer m_UFVideoPlayer;

UFVideoPlayer GetUFVideoPlayer(){
	if (m_UFVideoPlayer){
		return m_UFVideoPlayer;
	}
	return null;
}

class UFVideoPlayer extends ScriptedWidgetEventHandler {
	static string m_LayoutPath = "_UFramework/data/layouts/videoplayer.layout";

    protected VideoWidget m_Video;
	protected Widget m_LayoutRoot;
	protected ImageWidget m_icon;
	
	protected bool m_isAudioPlaying = false;
	
	protected autoptr TStringArray m_VideoQueue;

	void UFVideoPlayer(){
		Init();
	}

	void ~UFVideoPlayer(){
		Destroy();
	}
	
	bool isAudioPlaying(){
		return m_isAudioPlaying;
	}
	
	void Destroy(){
		if (!m_LayoutRoot) return;
		m_LayoutRoot.Show(false);
        if (m_Video){
			#ifndef NO_GUI
				m_icon.Show(false);
	            UFLog.Debug("Stopping: " + m_Video);
	            m_Video.Stop();
	            m_Video.Unload();
        	#endif
		}
		delete m_LayoutRoot;
	}

	void Init(){
        if (g_Game.IsDedicatedServer()) return;
		m_LayoutRoot = g_Game.GetWorkspace().CreateWidgets(m_LayoutPath, NULL, true);
        m_icon = ImageWidget.Cast(m_LayoutRoot.FindAnyWidget("icon"));
		m_VideoQueue = new TStringArray();
		m_icon.Show(false);
		#ifndef NO_GUI
        m_Video = VideoWidget.Cast(m_LayoutRoot.FindAnyWidget("videoPlayer"));
        #endif
	}

    void LoadAndPlay(string  oid, bool showIcon){
		if (isAudioPlaying()){
			Print("Trying to play but audio is already playing");
			 return;
		}
        if (g_Game.IsDedicatedServer()) return;
        if (!m_Video) return;
		string videoPath =  "$saves:" + oid + ".mp4";
		m_isAudioPlaying = true;
		#ifndef NO_GUI
            UFLog.Debug("Loading Video: " + videoPath);
            m_Video.Load(videoPath, false);
			int playTime = m_Video.GetTotalTime();
         	m_Video.Play();
			//m_Video.Stop();
           	g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(m_Video.Stop, 1, false);
            UFLog.Debug("Loading Video: " + videoPath + " Time:" + playTime);
            g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(this.Play, 650, false, showIcon);
			g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(this.Stop, playTime + 990, false);
        #endif

    }
    void LoadPath(string videoPath){
        if (g_Game.IsDedicatedServer()) return;
        if (!m_Video) return;
		#ifndef NO_GUI
            m_Video.Load(videoPath, false);
            UFLog.Debug("Loading Video: " + videoPath + " Time:" + m_Video.GetTotalTime());
        #endif

    }
	
    void Load(string oid){
        if (g_Game.IsDedicatedServer()) return;
        if (!m_Video) return;
		string videoPath =  "$saves:" + oid + ".mp4";
		#ifndef NO_GUI
            m_Video.Load(videoPath, false);
            UFLog.Debug("Loading Video: " + videoPath + " Time:" + m_Video.GetTotalTime());
        #endif

    }
	
	void Play(bool showIcon = true){
        if (g_Game.IsDedicatedServer()) return;
        if (!m_Video) return;
		#ifndef NO_GUI
			m_icon.Show(showIcon);
            UFLog.Debug("Playing Video: " + m_Video.GetTotalTime());
         	m_Video.Play();
        #endif
	}
	
    void Stop(){
        if (g_Game.IsDedicatedServer()) return;
        if (!m_Video) return;
		m_isAudioPlaying = false;
		#ifndef NO_GUI
			m_icon.Show(false);
            UFLog.Debug("Stopping: " + m_Video);
            m_Video.Stop();
            m_Video.Unload();
        #endif
		PlayNextInQueue();
    }
	
	void PlayNextInQueue(){
		if (m_VideoQueue.Count() > 0){
			string oid = m_VideoQueue.Get(0);
			LoadAndPlay(oid, true);
			m_VideoQueue.RemoveOrdered(0);
		}
	}
	
	void AddToQueue(string oid){
		if (!isAudioPlaying()){
			LoadAndPlay(oid, true);
		} else {
			m_VideoQueue.Insert(oid);
		}
	}
	
	void UCBHandlePlay(int cid, int status, string oid, string msg){
		if (status == UF_SUCCESS){
			AddToQueue(oid);
		} 
		else {
			UFLog.Err("Error playing audio " + oid + " cid" + cid + " Message: " + msg);
		}
	}

}