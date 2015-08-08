/*
 * This file is part of the Colobot: Gold Edition source code
 * Copyright (C) 2001-2014, Daniel Roux, EPSITEC SA & TerranovaTeam
 * http://epsiteс.ch; http://colobot.info; http://github.com/colobot
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://gnu.org/licenses
 */

/**
 * \file object/robotmain.h
 * \brief CRobotMain - main class of Colobot game engine
 */

#pragma once

#include "app/pausemanager.h"

#include "common/global.h"
#include "common/singleton.h"

#include "graphics/engine/camera.h"
#include "graphics/engine/particle.h"

#include "object/drive_type.h"
#include "object/level_category.h"
#include "object/mainmovie.h"
#include "object/mission_type.h"
#include "object/object_type.h"
#include "object/tool_type.h"

#include <deque>

enum Phase
{
    PHASE_WELCOME1,
    PHASE_WELCOME2,
    PHASE_WELCOME3,
    PHASE_PLAYER_SELECT,
    PHASE_APPERANCE,
    PHASE_MAIN_MENU,
    PHASE_LEVEL_LIST,
    PHASE_SIMUL,
    PHASE_SETUPd,
    PHASE_SETUPg,
    PHASE_SETUPp,
    PHASE_SETUPc,
    PHASE_SETUPs,
    PHASE_SETUPds,
    PHASE_SETUPgs,
    PHASE_SETUPps,
    PHASE_SETUPcs,
    PHASE_SETUPss,
    PHASE_WRITEs,
    PHASE_READ,
    PHASE_READs,
    PHASE_WIN,
    PHASE_LOST,
    PHASE_QUIT_SCREEN,
};
bool IsInSimulationConfigPhase(Phase phase);
bool IsPhaseWithWorld(Phase phase);
bool IsMainMenuPhase(Phase phase);


class CController;
class CEventQueue;
class CSoundInterface;
class CLevelParserLine;
class CInput;
class CObjectManager;
class CSceneEndCondition;
class CAudioChangeCondition;
class CPlayerProfile;
class CSettings;

namespace Gfx
{
class CEngine;
class CLightManager;
class CWater;
class CCloud;
class CLightning;
class CPlanet;
class CTerrain;
class CModelManager;
}

namespace Ui
{
class CMainUserInterface;
class CMainShort;
class CMainMap;
class CInterface;
class CDisplayText;
class CDisplayInfo;
}

const int MAXNEWSCRIPTNAME = 20;

struct NewScriptName
{
    bool        used;
    ObjectType  type;
    char        name[40];
};


const int MAXSHOWLIMIT      = 5;
const int MAXSHOWPARTI      = 200;
const float SHOWLIMITTIME   = 20.0f;

const int MAXSCENE = 999;

struct ShowLimit
{
    bool            used;
    Math::Vector    pos;
    float           radius;
    int             total;
    int             parti[MAXSHOWPARTI];
    CObject*        link;
    float           duration;
    float           time;
};


const int SATCOM_HUSTON     = 0;
const int SATCOM_SAT        = 1;
const int SATCOM_OBJECT     = 2;
const int SATCOM_LOADING    = 3;
const int SATCOM_PROG       = 4;
const int SATCOM_SOLUCE     = 5;
const int SATCOM_MAX        = 6;

class CRobotMain : public CSingleton<CRobotMain>
{
public:
    CRobotMain();
    virtual ~CRobotMain();

    Gfx::CCamera* GetCamera();
    Gfx::CTerrain* GetTerrain();
    Ui::CInterface* GetInterface();
    Ui::CDisplayText* GetDisplayText();

    void        CreateConfigFile();
    void        LoadConfigFile();

    void        ResetAfterDeviceChanged();

    void        ChangePhase(Phase phase);
    bool        ProcessEvent(Event &event);
    //! Execute any pre-render hooks on the render thread
    void        RenderUpdate();

    bool        CreateShortcuts();
    void        ScenePerso();

    void        SetMovieLock(bool lock);
    bool        GetMovieLock();
    bool        GetInfoLock();
    void        SetSatComLock(bool lock);
    bool        GetSatComLock();
    void        SetEditLock(bool lock, bool edit);
    bool        GetEditLock();
    void        SetEditFull(bool full);
    bool        GetEditFull();
    bool        GetFreePhoto();
    void        SetFriendAim(bool friendAim);
    bool        GetFriendAim();

    void        SetTracePrecision(float factor);
    float       GetTracePrecision();

    void        ChangePause(PauseType pause);

    void        SetSpeed(float speed);
    float       GetSpeed();

    void        UpdateShortcuts();
    void        SelectHuman();
    CObject*    SearchHuman();
    CObject*    SearchToto();
    CObject*    SearchNearest(Math::Vector pos, CObject* pExclu);
    bool        SelectObject(CObject* pObj, bool displayError=true);
    CObject*    GetSelectObject();
    CObject*    DeselectAll();
    bool        DeleteObject();

    void        ResetObject();
    void        UpdateAudio(bool frame);
    void        SetEndMission(Error result, float delay);
    Error       CheckEndMission(bool frame);
    Error       CheckEndMissionForGroup(std::vector<CSceneEndCondition*>& endTakes);
    int         GetObligatoryToken();
    char*       GetObligatoryToken(int i);
    int         IsObligatoryToken(const char* token);
    bool        IsProhibitedToken(const char* token);
    void        UpdateMap();
    bool        GetShowMap();

    MainMovieType GetMainMovie();

    void        FlushDisplayInfo();
    void        StartDisplayInfo(int index, bool movie);
    void        StartDisplayInfo(const char *filename, int index);
    void        StopDisplayInfo();
    char*       GetDisplayInfoName(int index);
    int         GetDisplayInfoPosition(int index);
    void        SetDisplayInfoPosition(int index, int pos);

    void        StartSuspend();
    void        StopSuspend();

    float       GetGameTime();

    char*       GetTitle();
    char*       GetResume();
    char*       GetScriptName();
    char*       GetScriptFile();
    bool        GetTrainerPilot();
    bool        GetFixScene();
    bool        GetInterfaceGlint();
    bool        GetSoluce4();
    bool        GetMovies();
    bool        GetNiceReset();
    bool        GetHimselfDamage();
    bool        GetShowSoluce();
    bool        GetSceneSoluce();
    bool        GetShowAll();
    bool        GetRadar();
    MissionType GetMissionType();

    int         GetGamerFace();
    int         GetGamerGlasses();
    bool        GetGamerOnlyHead();
    float       GetPersoAngle();

    void        SetLevel(LevelCategory cat, int chap, int rank);
    LevelCategory GetLevelCategory();
    int         GetLevelChap();
    int         GetLevelRank();
    std::string GetCustomLevelDir();
    void        SetReadScene(std::string path);
    void        UpdateChapterPassed();

    void        StartMusic();
    void        StartPauseMusic(PauseType pause);
    void        ClearInterface();
    void        ChangeColor();

    float       SearchNearestObject(Math::Vector center, CObject *exclu);
    bool        FreeSpace(Math::Vector &center, float minRadius, float maxRadius, float space, CObject *exclu);
    bool        FlatFreeSpace(Math::Vector &center, float minFlat, float minRadius, float maxRadius, float space, CObject *exclu);
    float       GetFlatZoneRadius(Math::Vector center, float maxRadius, CObject *exclu);
    void        HideDropZone(CObject* metal);
    void        ShowDropZone(CObject* metal, CObject* transporter);
    void        FlushShowLimit(int i);
    void        SetShowLimit(int i, Gfx::ParticleType parti, CObject *pObj, Math::Vector pos,
                             float radius, float duration=SHOWLIMITTIME);
    void        AdjustShowLimit(int i, Math::Vector pos);
    void        StartShowLimit();
    void        FrameShowLimit(float rTime);

    void        CompileScript(bool soluce);
    void        LoadOneScript(CObject *pObj);
    void        LoadFileScript(CObject *pObj, const char* filename, int objRank);
    void        SaveAllScript();
    void        SaveOneScript(CObject *pObj);
    void        SaveFileScript(CObject *pObj, const char* filename, int objRank);
    bool        SaveFileStack(CObject *pObj, FILE *file, int objRank);
    bool        ReadFileStack(CObject *pObj, FILE *file, int objRank);

    bool        FlushNewScriptName();
    bool        AddNewScriptName(ObjectType type, char *name);
    char*       GetNewScriptName(ObjectType type, int rank);

    void        SelectPlayer(std::string playerName);
    CPlayerProfile* GetPlayerProfile();

    bool        IsBusy();
    bool        IOWriteScene(std::string filename, std::string filecbot, std::string filescreenshot, char *info);
    void        IOWriteSceneFinished();
    CObject*    IOReadScene(std::string filename, std::string filecbot);
    void        IOWriteObject(CLevelParserLine *line, CObject* obj);
    CObject*    IOReadObject(CLevelParserLine *line, const char* filename, const std::string& objCounterText, float objectProgress, int objRank);

    int         CreateSpot(Math::Vector pos, Gfx::Color color);

    CObject*    GetSelect();

    void        DisplayError(Error err, CObject* pObj, float time=10.0f);
    void        DisplayError(Error err, Math::Vector goal, float height=15.0f, float dist=60.0f, float time=10.0f);

    void        UpdateCustomLevelList();
    std::string GetCustomLevelName(int id);
    const std::vector<std::string>& GetCustomLevelList();

    void        StartMissionTimer();

    void        SetAutosave(bool enable);
    bool        GetAutosave();
    void        SetAutosaveInterval(int interval);
    int         GetAutosaveInterval();
    void        SetAutosaveSlots(int slots);
    int         GetAutosaveSlots();

    //! Enable mode where completing mission closes the game
    void        SetExitAfterMission(bool exit);

    //! Returns true if player can interact with things manually
    bool        CanPlayerInteract();

    //! Returns team name for the given team id
    const std::string& GetTeamName(int id);

    //! Returns true if team-specific colored texture is available
    bool        IsTeamColorDefined(int id);

    //! Get/set enabled buildings
    //@{
    int         GetEnableBuild();
    void        SetEnableBuild(int enableBuild);
    //@}
    //! Get/set enabled researches
    //@{
    int         GetEnableResearch();
    void        SetEnableResearch(int enableResearch);
    //@}
    //! Get/set done researches
    //@{
    int         GetDoneResearch(int team);
    void        SetDoneResearch(int doneResearch, int team);
    //@}

    //! Returns true if the given building is enabled
    //@{
    bool        IsBuildingEnabled(BuildType type);
    bool        IsBuildingEnabled(ObjectType type);
    //@}
    //! Returns true if the given research is enabled
    bool        IsResearchEnabled(ResearchType type);
    //! Returns true if the given research is done
    bool        IsResearchDone(ResearchType type, int team);
    //! Marks research as done
    void        MarkResearchDone(ResearchType type, int team);

    //! Retruns true if all requirements to build this object are met (EnableBuild + DoneResearch)
    //@{
    bool        CanBuild(ObjectType type, int team);
    Error       CanBuildError(ObjectType type, int team);
    //@}

    //! Retruns true if all requirements to create this object in BotFactory are met (DoneResearch)
    //@{
    bool        CanFactory(ObjectType type, int team);
    Error       CanFactoryError(ObjectType type, int team);
    //@}

    void        RemoveFromSelectionHistory(CObject* object);

    //! Returns global magnifyDamage setting
    float       GetGlobalMagnifyDamage();

protected:
    bool        EventFrame(const Event &event);
    bool        EventObject(const Event &event);
    void        InitEye();

    void        ShowSaveIndicator(bool show);

    void        CreateScene(bool soluce, bool fixScene, bool resetObject);
    void        ResetCreate();

    void        LevelLoadingError(const std::string& error, const std::runtime_error& exception, Phase exitPhase = PHASE_LEVEL_LIST);

    Math::Vector LookatPoint(Math::Vector eye, float angleH, float angleV, float length);
    int         CreateLight(Math::Vector direction, Gfx::Color color);
    void        HiliteClear();
    void        HiliteObject(Math::Point pos);
    void        HiliteFrame(float rTime);
    void        CreateTooltip(Math::Point pos, const std::string& text);
    void        ClearTooltip();
    CObject*    DetectObject(Math::Point pos);
    void        ChangeCamera();
    void        RemoteCamera(float pan, float zoom, float rTime);
    void        KeyCamera(EventType event, InputSlot key);
    void        AbortMovie();
    bool        IsSelectable(CObject* pObj);
    void        SelectOneObject(CObject* pObj, bool displayError=true);
    void        HelpObject();
    bool        DeselectObject();
    void        DeleteAllObjects();
    void        UpdateInfoText();
    CObject*    SearchObject(ObjectType type);
    void        StartDisplayVisit(EventType event);
    void        FrameVisit(float rTime);
    void        StopDisplayVisit();
    void        ExecuteCmd(char *cmd);
    bool        TestGadgetQuantity(int rank);
    void        UpdateSpeedLabel();

    int         AutosaveRotate(bool freeOne);
    void        Autosave();
    void        PushToSelectionHistory(CObject* obj);
    CObject*    PopFromSelectionHistory();


protected:
    CController*        m_ctrl;
    CApplication*       m_app;
    CObjectManager*     m_objMan;
    CEventQueue*        m_eventQueue;
    CMainMovie*         m_movie;
    Gfx::CEngine*       m_engine;
    Gfx::CParticle*     m_particle;
    Gfx::CWater*        m_water;
    Gfx::CCloud*        m_cloud;
    Gfx::CLightning*    m_lightning;
    Gfx::CPlanet*       m_planet;
    Gfx::COldModelManager* m_oldModelManager;
    std::unique_ptr<Gfx::CModelManager> m_modelManager;
    Gfx::CLightManager* m_lightMan;
    Gfx::CTerrain*      m_terrain;
    Gfx::CCamera*       m_camera;
    std::unique_ptr<Ui::CMainUserInterface> m_ui;
    Ui::CMainShort*     m_short;
    Ui::CMainMap*       m_map;
    Ui::CInterface*     m_interface;
    Ui::CDisplayText*   m_displayText;
    Ui::CDisplayInfo*   m_displayInfo;
    CSoundInterface*    m_sound;
    CPauseManager*      m_pause;
    CInput*             m_input;
    CSettings*          m_settings;

    //! Progress of loaded player
    std::unique_ptr<CPlayerProfile> m_playerProfile;


    //! Time since level start, including pause and intro movie
    float           m_time;
    //! Playing time since level start
    float           m_gameTime;
    //! Playing time since level start, not dependent on simulation speed
    float           m_gameTimeAbsolute;

    LevelCategory   m_levelCategory;
    int             m_levelChap;
    int             m_levelRank;
    std::string     m_sceneReadPath;

    float           m_winDelay;
    float           m_lostDelay;
    bool            m_fixScene;        // scene fixed, no interraction
    CObject*        m_base;        // OBJECT_BASE exists in mission
    Math::Point     m_lastMousePos;
    CObject*        m_selectObject;

    Phase           m_phase;
    int             m_cameraRank;
    Gfx::Color      m_color;
    bool            m_freePhoto;
    bool            m_cmdEdit;
    bool            m_selectInsect;
    bool            m_showSoluce;
    bool            m_showAll;
    bool            m_cheatRadar;
    bool            m_shortCut;
    std::string     m_audioTrack;
    bool            m_audioRepeat;
    std::string     m_satcomTrack;
    bool            m_satcomRepeat;
    std::string     m_editorTrack;
    bool            m_editorRepeat;
    int             m_movieInfoIndex;

    CObject*        m_controller;

    MissionType     m_missionType;
    bool            m_immediatSatCom;  // SatCom immediately?
    bool            m_beginSatCom;     // messages SatCom poster?
    bool            m_lockedSatCom;    // SatCom locked?
    bool            m_movieLock;       // movie in progress?
    bool            m_satComLock;      // call of SatCom is possible?
    bool            m_editLock;        // edition in progress?
    bool            m_editFull;        // edition in full screen?
    bool            m_hilite;
    bool            m_trainerPilot;    // remote trainer?
    bool            m_friendAim;
    bool            m_resetCreate;
    bool            m_mapShow;
    bool            m_mapImage;
    char            m_mapFilename[100];

    bool            m_suspend;
    PauseType       m_suspendInitPause;
    Gfx::CameraType m_suspendInitCamera;

    Math::Point     m_tooltipPos;
    std::string     m_tooltipName;
    float           m_tooltipTime;

    char            m_infoFilename[SATCOM_MAX][100]; // names of text files
    CObject*        m_infoObject;
    int             m_infoIndex;
    int             m_infoPos[SATCOM_MAX];
    int             m_infoUsed;

    char            m_title[100];
    char            m_resume[500];
    char            m_scriptName[100];
    char            m_scriptFile[100];
    int             m_endingWinRank;
    int             m_endingLostRank;
    bool            m_winTerminate;

    float           m_globalMagnifyDamage;

    bool            m_exitAfterMission;

    bool            m_codeBattleInit;
    bool            m_codeBattleStarted;

    std::map<int, std::string> m_teamNames;

    NewScriptName   m_newScriptName[MAXNEWSCRIPTNAME];

    float           m_cameraPan;
    float           m_cameraZoom;

    EventType       m_visitLast;
    CObject*        m_visitObject;
    CObject*        m_visitArrow;
    float           m_visitTime;
    float           m_visitParticle;
    Math::Vector    m_visitPos;
    Math::Vector    m_visitPosArrow;

    std::vector<std::unique_ptr<CSceneEndCondition>> m_endTake;
    long            m_endTakeResearch;
    float           m_endTakeWinDelay;
    float           m_endTakeLostDelay;

    std::vector<std::unique_ptr<CAudioChangeCondition>> m_audioChange;

    int             m_obligatoryTotal;
    char            m_obligatoryToken[100][20];
    int             m_prohibitedTotal;
    char            m_prohibitedToken[100][20];

    //! Enabled buildings
    int             m_build;
    //! Available researches
    long            m_researchEnable;
    //! Done researches for each team
    std::map<int, int>  m_researchDone;

    Error           m_missionResult;

    ShowLimit       m_showLimit[MAXSHOWLIMIT];

    Gfx::Color      m_colorRefBot;
    std::map<int, Gfx::Color> m_colorNewBot;
    Gfx::Color      m_colorRefAlien;
    Gfx::Color      m_colorNewAlien;
    Gfx::Color      m_colorRefGreen;
    Gfx::Color      m_colorNewGreen;
    Gfx::Color      m_colorRefWater;
    Gfx::Color      m_colorNewWater;
    float           m_colorShiftWater;

    bool            m_missionTimerEnabled;
    bool            m_missionTimerStarted;
    float           m_missionTimer;

    bool            m_autosave;
    int             m_autosaveInterval;
    int             m_autosaveSlots;
    float           m_autosaveLast;

    int             m_shotSaving;
    std::string     m_shotName;

    bool            m_texturesNeedUpdate;

    std::deque<CObject*> m_selectionHistory;
};
