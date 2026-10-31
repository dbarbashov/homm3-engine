// NewMapDlg.h - the new map dialog (NewMapDlg.cpp, h3maped
// 0x48a5ac..0x48b8ef; not in Loki): the new map's version, size and
// levels, and the random map generator's settings, shown only while the
// generate check is set. It runs from an in-memory copy of its dialog
// template whose size it sets to the collapsed one before
// InitModalIndirect.
#ifndef HOMM3_EDITOR_NEWMAPDLG_H
#define HOMM3_EDITOR_NEWMAPDLG_H

#include "editor/MapDoc.h"
#include "editor/resource.h"

class TNewMapDlg : public CDialog {
public:
    TNewMapDlg(CWnd* pParent, const TMapDoc::TNewMapParams& params);
    virtual ~TNewMapDlg();

    const TMapDoc::TNewMapParams& getParams() const { return _m_params; }

    enum { IDD = IDD_NEW_MAP };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSelchangeHumanPlayersCombo();
    afx_msg void OnSelchangeComputerPlayersCombo();
    afx_msg void OnGenerateRandomMapCheck();
    DECLARE_MESSAGE_MAP()

private:
    CButton _m_randomMapCheck;
    CComboBox _m_computerTeamsCombo;
    CComboBox _m_humanTeamsCombo;
    CComboBox _m_computerPlayersCombo;
    CComboBox _m_humanPlayersCombo;
    int _m_size;
    int _m_bTwoLevel;
    int _m_version;
    int _m_waterContent;
    int _m_monsterStrength;
    TMapDoc::TNewMapParams _m_params;
    DLGTEMPLATE* _m_pTemplate;
    // The window's sizes without and with the random map settings.
    CSize _m_collapsedSize;
    CSize _m_expandedSize;
};

#endif  /* HOMM3_EDITOR_NEWMAPDLG_H */
