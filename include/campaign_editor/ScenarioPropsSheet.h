// ScenarioPropsSheet.h - the scenario properties sheet
// (ScenarioPropsSheet.cpp): edits a copy of the document's campaign
// through its general, prologue, epilogue, crossover and starting options
// pages, and gives the copy back to the document when OK changed it.
// Layout from the image: the general page's sheet interface at 0x88, the
// document at 0x8c, the scenario at 0x90, the campaign copy at 0x94 and
// the pages from 0x9c.
#ifndef HOMM3_CAMPAIGN_EDITOR_SCENARIOPROPSSHEET_H
#define HOMM3_CAMPAIGN_EDITOR_SCENARIOPROPSSHEET_H

#include <memory>

#include "campaign_editor/Campaign.h"
#include "campaign_editor/ScenarioPropsCrossoverPage.h"
#include "campaign_editor/ScenarioPropsGeneralPage.h"
#include "campaign_editor/ScenarioPropsProloguePage.h"
#include "campaign_editor/ScenarioPropsStartingOptionsPage.h"

class TCampaignDoc;

class TScenarioPropsSheet : public CPropertySheet, private TScenarioPropsGeneralPage::TParentSheet {
public:
    TScenarioPropsSheet(CWnd* pParent, TCampaignDoc* pDoc, int scenario);
    virtual ~TScenarioPropsSheet();

    bool wasModified() const;

    virtual int DoModal();

protected:
    DECLARE_MESSAGE_MAP()

private:
    virtual void setMap(std::auto_ptr<TCampaignScenarioMap> pMap);
    virtual void removeMap();

    TCampaignDoc* _m_pDoc;
    int _m_scenario;
    std::auto_ptr<TCampaign> _m_pCampaign;
    std::auto_ptr<TScenarioPropsGeneralPage> _m_pGeneralPage;
    std::auto_ptr<TScenarioPropsProloguePage> _m_pProloguePage;
    std::auto_ptr<TScenarioPropsEpiloguePage> _m_pEpiloguePage;
    std::auto_ptr<TScenarioPropsCrossoverPage> _m_pCrossoverPage;
    std::auto_ptr<TScenarioPropsStartingOptionsPage> _m_pStartingOptionsPage;
};

#endif  /* HOMM3_CAMPAIGN_EDITOR_SCENARIOPROPSSHEET_H */
