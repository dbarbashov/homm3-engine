// HeroPrototypePropsGeneralPage.h - the general page of the hero prototype
// sheet (HeroPrototypePropsGeneralPage.cpp; GOG only): the hero's name,
// experience, portrait and gender, and the players on the map who may hire
// the hero. It tells its sheet to hold OK off while the name is blank or
// no player may hire the hero.
//
// Layout from the image: the portrait spin, portrait, name edit and
// experience combo from 0x8c, the class name and gender radio index, the
// sheet, the hero, the original and edited prototypes, the players on the
// map, the modified and blank-name flags, the portrait, the players who
// may hire the hero and their count, the 163 portraits, the portrait
// bitmap and the eight players' checks (0x398 bytes).
#ifndef HOMM3_EDITOR_HEROPROTOTYPEPROPSGENERALPAGE_H
#define HOMM3_EDITOR_HEROPROTOTYPEPROPSGENERALPAGE_H

#include <memory>

#include "autoarrayptr.h"
#include "editor/DIBSection.h"
#include "editor/Hero.h"
#include "editor/Player.h"
#include "editor/resource.h"

class THeroPrototypePropsGeneralPage : public CPropertyPage {
public:
    // The sheet, told when its OK button may be pressed.
    class TParentSheet {
    public:
        virtual void onEnableOK() = 0;
        virtual void onDisableOK() = 0;
    };

    THeroPrototypePropsGeneralPage(TParentSheet* pParentSheet, THeroID heroID, const THeroPrototype& oldPrototype,
                                   THeroPrototype* pNewPrototype, const TPlayerMask& playersPresent);
    virtual ~THeroPrototypePropsGeneralPage();

    bool wasModified() const { return _m_bModified; }

    enum { IDD = IDD_HERO_PROTOTYPE_PROPS_GENERAL, s_kMaxExperience = 99999999 };
    CSpinButtonCtrl _m_portraitSpin;
    CStatic _m_portraitStatic;
    CEdit _m_nameEdit;
    CComboBox _m_experienceCombo;
    CString _m_className;
    int _m_gender;

    virtual BOOL OnInitDialog();
    virtual void OnOK();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    afx_msg void OnChangeNameEdit();
    afx_msg void OnDeltaposPortraitSpin(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnPlayer1Check();
    afx_msg void OnPlayer2Check();
    afx_msg void OnPlayer3Check();
    afx_msg void OnPlayer4Check();
    afx_msg void OnPlayer5Check();
    afx_msg void OnPlayer6Check();
    afx_msg void OnPlayer7Check();
    afx_msg void OnPlayer8Check();
    DECLARE_MESSAGE_MAP()

private:
    void _setPortrait();
    void _onPlayerCheck(TPlayer player);

    TParentSheet* _m_pParentSheet;
    THeroID _m_heroID;
    const THeroPrototype& _m_oldPrototype;
    THeroPrototype* _m_pNewPrototype;
    TPlayerMask _m_playersPresent;
    bool _m_bModified;
    bool _m_bNameBlank;
    int _m_portrait;
    TPlayerMask _m_availability;
    int _m_numAvailable;
    TAutoArrayPtr<std::auto_ptr<T16bppDIBSection> > _m_apPortrait;
    CBitmap _m_portraitBitmap;
    CButton _m_aPlayerCheck[kNumPlayers];
};

#endif  /* HOMM3_EDITOR_HEROPROTOTYPEPROPSGENERALPAGE_H */
