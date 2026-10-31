// MainFrame.h - the campaign editor's main frame (MainFrame.cpp): an MFC
// single-document frame (DECLARE_DYNCREATE, 0x1dc bytes) with a tool bar
// and a two-pane status bar, which shows the selected region's name for
// the view (TCampaignView::TStatusUI at +0xbc). The frame never grows past
// the map window and its bars, and it refuses displays of fewer than 15
// bits per pixel. Menu texts, command prompts and tool tips come from the
// editor's text files. Layout from the image: the status bar at 0xc0, the
// tool bar at 0x13c and the view at 0x1d8.
#ifndef HOMM3_CAMPAIGN_EDITOR_MAINFRAME_H
#define HOMM3_CAMPAIGN_EDITOR_MAINFRAME_H

#include "campaign_editor/CampaignView.h"

class TMainFrame : public CFrameWnd, private TCampaignView::TStatusUI {
protected:
    TMainFrame();
    DECLARE_DYNCREATE(TMainFrame)

public:
    virtual ~TMainFrame();

    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    virtual void GetMessageString(UINT nID, CString& rMessage) const;

protected:
    virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);

    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
    afx_msg void OnWindowPosChanging(WINDOWPOS* lpwndpos);
    afx_msg LRESULT OnCommandHelp(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnDisplayChange(WPARAM wParam, LPARAM lParam);
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg BOOL OnQueryOpen();
    afx_msg BOOL OnToolTipText(UINT id, NMHDR* pNMHDR, LRESULT* pResult);
    DECLARE_MESSAGE_MAP()

private:
    virtual void setScenarioName(const CString& name);

    CSize _getMaxSize();

    CStatusBar _m_statusBar;
    CToolBar _m_toolBar;
    TCampaignView* _m_pView;
};

#endif  /* HOMM3_CAMPAIGN_EDITOR_MAINFRAME_H */
