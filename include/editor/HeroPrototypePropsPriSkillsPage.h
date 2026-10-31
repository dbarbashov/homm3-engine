// HeroPrototypePropsPriSkillsPage.h - the primary skills page of the hero prototype sheet
// (HeroPrototypePropsPriSkillsPage.cpp; GOG only): the primary skills dialog over the page's frame,
// filled with the edited prototype's primary skills. Layout from the image:
// the original and edited prototypes at 0x8c, the modified flag and the
// dialog (0xa0 bytes).
#ifndef HOMM3_EDITOR_HEROPROTOTYPEPROPSPRISKILLSPAGE_H
#define HOMM3_EDITOR_HEROPROTOTYPEPROPSPRISKILLSPAGE_H

#include <memory>

#include "editor/resource.h"

class TPrimarySkillsDlg;
class THeroPrototype;

class THeroPrototypePropsPriSkillsPage : public CPropertyPage {
public:
    THeroPrototypePropsPriSkillsPage(const THeroPrototype& oldPrototype, THeroPrototype* pNewPrototype);
    virtual ~THeroPrototypePropsPriSkillsPage();

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_HERO_PROTOTYPE_PROPS_PRI_SKILLS };

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    DECLARE_MESSAGE_MAP()

private:
    const THeroPrototype& _m_oldPrototype;
    THeroPrototype* _m_pNewPrototype;
    bool _m_bModified;
    std::auto_ptr<TPrimarySkillsDlg> _m_pPrimarySkillsDlg;
};

#endif  /* HOMM3_EDITOR_HEROPROTOTYPEPROPSPRISKILLSPAGE_H */
