// CampaignView.h - the campaign view (CampaignView.cpp): an MFC CView
// (DECLARE_DYNCREATE, 0x4c bytes) that holds the campaign map window,
// names the selected region in the frame's status bar and opens the
// scenario and campaign property dialogs. The map window's client
// interface follows at +0x40, as the RTTI's displacement says; the status
// bar at 0x44 and the map window at 0x48.
#ifndef HOMM3_CAMPAIGN_EDITOR_CAMPAIGNVIEW_H
#define HOMM3_CAMPAIGN_EDITOR_CAMPAIGNVIEW_H

#include "campaign_editor/CampaignMapWnd.h"

class TCampaignDoc;

class TCampaignView : public CView, private TCampaignMapWndClient {
protected:
    TCampaignView();
    DECLARE_DYNCREATE(TCampaignView)

public:
    // The status bar: the main frame implements it (h3ccmped 0x489d2c).
    class TStatusUI {
    public:
        virtual void setScenarioName(const CString& name) = 0;
    };

    virtual ~TCampaignView();

    TCampaignDoc* GetDocument();
    void setStatusUI(TStatusUI* pStatusUI);
    CSize getMapSize();

    virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
    virtual void OnDraw(CDC* pDC);
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

protected:
    virtual void OnInitialUpdate();
    virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);

    afx_msg void OnDestroy();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnCampaignProperties();
    DECLARE_MESSAGE_MAP()

private:
    virtual const TCampaign* getCampaign();
    virtual void selectScenario(int scenario);
    virtual void editScenario(int scenario);

    TStatusUI* _m_pStatusUI;
    TCampaignMapWnd* _m_pMapWnd;
};

#endif  /* HOMM3_CAMPAIGN_EDITOR_CAMPAIGNVIEW_H */
