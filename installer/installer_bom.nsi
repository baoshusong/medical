; CT 超分重建系统 —— NSIS 安装脚本
; 编译: makensis.exe installer.nsi  (需 NSIS, 见 thirdparty/build_nsis.sh 可下载便携版)
; 产出: CT超分重建系统_安装包.exe (单文件自解压安装/卸载)

!define APP_NAME "CT超分重建系统"
!define APP_EXE  "AiMedicalWorkstation.exe"
!define APP_PUBL "SwinIR-Med"
!define APP_VER   "1.0.0"

Name "${APP_NAME}"
OutFile "CT超分重建系统_安装包.exe"
InstallDir "$LOCALAPPDATA\${APP_NAME}"
RequestExecutionLevel user
ShowInstDetails show
ShowUnInstDetails show
Unicode true

; 安装到本机用户目录 (无需管理员), 卸载也干净
Page directory
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles

Section "${APP_NAME} (主程序)"
  SectionIn RO
  SetOutPath "$INSTDIR"
  ; 把 dist/ 全部内容打包进去
  File /r "..\dist\*.*"

  ; 卸载程序
  WriteUninstaller "$INSTDIR\uninstall.exe"

  ; 开始菜单快捷方式
  CreateDirectory "$SMPROGRAMS\${APP_NAME}"
  CreateShortcut  "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}" "" "$INSTDIR\${APP_EXE}"
  CreateShortcut  "$SMPROGRAMS\${APP_NAME}\卸载.lnk" "$INSTDIR\uninstall.exe"

  ; 桌面快捷方式 (可选, 注释掉则不创建)
  CreateShortcut "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}" "" "$INSTDIR\${APP_EXE}"

  ; 注册表 (卸载入口, 控制面板可见)
  WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayName" "${APP_NAME}"
  WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "UninstallString" "$INSTDIR\uninstall.exe"
  WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayVersion" "${APP_VER}"
  WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "Publisher" "${APP_PUBL}"
SectionEnd

Section "Uninstall"
  ; 先删快捷方式
  Delete "$DESKTOP\${APP_NAME}.lnk"
  RMDir /r "$SMPROGRAMS\${APP_NAME}"
  ; 删除程序文件 (保留用户可能改动? 这里全删)
  Delete "$INSTDIR\${APP_EXE}"
  Delete "$INSTDIR\uninstall.exe"
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
