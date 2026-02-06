ref DiscordLoggedInWidget m_DiscordLoggedInWidget;

/**
 * Gets global Discord status widget instance.
 * 
 * @return DiscordLoggedInWidget singleton or null
 */
DiscordLoggedInWidget GetDiscordLoggedInWidget(){
	if (m_DiscordLoggedInWidget){
		return m_DiscordLoggedInWidget;
	}
	return null;
}

/**
 * UI widget for displaying Discord login status with avatar.
 * 
 * @note Shows Discord username and avatar in-game
 * @note Client-side only
 * 
 * @usage GetDiscordLoggedInWidget().UpdateData("Username", "path/to/avatar.edds");
 * @usage GetDiscordLoggedInWidget().ShowAvatar();
 */
class DiscordLoggedInWidget extends ScriptedWidgetEventHandler
{
    protected ImageWidget m_Avatar;
    protected TextWidget m_Username;
	protected Widget m_Root;
	protected bool m_isSet = false;
	protected bool m_isShowing = false;
    
    /**
     * Constructor: loads the Discord status widget layout.
     * 
     * @note Creates widget from layout file and initializes references to avatar and username elements.
     */
    void DiscordLoggedInWidget()
    {
        // Create the widget using the provided layout file.
        m_Root = g_Game.GetWorkspace().CreateWidgets("_UFramework/data/layouts/discordLoggedIn.Layout");
        
        // Find the ImageWidget named "avatar" and the TextWidget named "username".
        m_Avatar = ImageWidget.Cast(m_Root.FindAnyWidget("avatar"));
        m_Username = TextWidget.Cast(m_Root.FindAnyWidget("username"));
		m_isShowing = false;
		m_Root.Show(false);
    }
    
    /**
     * Destructor: cleans up widget resources.
     * 
     * @note Automatically hides and deletes the root widget.
     */
    void ~DiscordLoggedInWidget()
    {
        // Hide and delete the widgets if needed.
        if (m_Root)
            delete m_Root;
    }
    
    /**
     * Update the displayed Discord username and avatar image.
     * 
     * @param username The Discord username to display.
     * @param avatarImagePath Path to the avatar image file (.edds format).
     * 
     * @usage GetDiscordLoggedInWidget().UpdateData("Username#1234", "$saves:discordme.edds");
     * 
     * @note Sets the m_isSet flag to true when called.
     */
    void UpdateData(string username, string avatarImagePath)
    {
        if (m_Username)
        {
            m_Username.SetText(username);
        }
        if (m_Avatar)
        {
            m_Avatar.LoadImageFile(0, avatarImagePath);
        }
		m_isSet = true;
    }
    
    /**
     * Hide the Discord status widget.
     * 
     * @usage GetDiscordLoggedInWidget().HideAvatar();
     */
    void HideAvatar()
    {
        if (m_Root)
            m_Root.Show(false);
    }
    
    /**
     * Show the Discord status widget.
     * 
     * @usage GetDiscordLoggedInWidget().ShowAvatar();
     */
    void ShowAvatar()
    {
        if (m_Root)
            m_Root.Show(true);
    }
	
	/**
	 * Check if Discord data has been loaded.
	 * 
	 * @return True if UpdateData() has been called, false otherwise.
	 */
	bool IsSet(){
		return m_isSet;
	}
	
	/**
	 * Toggle the visibility of the Discord status widget.
	 * 
	 * @usage GetDiscordLoggedInWidget().ToggleAvatar();
	 * 
	 * @note Switches between shown and hidden state.
	 */
	void ToggleAvatar(){
		if (m_Root)
		{
			m_isShowing = !m_isShowing;
			m_Root.Show(m_isShowing);
		}
	}
}
