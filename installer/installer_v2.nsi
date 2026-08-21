; CT 超分重建系统 V2 —— NSIS 安装脚本
!define APP_NAME "CT超分重建系统V2"
!define APP_EXE  "AiMedicalWorkstation.exe"
!define APP_PUBL "SwinIR-Med"
!define APP_VER   "2.0.0"

Name "${APP_NAME}"
OutFile "CT超分重建系统V2.exe"
InstallDir "$LOCALAPPDATA\${APP_NAME}"
RequestExecutionLevel user
ShowInstDetails show
ShowUnInstDetails show
Unicode true
Icon "..\resources\ai_medical_icon.ico"

Page directory
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles

Section "${APP_NAME} (主程序)"
  SectionIn RO
  SetOutPath "$INSTDIR"
  File /r "..\dist\*.*"
  File "..\resources\ai_medical_icon.ico"
  WriteUninstaller "$INSTDIR\uninstall.exe"

  CreateDirectory "$SMPROGRAMS\${APP_NAME}"
  CreateShortcut "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}" "" "$INSTDIR\ai_medical_icon.ico"
  CreateShortcut "$SMPROGRAMS\${APP_NAME}\卸载.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\ai_medical_icon.ico"
  CreateShortcut "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}" "" "$INSTDIR\ai_medical_icon.ico"

  WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayName" "${APP_NAME}"
  WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "UninstallString" "$INSTDIR\uninstall.exe"
  WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayVersion" "${APP_VER}"
  WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "Publisher" "${APP_PUBL}"
SectionEnd

Section "Uninstall"
  Delete "$DESKTOP\${APP_NAME}.lnk"
  RMDir /r "$SMPROGRAMS\${APP_NAME}"
  Delete "$INSTDIR\${APP_EXE}"
  Delete "$INSTDIR\uninstall.exe"
  Delete "$INSTDIR\ai_medical_icon.ico"
  Delete "$INSTDIR\*.dll"
  Delete "$INSTDIR\*.bat"
  RMDir /r "$INSTDIR\platforms"
  RMDir /r "$INSTDIR\styles"
  RMDir /r "$INSTDIR\imageformats"
  RMDir /r "$INSTDIR\iconengines"
  RMDir /r "$INSTDIR\generic"
  RMDir /r "$INSTDIR\networkinformation"
  RMDir /r "$INSTDIR\tls"
  RMDir /r "$INSTDIR\sqldrivers"
  RMDir /r "$INSTDIR\model"
  RMDir /r "$INSTDIR\thirdparty"
  RMDir /r "$INSTDIR"
  DeleteRegKey SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"
SectionEnd
