// HeroPrototypePropsArtifactsPage.h - the artifacts page of the hero
// prototype sheet (HeroPrototypePropsArtifactsPage.cpp; GOG only): the
// artifacts list over the page's frame, filled with the edited prototype's
// artifacts. Layout from the image: the original and edited prototypes at
// 0x8c, the map's version, the modified flag and the list (0xa4 bytes).
#ifndef HOMM3_EDITOR_HEROPROTOTYPEPROPSARTIFACTSPAGE_H
#define HOMM3_EDITOR_HEROPROTOTYPEPROPSARTIFACTSPAGE_H

#include <memory>

#include "gameversion.h"
#include "editor/resource.h"

class TArtifactsDlg;
class THeroPrototype;

class THeroPrototypePropsArtifactsPage : public CPropertyPage {
public:
    THeroPrototypePropsArtifactsPage(const THeroPrototype& oldPrototype, THeroPrototype* pNewPrototype,
                                     EGameVersion mapVersion);
    virtual ~THeroPrototypePropsArtifactsPage();

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_HERO_PROTOTYPE_PROPS_ARTIFACTS };

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    DECLARE_MESSAGE_MAP()

private:
    const THeroPrototype& _m_oldPrototype;
    THeroPrototype* _m_pNewPrototype;
    EGameVersion _m_mapVersion;
    bool _m_bModified;
    std::auto_ptr<TArtifactsDlg> _m_pArtifactsDlg;
};

#endif  /* HOMM3_EDITOR_HEROPROTOTYPEPROPSARTIFACTSPAGE_H */
