// QuestGuardPropsDlg.h - the Quest Guard's properties dialog
// (QuestGuardPropsDlg.cpp; GOG only, Loki's port has no counterpart): the
// quest location dialog over a copy of the map, which DoModal assigns
// back when OK changed it. The guard's default messages are seerhut.txt's
// fifth column.
//
// Layout from the image: the map at 0x428 and its copy at 0x42c.
#ifndef HOMM3_EDITOR_QUESTGUARDPROPSDLG_H
#define HOMM3_EDITOR_QUESTGUARDPROPSDLG_H

#include <memory>

#include "editor/QuestLocationPropsDlg.h"
#include "editor/resource.h"

class TGameMap;

class TQuestGuardPropsDlg : public TQuestLocationPropsDlg {
public:
    TQuestGuardPropsDlg(CWnd* pParent, TGameMap* pMap, bool bSecondLayer, unsigned int objID);
    virtual ~TQuestGuardPropsDlg();

    virtual int DoModal();

    enum { IDD = IDD_QUEST_GUARD_PROPS };

private:
    TGameMap* _m_pMap;
    std::auto_ptr<TGameMap> _m_pNewMap;
};

#endif  /* HOMM3_EDITOR_QUESTGUARDPROPSDLG_H */
