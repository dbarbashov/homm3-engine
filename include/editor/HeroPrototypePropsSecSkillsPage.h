// HeroPrototypePropsSecSkillsPage.h - the secondary skills page of the hero prototype sheet
// (HeroPrototypePropsSecSkillsPage.cpp; GOG only): the secondary skills dialog over the page's frame,
// filled with the edited prototype's secondary skills. Layout from the image:
// the original and edited prototypes at 0x8c, the modified flag and the
// dialog (0xa0 bytes).
#ifndef HOMM3_EDITOR_HEROPROTOTYPEPROPSSECSKILLSPAGE_H
#define HOMM3_EDITOR_HEROPROTOTYPEPROPSSECSKILLSPAGE_H

#include <memory>

#include "editor/resource.h"

class TSecondarySkillsDlg;
class THeroPrototype;

class THeroPrototypePropsSecSkillsPage : public CPropertyPage {
public:
    THeroPrototypePropsSecSkillsPage(const THeroPrototype& oldPrototype, THeroPrototype* pNewPrototype);
    virtual ~THeroPrototypePropsSecSkillsPage();

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_HERO_PROTOTYPE_PROPS_SEC_SKILLS };

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    DECLARE_MESSAGE_MAP()

private:
    const THeroPrototype& _m_oldPrototype;
    THeroPrototype* _m_pNewPrototype;
    bool _m_bModified;
    std::auto_ptr<TSecondarySkillsDlg> _m_pSecondarySkillsDlg;
};

#endif  /* HOMM3_EDITOR_HEROPROTOTYPEPROPSSECSKILLSPAGE_H */
