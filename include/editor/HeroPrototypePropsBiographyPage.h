// HeroPrototypePropsBiographyPage.h - the biography page of the hero
// prototype sheet (HeroPrototypePropsBiographyPage.cpp; GOG only): an edit
// of the edited prototype's biography, its lines ended by CR LF. Layout
// from the image: the edit at 0x8c, the original and edited prototypes and
// the modified flag (0xd4 bytes).
#ifndef HOMM3_EDITOR_HEROPROTOTYPEPROPSBIOGRAPHYPAGE_H
#define HOMM3_EDITOR_HEROPROTOTYPEPROPSBIOGRAPHYPAGE_H

#include "editor/resource.h"

class THeroPrototype;

class THeroPrototypePropsBiographyPage : public CPropertyPage {
public:
    THeroPrototypePropsBiographyPage(const THeroPrototype& oldPrototype, THeroPrototype* pNewPrototype);
    virtual ~THeroPrototypePropsBiographyPage();

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_HERO_PROTOTYPE_PROPS_BIOGRAPHY };
    CEdit _m_biographyEdit;

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    DECLARE_MESSAGE_MAP()

private:
    const THeroPrototype& _m_oldPrototype;
    THeroPrototype* _m_pNewPrototype;
    bool _m_bModified;
};

#endif  /* HOMM3_EDITOR_HEROPROTOTYPEPROPSBIOGRAPHYPAGE_H */
