// MapEditorText.h - the editor's text strings (MapEditorText.cpp; Loki h3maped object 18).
// SText::initialize loads editor.txt once: the general strings, then one
// group per dialog, page and menu, each after its heading line. Every
// string is exported as a reference to its loaded pointer: users load the
// reference, then the pointer. The groups are namespaces (Loki's image
// names SArmyDlgText::{anonymous}::akStringImp, and __PRETTY_FUNCTION__
// prints "void SText::initialize()" without "static").
//
// Groups, counts and order are GOG's: each reference's dynamic initializer
// (0x462a63..0x4690fd) stores the address of its group's array element, so
// the initializers give every group's members in index order, and the
// image's editor.txt gives each group's heading and texts. GOG's general
// group no longer starts with the registry key. Names Loki's symbols prove
// are kept where the text keeps its meaning; the GOG-only strings and
// groups are named from their texts and headings, as no symbol survives.
// The counts' type is not recorded.
#ifndef HOMM3_EDITOR_MAPEDITORTEXT_H
#define HOMM3_EDITOR_MAPEDITORTEXT_H

enum { kNumGeneralStrings = 248 };
extern const char* const& kAppTitleStr;
extern const char* const& kCopyrightStr;
extern const char* const& kVersionFmtStr;
extern const char* const& kMapFileFilterStr;
extern const char* const& kMapFileRegNameStr;
extern const char* const& kMapSpecsSheetCaptionStr;
extern const char* const& kObjectPropertiesCaptionFmtStr;
extern const char* const& kEditTimedEventSheetCaptionStr;
extern const char* const& kSelNoneStr;
extern const char* const& kRandomStr;
extern const char* const& kUnknownStr;
extern const char* const& kBackpackStr;
extern const char* const& kExperienceStr;
extern const char* const& kManaStr;
extern const char* const& kMoraleStr;
extern const char* const& kLuckStr;
extern const char* const& kResourceStr;
extern const char* const& kResourcesStr;
extern const char* const& kPrimarySkillStr;
extern const char* const& kPrimarySkillsStr;
extern const char* const& kArtifactStr;
extern const char* const& kArtifactsStr;
extern const char* const& kSpellStr;
extern const char* const& kSpellsStr;
extern const char* const& kCreatureStr;
extern const char* const& kCreaturesStr;
extern const char* const& kSecondarySkillStr;
extern const char* const& kSecondarySkillsStr;
extern const char* const& kPlayerNameFmtStr;
extern const char* const& kPlayerNoneStr;
extern const char* const& kNeverStr;
extern const char* const& kEveryDayStr;
extern const char* const& kEvery2DaysStr;
extern const char* const& kEvery3DaysStr;
extern const char* const& kEvery4DaysStr;
extern const char* const& kEvery5DaysStr;
extern const char* const& kEvery6DaysStr;
extern const char* const& kEvery7DaysStr;
extern const char* const& kEvery14DaysStr;
extern const char* const& kEvery21DaysStr;
extern const char* const& kEvery28DaysStr;
extern const char* const& kMaxObjectsOfTypeFmtStr;
extern const char* const& kNoMoreClassHeroesStr;
extern const char* const& kSelectPlayerBeforePlacingHeroStr;
extern const char* const& kMaxHeroesPerPlayerFmtStr;
extern const char* const& kGrailAlreadyPlacedStr;
extern const char* const& kGrailPlacedTooCloseToEdgeFmtStr;
extern const char* const& kNowhereToPasteStr;
extern const char* const& kDebugBuildStr;
extern const char* const& kReleaseBuildStr;
extern const char* const& kNeed16BitColorStr;
extern const char* const& kInvalidMapVersionFmtStr;
extern const char* const& kInvalidMapFileStr;
extern const char* const& k2DaysStr;
extern const char* const& k3DaysStr;
extern const char* const& k4DaysStr;
extern const char* const& k5DaysStr;
extern const char* const& k6DaysStr;
extern const char* const& k1WeekStr;
extern const char* const& k2WeeksStr;
extern const char* const& k3WeeksStr;
extern const char* const& k4WeeksStr;
extern const char* const& k5WeeksStr;
extern const char* const& k6WeeksStr;
extern const char* const& k7WeeksStr;
extern const char* const& k2MonthsStr;
extern const char* const& k3MonthsStr;
extern const char* const& k4MonthsStr;
extern const char* const& k5MonthsStr;
extern const char* const& k6MonthsStr;
extern const char* const& k7MonthsStr;
extern const char* const& k8MonthsStr;
extern const char* const& k9MonthsStr;
extern const char* const& k10MonthsStr;
extern const char* const& k11MonthsStr;
extern const char* const& k12MonthsStr;
extern const char* const& kAnyTownStr;
extern const char* const& kObjectAtLocationFmtStr;
extern const char* const& kSpecificHeroAndClassFmtStr;
extern const char* const& kRandomMonsterStr;
extern const char* const& kRandomMonstersStr;
extern const char* const& kWarriorStr;
extern const char* const& kBuilderStr;
extern const char* const& kExplorerStr;
extern const char* const& kWarningStr;
extern const char* const& kHeroesNeedOwnerStr;
extern const char* const& kPlayerHasMaxHeroesStr;
extern const char* const& kContinuingWillDeleteHeroFmtStr;
extern const char* const& kNoMoreHeroesStr;
extern const char* const& kNoPlayersOnMapStr;
extern const char* const& kRandomTownStr;
extern const char* const& kTeamFmtStr;
extern const char* const& kThereAreNoPlayersOnMapStr;
extern const char* const& kThereAreNoTownsOnMapStr;
extern const char* const& kMapHasNoNameStr;
extern const char* const& kMapHasNoDescStr;
extern const char* const& kGrailPlacedButNoObelisksStr;
extern const char* const& kPlayerOwnsObjectsButNotPresentFmtStr;
extern const char* const& kTownHasShipyardButIsLandlockedFmtStr;
extern const char* const& kOnlyOneMonolithOnMapFmtStr;
extern const char* const& kMonolithEntranceButNoExitFmtStr;
extern const char* const& kMonolithExitButNoEntranceFmtStr;
extern const char* const& kSubterraneanGatesOnOneLayerMapStr;
extern const char* const& kMoreSubterraneanGatesAboveThanBelowStr;
extern const char* const& kMoreSubterraneanGatesBelowThanAboveStr;
extern const char* const& kSeersHutQuestArtifactNotOnMapFmtStr;
extern const char* const& kOnePlayerPresentButNoSpecialVictoryConditionStr;
extern const char* const& kOnePlayerPresentButNormalVictoryNotDisabledStr;
extern const char* const& kThereAreNoProblemsWithMapStr;
extern const char* const& kSVCAquireArtifactButArtifactNotPresentFmtStr;
extern const char* const& kSVCAquireArtifactButHeroHasArtifactFmtStr;
extern const char* const& kSVCAccumCreaturePlayerAlreadyHasEnoughCreaturesFmtStr;
extern const char* const& kSVCUpgradeTownBuildingsAlreadyBuiltFmtStr;
extern const char* const& kSVCUpgradeTownBuildingsDisabledFmtStr;
extern const char* const& kSVCBuildGrailStructAlreadyBuiltFmtStr;
extern const char* const& kSVCBuildGrailStructDisabledFmtStr;
extern const char* const& kSVCBuildGrailStructNowhereToBuildStr;
extern const char* const& kSVCDefeatHeroBelongsToHumanFmtStr;
extern const char* const& kSVCCaptureTownAlreadyOwnedFmtStr;
extern const char* const& kSVCCaptureTownOwnedByHumanFmtStr;
extern const char* const& kSVCFlagGeneratorsNoGeneratorsOnMapStr;
extern const char* const& kSVCFlagGeneratorsPlayerOwnsAllGeneratorsFmtStr;
extern const char* const& kSVCFlagMinesNoMinesOnMapStr;
extern const char* const& kSVCFlagMinesPlayerAlreadyOwnsAllMinesFmtStr;
extern const char* const& kSVCTransportArtifactNotPresentFmtStr;
extern const char* const& kSLCLoseTownNotOwnedFmtStr;
extern const char* const& kSLCLoseTownTeamNotHumanFmtStr;
extern const char* const& kSLCLoseTownMultipleHumanTeamsFmtStr;
extern const char* const& kSLCLoseTownPlayerNotHumanFmtStr;
extern const char* const& kSLCLoseTownMultipleHumanPlayersFmtStr;
extern const char* const& kSLCLoseHeroNotOwnedFmtStr;
extern const char* const& kSLCLoseHeroTeamNotHumanFmtStr;
extern const char* const& kSLCLoseHeroMultipleHumanTeamsFmtStr;
extern const char* const& kSLCLoseHeroPlayerNotHumanFmtStr;
extern const char* const& kSLCLoseHeroMultipleHumanPlayersFmtStr;
extern const char* const& kRecoverAutosavedMapStr;
extern const char* const& kTextFileFilterStr;
extern const char* const& kUnableToImportTextFmtStr;
extern const char* const& kTextImportedSuccessfullyFmtStr;
extern const char* const& kMapNameStr;
extern const char* const& kMapDescriptionStr;
extern const char* const& kRumorsStr;
extern const char* const& kTimedEventsStr;
extern const char* const& kObjectsStr;
extern const char* const& kNameStr;
extern const char* const& kMessageStr;
extern const char* const& kTextStr;
extern const char* const& kVisitingHeroFmtStr;
extern const char* const& kEndOfFileStr;
extern const char* const& kTimedEventNameFmtStr;
extern const char* const& kObjectNotFoundStr;
extern const char* const& kToolbarCaptionStr;
extern const char* const& kModeBarCaptionStr;
extern const char* const& kOneWhirlpoolStr;
extern const char* const& kObjectIsUnreachableFmtStr;
extern const char* const& kOKStr;
extern const char* const& kCancelStr;
extern const char* const& kHelpStr;
extern const char* const& kGeneralPageCaptionStr;
extern const char* const& kContentsPageCaptionStr;
extern const char* const& kResourcesPageCaptionStr;
extern const char* const& kBuildingsPageCaptionStr;
extern const char* const& kCreaturesPageCaptionStr;
extern const char* const& kSecondarySkillsPageCaptionStr;
extern const char* const& kSpecialLossConditionPageCaptionStr;
extern const char* const& kPlayerSpecsPageCaptionStr;
extern const char* const& kRumorsPageCaptionStr;
extern const char* const& kTeamsPageCaptionStr;
extern const char* const& kTimedEventsPageCaptionStr;
extern const char* const& kSpecialVictoryConditionPageCaptionStr;
extern const char* const& kTreasurePageCaptionStr;
extern const char* const& kGarrisonPageCaptionStr;
extern const char* const& kSpellsPageCaptionStr;
extern const char* const& kGuardiansPageCaptionStr;
extern const char* const& kArtifactsPageCaptionStr;
extern const char* const& kAboutBoxCaptionStr;
extern const char* const& kEditArtifactCaptionStr;
extern const char* const& kEditCreatureStackCaptionStr;
extern const char* const& kEditRumorCaptionStr;
extern const char* const& kEditSecondarySkillCaptionStr;
extern const char* const& kFindCaptionStr;
extern const char* const& kMapValidationCaptionStr;
extern const char* const& kNewMapCaptionStr;
extern const char* const& kOptionsCaptionStr;
extern const char* const& kSelectArtifactCaptionStr;
extern const char* const& kSelectHeroClassCaptionStr;
extern const char* const& kSelectSpellCaptionStr;
extern const char* const& kCustomizeCheckStr;
extern const char* const& kBrushStr;
extern const char* const& kTerrainTypeStr;
extern const char* const& kRiverTypeStr;
extern const char* const& kRoadTypeStr;
extern const char* const& kPlacementRadiusStaticStr;
extern const char* const& kStandStillStr;
extern const char* const& kRadiusOneSquareStr;
extern const char* const& kRadiusTwoSquaresStr;
extern const char* const& kRadiusThreeSquaresStr;
extern const char* const& kRadiusFourSquaresStr;
extern const char* const& kRadiusFiveSquaresStr;
extern const char* const& kRadiusSixSquaresStr;
extern const char* const& kRadiusSevenSquaresStr;
extern const char* const& kRadiusEightSquaresStr;
extern const char* const& kRadiusNineSquaresStr;
extern const char* const& kRadiusTenSquaresStr;
extern const char* const& kProblemsStaticStr;
extern const char* const& kNoPropertiesStr;
extern const char* const& kContinuingWillDeleteUndergroundLayerStr;
extern const char* const& kRandomDwellingFmtStr;
extern const char* const& kRandomTownDwellingFmtStr;
extern const char* const& kBiographyStr;
extern const char* const& kMapGenerationProgressCaptionStr;
extern const char* const& kGeneratingMapStr;
extern const char* const& kUnableToGenerateRandomMapStr;
extern const char* const& kHeroesStr;
extern const char* const& kQuestObjectHasNoQuestFmtStr;
extern const char* const& kObjectNeedsABOrSoDStr;
extern const char* const& kNoQuestStr;
extern const char* const& kQuestExperienceLevelFmtStr;
extern const char* const& kQuestPrimarySkillLevelFmtStr;
extern const char* const& kQuestAchieveFmtStr;
extern const char* const& kQuestAchieveTheFollowingStr;
extern const char* const& kQuestDefeatHeroFmtStr;
extern const char* const& kQuestDefeatMonsterFmtStr;
extern const char* const& kQuestReturnWithTheFollowingStr;
extern const char* const& kQuestReturnWithArtifactFmtStr;
extern const char* const& kQuestReturnWithCreaturesFmtStr;
extern const char* const& kQuestReturnWithResourcesFmtStr;
extern const char* const& kProposalMessageStr;
extern const char* const& kProgressMessageStr;
extern const char* const& kCompletionMessageStr;
extern const char* const& kAndStr;
extern const char* const& kUndergroundStr;
extern const char* const& kNoRewardStr;
extern const char* const& kRewardExperienceFmtStr;
extern const char* const& kRewardManaFmtStr;
extern const char* const& kRewardMoraleFmtStr;
extern const char* const& kRewardLuckFmtStr;
extern const char* const& kRewardSpellFmtStr;
extern const char* const& kKeymastersTentButNoBorderFmtStr;
extern const char* const& kBorderGuardButNoKeymastersTentFmtStr;
extern const char* const& kBorderGateButNoKeymastersTentFmtStr;
extern const char* const& kHutOfTheMagiButNoEyeStr;
extern const char* const& kEyeOfTheMagiButNoHutStr;
extern const char* const& kTooFewAvailableHeroesFmtStr;
extern const char* const& kAllArtifactsDisabledOrReservedStr;
extern const char* const& kObjectNeedsSoDStr;
extern const char* const& kObjectNeedsABInstalledStr;
extern const char* const& kHeroesPageCaptionStr;

// editor.txt "Abandoned Mine Properties Dialog"
namespace SAbandonedMinePropsDlgText {
enum { kNumStrings = 7 };
extern const char* const& kPotentialResourcesStaticStr;
extern const char* const& kRes2CheckStr;
extern const char* const& kRes3CheckStr;
extern const char* const& kRes4CheckStr;
extern const char* const& kRes5CheckStr;
extern const char* const& kRes6CheckStr;
extern const char* const& kRes7CheckStr;
}

// editor.txt "Army Dialog"
namespace SArmyDlgText {
enum { kNumStrings = 3 };
extern const char* const& kTypeStaticStr;
extern const char* const& kQuantityStaticStr;
extern const char* const& kSlotStaticFmtStr;
}

// editor.txt "Garrison Properties Dialog"
namespace SGarrisonPropertiesDlgText {
enum { kNumStrings = 5 };
extern const char* const& kOwnerStaticStr;
extern const char* const& kNoneRadioStr;
extern const char* const& kPlayerRadioFmtStr;
extern const char* const& kCreaturesStaticStr;
extern const char* const& kRemovableCheckStr;
}

// editor.txt "Artifact Properties General Page"
namespace SArtifactPropsGeneralPageText {
enum { kNumStrings = 4 };
extern const char* const& kTypeStaticStr;
extern const char* const& kMessageStaticStr;
extern const char* const& kNoteStaticStr;
extern const char* const& kSpellStaticStr;
}

// editor.txt "Black Box Properties General Page"
namespace SBlackBoxPropsGeneralPageText {
enum { kNumStrings = 1 };
extern const char* const& kMessageStaticStr;
}

// editor.txt "Event Properties General Page"
namespace SEventPropsGeneralPageText {
enum { kNumStrings = 4 };
extern const char* const& kAllowedPlayersStaticStr;
extern const char* const& kPlayerCheckFmtStr;
extern const char* const& kAllowComputerCheckStr;
extern const char* const& kCancelCheckStr;
}

// editor.txt "Black Box Properties Contents Page"
namespace SBlackBoxPropsContentsPageText {
enum { kNumStrings = 28 };
extern const char* const& kCategoryStaticStr;
extern const char* const& kAddButtonStr;
extern const char* const& kEditButtonStr;
extern const char* const& kRemoveButtonStr;
extern const char* const& kRemoveAllButtonStr;
extern const char* const& kArtifactsStaticStr;
extern const char* const& kCreatureTypeStaticStr;
extern const char* const& kQuantityStaticStr;
extern const char* const& kExperienceBonusStaticStr;
extern const char* const& kLuckBonusStaticStr;
extern const char* const& kNoneRadioStr;
extern const char* const& kPlusOneRadioStr;
extern const char* const& kPlusTwoRadioStr;
extern const char* const& kPlusThreeRadioStr;
extern const char* const& kMinusOneRadioStr;
extern const char* const& kMinusTwoRadioStr;
extern const char* const& kMinusThreeRadioStr;
extern const char* const& kManaBonusStaticStr;
extern const char* const& kGiveRadioStr;
extern const char* const& kTakeRadioStr;
extern const char* const& kMoraleBonusStaticStr;
extern const char* const& kAttackSkillStaticStr;
extern const char* const& kDefenseSkillStaticStr;
extern const char* const& kSpellPowerStaticStr;
extern const char* const& kKnowledgeStaticStr;
extern const char* const& kSecondarySkillStaticStr;
extern const char* const& kMasteryStaticStr;
extern const char* const& kSpellsStaticStr;
}

// editor.txt "Resource Quantities Dialog"
namespace SResourceQuantitiesDlgText {
enum { kNumStrings = 9 };
extern const char* const& kRes1StaticStr;
extern const char* const& kRes2StaticStr;
extern const char* const& kRes3StaticStr;
extern const char* const& kRes4StaticStr;
extern const char* const& kRes5StaticStr;
extern const char* const& kRes6StaticStr;
extern const char* const& kRes7StaticStr;
extern const char* const& kGiveRadioStr;
extern const char* const& kTakeRadioStr;
}

// editor.txt "Edit Secondary Skill Dialog"
namespace SEditSecondarySkillDlgText {
enum { kNumStrings = 5 };
extern const char* const& kTypeStaticStr;
extern const char* const& kMasteryStaticStr;
extern const char* const& kBasicRadioStr;
extern const char* const& kAdvancedRadioStr;
extern const char* const& kExpertRadioStr;
}

// editor.txt "Edit Creature Stack Dialog"
namespace SEditCreatureStackDlgText {
enum { kNumStrings = 2 };
extern const char* const& kTypeStaticStr;
extern const char* const& kQuantityStaticStr;
}

// editor.txt "Edit Timed Event General Page"
namespace SEditTimedEventGeneralPageText {
enum { kNumStrings = 8 };
extern const char* const& kEventNameStaticStr;
extern const char* const& kMessageStaticStr;
extern const char* const& kAllowedPlayersStaticStr;
extern const char* const& kPlayerCheckFmtStr;
extern const char* const& kAllowComputerCheckStr;
extern const char* const& kFirstOccurenceStaticStr;
extern const char* const& kSubsequentOccurenceStaticStr;
extern const char* const& kAllowHumanCheckStr;
}

// editor.txt "Edit Town Event Buildings Page"
namespace SEditTownEventBuildingsPageText {
enum { kNumStrings = 2 };
extern const char* const& kBuildingTreeStaticStr;
extern const char* const& kBuildCheckStr;
}

// editor.txt "Edit Town Event Creatures Page"
namespace SEditTownEventCreaturesPageText {
enum { kNumStrings = 3 };
extern const char* const& kGeneratorNoStaticStr;
extern const char* const& kQuantityStaticStr;
extern const char* const& kNoteStaticStr;
}

// editor.txt "Edit Artifact Dialog"
namespace SEditArtifactDlgText {
enum { kNumStrings = 2 };
extern const char* const& kEquipWhereStaticStr;
extern const char* const& kArtifactStaticStr;
}

// editor.txt "Edit Rumor Dialog"
namespace SEditRumorDlgText {
enum { kNumStrings = 2 };
extern const char* const& kNameStaticStr;
extern const char* const& kTextStaticStr;
}

// editor.txt "Find Dialog"
namespace SFindDlgText {
enum { kNumStrings = 3 };
extern const char* const& kFindWhatStaticStr;
extern const char* const& kFindNextButtonStr;
extern const char* const& kFindPrevButtonStr;
}

// editor.txt "Flaggable Properties Dialog"
namespace SFlaggablePropsDlgText {
enum { kNumStrings = 3 };
extern const char* const& kOwnerStaticStr;
extern const char* const& kNoneRadioStr;
extern const char* const& kPlayerRadioFmtStr;
}

// editor.txt "Hero Properties General Page"
namespace SHeroPropsGeneralPageText {
enum { kNumStrings = 16 };
extern const char* const& kClassStaticStr;
extern const char* const& kPlayerStaticStr;
extern const char* const& kIdentityStaticStr;
extern const char* const& kNameStaticStr;
extern const char* const& kExperienceStaticStr;
extern const char* const& kPortraitStaticStr;
extern const char* const& kPatrolStaticStr;
extern const char* const& kCustomizeNameCheckStr;
extern const char* const& kCustomizePortraitCheckStr;
extern const char* const& kGenderStaticStr;
extern const char* const& kDefaultGenderRadioFmtStr;
extern const char* const& kMaleStr;
extern const char* const& kFemaleStr;
extern const char* const& kMaleRadioStr;
extern const char* const& kFemaleRadioStr;
extern const char* const& kCustomizeGenderCheckStr;
}

// editor.txt "Hero Properties Creatures Page"
namespace SHeroPropsCreaturesPageText {
enum { kNumStrings = 3 };
extern const char* const& kFormationStaticStr;
extern const char* const& kSpreadRadioStr;
extern const char* const& kGroupedRadioStr;
}

// editor.txt "Secondary Skills Dialog"
namespace SHeroPropsSecSkillsPageText {
enum { kNumStrings = 3 };
extern const char* const& kTypeStaticStr;
extern const char* const& kMasteryStaticStr;
extern const char* const& kSkillStaticFmtStr;
}

// editor.txt "Hero Properties Artifacts Page"
namespace SHeroPropsArtifactsPageText {
enum { kNumStrings = 8 };
extern const char* const& kHasSpellbookCheckStr;
extern const char* const& kArtifactsStaticStr;
extern const char* const& kNameColumnStr;
extern const char* const& kLocationColumnStr;
extern const char* const& kAddButtonStr;
extern const char* const& kEditButtonStr;
extern const char* const& kRemoveButtonStr;
extern const char* const& kRemoveAllButtonStr;
}

// editor.txt "Map Specs General Page"
namespace SMapSpecsGeneralPageText {
enum { kNumStrings = 14 };
extern const char* const& kDifficultyStaticStr;
extern const char* const& kEasyRadioStr;
extern const char* const& kNormalRadioStr;
extern const char* const& kToughRadioStr;
extern const char* const& kExpertRadioStr;
extern const char* const& kImpossibleRadioStr;
extern const char* const& kTwoLevelMapCheckStr;
extern const char* const& kMapNameStaticStr;
extern const char* const& kDescriptionStaticStr;
extern const char* const& kLimitHeroLevelCheckStr;
extern const char* const& kMapVersionStaticStr;
extern const char* const& kRoERadioStr;
extern const char* const& kABRadioStr;
extern const char* const& kSoDRadioStr;
}

// editor.txt "Map Specs Player Specs Page"
namespace SMapSpecsPlayerSpecsPageText {
enum { kNumStrings = 19 };
extern const char* const& kPlayerStaticStr;
extern const char* const& kGenerateHeroCheckStr;
extern const char* const& kPlayabilityStaticStr;
extern const char* const& kHumanPlayableCheckStr;
extern const char* const& kComputerPlayableCheckStr;
extern const char* const& kBehaviorStaticStr;
extern const char* const& kHasMainTownCheckStr;
extern const char* const& kAllowedAlignmentsStaticStr;
extern const char* const& kCustomizeCheckStr;
extern const char* const& kRandomTownCheckStr;
extern const char* const& kCastleCheckStr;
extern const char* const& kRampartCheckStr;
extern const char* const& kTowerCheckStr;
extern const char* const& kInfernoCheckStr;
extern const char* const& kNecropolisCheckStr;
extern const char* const& kDungeonCheckStr;
extern const char* const& kStrongholdCheckStr;
extern const char* const& kFortressCheckStr;
extern const char* const& kConfluxCheckStr;
}

// editor.txt "Map Specs Teams Page"
namespace SMapSpecsTeamsPageText {
enum { kNumStrings = 5 };
extern const char* const& kEnableTeamsCheckStr;
extern const char* const& kNumberOfTeamsStaticStr;
extern const char* const& kTeamAssignmentsStaticStr;
extern const char* const& kTeamNumberStaticStr;
extern const char* const& kPlayerStaticFmtStr;
}

// editor.txt "Map Specs Rumors Page"
namespace SMapSpecsRumorsPageText {
enum { kNumStrings = 5 };
extern const char* const& kRumorsStaticStr;
extern const char* const& kAddButtonStr;
extern const char* const& kEditButtonStr;
extern const char* const& kRemoveButtonStr;
extern const char* const& kRemoveAllButtonStr;
}

// editor.txt "Map Specs Timed Events Page"
namespace SMapSpecsTimedEventsPageText {
enum { kNumStrings = 7 };
extern const char* const& kEventsStaticStr;
extern const char* const& kAddButtonStr;
extern const char* const& kEditButtonStr;
extern const char* const& kRemoveButtonStr;
extern const char* const& kRemoveAllButtonStr;
extern const char* const& kMoveUpButtonStr;
extern const char* const& kMoveDownButtonStr;
}

// editor.txt "Map Specs Special Victory Condition Page"
namespace SMapSpecsVictoryCondPageText {
enum { kNumStrings = 31 };
extern const char* const& kSelectVictoryConditionStaticStr;
extern const char* const& kNoneRadioStr;
extern const char* const& kAquireArtifactRadioStr;
extern const char* const& kAccumCreaturesRadioStr;
extern const char* const& kAccumResourcesRadioStr;
extern const char* const& kUpgradeTownRadioStr;
extern const char* const& kBuildGrailRadioStr;
extern const char* const& kDefeatHeroRadioStr;
extern const char* const& kCaptureTownRadioStr;
extern const char* const& kDefeatMonsterRadioStr;
extern const char* const& kFlagGeneratorsRadioStr;
extern const char* const& kFlagMinesRadioStr;
extern const char* const& kTransportArtifactRadioStr;
extern const char* const& kNormalVictoryCheckStr;
extern const char* const& kComputerAlsoCheckStr;
extern const char* const& kArtifactStaticStr;
extern const char* const& kCreatureTypeStaticStr;
extern const char* const& kQuantityStaticStr;
extern const char* const& kResourceTypeStaticStr;
extern const char* const& kTownStaticStr;
extern const char* const& kHallLevelStaticStr;
extern const char* const& kTownHallRadioStr;
extern const char* const& kCityHallRadioStr;
extern const char* const& kCapitolHallRadioStr;
extern const char* const& kCastleLevelStaticStr;
extern const char* const& kFortCastleRadioStr;
extern const char* const& kCitadelCastleRadioStr;
extern const char* const& kCastleCastleRadioStr;
extern const char* const& kHeroStaticStr;
extern const char* const& kMonsterStaticStr;
extern const char* const& kDestinationStaticStr;
}

// editor.txt "Map Specs Special Loss Condition Page"
namespace SMapSpecsLossCondPageText {
enum { kNumStrings = 8 };
extern const char* const& kSelectLossConditionStaticStr;
extern const char* const& kNoneRadioStr;
extern const char* const& kLoseTownRadioStr;
extern const char* const& kLoseHeroRadioStr;
extern const char* const& kTimeExpiresRadioStr;
extern const char* const& kTownStaticStr;
extern const char* const& kHeroStaticStr;
extern const char* const& kTimeLimitStaticStr;
}

// editor.txt "Monster Properties General Page"
namespace SMonsterPropsGeneralPageText {
enum { kNumStrings = 13 };
extern const char* const& kTypeStaticStr;
extern const char* const& kQuantityStaticStr;
extern const char* const& kRandomQtyRadioStr;
extern const char* const& kCustomQtyRadioStr;
extern const char* const& kNeverGrowsCheckStr;
extern const char* const& kDispositionStaticStr;
extern const char* const& kDispositionCompliantRadioStr;
extern const char* const& kDispositionFriendlyRadioStr;
extern const char* const& kDispositionAggressiveRadioStr;
extern const char* const& kDispositionHostileRadioStr;
extern const char* const& kDispositionSavageRadioStr;
extern const char* const& kNeverFleesCheckStr;
extern const char* const& kMessageStaticStr;
}

// editor.txt "Monster Properties Treasure Page"
namespace SMonsterPropsTreasurePageText {
enum { kNumStrings = 9 };
extern const char* const& kResourcesStaticStr;
extern const char* const& kRes1StaticStr;
extern const char* const& kRes2StaticStr;
extern const char* const& kRes3StaticStr;
extern const char* const& kRes4StaticStr;
extern const char* const& kRes5StaticStr;
extern const char* const& kRes6StaticStr;
extern const char* const& kRes7StaticStr;
extern const char* const& kArtifactStaticStr;
}

// editor.txt "New Map Dialog"
namespace SNewMapDlgText {
enum { kNumStrings = 25 };
extern const char* const& kMapSizeStaticStr;
extern const char* const& k36x36RadioStr;
extern const char* const& k72x72RadioStr;
extern const char* const& k108x108RadioStr;
extern const char* const& k144x144RadioStr;
extern const char* const& kTwoLevelMapCheckStr;
extern const char* const& kMapVersionStaticStr;
extern const char* const& kRoERadioStr;
extern const char* const& kABRadioStr;
extern const char* const& kGenerateRandomMapCheckStr;
extern const char* const& kNumberOfPlayersStaticStr;
extern const char* const& kHumanOrComputerStaticStr;
extern const char* const& kComputerOnlyStaticStr;
extern const char* const& kNumberOfTeamsStaticStr;
extern const char* const& kWaterContentStaticStr;
extern const char* const& kWaterRandomRadioStr;
extern const char* const& kWaterNoneRadioStr;
extern const char* const& kWaterNormalRadioStr;
extern const char* const& kWaterIslandsRadioStr;
extern const char* const& kMonsterStrengthStaticStr;
extern const char* const& kMonsterRandomRadioStr;
extern const char* const& kMonsterWeakRadioStr;
extern const char* const& kMonsterNormalRadioStr;
extern const char* const& kMonsterStrongRadioStr;
extern const char* const& kSoDRadioStr;
}

// editor.txt "Options Dialog"
namespace SOptionsDlgText {
enum { kNumStrings = 8 };
extern const char* const& kTileFrequencyStaticStr;
extern const char* const& kNoneStaticStr;
extern const char* const& kLotsStaticStr;
extern const char* const& kRepaintCheckStr;
extern const char* const& kAutosaveOptionsStaticStr;
extern const char* const& kEnableAutosaveCheckStr;
extern const char* const& kAutosaveEveryStaticStr;
extern const char* const& kMinutesStaticStr;
}

// editor.txt "Resource Properties General Page"
namespace SResourcePropsGeneralPageText {
enum { kNumStrings = 7 };
extern const char* const& kTypeStaticStr;
extern const char* const& kQuantityStaticStr;
extern const char* const& kRandomQtyRadioStr;
extern const char* const& kCustomQtyRadioStr;
extern const char* const& kQuantityNoteStaticStr;
extern const char* const& kMessageStaticStr;
extern const char* const& kNoteStaticStr;
}

// editor.txt "Scholar Properties Dialog"
namespace SScholarPropsDlgText {
enum { kNumStrings = 5 };
extern const char* const& kRewardStaticStr;
extern const char* const& kRandomRewardRadioStr;
extern const char* const& kPriSkillRewardRadioStr;
extern const char* const& kSecSkillRewardRadioStr;
extern const char* const& kSpellRewardRadioStr;
}

// editor.txt "Seer's Hut Properties Dialog"
namespace SSeersHutPropsDlgText {
enum { kNumStrings = 11 };
extern const char* const& kQuestStaticStr;
extern const char* const& kEditQuestButtonStr;
extern const char* const& kHasDeadlineCheckStr;
extern const char* const& kMonthStaticStr;
extern const char* const& kWeekStaticStr;
extern const char* const& kDayStaticStr;
extern const char* const& kCustomizeCheckStr;
extern const char* const& kRewardStaticStr;
extern const char* const& kEditRewardButtonStr;
extern const char* const& kBeHeroFmtStr;
extern const char* const& kBelongToPlayerFmtStr;
}

// editor.txt "Shrine Properties Dialog"
namespace SShrinePropsDlgText {
enum { kNumStrings = 3 };
extern const char* const& kSpellStaticFmtStr;
extern const char* const& kRandomSpellRadioStr;
extern const char* const& kCustomSpellRadioStr;
}

// editor.txt "Sign Properties Dialog"
namespace SSignPropsDlgText {
enum { kNumStrings = 1 };
extern const char* const& kMessageStaticStr;
}

// editor.txt "Town Properties General Page"
namespace STownPropsGeneralPageText {
enum { kNumStrings = 13 };
extern const char* const& kTownTypeStaticStr;
extern const char* const& kPlayerStaticStr;
extern const char* const& kTownNameStaticStr;
extern const char* const& kCustomizeCheckStr;
extern const char* const& kVisitingHeroStaticStr;
extern const char* const& kHeroNameStaticStr;
extern const char* const& kHeroClassStaticStr;
extern const char* const& kAddButtonStr;
extern const char* const& kEditButtonStr;
extern const char* const& kRemoveButtonStr;
extern const char* const& kAlignmentStaticStr;
extern const char* const& kSameAsTownFmtStr;
extern const char* const& kSameAsOwnerOrRandomStr;
}

// editor.txt "Town Properties Garrison Page"
namespace STownPropsGarrisonPageText {
enum { kNumStrings = 3 };
extern const char* const& kFormationStaticStr;
extern const char* const& kSpreadRadioStr;
extern const char* const& kGroupedRadioStr;
}

// editor.txt "Town Properties Buildings Page"
namespace STownPropsBuildingsPageText {
enum { kNumStrings = 6 };
extern const char* const& kHasFortCheckStr;
extern const char* const& kBuildingTreeStaticStr;
extern const char* const& kEnabledCheckStr;
extern const char* const& kBuiltCheckStr;
extern const char* const& kBuildAllButtonStr;
extern const char* const& kDemolishAllButtonStr;
}

// editor.txt "Town Properties Spells Page"
namespace STownPropsSpellsPageText {
enum { kNumStrings = 3 };
extern const char* const& kSpellLevelStaticStr;
extern const char* const& kMustAppearStaticStr;
extern const char* const& kMayAppearStaticStr;
}

// editor.txt "Town Properties Timed Events Page"
namespace STownPropsTimedEventsPageText {
enum { kNumStrings = 7 };
extern const char* const& kEventsStaticStr;
extern const char* const& kAddButtonStr;
extern const char* const& kEditButtonStr;
extern const char* const& kRemoveButtonStr;
extern const char* const& kRemoveAllButtonStr;
extern const char* const& kMoveUpButtonStr;
extern const char* const& kMoveDownButtonStr;
}

// editor.txt "Edit Quest Dialog"
namespace SEditQuestDlgText {
enum { kNumStrings = 36 };
extern const char* const& kCaptionStr;
extern const char* const& kSelectQuestTypeStaticStr;
extern const char* const& kNoneRadioStr;
extern const char* const& kExperienceLevelRadioStr;
extern const char* const& kPrimarySkillLevelRadioStr;
extern const char* const& kDefeatHeroRadioStr;
extern const char* const& kDefeatMonsterRadioStr;
extern const char* const& kArtifactsRadioStr;
extern const char* const& kCreaturesRadioStr;
extern const char* const& kResourcesRadioStr;
extern const char* const& kExperienceLevelStaticStr;
extern const char* const& kAttackSkillStaticStr;
extern const char* const& kDefenseSkillStaticStr;
extern const char* const& kSpellPowerStaticStr;
extern const char* const& kKnowledgeStaticStr;
extern const char* const& kHeroStaticStr;
extern const char* const& kMonsterStaticStr;
extern const char* const& kArtifactsStaticStr;
extern const char* const& kAddButtonStr;
extern const char* const& kEditButtonStr;
extern const char* const& kRemoveButtonStr;
extern const char* const& kRemoveAllButtonStr;
extern const char* const& kCreatureTypeStaticStr;
extern const char* const& kQuantityStaticStr;
extern const char* const& kRes0StaticStr;
extern const char* const& kRes1StaticStr;
extern const char* const& kRes2StaticStr;
extern const char* const& kRes3StaticStr;
extern const char* const& kRes4StaticStr;
extern const char* const& kRes5StaticStr;
extern const char* const& kRes6StaticStr;
extern const char* const& kBeHeroRadioStr;
extern const char* const& kBelongToPlayerRadioStr;
extern const char* const& kClassStaticStr;
extern const char* const& kIdentityStaticStr;
extern const char* const& kPlayerStaticStr;
}

// editor.txt "Edit Reward Dialog"
namespace SEditRewardDlgText {
enum { kNumStrings = 26 };
extern const char* const& kCaptionStr;
extern const char* const& kSelectRewardTypeStaticStr;
extern const char* const& kNoneRadioStr;
extern const char* const& kExperienceRadioStr;
extern const char* const& kManaRadioStr;
extern const char* const& kMoraleRadioStr;
extern const char* const& kLuckRadioStr;
extern const char* const& kResourceRadioStr;
extern const char* const& kPrimarySkillRadioStr;
extern const char* const& kSecondarySkillRadioStr;
extern const char* const& kArtifactRadioStr;
extern const char* const& kSpellRadioStr;
extern const char* const& kCreaturesRadioStr;
extern const char* const& kExperienceBonusStaticStr;
extern const char* const& kManaBonusStaticStr;
extern const char* const& kMoraleBonusStaticStr;
extern const char* const& kLuckBonusStaticStr;
extern const char* const& kResourceTypeStaticStr;
extern const char* const& kQuantityStaticStr;
extern const char* const& kPrimarySkillStaticStr;
extern const char* const& kBonusStaticStr;
extern const char* const& kSecondarySkillStaticStr;
extern const char* const& kMasteryStaticStr;
extern const char* const& kArtifactStaticStr;
extern const char* const& kSpellStaticStr;
extern const char* const& kCreatureTypeStaticStr;
}

// editor.txt "Hero Placeholder Properties Dialog"
namespace SHeroPlaceholderPropsDlgText {
enum { kNumStrings = 7 };
extern const char* const& kOwnerStaticStr;
extern const char* const& kIdentityStaticStr;
extern const char* const& kAnyHeroRadioStr;
extern const char* const& kSpecificHeroRadioStr;
extern const char* const& kPowerRatingStaticStr;
extern const char* const& kClassStaticStr;
extern const char* const& kHeroIdentityStaticStr;
}

// editor.txt "Hero Properties Biography Page"
namespace SHeroPropsBiographyPageText {
enum { kNumStrings = 2 };
extern const char* const& kCaptionStr;
extern const char* const& kCustomizeCheckStr;
}

// editor.txt "Map Specs Artifacts Page"
namespace SMapSpecsArtifactsPageText {
enum { kNumStrings = 1 };
extern const char* const& kEnabledArtifactsStaticStr;
}

// editor.txt "Map Specs Available Heroes Page"
namespace SMapSpecsAvailableHeroesPageText {
enum { kNumStrings = 2 };
extern const char* const& kAvailableHeroesStaticStr;
extern const char* const& kPropertiesButtonStr;
}

// editor.txt "Random Generator Properties Dialog"
namespace SRandomGeneratorPropsDlgText {
enum { kNumStrings = 16 };
extern const char* const& kOwnerStaticStr;
extern const char* const& kCreatureLevelStaticStr;
extern const char* const& kMinimumStaticStr;
extern const char* const& kMaximumStaticStr;
extern const char* const& kAlignmentStaticStr;
extern const char* const& kSelectOneOfRadioStr;
extern const char* const& kSameAsRadioStr;
extern const char* const& kCastleCheckStr;
extern const char* const& kRampartCheckStr;
extern const char* const& kTowerCheckStr;
extern const char* const& kInfernoCheckStr;
extern const char* const& kNecropolisCheckStr;
extern const char* const& kDungeonCheckStr;
extern const char* const& kStrongholdCheckStr;
extern const char* const& kFortressCheckStr;
extern const char* const& kConfluxCheckStr;
}

// editor.txt "Witch Hut Properties Dialog"
namespace SWitchHutPropsDlgText {
enum { kNumStrings = 1 };
extern const char* const& kPotentialSkillsStaticStr;
}

// editor.txt "Hero Properties Primary Skils Page"
namespace SHeroPropsPriSkillsPageText {
enum { kNumStrings = 2 };
extern const char* const& kCaptionStr;
extern const char* const& kCustomizeCheckStr;
}

// editor.txt "Hero Properties Spells Page"
namespace SHeroPropsSpellsPageText {
enum { kNumStrings = 2 };
extern const char* const& kCaptionStr;
extern const char* const& kCustomizeCheckStr;
}

// editor.txt "Hero Prototype Properties Artifacts Page"
namespace SHeroPrototypePropsArtifactsPageText {
enum { kNumStrings = 1 };
extern const char* const& kCaptionStr;
}

// editor.txt "Hero Prototype Properties Biography Page"
namespace SHeroPrototypePropsBiographyPageText {
enum { kNumStrings = 1 };
extern const char* const& kCaptionStr;
}

// editor.txt "Hero Prototype Properties General Page"
namespace SHeroPrototypePropsGeneralPageText {
enum { kNumStrings = 3 };
extern const char* const& kCaptionStr;
extern const char* const& kMayBeHiredByStaticStr;
extern const char* const& kPlayerCheckFmtStr;
}

// editor.txt "Hero Prototype Properties Primary Skills Page"
namespace SHeroPrototypePropsPriSkillsPageText {
enum { kNumStrings = 1 };
extern const char* const& kCaptionStr;
}

// editor.txt "Hero Prototype Properties Secondary Skills Page"
namespace SHeroPrototypePropsSecSkillsPageText {
enum { kNumStrings = 1 };
extern const char* const& kCaptionStr;
}

// editor.txt "Hero Prototype Properties Sheet"
namespace SHeroPrototypePropsSheetText {
enum { kNumStrings = 1 };
extern const char* const& kCaptionStr;
}

// editor.txt "Hero Prototype Properties Spells Page"
namespace SHeroPrototypePropsSpellsPageText {
enum { kNumStrings = 1 };
extern const char* const& kCaptionStr;
}

// editor.txt "Primary Skills Dialog"
namespace SPrimarySkillsDlgText {
enum { kNumStrings = 4 };
extern const char* const& kAttackSkillStaticStr;
extern const char* const& kDefenseSkillStaticStr;
extern const char* const& kSpellPowerStaticStr;
extern const char* const& kKnowledgeStaticStr;
}

// editor.txt "Spells Dialog"
namespace SSpellsDlgText {
enum { kNumStrings = 1 };
extern const char* const& kSpellsStaticStr;
}

// editor.txt "Map Specs Secondary Skills Page"
namespace SMapSpecsSecSkillsPageText {
enum { kNumStrings = 2 };
extern const char* const& kCaptionStr;
extern const char* const& kAvailableSecSkillsStaticStr;
}

// editor.txt "Map Specs Spells Page"
namespace SMapSpecsSpellsPageText {
enum { kNumStrings = 2 };
extern const char* const& kCaptionStr;
extern const char* const& kAvailableSpellsStaticStr;
}

// editor.txt "Main Menu"
namespace SMainMenuText {
enum { kNumStrings = 6 };
extern const char* const& kFileStr;
extern const char* const& kEditStr;
extern const char* const& kViewStr;
extern const char* const& kToolsStr;
extern const char* const& kPlayerStr;
extern const char* const& kHelpStr;
}

// editor.txt "File Menu"
namespace SFileMenuText {
enum { kNumStrings = 8 };
extern const char* const& kNewStr;
extern const char* const& kOpenStr;
extern const char* const& kSaveStr;
extern const char* const& kSaveAsStr;
extern const char* const& kExportTextStr;
extern const char* const& kImportTextStr;
extern const char* const& kRecentFileStr;
extern const char* const& kExitStr;
}

// editor.txt "Edit Menu"
namespace SEditMenuText {
enum { kNumStrings = 10 };
extern const char* const& kUndoStr;
extern const char* const& kRedoStr;
extern const char* const& kCutStr;
extern const char* const& kCopyStr;
extern const char* const& kPasteStr;
extern const char* const& kDeleteStr;
extern const char* const& kFindStr;
extern const char* const& kFindNextStr;
extern const char* const& kFindPrevStr;
extern const char* const& kPropertiesStr;
}

// editor.txt "View Menu"
namespace SViewMenuText {
enum { kNumStrings = 10 };
extern const char* const& kZoomInStr;
extern const char* const& kZoomOutStr;
extern const char* const& kUndergroundStr;
extern const char* const& kGridStr;
extern const char* const& kPassabilityStr;
extern const char* const& kObjectAnimationStr;
extern const char* const& kTerrainAnimationStr;
extern const char* const& kToolbarStr;
extern const char* const& kModeBarStr;
extern const char* const& kStatusBarStr;
}

// editor.txt "Tools Menu"
namespace SToolsMenuText {
enum { kNumStrings = 9 };
extern const char* const& kTerrainStr;
extern const char* const& kRiversStr;
extern const char* const& kRoadsStr;
extern const char* const& kEraseStr;
extern const char* const& kObjectsStr;
extern const char* const& kMapSpecificationsStr;
extern const char* const& kValidateMapStr;
extern const char* const& kOptionsStr;
extern const char* const& kObstaclesStr;
}

// editor.txt "Tools|Terrain Menu"
namespace SToolsTerrainMenuText {
enum { kNumStrings = 14 };
extern const char* const& k1x1Str;
extern const char* const& k2x2Str;
extern const char* const& k4x4Str;
extern const char* const& kFillStr;
extern const char* const& kDirtStr;
extern const char* const& kSandStr;
extern const char* const& kGrassStr;
extern const char* const& kSnowStr;
extern const char* const& kSwampStr;
extern const char* const& kRoughStr;
extern const char* const& kSubterraneanStr;
extern const char* const& kLavaStr;
extern const char* const& kWaterStr;
extern const char* const& kRockStr;
}

// editor.txt "Tools|Rivers Menu"
namespace SToolsRiversMenuText {
enum { kNumStrings = 5 };
extern const char* const& kClearStr;
extern const char* const& kIcyStr;
extern const char* const& kMuddyStr;
extern const char* const& kLavaStr;
extern const char* const& kEraserStr;
}

// editor.txt "Tools|Roads Menu"
namespace SToolsRoadsMenuText {
enum { kNumStrings = 4 };
extern const char* const& kDirtStr;
extern const char* const& kGravelStr;
extern const char* const& kCobblestoneStr;
extern const char* const& kEraserStr;
}

// editor.txt "Tools|Erase Menu"
namespace SToolsEraseMenuText {
enum { kNumStrings = 4 };
extern const char* const& k1x1Str;
extern const char* const& k2x2Str;
extern const char* const& k4x4Str;
extern const char* const& kFillStr;
}

// editor.txt "Tools|Objects Menu"
namespace SToolsObjectsMenuText {
enum { kNumStrings = 15 };
extern const char* const& kDirtObjectsStr;
extern const char* const& kSandObjectsStr;
extern const char* const& kGrassObjectsStr;
extern const char* const& kSnowObjectsStr;
extern const char* const& kSwampObjectsStr;
extern const char* const& kRoughObjectsStr;
extern const char* const& kSubterraneanObjectsStr;
extern const char* const& kLavaObjectsStr;
extern const char* const& kWaterObjectsStr;
extern const char* const& kAllTerrainObjectsStr;
extern const char* const& kTownsStr;
extern const char* const& kMonstersStr;
extern const char* const& kHeroesStr;
extern const char* const& kArtifactsStr;
extern const char* const& kTreasuresStr;
}

// editor.txt "Player Menu"
namespace SPlayerMenuText {
enum { kNumStrings = 9 };
extern const char* const& kNoneStr;
extern const char* const& kPlayer1FmtStr;
extern const char* const& kPlayer2FmtStr;
extern const char* const& kPlayer3FmtStr;
extern const char* const& kPlayer4FmtStr;
extern const char* const& kPlayer5FmtStr;
extern const char* const& kPlayer6FmtStr;
extern const char* const& kPlayer7FmtStr;
extern const char* const& kPlayer8FmtStr;
}

// editor.txt "Help Menu"
namespace SHelpMenuText {
enum { kNumStrings = 2 };
extern const char* const& kHelpTopicsStr;
extern const char* const& kAboutMapEditorStr;
}

// editor.txt "Context Menu"
namespace SContextMenuText {
enum { kNumStrings = 12 };
extern const char* const& kWhatsThisStr;
extern const char* const& kUndoStr;
extern const char* const& kRedoStr;
extern const char* const& kCutStr;
extern const char* const& kCopyStr;
extern const char* const& kPasteStr;
extern const char* const& kDeleteStr;
extern const char* const& kFindStr;
extern const char* const& kFindNextStr;
extern const char* const& kFindPrevStr;
extern const char* const& kPropertiesStr;
extern const char* const& kPlaceObstaclesStr;
}

// editor.txt "Tools|Erase Menu"
namespace SToolsBrushMenuText {
enum { kNumStrings = 17 };
extern const char* const& k1x1SolidBrushStr;
extern const char* const& k2x2SolidBrushStr;
extern const char* const& k4x4SolidBrushStr;
extern const char* const& kSolidAreaBrushStr;
extern const char* const& k1x1FuzzyBrushStr;
extern const char* const& k2x2FuzzyBrushStr;
extern const char* const& k4x4FuzzyBrushStr;
extern const char* const& kFuzzyAreaBrushStr;
extern const char* const& k1x1SolidEraserStr;
extern const char* const& k2x2SolidEraserStr;
extern const char* const& k4x4SolidEraserStr;
extern const char* const& kSolidAreaEraserStr;
extern const char* const& k1x1FuzzyEraserStr;
extern const char* const& k2x2FuzzyEraserStr;
extern const char* const& k4x4FuzzyEraserStr;
extern const char* const& kFuzzyAreaEraserStr;
extern const char* const& kPlaceObstaclesStr;
}

namespace SText {
void initialize();
}

#endif  /* HOMM3_EDITOR_MAPEDITORTEXT_H */
