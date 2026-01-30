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
    
    // Constructor: load the layout and obtain references to the child widgets.
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
    
    // Destructor: clean up the widget references.
    void ~DiscordLoggedInWidget()
    {
        // Hide and delete the widgets if needed.
        if (m_Root)
            delete m_Root;
    }
    
    // UpdateData – This method sets the username text and loads a new image for the avatar.
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
    
    // HideAvatar – Hides the avatar image from view.
    void HideAvatar()
    {
        if (m_Root)
            m_Root.Show(false);
    }
    
    // ShowAvatar – Shows the avatar image.
    void ShowAvatar()
    {
        if (m_Root)
            m_Root.Show(true);
    }
	
	bool IsSet(){
		return m_isSet;
	}
	
	void ToggleAvatar(){
		if (m_Root)
		{
			m_isShowing = !m_isShowing;
			m_Root.Show(m_isShowing);
		}
	}
}
