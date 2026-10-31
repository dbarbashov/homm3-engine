// SeersHutPropsDlg.h - the Seer's Hut's properties dialog
// (SeersHutPropsDlg.cpp; GOG only, Loki's port has no counterpart): the
// quest location dialog over a copy of the map with the hut's reward, its
// description and the reward editor's button; the reward needs a quest.
// DoModal assigns the copy back when OK changed the quest location or the
// reward. The hut's default messages are seerhut.txt's second column.
//
// Layout from the image: the map, layer and id at 0x428, the old hut at
// 0x434, the map's copy at 0x438, its hut at 0x440, the edited reward at
// 0x444, the modified flag at 0x44c and the controls from 0x450.
#ifndef HOMM3_EDITOR_SEERSHUTPROPSDLG_H
#define HOMM3_EDITOR_SEERSHUTPROPSDLG_H

#include <memory>

#include "editor/QuestLocationPropsDlg.h"
#include "editor/SeersHut.h"
#include "editor/resource.h"

class TGameMap;

class TSeersHutPropsDlg : public TQuestLocationPropsDlg {
public:
    TSeersHutPropsDlg(CWnd* pParent, TGameMap* pMap, bool bSecondLayer, unsigned int objID);
    virtual ~TSeersHutPropsDlg();

    virtual int DoModal();
    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_SEERS_HUT_PROPS };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    virtual void _onQuestChanged(const TQuest* pNewQuest);
    afx_msg void OnEditRewardButton();
    DECLARE_MESSAGE_MAP()

private:
    const TSeersHut* _getOldHut() const;

    TGameMap* _m_pMap;
    bool _m_bSecondLayer;
    unsigned int _m_objID;
    const TSeersHut* _m_pOldHut;
    std::auto_ptr<TGameMap> _m_pNewMap;
    TSeersHut* _m_pNewHut;
    std::auto_ptr<TSeersHut::TReward> _m_pReward;
    bool _m_bModified;
    CStatic _m_rewardStatic;
    CButton _m_editRewardButton;
};

#endif  /* HOMM3_EDITOR_SEERSHUTPROPSDLG_H */
