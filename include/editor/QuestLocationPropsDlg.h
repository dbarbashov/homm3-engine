// QuestLocationPropsDlg.h - the properties dialog a Seer's Hut and a Quest
// Guard share (QuestLocationPropsDlg.cpp; GOG only, Loki's port has no
// counterpart): the quest's description and its edit button, the optional
// deadline as month, week and day, and the proposal, progress and
// completion messages on a tab each. A message keeps the default text of
// the quest until customized; OK stays disabled while a customized
// message is blank. The derived dialogs own the map the dialog edits and
// pass it to _setNewMap; seerhut.txt's column of the default messages is
// theirs too (the Seer's Hut's 1, the Quest Guard's 4).
//
// Layout from the image: the map at 0x5c, the layer and id at 0x60 and
// 0x64, the text column at 0x68, the modified flag at 0x6c, the old
// location at 0x70, the edited map and location at 0x74 and 0x78, the
// edited quest at 0x7c, the shown message at 0x84, the messages at 0x88,
// the controls from 0xa0 and the number of blank customized messages at
// 0x424.
#ifndef HOMM3_EDITOR_QUESTLOCATIONPROPSDLG_H
#define HOMM3_EDITOR_QUESTLOCATIONPROPSDLG_H

#include <memory>

#include "editor/QuestLocation.h"
#include "editor/resource.h"

class TGameMap;
class TQuest;

class TQuestLocationPropsDlg : public CDialog {
public:
    TQuestLocationPropsDlg(UINT nIDTemplate, CWnd* pParent, TGameMap* pMap, bool bSecondLayer, unsigned int objID,
                           int textColumn);
    virtual ~TQuestLocationPropsDlg() = 0;

    bool wasModified() const { return _m_bModified; }

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    // The quest is about to change to pNewQuest (none when NULL): the
    // deadline and the messages follow it.
    virtual void _onQuestChanged(const TQuest* pNewQuest);
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnEditQuestButton();
    afx_msg void OnDeadlineCheck();
    afx_msg void OnKillFocusMonthEdit();
    afx_msg void OnKillFocusWeekEdit();
    afx_msg void OnKillFocusDayEdit();
    afx_msg void OnSelChangeMessageTab(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnCustomizeMessageCheck();
    afx_msg void OnChangeMessageEdit();
    DECLARE_MESSAGE_MAP()

    const TQuestLocation* _getOldLocation() const;
    void _setNewMap(TGameMap* pNewMap);

private:
    struct _TMessage {
        CString m_text;
        bool m_bCustomized;
        bool m_bBlank;
    };

    void _addBlankMessage();
    void _removeBlankMessage();
    CString _getDefaultMessage(const TQuest* pQuest, unsigned int message) const;

    TGameMap* _m_pMap;
    bool _m_bSecondLayer;
    unsigned int _m_objID;
    int _m_textColumn;
    bool _m_bModified;
    const TQuestLocation* _m_pOldLocation;
    TGameMap* _m_pNewMap;
    TQuestLocation* _m_pNewLocation;
    std::auto_ptr<TQuest> _m_pQuest;
    unsigned int _m_curMessage;
    _TMessage _m_aMessage[TQuestLocation::s_kNumMessages];
    CEdit _m_questEdit;
    CButton _m_deadlineCheck;
    CStatic _m_monthStatic;
    CEdit _m_monthEdit;
    CSpinButtonCtrl _m_monthSpin;
    CStatic _m_weekStatic;
    CEdit _m_weekEdit;
    CSpinButtonCtrl _m_weekSpin;
    CStatic _m_dayStatic;
    CEdit _m_dayEdit;
    CSpinButtonCtrl _m_daySpin;
    CTabCtrl _m_messageTab;
    CButton _m_customizeMessageCheck;
    CEdit _m_messageEdit;
    CButton _m_okButton;
    unsigned int _m_numBlankMessages;
};

#endif  /* HOMM3_EDITOR_QUESTLOCATIONPROPSDLG_H */
