!include "MUI2.nsh"

!macro customWelcomePage
  !define MUI_PAGE_CUSTOMFUNCTION_PRE WelcomePagePre
  !insertmacro MUI_PAGE_WELCOME

Function WelcomePagePre
  FindWindow $0 "#32770" "" $HWNDPARENT
  GetDlgItem $1 $0 1030  ; Get the welcome page text area

  # Explicitly set the Welcome Page message before opening the link
  SendMessage $1 ${WM_SETTEXT} 0 "STR:Welcome to Universal Framework Service!$\n$\nVisit daemonforge.dev for more info.$\n\n💜 Support development by sponsoring on GitHub!\n\nOpening donation page..."
  
  # Slight delay to ensure text updates before opening the browser
  Sleep 500
  
  # Auto-open the GitHub Sponsors link
  ExecShell "open" "https://github.com/sponsors/DaemonF0rge"
FunctionEnd

!macroend