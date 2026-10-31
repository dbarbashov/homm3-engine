// MapEditor.h - the map editor application ("Map Editor.cpp"; GOG only,
// Loki's GTK+ port starts from cppbridge.cpp's init_objects instead). An
// AppWizard MFC application: the single document template, the splash
// window and the about box are the source file's own classes. The
// application keeps its directory, the animation and idle clocks, and a
// watchdog thread that kicks the idle loop when the editor has been busy
// for ten seconds, so autosave and the animations go on. Layout from the
// constructor (0x140 bytes); the member names are not recorded.
#ifndef HOMM3_EDITOR_MAPEDITOR_H
#define HOMM3_EDITOR_MAPEDITOR_H

#include "editor/stdafx.h"

#include <afxmt.h>

class TMapEditorApp : public CWinApp {
public:
    TMapEditorApp();
    virtual ~TMapEditorApp();

    virtual BOOL InitInstance();
    virtual int Run();
    virtual BOOL OnIdle(LONG lCount);
    virtual int ExitInstance();
    virtual LRESULT ProcessWndProcException(CException* e, const MSG* pMsg);
    virtual CDocument* OpenDocumentFile(LPCTSTR lpszFileName);

protected:
    afx_msg void OnAppAbout();
    afx_msg void OnFileNew();
    afx_msg void OnViewObjectAnimation();
    afx_msg void OnUpdateViewObjectAnimation(CCmdUI* pCmdUI);
    afx_msg void OnViewTerrainAnimation();
    afx_msg void OnUpdateViewTerrainAnimation(CCmdUI* pCmdUI);
    afx_msg void OnViewGrid();
    afx_msg void OnUpdateViewGrid(CCmdUI* pCmdUI);
    afx_msg void OnViewPassability();
    afx_msg void OnUpdateViewPassability(CCmdUI* pCmdUI);
    afx_msg void OnToolsOptions();
    afx_msg void OnViewZoomIn();
    afx_msg void OnUpdateViewZoomIn(CCmdUI* pCmdUI);
    afx_msg void OnViewZoomOut();
    afx_msg void OnUpdateViewZoomOut(CCmdUI* pCmdUI);
    DECLARE_MESSAGE_MAP()

private:
    static UINT _watchdogThreadProc(LPVOID pParam);
    void _watchdog();

    CString _m_path;
    DWORD _m_animationTime;
    volatile DWORD _m_lastIdleTime;
    bool _m_bWatchdogStarted;
    CWinThread _m_watchdogThread;
    CEvent _m_quitEvent;
};

#endif  /* HOMM3_EDITOR_MAPEDITOR_H */
