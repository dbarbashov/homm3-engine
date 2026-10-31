// ScenarioPropsStartingOptionsPage.h - the starting options page of the
// scenario properties sheet (ScenarioPropsStartingOptionsPage.cpp): a radio
// per option type (bonuses, a crossover from earlier scenarios, a starting
// hero) selects which of three child dialogs (the .cpp's own classes) shows
// in the frame and edits the scenario's options. The page visits the
// options it starts with to select their type. Layout from the image: the
// type at 0x90, the campaigns before and during the sheet at 0x94 and 0x98,
// the scenario at 0x9c, the modified flag at 0xa0, the type dialogs at 0xa4
// and the type radios at 0xac.
#ifndef HOMM3_CAMPAIGN_EDITOR_SCENARIOPROPSSTARTINGOPTIONSPAGE_H
#define HOMM3_CAMPAIGN_EDITOR_SCENARIOPROPSSTARTINGOPTIONSPAGE_H

#include <memory>

#include "campaign_editor/Campaign.h"
#include "campaign_editor/resource.h"

class TScenarioPropsStartingOptionsPage : public CPropertyPage, private TScenarioStartingOptions::TVisitor {
public:
    // The option types the radios offer, in dialog order: TScenarioOptionsType
    // less its none.
    enum { kNumTypes = 3 };

    TScenarioPropsStartingOptionsPage(const TCampaign& oldCampaign, TCampaign& newCampaign, int scenario,
                                      int campaignVersion);
    virtual ~TScenarioPropsStartingOptionsPage();

    bool wasModified() const { return _m_bModified; }
    // The page reads the scenario's map when it becomes active: nothing to
    // follow when the map changes.
    void update();

    enum { IDD = IDD_SCENARIO_PROPS_STARTING_OPTIONS };
    int _m_type;

    virtual BOOL OnInitDialog();
    virtual void OnOK();
    virtual BOOL OnSetActive();
    virtual BOOL OnKillActive();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    afx_msg void OnDestroy();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnTypeRadio();
    afx_msg void OnEnable(BOOL bEnable);
    DECLARE_MESSAGE_MAP()

private:
    struct _TTypeDlgs;

    virtual void visit(const TScenarioOptionsBonus& options);
    virtual void visit(const TScenarioOptionsCrossoverScenario& options);
    virtual void visit(const TScenarioOptionsStartingHero& options);

    // The page's own controls, before the type dialogs in the z-order.
    void _enableControls(bool bEnable);
    void _setType(int type);

    const TCampaign& _m_oldCampaign;
    TCampaign& _m_newCampaign;
    int _m_scenario;
    bool _m_bModified;
    std::auto_ptr<_TTypeDlgs> _m_pTypeDlgs;
    CButton _m_aTypeRadios[kNumTypes];
};

#endif  /* HOMM3_CAMPAIGN_EDITOR_SCENARIOPROPSSTARTINGOPTIONSPAGE_H */
