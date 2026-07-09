#ifndef __CUTSCENE_H__
#define __CUTSCENE_H__

#include "misc/KeyCodes.h"
#include "Zombie.h"

using namespace Sexy;
class LawnApp;
class Board;
class ChallengeScreen;

class CutScene
{
public:
    LawnApp*                    mApp;                             //+0x0
    Board*                      mBoard;                           //+0x4
    int                         mCutsceneTime;                    //+0x8
    int                         mSodTime;                         //+0xC
    int                         mGraveStoneTime;                  //+0x10
    int                         mReadySetPlantTime;               //+0x14
    int                         mFogTime;                         //+0x18
    int                         mBossTime;                        //+0x1C
    int                         mCrazyDaveTime;                   //+0x20
    int                         mLawnMowerTime;                   //+0x24
    int                         mCrazyDaveDialogStart;            //+0x28
    bool                        mSeedChoosing;                    //+0x2C
    ReanimationID               mZombiesWonReanimID;              //+0x30
    bool                        mPreloaded;                       //+0x34
    bool                        mPlacedZombies;                   //+0x35
    bool                        mPlacedLawnItems;                 //+0x36
    int                         mCrazyDaveCountDown;              //+0x38 ���������򵹼�ʱ��
    int                         mCrazyDaveLastTalkIndex;          //+0x3C �����������Ļ��ı�š�
    bool                        mUpsellHideBoard;                 //+0x40 ���������عؿ����桿
    ChallengeScreen*            mUpsellChallengeScreen;           //+0x44 �����������С��Ϸ���桿
    bool                        mPreUpdatingBoard;                //+0x48 ���������ý׶εĹؿ�Ԥ���¡�

	std::vector<std::string> mLoadedResourceNames;

public:
    CutScene();
    ~CutScene();

    void                        StartLevelIntro();
    void                        CancelIntro();
    void                        Update();
    void                        AnimateBoard();
    /*inline*/ void             StartSeedChooser();
    /*inline*/ void             EndSeedChooser();
    /*inline*/ int              CalcPosition(int theTimeStart, int theTimeEnd, int thePositionStart, int thePositionEnd);
    void                        PlaceStreetZombies();
    void                        AddGraveStoneParticles();
    void                        PlaceAZombie(ZombieType theZombieType, int theGridX, int theGridY);
    bool                        CanZombieGoInGridSpot(ZombieType theZombieType, int theGridX, int theGridY, bool theZombieGrid[5][5]);
    /*inline*/ bool             IsSurvivalRepick();
    /*inline*/ bool             IsAfterSeedChooser();
    void                        AddFlowerPots();
    void                        UpdateZombiesWon();
    void                        StartZombiesWon();
    /*inline*/ bool             ShowZombieWalking();
    /*inline*/ bool             IsCutSceneOver();
    void                        ZombieWonClick();
    void                        MouseDown(int theX, int theY);
    void                        KeyDown(KeyCode theKey);
    /*inline*/ void             AdvanceCrazyDaveDialog(bool theJustSkipping);
    void                        ShowShovel();
    bool                        CanGetPacketUpgrade();
    bool                        CanGetPacketUpgrade(int theIndex);
    void                        FindPlaceForStreetZombies(ZombieType theZombieType, bool theZombieGrid[5][5], int& thePosX, int& thePosY);
    void                        FindAndPlaceZombie(ZombieType theZombieType, bool theZombieGrid[5][5]);
    static /*inline*/ bool      Is2x2Zombie(ZombieType theZombieType);
    void                        PreloadResources();
    void                        PreloadSeedBankResources();
    /*inline*/ bool             IsBeforePreloading();
    /*inline*/ bool             IsShowingCrazyDave();
    bool                        IsNonScrollingCutscene();
    bool                        IsScrolledLeftAtStart();
    /*inline*/ bool             IsInShovelTutorial();
    void                        PlaceLawnItems();
    bool                        CanGetSecondPacketUpgrade();
    int                         ParseDelayTimeFromMessage();
    int                         ParseTalkTimeFromMessage();
    void                        ClearUpsellBoard();
    void                        LoadIntroBoard();
    /*inline*/ void             AddUpsellZombie(ZombieType theZombieType, int thePixelX, int theGridY);
    void                        LoadUpsellBoardPool();
    void                        LoadUpsellBoardFog();
    void                        LoadUpsellChallengeScreen();
    void                        LoadUpsellBoardRoof();
    void                        UpdateUpsell();
    void                        DrawUpsell(Graphics* g);
    void                        UpdateIntro();
    void                        DrawIntro(Graphics* g);
    /*inline*/ bool             ShouldRunUpsellBoard();
};

#endif
