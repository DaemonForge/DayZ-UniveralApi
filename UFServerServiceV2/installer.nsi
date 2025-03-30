!macro customWelcomePage
  !define MUI_PAGE_CUSTOMFUNCTION_PRE WelcomePagePre
  !define MUI_WELCOMEPAGE_TITLE "Welcome to Universal Framework Service"
  !define MUI_WELCOMEPAGE_TEXT "Visit daemonforge.dev for more info.$\r$\n💜 Support development by sponsoring on GitHub!$\r$\n$\r$\nOpening donation page..."
  !insertmacro MUI_PAGE_WELCOME

Function WelcomePagePre
  # Slight delay to ensure text updates before opening the browser
  Sleep 1800
  
  # Auto-open the GitHub Sponsors link
  ExecShell "open" "https://github.com/sponsors/DaemonF0rge"
FunctionEnd

!macroend