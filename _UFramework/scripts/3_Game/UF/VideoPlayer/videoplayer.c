ref UFVideoPlayer m_UFVideoPlayer;

/**
 * Gets global video player instance.
 * 
 * @return UFVideoPlayer singleton or null
 */
UFVideoPlayer GetUFVideoPlayer(){
	if (m_UFVideoPlayer){
		return m_UFVideoPlayer;
	}
	return null;
}

/**
 * Video player for TTS audio playback with queue support.
 * 
 * @note Automatically downloads and plays TTS audio files
 * @note Supports queuing multiple audio files
 * @note Client-side only - no-op on dedicated server
 * 
 * @usage GetUFVideoPlayer().LoadAndPlay("ttsId", true);
 * @usage GetUFVideoPlayer().AddToQueue("ttsId");
 */
class UFVideoPlayer extends ScriptedWidgetEventHandler {
	static string m_LayoutPath = "_UFramework/data/layouts/videoplayer.layout";

    protected VideoWidget m_Video;
	protected Widget m_LayoutRoot;
	protected ImageWidget m_icon;
	
	protected bool m_isAudioPlaying = false;
	
	protected autoptr TStringArray m_VideoQueue;

	/**
	 * Constructor: initializes the video player widget and queue.
	 * 
	 * @note Automatically called when creating new UFVideoPlayer instance.
	 */
	void UFVideoPlayer(){
		Init();
	}

	/**
	 * Destructor: cleans up video player resources.
	 * 
	 * @note Automatically stops playback and destroys widgets.
	 */
	void ~UFVideoPlayer(){
		Destroy();
	}
	
	/**
	 * Check if audio is currently playing.
	 * 
	 * @return True if audio playback is active, false otherwise.
	 */
	bool isAudioPlaying(){
		return m_isAudioPlaying;
	}
	
	/**
	 * Destroy the video player and clean up resources.
	 * 
	 * @note Stops playback, unloads video, and deletes layout widgets.
	 */
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

	/**
	 * Initialize the video player widget and queue.
	 * 
	 * @note Creates layout widgets, initializes queue, and prepares video widget.
	 * @note No-op on dedicated server.
	 */
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

    /**
     * Load and play a TTS audio file by ID.
     * 
     * @param oid The TTS object ID (filename without extension).
     * @param showIcon If true, shows the audio playback icon.
     * 
     * @usage GetUFVideoPlayer().LoadAndPlay("tts_12345", true);
     * 
     * @note Does nothing if audio is already playing (use AddToQueue instead).
     * @note Expects file at $saves:{oid}.mp4
     */
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
    /**
     * Load a video/audio file by absolute path.
     * 
     * @param videoPath The full path to the video/audio file.
     * 
     * @usage GetUFVideoPlayer().LoadPath("$saves:myaudio.mp4");
     * 
     * @note Does not auto-play, use Play() afterwards.
     */
    void LoadPath(string videoPath){
        if (g_Game.IsDedicatedServer()) return;
        if (!m_Video) return;
		#ifndef NO_GUI
            m_Video.Load(videoPath, false);
            UFLog.Debug("Loading Video: " + videoPath + " Time:" + m_Video.GetTotalTime());
        #endif

    }
	
    /**
     * Load a TTS audio file by ID without playing.
     * 
     * @param oid The TTS object ID (filename without extension).
     * 
     * @usage GetUFVideoPlayer().Load("tts_12345");
     * 
     * @note Expects file at $saves:{oid}.mp4. Use Play() to start playback.
     */
    void Load(string oid){
        if (g_Game.IsDedicatedServer()) return;
        if (!m_Video) return;
		string videoPath =  "$saves:" + oid + ".mp4";
		#ifndef NO_GUI
            m_Video.Load(videoPath, false);
            UFLog.Debug("Loading Video: " + videoPath + " Time:" + m_Video.GetTotalTime());
        #endif

    }
	
	/**
	 * Play the currently loaded audio/video.
	 * 
	 * @param showIcon If true, shows the playback icon (default: true).
	 * 
	 * @usage GetUFVideoPlayer().Play(true);
	 * 
	 * @note Requires Load() or LoadPath() to be called first.
	 */
	void Play(bool showIcon = true){
        if (g_Game.IsDedicatedServer()) return;
        if (!m_Video) return;
		#ifndef NO_GUI
			m_icon.Show(showIcon);
            UFLog.Debug("Playing Video: " + m_Video.GetTotalTime());
         	m_Video.Play();
        #endif
	}
	
    /**
     * Stop playback and unload the current audio/video.
     * 
     * @note Automatically plays next queued item if any.
     * @note Hides the playback icon.
     */
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
	
	/**
	 * Internal: plays the next queued audio file.
	 * 
	 * @note Automatically called by Stop() when playback finishes.
	 */
	void PlayNextInQueue(){
		if (m_VideoQueue.Count() > 0){
			string oid = m_VideoQueue.Get(0);
			LoadAndPlay(oid, true);
			m_VideoQueue.RemoveOrdered(0);
		}
	}
	
	/**
	 * Adds TTS audio to playback queue.
	 * 
	 * @param oid TTS ID to queue
	 * 
	 * @note Plays immediately if nothing playing, otherwise queues
	 */
	void AddToQueue(string oid){
		if (!isAudioPlaying()){
			LoadAndPlay(oid, true);
		} else {
			m_VideoQueue.Insert(oid);
		}
	}
	
	/**
	 * Callback handler for TTS playback requests.
	 * 
	 * @param cid Callback ID.
	 * @param status Status code (UF_SUCCESS or error).
	 * @param oid TTS object ID.
	 * @param msg Error message if status != UF_SUCCESS.
	 * 
	 * @note Automatically adds audio to queue on success.
	 */
	void UCBHandlePlay(int cid, int status, string oid, string msg){
		if (status == UF_SUCCESS){
			AddToQueue(oid);
		} 
		else {
			UFLog.Err("Error playing audio " + oid + " cid" + cid + " Message: " + msg);
		}
	}

}