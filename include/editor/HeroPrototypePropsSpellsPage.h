// HeroPrototypePropsSpellsPage.h - the spells page of the hero prototype sheet
// (HeroPrototypePropsSpellsPage.cpp; GOG only): the spells dialog over the page's frame,
// filled with the edited prototype's spells. Layout from the image:
// the original and edited prototypes at 0x8c, the map version, the modified flag and the
// dialog (0xa4 bytes).
#ifndef HOMM3_EDITOR_HEROPROTOTYPEPROPSSPELLSPAGE_H
#define HOMM3_EDITOR_HEROPROTOTYPEPROPSSPELLSPAGE_H

#include <memory>

#include "gameversion.h"
#include "editor/resource.h"

class TSpellsDlg;
class THeroPrototype;

class THeroPrototypePropsSpellsPage : public CPropertyPage {
public:
    THeroPrototypePropsSpellsPage(const THeroPrototype& oldPrototype, THeroPrototype* pNewPrototype, EGameVersion mapVersion);
    virtual ~THeroPrototypePropsSpellsPage();

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_HERO_PROTOTYPE_PROPS_SPELLS };

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    DECLARE_MESSAGE_MAP()

private:
    const THeroPrototype& _m_oldPrototype;
    THeroPrototype* _m_pNewPrototype;
    EGameVersion _m_mapVersion;
    bool _m_bModified;
    std::auto_ptr<TSpellsDlg> _m_pSpellsDlg;
};

#endif  /* HOMM3_EDITOR_HEROPROTOTYPEPROPSSPELLSPAGE_H */
