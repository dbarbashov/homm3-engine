// CampaignEditor.h - the campaign editor application (CampaignEditor.cpp,
// the original's "Campaign Editor.cpp"; the unit name has no space). The
// application keeps the directory it found the game's Data directory in
// (0xc0): new campaigns start in its Maps directory, and opening a
// campaign moves the current directory to the campaign's.
#ifndef HOMM3_CAMPAIGN_EDITOR_CAMPAIGNEDITOR_H
#define HOMM3_CAMPAIGN_EDITOR_CAMPAIGNEDITOR_H

class TCampaignEditorApp : public CWinApp {
public:
    TCampaignEditorApp();

    virtual BOOL InitInstance();
    virtual int Run();
    virtual LRESULT ProcessWndProcException(CException* e, const MSG* pMsg);
    virtual CDocument* OpenDocumentFile(LPCTSTR lpszFileName);

protected:
    afx_msg void OnAppAbout();
    afx_msg void OnFileNew();
    DECLARE_MESSAGE_MAP()

private:
    CString _m_directory;
};

#endif  /* HOMM3_CAMPAIGN_EDITOR_CAMPAIGNEDITOR_H */
