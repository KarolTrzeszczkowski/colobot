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

#include "object/robotmain.h"

#include "CBot/CBotDll.h"

#include "app/app.h"
#include "app/input.h"

#include "common/config_file.h"
#include "common/event.h"
#include "common/global.h"
#include "common/logger.h"
#include "common/make_unique.h"
#include "common/misc.h"
#include "common/restext.h"
#include "common/settings.h"
#include "common/stringutils.h"

#include "common/resources/inputstream.h"
#include "common/resources/outputstream.h"
#include "common/resources/resourcemanager.h"

#include "graphics/engine/camera.h"
#include "graphics/engine/cloud.h"
#include "graphics/engine/engine.h"
#include "graphics/engine/lightman.h"
#include "graphics/engine/lightning.h"
#include "graphics/engine/oldmodelmanager.h"
#include "graphics/engine/particle.h"
#include "graphics/engine/planet.h"
#include "graphics/engine/pyro_manager.h"
#include "graphics/engine/terrain.h"
#include "graphics/engine/text.h"
#include "graphics/engine/water.h"

#include "graphics/model/model_manager.h"

#include "math/const.h"
#include "math/geometry.h"

#include "object/brain.h"
#include "object/mainmovie.h"
#include "object/object.h"
#include "object/object_create_exception.h"
#include "object/object_manager.h"
#include "object/player_profile.h"
#include "object/scene_conditions.h"

#include "object/auto/auto.h"
#include "object/auto/autobase.h"

#include "object/level/parser.h"

#include "object/motion/motion.h"
#include "object/motion/motionhuman.h"
#include "object/motion/motiontoto.h"

#include "object/subclass/exchange_post.h"

#include "object/task/task.h"
#include "object/task/taskbuild.h"
#include "object/task/taskmanip.h"

#include "physics/physics.h"

#include "script/cbottoken.h"
#include "script/script.h"
#include "script/scriptfunc.h"

#include "sound/sound.h"

#include "ui/displayinfo.h"
#include "ui/displaytext.h"
#include "ui/maindialog.h"
#include "ui/mainmap.h"
#include "ui/mainshort.h"
#include "ui/mainui.h"

#include "ui/controls/button.h"
#include "ui/controls/edit.h"
#include "ui/controls/interface.h"
#include "ui/controls/label.h"
#include "ui/controls/map.h"
#include "ui/controls/shortcut.h"
#include "ui/controls/slider.h"
#include "ui/controls/window.h"

#include "ui/screen/screen_loading.h"

#include <iomanip>

#include <clipboard/clipboard.h>
#include <boost/lexical_cast.hpp>


template<> CRobotMain* CSingleton<CRobotMain>::m_instance = nullptr;


// TODO: remove once using std::string
const int MAX_FNAME = 255;



const float UNIT = 4.0f;



// Global variables.

float   g_unit;             // conversion factor





//! Constructor of robot application
CRobotMain::CRobotMain()
{
    m_app        = CApplication::GetInstancePointer();

    m_eventQueue = m_app->GetEventQueue();
    m_sound      = m_app->GetSound();

    m_engine     = Gfx::CEngine::GetInstancePointer();
    m_oldModelManager = m_engine->GetModelManager();
    m_modelManager = MakeUnique<Gfx::CModelManager>();
    m_lightMan   = m_engine->GetLightManager();
    m_particle   = m_engine->GetParticle();
    m_water      = m_engine->GetWater();
    m_cloud      = m_engine->GetCloud();
    m_lightning  = m_engine->GetLightning();
    m_planet     = m_engine->GetPlanet();
    m_pause      = CPauseManager::GetInstancePointer();
    m_input      = CInput::GetInstancePointer();
    m_settings   = new CSettings();

    m_interface   = new Ui::CInterface();
    m_terrain     = new Gfx::CTerrain();
    m_camera      = new Gfx::CCamera();
    m_displayText = new Ui::CDisplayText();
    m_movie       = new CMainMovie();
    m_ui          = MakeUnique<Ui::CMainUserInterface>();
    m_short       = new Ui::CMainShort();
    m_map         = new Ui::CMainMap();
    m_displayInfo = nullptr;

    m_objMan = new CObjectManager(m_engine,
                                  m_terrain,
                                  m_oldModelManager,
                                  m_modelManager.get(),
                                  m_particle);

    m_time = 0.0f;
    m_gameTime = 0.0f;
    m_gameTimeAbsolute = 0.0f;

    m_levelCategory = LevelCategory::Exercises;
    m_levelChap = 0;
    m_levelRank = 0;
    m_sceneReadPath = "";

    m_missionTimerEnabled = false;
    m_missionTimerStarted = false;
    m_missionTimer = 0.0f;

    m_phase       = PHASE_PLAYER_SELECT;
    m_cameraRank  = -1;
    m_visitLast   = EVENT_NULL;
    m_visitObject = 0;
    m_visitArrow  = 0;
    m_audioTrack  = "";
    m_audioRepeat = true;
    m_satcomTrack  = "";
    m_satcomRepeat = true;
    m_editorTrack  = "";
    m_editorRepeat = true;
    m_selectObject = 0;
    m_infoUsed     = 0;

    m_controller   = nullptr;
    m_missionType  = MISSION_NORMAL;
    m_immediatSatCom = false;
    m_beginSatCom  = false;
    m_lockedSatCom = false;
    m_movieLock    = false;
    m_satComLock   = false;
    m_editLock     = false;
    m_editFull     = false;
    m_hilite       = false;
    m_freePhoto    = false;
    m_selectInsect = false;
    m_showSoluce   = false;

    m_codeBattleInit = false;
    m_codeBattleStarted = false;

    m_teamNames.clear();

    #if DEV_BUILD
    m_showAll      = true; // for development
    #else
    m_showAll      = false;
    #endif

    m_cheatRadar   = false;
    m_fixScene     = false;
    m_trainerPilot = false;
    m_suspend      = false;
    m_friendAim    = false;
    m_resetCreate  = false;
    m_shortCut     = true;

    m_movieInfoIndex = -1;

    m_tooltipPos = Math::Point(0.0f, 0.0f);
    m_tooltipName.clear();
    m_tooltipTime = 0.0f;

    m_endingWinRank   = 0;
    m_endingLostRank  = 0;
    m_winTerminate   = false;

    m_globalMagnifyDamage = 1.0f;

    m_exitAfterMission = false;

    m_autosave = true;
    m_autosaveInterval = 5;
    m_autosaveSlots = 3;
    m_autosaveLast = 0.0f;

    m_shotSaving = 0;

    m_cameraPan  = 0.0f;
    m_cameraZoom = 0.0f;

    m_build = 0;
    m_researchDone.clear();  // no research done
    m_researchDone[0] = 0;
    m_researchEnable = 0;
    g_unit = UNIT;

    m_texturesNeedUpdate = false;

    for (int i = 0; i < MAXSHOWLIMIT; i++)
    {
        m_showLimit[i].used = false;
        m_showLimit[i].total = 0;
        m_showLimit[i].link = 0;
    }

    m_engine->SetTerrain(m_terrain);

    m_app->SetMouseMode(MOUSE_ENGINE);

    m_movie->Flush();

    FlushDisplayInfo();

    InitEye();

    m_engine->SetTracePrecision(1.0f);

    SelectPlayer(CPlayerProfile::GetLastName());

    CScriptFunctions::Init();
}

//! Destructor of robot application
CRobotMain::~CRobotMain()
{
    delete m_settings;
    m_settings = nullptr;

    delete m_displayText;
    m_displayText = nullptr;

    delete m_interface;
    m_interface = nullptr;

    delete m_terrain;
    m_terrain = nullptr;

    delete m_camera;
    m_camera = nullptr;

    delete m_displayText;
    m_displayText = nullptr;

    delete m_movie;
    m_movie = nullptr;

    delete m_short;
    m_short = nullptr;

    delete m_map;
    m_map = nullptr;

    delete m_objMan;
    m_objMan = nullptr;
}

Gfx::CCamera* CRobotMain::GetCamera()
{
    return m_camera;
}

Gfx::CTerrain* CRobotMain::GetTerrain()
{
    return m_terrain;
}

Ui::CInterface* CRobotMain::GetInterface()
{
    return m_interface;
}

Ui::CDisplayText* CRobotMain::GetDisplayText()
{
    return m_displayText;
}

void CRobotMain::ResetAfterDeviceChanged()
{
    if (m_phase == PHASE_SETUPds ||
       m_phase == PHASE_SETUPgs ||
       m_phase == PHASE_SETUPps ||
       m_phase == PHASE_SETUPcs ||
       m_phase == PHASE_SETUPss ||
       m_phase == PHASE_SIMUL ||
       m_phase == PHASE_WIN ||
       m_phase == PHASE_LOST)
    ChangeColor();
    UpdateMap();
}


//! Creates the file colobot.ini at the first time
void CRobotMain::CreateConfigFile()
{
    m_settings->SaveSettings();
    m_settings->SaveResolutionSettings(m_app->GetVideoConfig());

    GetConfigFile().Save();
}

void CRobotMain::LoadConfigFile()
{
    m_settings->LoadSettings();
}

bool IsInSimulationConfigPhase(Phase phase)
{
    return (phase >= PHASE_SETUPds && phase <= PHASE_SETUPss) || phase == PHASE_READs || phase == PHASE_WRITEs;
}

bool IsPhaseWithWorld(Phase phase)
{
    if (phase == PHASE_SIMUL    ) return true;
    if (phase == PHASE_WIN      ) return true;
    if (phase == PHASE_LOST     ) return true;
    if (phase == PHASE_APPERANCE) return true;
    if (IsInSimulationConfigPhase(phase)) return true;
    return false;
}

bool IsMainMenuPhase(Phase phase)
{
    return !IsPhaseWithWorld(phase);
}

//! Changes phase
void CRobotMain::ChangePhase(Phase phase)
{
    bool resetWorld = false;
    if ((IsPhaseWithWorld(m_phase) || IsPhaseWithWorld(phase)) && !IsInSimulationConfigPhase(m_phase) && !IsInSimulationConfigPhase(phase))
    {
        GetLogger()->Info("Reseting world on phase change...\n");
        resetWorld = true;
    }

    if (resetWorld)
    {
        m_missionTimerEnabled = m_missionTimerStarted = false;
        m_missionTimer = 0.0f;

        if (m_phase == PHASE_SIMUL)  // ends a simulation?
        {
            SaveAllScript();
            m_sound->StopMusic(0.0f);
            m_camera->SetControllingObject(0);

            if (m_gameTime > 10.0f)  // did you play at least 10 seconds?
            {
                m_playerProfile->IncrementLevelTryCount(m_levelCategory, m_levelChap, m_levelRank);
            }
        }

        if (phase == PHASE_WIN)  // wins a simulation?
        {
            m_playerProfile->SetLevelPassed(m_levelCategory, m_levelChap, m_levelRank, true);
            m_ui->NextMission();  // passes to the next mission
        }

        m_app->SetLowCPU(true); // doesn't use much CPU in interface phases

        DeleteAllObjects();  // removes all the current 3D Scene
    }

    m_phase = phase;

    if (resetWorld)
    {
        m_winDelay     = 0.0f;
        m_lostDelay    = 0.0f;
        m_beginSatCom = false;
        m_movieLock   = false;
        m_satComLock  = false;
        m_editLock    = false;
        m_freePhoto   = false;
        m_resetCreate = false;
        m_infoObject  = nullptr;

        ChangePause(PAUSE_NONE);
        FlushDisplayInfo();
        m_engine->SetRankView(0);
        m_terrain->FlushRelief();
        m_engine->DeleteAllObjects();
        m_oldModelManager->DeleteAllModelCopies();
        m_engine->SetWaterAddColor(Gfx::Color(0.0f, 0.0f, 0.0f, 0.0f));
        m_engine->SetBackground("");
        m_engine->SetBackForce(false);
        m_engine->SetForegroundName("");
        m_engine->SetOverColor();
        m_engine->DeleteGroundMark(0);
        SetSpeed(1.0f);
        m_terrain->SetWind(Math::Vector(0.0f, 0.0f, 0.0f));
        m_terrain->FlushBuildingLevel();
        m_terrain->FlushFlyingLimit();
        m_lightMan->FlushLights();
        m_particle->FlushParticle();
        m_water->Flush();
        m_cloud->Flush();
        m_lightning->Flush();
        m_planet->Flush();
        m_interface->Flush();
        FlushNewScriptName();
        m_sound->SetListener(Math::Vector(0.0f, 0.0f, 0.0f), Math::Vector(0.0f, 0.0f, 1.0f));
        m_camera->SetType(Gfx::CAM_TYPE_DIALOG);
        m_movie->Flush();
        m_movieInfoIndex = -1;
        m_cameraPan  = 0.0f;
        m_cameraZoom = 0.0f;
        m_shortCut = true;
    }
    ClearInterface();

    Math::Point dim, pos;

    // Creates and hide the command console.
    dim.x = 200.0f/640.0f;
    dim.y =  18.0f/480.0f;
    pos.x =  20.0f/640.0f;
    pos.y = 100.0f/480.0f;
    Ui::CEdit* pe = m_interface->CreateEdit(pos, dim, 0, EVENT_CMD);
    pe->ClearState(Ui::STATE_VISIBLE);
    pe->SetMaxChar(100);
    m_cmdEdit = false;  // hidden for now

    // Creates the speedometer.
    dim.x =  30.0f/640.0f;
    dim.y =  20.0f/480.0f;
    pos.x =   4.0f/640.0f;
    pos.y = 426.0f/480.0f;

    // Creates the save indicator
    Ui::CButton* pb = m_interface->CreateButton(pos, dim, 0, EVENT_SPEED);
    pb->SetState(Ui::STATE_SIMPLY);
    pb->ClearState(Ui::STATE_VISIBLE);

    m_ui->ChangePhase(m_phase);
    if (!resetWorld) return;

    dim.x = 32.0f/640.0f;
    dim.y = 32.0f/480.0f;
    float ox = 3.0f/640.0f;
    float oy = 3.0f/480.0f;
    float sx = (32.0f+2.0f)/640.0f;
    float sy = (32.0f+2.0f)/480.0f;

    if (m_phase != PHASE_APPERANCE)
    {
        m_engine->SetDrawWorld(true);
        m_engine->SetDrawFront(false);
        m_fixScene = false;
    }

    if (m_phase == PHASE_SIMUL)
    {
        m_app->SetLowCPU(false); // high CPU for simulation

        bool loading = !m_sceneReadPath.empty();

        m_ui->ShowLoadingScreen(true);
        m_ui->GetLoadingScreen()->SetProgress(0.0f, RT_LOADING_INIT);

        m_map->CreateMap();
        m_map->ShowMap(false);

        try
        {
            CreateScene(m_ui->GetSceneSoluce(), false, false);  // interactive scene
            if (m_mapImage)
                m_map->SetFixImage(m_mapFilename);

            m_app->ResetTimeAfterLoading();

            if (m_immediatSatCom && !loading  &&
                m_infoFilename[SATCOM_HUSTON][0] != 0)
                StartDisplayInfo(SATCOM_HUSTON, false);  // shows the instructions

            m_sound->StopMusic(0.0f);
            if (m_base == nullptr || loading) StartMusic();
        }
        catch (const std::runtime_error& e)
        {
            LevelLoadingError("An error occured while trying to load a level", e);
        }
    }

    if (m_phase == PHASE_WIN)
    {
        m_sound->StopAll();
        if (m_endingWinRank == -1)
        {
            ChangePhase(PHASE_LEVEL_LIST);
        }
        else
        {
            m_winTerminate = (m_endingWinRank == 904);
            SetLevel(LevelCategory::Win, 0, m_endingWinRank);
            try
            {
                CreateScene(false, true, false);  // sets scene

                pos.x = ox+sx*1;  pos.y = oy+sy*1;
                Math::Point ddim;
                ddim.x = dim.x*2;  ddim.y = dim.y*2;
                m_interface->CreateButton(pos, ddim, 16, EVENT_BUTTON_OK);

                if (m_winTerminate)
                {
                    pos.x = ox+sx*3;  pos.y = oy+sy*0.2f;
                    ddim.x = dim.x*15;  ddim.y = dim.y*3.0f;
                    pe = m_interface->CreateEdit(pos, ddim, 0, EVENT_EDIT0);
                    pe->SetGenericMode(true);
                    pe->SetFontType(Gfx::FONT_COLOBOT);
                    pe->SetEditCap(false);
                    pe->SetHighlightCap(false);
                    pe->ReadText(std::string("help/") + m_app->GetLanguageChar() + std::string("/win.txt"));
                }
                else
                {
                    m_displayText->DisplayError(INFO_WIN, Math::Vector(0.0f,0.0f,0.0f), 15.0f, 60.0f, 1000.0f);
                }
                StartMusic();
            }
            catch (const std::runtime_error& e)
            {
                LevelLoadingError("An error occured while trying to load win scene", e);
            }
        }
    }

    if (m_phase == PHASE_LOST)
    {
        m_sound->StopAll();
        if (m_endingLostRank == -1)
        {
            ChangePhase(PHASE_LEVEL_LIST);
        }
        else
        {
            m_winTerminate = false;
            SetLevel(LevelCategory::Lost, 0, m_endingLostRank);
            try
            {
                CreateScene(false, true, false);  // sets scene

                pos.x = ox+sx*1;  pos.y = oy+sy*1;
                Math::Point ddim;
                ddim.x = dim.x*2;  ddim.y = dim.y*2;
                m_interface->CreateButton(pos, ddim, 16, EVENT_BUTTON_OK);
                m_displayText->DisplayError(INFO_LOST, Math::Vector(0.0f,0.0f,0.0f), 15.0f, 60.0f, 1000.0f);

                StartMusic();
            }
            catch (const std::runtime_error& e)
            {
                LevelLoadingError("An error occured while trying to load lost scene", e);
            }
        }
    }

    m_engine->LoadAllTextures();
}

void CRobotMain::RenderUpdate()
{
    if (m_texturesNeedUpdate)
    {
        ChangeColor();
        UpdateMap();
        m_texturesNeedUpdate = false;
    }

    if (!m_shotName.empty())
    {
        ShowSaveIndicator(false); // force hide for screenshot
        MouseMode oldMouseMode = m_app->GetMouseMode();
        m_app->SetMouseMode(MOUSE_NONE); // disable the mouse
        m_displayText->HideText(true); // hide
        m_engine->SetScreenshotMode(true);

        m_engine->Render(); // update (but don't show, we're not swapping buffers here!)
        m_engine->WriteScreenShot(m_shotName);
        m_shotSaving++;

        m_engine->SetScreenshotMode(false);
        m_displayText->HideText(false);
        m_app->SetMouseMode(oldMouseMode);

        m_shotName = "";
    }
}

//! Processes an event
bool CRobotMain::ProcessEvent(Event &event)
{
    if (!m_ui->EventProcess(event)) return false;

    if (event.type == EVENT_FRAME)
    {
        if (!m_movie->EventProcess(event))  // end of the movie?
        {
            MainMovieType type = m_movie->GetStopType();
            if (type == MM_SATCOMopen)
            {
                ChangePause(PAUSE_NONE);
                SelectObject(m_infoObject, false);  // hands over the command buttons
                m_map->ShowMap(m_mapShow);
                m_displayText->HideText(false);
                int i = m_movieInfoIndex;
                StartDisplayInfo(m_movieInfoIndex, false);
                m_movieInfoIndex = i;
            }
        }

        m_displayText->EventProcess(event);
        RemoteCamera(m_cameraPan, m_cameraZoom, event.rTime);

        m_interface->EventProcess(event);
        if (m_displayInfo != nullptr)  // current edition?
            m_displayInfo->EventProcess(event);

        UpdateInfoText();

        return EventFrame(event);
    }

    if (event.type == EVENT_WRITE_SCENE_FINISHED)
    {
        IOWriteSceneFinished();
        return false;
    }

    // Management of the console.
    if (event.type == EVENT_KEY_DOWN &&
        event.GetData<KeyEventData>()->key == KEY(BACKQUOTE))  // Pause ?
    {
        if (m_phase != PHASE_PLAYER_SELECT &&
            !m_movie->IsExist()   &&
            !m_movieLock && !m_editLock)
        {
            Ui::CEdit* pe = static_cast<Ui::CEdit*>(m_interface->SearchControl(EVENT_CMD));
            if (pe == nullptr) return false;
            pe->SetState(Ui::STATE_VISIBLE);
            m_interface->SetFocus(pe);
            if (m_phase == PHASE_SIMUL) ChangePause(PAUSE_CHEAT);
            m_cmdEdit = true;
        }
        return false;
    }

    if (event.type == EVENT_KEY_DOWN &&
        event.GetData<KeyEventData>()->key == KEY(RETURN) && m_cmdEdit)
    {
        char cmd[50];
        Ui::CEdit* pe = static_cast<Ui::CEdit*>(m_interface->SearchControl(EVENT_CMD));
        if (pe == nullptr) return false;
        pe->GetText(cmd, 50);
        pe->SetText("");
        pe->ClearState(Ui::STATE_VISIBLE);
        if (m_phase == PHASE_SIMUL) ChangePause(PAUSE_NONE);
        ExecuteCmd(cmd);
        m_cmdEdit = false;
        return false;
    }

    if (event.type == EVENT_KEY_DOWN && m_cmdEdit)
        return false; // cheat console active, so ignore keys

    // Management of the speed change.
    if (event.type == EVENT_SPEED)
        SetSpeed(1.0f);

    if (event.type == EVENT_MOUSE_MOVE)
    {
        m_lastMousePos = event.mousePos;
        HiliteObject(event.mousePos);
    }

    if (!m_displayText->EventProcess(event))
        return false;

    if (event.type == EVENT_MOUSE_MOVE)
    {
        m_lastMousePos = event.mousePos;
        HiliteObject(event.mousePos);
    }

    if (m_displayInfo != nullptr)  // current info?
    {
        m_displayInfo->EventProcess(event);

        if (event.type == EVENT_KEY_DOWN)
        {
            auto data = event.GetData<KeyEventData>();

            if (data->slot == INPUT_SLOT_HELP ||
                data->slot == INPUT_SLOT_PROG ||
                data->key == KEY(ESCAPE))
            {
                StopDisplayInfo();
            }
        }

        if (event.type == EVENT_OBJECT_INFOOK)
            StopDisplayInfo();

        return false;
    }

    CObject* obj;

    // Simulation phase of the game
    if (m_phase == PHASE_SIMUL)
    {
        if (!m_editFull)
            m_camera->EventProcess(event);

        switch (event.type)
        {
            case EVENT_KEY_DOWN:
            {
                auto data = event.GetData<KeyEventData>();

                KeyCamera(event.type, data->slot);
                HiliteClear();
                if (data->key == KEY(F11))
                {
                    m_particle->WriteWheelTrace("Savegame/t.png", 256, 256, Math::Vector(16.0f, 0.0f, -368.0f), Math::Vector(140.0f, 0.0f, -248.0f));
                    return false;
                }
                if (m_editLock)  // current edition?
                {
                    if (data->slot == INPUT_SLOT_HELP)
                    {
                        StartDisplayInfo(SATCOM_HUSTON, false);
                        return false;
                    }
                    if (data->slot == INPUT_SLOT_PROG)
                    {
                        StartDisplayInfo(SATCOM_PROG, false);
                        return false;
                    }
                    break;
                }
                if (m_movieLock)  // current movie?
                {
                    if (data->slot == INPUT_SLOT_QUIT ||
                        data->key == KEY(ESCAPE))
                    {
                        AbortMovie();
                    }
                    return false;
                }
                if (m_camera->GetType() == Gfx::CAM_TYPE_VISIT)
                {
                    if (data->slot == INPUT_SLOT_VISIT)
                    {
                        StartDisplayVisit(EVENT_NULL);
                    }
                    if (data->slot == INPUT_SLOT_QUIT ||
                        data->key == KEY(ESCAPE))
                    {
                        StopDisplayVisit();
                    }
                    return false;
                }
                if (data->slot == INPUT_SLOT_QUIT)
                {
                    if (m_movie->IsExist())
                        StartDisplayInfo(SATCOM_HUSTON, false);
                    else if (m_winDelay > 0.0f)
                        ChangePhase(PHASE_WIN);
                    else if (m_lostDelay > 0.0f)
                        ChangePhase(PHASE_LOST);
                    else if (!m_cmdEdit)
                        m_ui->GetDialog()->StartPauseMenu();  // do you want to leave?
                }
                if (data->slot == INPUT_SLOT_PAUSE)
                {
                    if (!m_movieLock && !m_editLock && !m_cmdEdit &&
                        m_camera->GetType() != Gfx::CAM_TYPE_VISIT &&
                        !m_movie->IsExist())
                    {
                        ChangePause(m_pause->GetPause(PAUSE_USER) || m_pause->GetPause(PAUSE_CODE_BATTLE_LOCK) ? PAUSE_NONE : PAUSE_USER);
                    }
                }
                if (data->slot == INPUT_SLOT_CAMERA)
                {
                    ChangeCamera();
                }
                if (data->slot == INPUT_SLOT_DESEL)
                {
                    if (m_shortCut)
                        DeselectObject();
                }
                if (data->slot == INPUT_SLOT_HUMAN)
                {
                    SelectHuman();
                }
                if (data->slot == INPUT_SLOT_NEXT && ((event.kmodState & KEY_MOD(CTRL)) != 0))
                {
                    m_short->SelectShortcut(EVENT_OBJECT_SHORTCUT_MODE); // switch bots <-> buildings
                    return false;
                }
                if (data->slot == INPUT_SLOT_NEXT)
                {
                    if (m_shortCut)
                        m_short->SelectNext();
                }
                if (data->slot == INPUT_SLOT_HELP)
                {
                    StartDisplayInfo(SATCOM_HUSTON, true);
                }
                if (data->slot == INPUT_SLOT_PROG)
                {
                    StartDisplayInfo(SATCOM_PROG, true);
                }
                if (data->slot == INPUT_SLOT_VISIT)
                {
                    StartDisplayVisit(EVENT_NULL);
                }
                if (data->slot == INPUT_SLOT_SPEED05)
                {
                    SetSpeed(0.5f);
                }
                if (data->slot == INPUT_SLOT_SPEED10)
                {
                    SetSpeed(1.0f);
                }
                if (data->slot == INPUT_SLOT_SPEED15)
                {
                    SetSpeed(1.5f);
                }
                if (data->slot == INPUT_SLOT_SPEED20)
                {
                    SetSpeed(2.0f);
                }
                if (data->slot == INPUT_SLOT_SPEED30)
                {
                    SetSpeed(3.0f);
                }
                if (data->slot == INPUT_SLOT_SPEED40)
                {
                    SetSpeed(4.0f);
                }
                if (data->slot == INPUT_SLOT_SPEED60)
                {
                    SetSpeed(6.0f);
                }
                if (data->key == KEY(c) && ((event.kmodState & KEY_MOD(CTRL)) != 0) && m_engine->GetShowStats())
                {
                    CObject* obj = GetSelect();
                    if (obj != nullptr)
                    {
                        CLevelParserLine line("CreateObject");
                        line.AddParam("type", MakeUnique<CLevelParserParam>(obj->GetType()));
                        Math::Vector pos = obj->GetPosition()/g_unit;
                        pos.y = 0.0f;
                        line.AddParam("pos", MakeUnique<CLevelParserParam>(pos));
                        line.AddParam("dir", MakeUnique<CLevelParserParam>(obj->GetRotationY()));

                        std::stringstream ss;
                        ss << line;
                        widgetSetClipboardText(ss.str().c_str());
                    }
                }
                break;
            }

            case EVENT_KEY_UP:
            {
                auto data = event.GetData<KeyEventData>();
                KeyCamera(event.type, data->slot);
                break;
            }

            case EVENT_MOUSE_BUTTON_DOWN:
            {
                if (event.GetData<MouseButtonEventData>()->button != MOUSE_BUTTON_LEFT) // only left mouse button
                    break;

                obj = DetectObject(event.mousePos);
                if (!m_shortCut) obj = nullptr;
                if (obj != nullptr && obj->GetType() == OBJECT_TOTO)
                {
                    if (m_displayInfo != nullptr)  // current info?
                    {
                        StopDisplayInfo();
                    }
                    else
                    {
                        if (!m_editLock)
                            StartDisplayInfo(SATCOM_HUSTON, true);
                    }
                }
                else
                {
                    SelectObject(obj);
                }
                break;
            }

            case EVENT_MOUSE_BUTTON_UP:
                if (event.GetData<MouseButtonEventData>()->button != MOUSE_BUTTON_LEFT) // only left mouse button
                    break;

                m_cameraPan  = 0.0f;
                m_cameraZoom = 0.0f;
                break;

            case EVENT_OBJECT_LIMIT:
                StartShowLimit();
                break;

            case EVENT_OBJECT_DESELECT:
                if (m_shortCut)
                    DeselectObject();
                break;

            case EVENT_OBJECT_HELP:
                HelpObject();
                break;

            case EVENT_OBJECT_CAMERA:
                ChangeCamera();
                break;

            case EVENT_OBJECT_CAMERAleft:
                m_cameraPan = -1.0f;
                break;
            case EVENT_OBJECT_CAMERAright:
                m_cameraPan = 1.0f;
                break;
            case EVENT_OBJECT_CAMERAnear:
                m_cameraZoom = -1.0f;
                break;
            case EVENT_OBJECT_CAMERAaway:
                m_cameraZoom = 1.0f;
                break;

            case EVENT_OBJECT_DELETE:
                m_ui->GetDialog()->StartQuestion(RT_DIALOG_DELOBJ, true, false, false, [&]() {
                    DeleteObject();
                });
                break;

            case EVENT_OBJECT_BHELP:
                StartDisplayInfo(SATCOM_HUSTON, true);
                break;

            case EVENT_OBJECT_SOLUCE:
                StartDisplayInfo(SATCOM_SOLUCE, true);
                break;

            case EVENT_OBJECT_MAPZOOM:
                m_map->ZoomMap();
                break;

            case EVENT_DT_VISIT0:
            case EVENT_DT_VISIT1:
            case EVENT_DT_VISIT2:
            case EVENT_DT_VISIT3:
            case EVENT_DT_VISIT4:
                StartDisplayVisit(event.type);
                break;

            case EVENT_DT_END:
                StopDisplayVisit();
                break;

            case EVENT_OBJECT_MOVIELOCK:
                AbortMovie();
                break;

            case EVENT_WIN:
                m_missionTimerEnabled = m_missionTimerStarted = false;
                ChangePhase(PHASE_WIN);
                break;

            case EVENT_LOST:
                m_missionTimerEnabled = m_missionTimerStarted = false;
                ChangePhase(PHASE_LOST);
                break;

            default:
                break;
        }

        if (event.type >= EVENT_OBJECT_SHORTCUT_MODE && event.type <= EVENT_OBJECT_SHORTCUT_MAX)
        {
            m_short->SelectShortcut(event.type);
        }

        EventObject(event);
        return false;
    }

    if (m_phase == PHASE_APPERANCE)
        EventObject(event);

    if (m_phase == PHASE_WIN  ||
        m_phase == PHASE_LOST)
    {
        EventObject(event);

        switch (event.type)
        {
            case EVENT_KEY_DOWN:
            {
                auto data = event.GetData<KeyEventData>();

                if (data->key == KEY(ESCAPE) ||
                    data->key == KEY(RETURN))
                {
                    if (m_winTerminate)
                        ChangePhase(PHASE_MAIN_MENU);
                    else
                        ChangePhase(PHASE_LEVEL_LIST);
                }
                break;
            }

            case EVENT_BUTTON_OK:
                if (m_winTerminate)
                    ChangePhase(PHASE_MAIN_MENU);
                else
                    ChangePhase(PHASE_LEVEL_LIST);

                break;

            default:
                break;
        }
    }

    return true;
}



//! Executes a command
void CRobotMain::ExecuteCmd(char *cmd)
{
    if (cmd[0] == 0) return;

    if (m_phase == PHASE_SIMUL)
    {
        if (strcmp(cmd, "winmission") == 0)
            m_eventQueue->AddEvent(Event(EVENT_WIN));

        if (strcmp(cmd, "lostmission") == 0)
            m_eventQueue->AddEvent(Event(EVENT_LOST));

        if (strcmp(cmd, "trainerpilot") == 0)
        {
            m_trainerPilot = !m_trainerPilot;
            return;
        }

        if (strcmp(cmd, "fly") == 0)
        {
            m_researchDone[0] |= RESEARCH_FLY;

            m_eventQueue->AddEvent(Event(EVENT_UPDINTERFACE));
            return;
        }

        if (strcmp(cmd, "allresearch") == 0)
        {
            m_researchDone[0] = -1;  // all research are done

            m_eventQueue->AddEvent(Event(EVENT_UPDINTERFACE));
            return;
        }

        if (strcmp(cmd, "allbuildings") == 0)
        {
            m_build = -1;  // all buildings are available

            m_eventQueue->AddEvent(Event(EVENT_UPDINTERFACE));
            return;
        }

        if (strcmp(cmd, "all") == 0)
        {
            m_researchDone[0] = -1;  // all research are done
            m_build = -1;  // all buildings are available

            m_eventQueue->AddEvent(Event(EVENT_UPDINTERFACE));
            return;
        }

        if (strcmp(cmd, "nolimit") == 0)
        {
            m_terrain->SetFlyingMaxHeight(280.0f);
            return;
        }

        if (strcmp(cmd, "controller") == 0)
        {
            if (m_controller != nullptr)
            {
                // Don't use SelectObject because it checks if the object is selectable
                if (m_camera->GetType() == Gfx::CAM_TYPE_VISIT)
                    StopDisplayVisit();

                CObject* prev = DeselectAll();
                if (prev != nullptr && prev != m_controller)
                    PushToSelectionHistory(prev);

                SelectOneObject(m_controller, true);
                m_short->UpdateShortcuts();
            }
            return;
        }

        if (strcmp(cmd, "photo1") == 0)
        {
            m_freePhoto = !m_freePhoto;
            if (m_freePhoto)
            {
                m_camera->SetType(Gfx::CAM_TYPE_FREE);
                ChangePause(PAUSE_PHOTO);
            }
            else
            {
                m_camera->SetType(Gfx::CAM_TYPE_BACK);
                ChangePause(PAUSE_NONE);
            }
            return;
        }

        if (strcmp(cmd, "photo2") == 0)
        {
            m_freePhoto = !m_freePhoto;
            if (m_freePhoto)
            {
                m_camera->SetType(Gfx::CAM_TYPE_FREE);
                DeselectAll();  // removes the control buttons
                ChangePause(PAUSE_PHOTO);
                m_map->ShowMap(false);
                m_displayText->HideText(true);
            }
            else
            {
                m_camera->SetType(Gfx::CAM_TYPE_BACK);
                ChangePause(PAUSE_NONE);
                m_map->ShowMap(m_mapShow);
                m_displayText->HideText(false);
            }
            return;
        }

        if (strcmp(cmd, "noclip") == 0)
        {
            CObject* object = GetSelect();
            if (object != nullptr)
                object->SetClip(false);
            return;
        }

        if (strcmp(cmd, "clip") == 0)
        {
            CObject* object = GetSelect();
            if (object != nullptr)
                object->SetClip(true);
            return;
        }

        if (strcmp(cmd, "addhusky") == 0)
        {
            CObject* object = GetSelect();
            if (object != nullptr)
                object->SetMagnifyDamage(object->GetMagnifyDamage()*0.1f);
            return;
        }

        if (strcmp(cmd, "addfreezer") == 0)
        {
            CObject* object = GetSelect();
            if (object != nullptr)
                object->SetRange(object->GetRange()*10.0f);
            return;
        }

        if (strcmp(cmd, "\155\157\157") == 0)
        {
            // VGhpcyBpcyBlYXN0ZXItZWdnIGFuZCBzbyBpdCBzaG91bGQgYmUgb2JmdXNjYXRlZCEgRG8gbm90
            // IGNsZWFuLXVwIHRoaXMgY29kZSEK
            GetLogger()->Info(" _________________________\n");
            GetLogger()->Info("< \x50\x6F\x6C\x73\x6B\x69 \x50\x6F\x72\x74\x61\x6C C\x6F\x6C\x6F\x62\x6F\x74\x61! \x3E\n");
            GetLogger()->Info(" -------------------------\n");
            GetLogger()->Info("        \x5C\x20\x20\x20\x5E\x5F\x5F\x5E\n");
            GetLogger()->Info("        \x20\x5C\x20\x20\x28\x6F\x6F\x29\x5C\x5F\x5F\x5F\x5F\x5F\x5F\x5F\n");
            GetLogger()->Info("            \x28\x5F\x5F\x29\x5C   \x20\x20\x20\x20\x29\x5C\x2F\x5C\n");
            GetLogger()->Info("            \x20\x20\x20\x20\x7C|\x2D\x2D\x2D\x2D\x77\x20\x7C\n");
            GetLogger()->Info("          \x20\x20    \x7C\x7C\x20\x20\x20\x20 ||\n");
        }

        if (strcmp(cmd, "fullpower") == 0)
        {
            CObject* object = GetSelect();
            if (object != nullptr)
            {
                if (object->Implements(ObjectInterfaceType::Powered))
                {
                    CObject* power = dynamic_cast<CPoweredObject*>(object)->GetPower();
                    if (power != nullptr)
                        power->SetEnergy(1.0f);
                }

                object->SetShield(1.0f);

                CPhysics* physics = object->GetPhysics();
                if (physics != nullptr)
                    physics->SetReactorRange(1.0f);
            }
            return;
        }

        if (strcmp(cmd, "fullenergy") == 0)
        {
            CObject* object = GetSelect();

            if (object != nullptr)
            {
                if (object->Implements(ObjectInterfaceType::Powered))
                {
                    CObject* power = dynamic_cast<CPoweredObject*>(object)->GetPower();
                    if (power != nullptr)
                        power->SetEnergy(1.0f);
                }
            }
            return;
        }

        if (strcmp(cmd, "fullshield") == 0)
        {
            CObject* object = GetSelect();
            if (object != nullptr)
                object->SetShield(1.0f);
            return;
        }

        if (strcmp(cmd, "fullrange") == 0)
        {
            CObject* object = GetSelect();
            if (object != nullptr)
            {
                CPhysics* physics = object->GetPhysics();
                if (physics != nullptr)
                    physics->SetReactorRange(1.0f);
            }
            return;
        }
    }

    if (strcmp(cmd, "debugmode") == 0)
    {
        if (m_app->IsDebugModeActive(DEBUG_ALL))
        {
            m_app->SetDebugModeActive(DEBUG_ALL, false);
        }
        else
        {
            m_app->SetDebugModeActive(DEBUG_ALL, true);
        }
        return;
    }

    if (strcmp(cmd, "showstat") == 0)
    {
        m_engine->SetShowStats(!m_engine->GetShowStats());
        return;
    }

    if (strcmp(cmd, "invshadow") == 0)
    {
        m_engine->SetShadow(!m_engine->GetShadow());
        return;
    }

    if (strcmp(cmd, "invdirty") == 0)
    {
        m_engine->SetDirty(!m_engine->GetDirty());
        return;
    }

    if (strcmp(cmd, "invfog") == 0)
    {
        m_engine->SetFog(!m_engine->GetFog());
        return;
    }

    if (strcmp(cmd, "invlens") == 0)
    {
        m_engine->SetLensMode(!m_engine->GetLensMode());
        return;
    }

    if (strcmp(cmd, "invwater") == 0)
    {
        m_engine->SetWaterMode(!m_engine->GetWaterMode());
        return;
    }

    if (strcmp(cmd, "invsky") == 0)
    {
        m_engine->SetSkyMode(!m_engine->GetSkyMode());
        return;
    }

    if (strcmp(cmd, "invplanet") == 0)
    {
        m_engine->SetPlanetMode(!m_engine->GetPlanetMode());
        return;
    }

    if (strcmp(cmd, "selectinsect") == 0)
    {
        m_selectInsect = !m_selectInsect;
        return;
    }

    if (strcmp(cmd, "showsoluce") == 0)
    {
        m_showSoluce = !m_showSoluce;
        m_ui->ShowSoluceUpdate();
        return;
    }

    if (strcmp(cmd, "allmission") == 0)
    {
        m_showAll = !m_showAll;
        m_ui->AllMissionUpdate();
        return;
    }

    if (strcmp(cmd, "invradar") == 0)
    {
        m_cheatRadar = !m_cheatRadar;
        return;
    }

    float speed;
    if (sscanf(cmd, "speed %f", &speed) > 0)
    {
        SetSpeed(speed);
        UpdateSpeedLabel();
        return;
    }

    if (m_phase == PHASE_SIMUL)
        m_displayText->DisplayError(ERR_CMD, Math::Vector(0.0f,0.0f,0.0f));
}



//! Returns the type of current movie
MainMovieType CRobotMain::GetMainMovie()
{
    return m_movie->GetType();
}


//! Clears the display of instructions
void CRobotMain::FlushDisplayInfo()
{
    for (int i = 0; i < SATCOM_MAX; i++)
    {
        m_infoFilename[i][0] = 0;
        m_infoPos[i] = 0;
    }
    strcpy(m_infoFilename[SATCOM_OBJECT], "objects.txt");
    m_infoIndex = 0;
}

//! Beginning of the displaying of instructions.
//! index: SATCOM_*
void CRobotMain::StartDisplayInfo(int index, bool movie)
{
    if (m_cmdEdit || m_satComLock || m_lockedSatCom) return;

    CObject* obj = GetSelect();
    bool human = obj != nullptr && obj->GetType() == OBJECT_HUMAN;

    if (!m_editLock && movie && !m_movie->IsExist() && human)
    {
        CMotion* motion = obj->GetMotion();
        if (motion != nullptr && motion->GetAction() == -1)
        {
            m_movieInfoIndex = index;
            m_movie->Start(MM_SATCOMopen, 2.5f);
            ChangePause(PAUSE_SATCOMMOVIE);
            m_infoObject = DeselectAll();  // removes the control buttons
            m_displayText->HideText(true);
            return;
        }
    }

    if (m_movie->IsExist())
    {
        m_movie->Stop();
        ChangePause(PAUSE_NONE);
        SelectObject(m_infoObject, false);  // hands over the command buttons
        m_displayText->HideText(false);
    }

    StartDisplayInfo(m_infoFilename[index], index);
}

//! Beginning of the displaying of instructions
void CRobotMain::StartDisplayInfo(const char *filename, int index)
{
    if (m_cmdEdit) return;

    m_movieInfoIndex = -1;
    ClearInterface();  // removes setting evidence and tooltip

    if (!m_editLock)
    {
        m_infoObject = DeselectAll();  // removes the control buttons
        m_displayText->HideText(true);
        m_sound->MuteAll(true);
    }

    bool soluce = m_ui->GetSceneSoluce();

    m_displayInfo = new Ui::CDisplayInfo();
    m_displayInfo->StartDisplayInfo(filename, index, soluce);

    m_infoIndex = index;
    if (index != -1)
        m_displayInfo->SetPosition(m_infoPos[index]);
}

//! End of displaying of instructions
void CRobotMain::StopDisplayInfo()
{
    if (m_cmdEdit) return;

    if (m_movieInfoIndex != -1)  // film to read the SatCom?
        m_movie->Start(MM_SATCOMclose, 2.0f);

    if (m_infoIndex != -1)
        m_infoPos[m_infoIndex] = m_displayInfo->GetPosition();

    m_displayInfo->StopDisplayInfo();

    delete m_displayInfo;
    m_displayInfo = nullptr;

    if (!m_editLock)
    {
        SelectObject(m_infoObject, false);  // gives the command buttons
        m_displayText->HideText(false);

        m_sound->MuteAll(false);
    }

    if (m_infoUsed == 0)
        m_displayText->ClearText();  // removes message "see SatCom ..."
    m_infoUsed ++;
}

//! Returns the name of the text display
char* CRobotMain::GetDisplayInfoName(int index)
{
    return m_infoFilename[index];
}

//! Returns the name of the text display
int CRobotMain::GetDisplayInfoPosition(int index)
{
    return m_infoPos[index];
}

//! Returns the name of the text display
void CRobotMain::SetDisplayInfoPosition(int index, int pos)
{
    m_infoPos[index] = pos;
}


//! Beginning of a dialogue during the game
void CRobotMain::StartSuspend()
{
    m_sound->MuteAll(true);
    ClearInterface();
    m_suspendInitPause = m_pause->GetPauseType();
    m_pause->SetPause(PAUSE_DIALOG);
    m_engine->SetOverFront(false);  // over flat behind
    CreateShortcuts();

    m_map->ShowMap(false);
    m_infoObject = DeselectAll();  // removes the control buttons
    m_displayText->HideText(true);

    m_suspendInitCamera = m_camera->GetType();
    m_camera->SetType(Gfx::CAM_TYPE_DIALOG);

    m_suspend = true;
}

//! End of dialogue during the game
void CRobotMain::StopSuspend()
{
    m_sound->MuteAll(false);
    ClearInterface();
    m_pause->SetPause(m_suspendInitPause);
    m_engine->SetOverFront(true);  // over flat front
    CreateShortcuts();

    if(m_infoObject != nullptr)
        SelectObject(m_infoObject, false);  // gives the command buttons
    m_map->ShowMap(m_mapShow);
    m_displayText->HideText(false);

    m_camera->SetType(m_suspendInitCamera);

    m_suspend = false;
}


//! Returns the absolute time of the game
float CRobotMain::GetGameTime()
{
    return m_gameTime;
}


//! Start of the visit instead of an error
void CRobotMain::StartDisplayVisit(EventType event)
{
    if (m_editLock) return;

    Ui::CWindow* pw = static_cast<Ui::CWindow*>(m_interface->SearchControl(EVENT_WINDOW2));
    if (pw == nullptr) return;

    if (event == EVENT_NULL)  // visit by keyboard shortcut?
    {
        int i;
        if (m_visitLast != EVENT_NULL)  // already a current visit?
            i = m_visitLast-EVENT_DT_VISIT0;
        else
            i = Ui::MAXDTLINE;

        // Seeks the last.
        for (int j = 0; j < Ui::MAXDTLINE; j++)
        {
            i --;
            if (i < 0) i = Ui::MAXDTLINE-1;

            Ui::CButton* button = static_cast<Ui::CButton*>(pw->SearchControl(static_cast<EventType>(EVENT_DT_VISIT0+i)));
            if (button == nullptr || !button->TestState(Ui::STATE_ENABLE)) continue;

            Ui::CGroup* group = static_cast<Ui::CGroup*>(pw->SearchControl(static_cast<EventType>(EVENT_DT_GROUP0+i)));
            if (group != nullptr)
            {
                event = static_cast<EventType>(EVENT_DT_VISIT0+i);
                break;
            }
        }
    }
    if (event == EVENT_NULL)
    {
        m_sound->Play(SOUND_TZOING);  // nothing to do!
        return;
    }

    m_visitLast = event;

    ClearInterface();  // removes setting evidence and tooltip

    if (m_camera->GetType() == Gfx::CAM_TYPE_VISIT)  // already a current visit?
    {
        m_camera->StopVisit();
        m_displayText->ClearVisit();
    }
    else
    {
        m_visitObject = DeselectAll();  // removes the control buttons
    }

    // Creates the "continue" button.
    if (m_interface->SearchControl(EVENT_DT_END) == 0)
    {
        Math::Point pos, dim;
        pos.x = 10.0f/640.0f;
        pos.y = 10.0f/480.0f;
        dim.x = 50.0f/640.0f;
        dim.y = 50.0f/480.0f;
        m_interface->CreateButton(pos, dim, 16, EVENT_DT_END);
    }

    // Creates the arrow to show the place.
    if (m_visitArrow != 0)
    {
        CObjectManager::GetInstancePointer()->DeleteObject(m_visitArrow);
        m_visitArrow = nullptr;
    }

    Math::Vector goal = m_displayText->GetVisitGoal(event);
    m_visitArrow = m_objMan->CreateObject(goal, 0.0f, OBJECT_SHOW, -1.0f, 1.0f, 10.0f);

    m_visitPos = m_visitArrow->GetPosition();
    m_visitPosArrow = m_visitPos;
    m_visitPosArrow.y += m_displayText->GetVisitHeight(event);
    m_visitArrow->SetPosition(m_visitPosArrow);

    m_visitTime = 0.0;
    m_visitParticle = 0.0f;

    m_particle->DeleteParticle(Gfx::PARTISHOW);

    m_camera->StartVisit(m_displayText->GetVisitGoal(event),
                         m_displayText->GetVisitDist(event));
    m_displayText->SetVisit(event);
    ChangePause(PAUSE_VISIT);
}

//! Move the arrow to visit
void CRobotMain::FrameVisit(float rTime)
{
    if (m_visitArrow == 0) return;

    // Moves the arrow.
    m_visitTime += rTime;

    Math::Vector pos = m_visitPosArrow;
    pos.y += 1.5f+sinf(m_visitTime*4.0f)*4.0f;
    m_visitArrow->SetPosition(pos);
    m_visitArrow->SetRotationY(m_visitTime*2.0f);

    // Manages the particles "arrows".
    m_visitParticle -= rTime;
    if (m_visitParticle <= 0.0f)
    {
        m_visitParticle = 1.5f;

        pos = m_visitPos;
        float level = m_terrain->GetFloorLevel(pos)+2.0f;
        if (pos.y < level) pos.y = level;  // not below the ground
        Math::Vector speed(0.0f, 0.0f, 0.0f);
        Math::Point dim;
        dim.x = 30.0f;
        dim.y = dim.x;
        m_particle->CreateParticle(pos, speed, dim, Gfx::PARTISHOW, 2.0f);
    }
}

//! End of the visit instead of an error
void CRobotMain::StopDisplayVisit()
{
    m_visitLast = EVENT_NULL;

    // Removes the button.
    m_interface->DeleteControl(EVENT_DT_END);

    // Removes the arrow.
    if (m_visitArrow != nullptr)
    {
        CObjectManager::GetInstancePointer()->DeleteObject(m_visitArrow);
        m_visitArrow = nullptr;
    }

    // Removes particles "arrows".
    m_particle->DeleteParticle(Gfx::PARTISHOW);

    m_camera->StopVisit();
    m_displayText->ClearVisit();
    ChangePause(PAUSE_NONE);
    if (m_visitObject != 0)
    {
        SelectObject(m_visitObject, false);  // gives the command buttons
        m_visitObject = 0;
    }
}



//! Updates all the shortcuts
void CRobotMain::UpdateShortcuts()
{
    m_short->UpdateShortcuts();
}

//! Returns the object that default was select after the creation of a scene
CObject* CRobotMain::GetSelectObject()
{
    if (m_selectObject != nullptr) return m_selectObject;
    return SearchHuman();
}

//! Deselects everything, and returns the object that was selected
CObject* CRobotMain::DeselectAll()
{
    CObject* prev = nullptr;
    for (CObject* obj : m_objMan->GetAllObjects())
    {
        if (obj->GetSelect()) prev = obj;
        obj->SetSelect(false);
    }
    return prev;
}

//! Selects an object, without attending to deselect the rest
void CRobotMain::SelectOneObject(CObject* obj, bool displayError)
{
    obj->SetSelect(true, displayError);
    m_camera->SetControllingObject(obj);

    ObjectType type = obj->GetType();
    if ( type == OBJECT_HUMAN    ||
         type == OBJECT_MOBILEfa ||
         type == OBJECT_MOBILEta ||
         type == OBJECT_MOBILEwa ||
         type == OBJECT_MOBILEia ||
         type == OBJECT_MOBILEfc ||
         type == OBJECT_MOBILEtc ||
         type == OBJECT_MOBILEwc ||
         type == OBJECT_MOBILEic ||
         type == OBJECT_MOBILEfi ||
         type == OBJECT_MOBILEti ||
         type == OBJECT_MOBILEwi ||
         type == OBJECT_MOBILEii ||
         type == OBJECT_MOBILEfs ||
         type == OBJECT_MOBILEts ||
         type == OBJECT_MOBILEws ||
         type == OBJECT_MOBILEis ||
         type == OBJECT_MOBILErt ||
         type == OBJECT_MOBILErc ||
         type == OBJECT_MOBILErr ||
         type == OBJECT_MOBILErs ||
         type == OBJECT_MOBILEsa ||
         type == OBJECT_MOBILEft ||
         type == OBJECT_MOBILEtt ||
         type == OBJECT_MOBILEwt ||
         type == OBJECT_MOBILEit ||
         type == OBJECT_MOBILEdr ||
         type == OBJECT_APOLLO2  )
    {
        m_camera->SetType(obj->GetCameraType());
        m_camera->SetDist(obj->GetCameraDist());
    }
    else
    {
        m_camera->SetType(Gfx::CAM_TYPE_BACK);
    }

    CObject* toto = SearchToto();
    if (toto != nullptr)
    {
        CMotionToto* mt = static_cast<CMotionToto*>(toto->GetMotion());
        if (mt != nullptr)
            mt->SetLinkType(type);
    }
}

//! Selects the object aimed by the mouse
bool CRobotMain::SelectObject(CObject* obj, bool displayError)
{
    if (m_camera->GetType() == Gfx::CAM_TYPE_VISIT)
        StopDisplayVisit();

    if (m_movieLock || m_editLock) return false;
    if (m_movie->IsExist()) return false;
    if (obj == nullptr || !IsSelectable(obj)) return false;

    CObject* prev = DeselectAll();

    if (prev != nullptr && prev != obj)
       PushToSelectionHistory(prev);

    SelectOneObject(obj, displayError);
    m_short->UpdateShortcuts();
    return true;
}

//! Deselects the selected object
bool CRobotMain::DeselectObject()
{
    DeselectAll();

    CObject* obj = PopFromSelectionHistory();
    if (obj == nullptr)
        obj = SearchHuman();

    if (obj != nullptr)
        SelectOneObject(obj);
    else
        m_camera->SetType(Gfx::CAM_TYPE_FREE);

    m_short->UpdateShortcuts();
    return true;
}

//! Quickly removes all objects
void CRobotMain::DeleteAllObjects()
{
    m_engine->GetPyroManager()->DeleteAll();

    // Removes the arrow.
    if (m_visitArrow != nullptr)
    {
        CObjectManager::GetInstancePointer()->DeleteObject(m_visitArrow);
        m_visitArrow = nullptr;
    }

    for (int i = 0; i < MAXSHOWLIMIT; i++)
        FlushShowLimit(i);

    m_objMan->DeleteAllObjects();
}

//! Selects the human
void CRobotMain::SelectHuman()
{
    SelectObject(SearchHuman());
}

//! Returns the object human
CObject* CRobotMain::SearchHuman()
{
    return m_objMan->FindNearest(nullptr, OBJECT_HUMAN);
}

//! Returns the object toto
CObject* CRobotMain::SearchToto()
{
    return m_objMan->FindNearest(nullptr, OBJECT_TOTO);
}

//! Returns the nearest selectable object from a given position
CObject* CRobotMain::SearchNearest(Math::Vector pos, CObject* exclu)
{
    float min = 100000.0f;
    CObject* best = 0;
    for (CObject* obj : m_objMan->GetAllObjects())
    {
        if (obj == exclu) continue;
        if (!IsSelectable(obj)) continue;

        ObjectType type = obj->GetType();
        if (type == OBJECT_TOTO) continue;

        Math::Vector oPos = obj->GetPosition();
        float dist = Math::DistanceProjected(oPos, pos);
        if (dist < min)
        {
            min = dist;
            best = obj;
        }
    }
    return best;
}

//! Returns the selected object
CObject* CRobotMain::GetSelect()
{
    for (CObject* obj : m_objMan->GetAllObjects())
    {
        if (obj->GetSelect())
            return obj;
    }
    return nullptr;
}

CObject* CRobotMain::SearchObject(ObjectType type)
{
    return m_objMan->FindNearest(nullptr, type);
}

//! Detects the object aimed by the mouse
CObject* CRobotMain::DetectObject(Math::Point pos)
{
    int objRank = m_engine->DetectObject(pos);

    for (CObject* obj : m_objMan->GetAllObjects())
    {
        if (!obj->GetActive()) continue;

        CObject* transporter = nullptr;
        if (obj->Implements(ObjectInterfaceType::Transportable))
            transporter = dynamic_cast<CTransportableObject*>(obj)->GetTransporter();

        if (transporter != nullptr && !transporter->GetActive()) continue;
        if (obj->GetProxyActivate()) continue;

        CObject* target = nullptr;
        ObjectType type = obj->GetType();
        if ( type == OBJECT_PORTICO      ||
             type == OBJECT_BASE         ||
             type == OBJECT_DERRICK      ||
             type == OBJECT_FACTORY      ||
             type == OBJECT_REPAIR       ||
             type == OBJECT_DESTROYER    ||
             type == OBJECT_STATION      ||
             type == OBJECT_CONVERT      ||
             type == OBJECT_TOWER        ||
             type == OBJECT_RESEARCH     ||
             type == OBJECT_RADAR        ||
             type == OBJECT_INFO         ||
             type == OBJECT_ENERGY       ||
             type == OBJECT_LABO         ||
             type == OBJECT_NUCLEAR      ||
             type == OBJECT_PARA         ||
             type == OBJECT_SAFE         ||
             type == OBJECT_HUSTON       ||
             type == OBJECT_TARGET1      ||
             type == OBJECT_TARGET2      ||
             type == OBJECT_START        ||
             type == OBJECT_END          ||
             type == OBJECT_STONE        ||
             type == OBJECT_URANIUM      ||
             type == OBJECT_BULLET       ||
             type == OBJECT_METAL        ||
             type == OBJECT_BBOX         ||
             type == OBJECT_KEYa         ||
             type == OBJECT_KEYb         ||
             type == OBJECT_KEYc         ||
             type == OBJECT_KEYd         ||
             type == OBJECT_TNT          ||
             type == OBJECT_SCRAP1       ||
             type == OBJECT_SCRAP2       ||
             type == OBJECT_SCRAP3       ||
             type == OBJECT_SCRAP4       ||
             type == OBJECT_SCRAP5       ||
             type == OBJECT_BOMB         ||
             type == OBJECT_BAG          ||
             type == OBJECT_WAYPOINT     ||
             type == OBJECT_FLAGb        ||
             type == OBJECT_FLAGr        ||
             type == OBJECT_FLAGg        ||
             type == OBJECT_FLAGy        ||
             type == OBJECT_FLAGv        ||
             type == OBJECT_MARKPOWER    ||
             type == OBJECT_MARKSTONE    ||
             type == OBJECT_MARKURANIUM  ||
             type == OBJECT_MARKKEYa     ||
             type == OBJECT_MARKKEYb     ||
             type == OBJECT_MARKKEYc     ||
             type == OBJECT_MARKKEYd     ||
             type == OBJECT_HUMAN        ||
             type == OBJECT_TECH         ||
             type == OBJECT_TOTO         ||
             type == OBJECT_MOBILEfa     ||
             type == OBJECT_MOBILEta     ||
             type == OBJECT_MOBILEwa     ||
             type == OBJECT_MOBILEia     ||
             type == OBJECT_MOBILEfc     ||
             type == OBJECT_MOBILEtc     ||
             type == OBJECT_MOBILEwc     ||
             type == OBJECT_MOBILEic     ||
             type == OBJECT_MOBILEfi     ||
             type == OBJECT_MOBILEti     ||
             type == OBJECT_MOBILEwi     ||
             type == OBJECT_MOBILEii     ||
             type == OBJECT_MOBILEfs     ||
             type == OBJECT_MOBILEts     ||
             type == OBJECT_MOBILEws     ||
             type == OBJECT_MOBILEis     ||
             type == OBJECT_MOBILErt     ||
             type == OBJECT_MOBILErc     ||
             type == OBJECT_MOBILErr     ||
             type == OBJECT_MOBILErs     ||
             type == OBJECT_MOBILEsa     ||
             type == OBJECT_MOBILEtg     ||
             type == OBJECT_MOBILEft     ||
             type == OBJECT_MOBILEtt     ||
             type == OBJECT_MOBILEwt     ||
             type == OBJECT_MOBILEit     ||
             type == OBJECT_MOBILEdr     ||
             type == OBJECT_MOTHER       ||
             type == OBJECT_ANT          ||
             type == OBJECT_SPIDER       ||
             type == OBJECT_BEE          ||
             type == OBJECT_WORM         ||
             type == OBJECT_EGG          ||
             type == OBJECT_RUINmobilew1 ||
             type == OBJECT_RUINmobilew2 ||
             type == OBJECT_RUINmobilet1 ||
             type == OBJECT_RUINmobilet2 ||
             type == OBJECT_RUINmobiler1 ||
             type == OBJECT_RUINmobiler2 ||
             type == OBJECT_RUINfactory  ||
             type == OBJECT_RUINdoor     ||
             type == OBJECT_RUINsupport  ||
             type == OBJECT_RUINradar    ||
             type == OBJECT_RUINconvert  ||
             type == OBJECT_RUINbase     ||
             type == OBJECT_RUINhead     ||
             type == OBJECT_APOLLO1      ||
             type == OBJECT_APOLLO2      ||
             type == OBJECT_APOLLO3      ||
             type == OBJECT_APOLLO4      ||
             type == OBJECT_APOLLO5      )
        {
            target = obj;
        }
        else if (type == OBJECT_POWER || type == OBJECT_ATOMIC)
        {
            target = dynamic_cast<CTransportableObject*>(obj)->GetTransporter();  // battery connected
            if (!target) target = obj; // standalone battery
        }

        for (int j = 0; j < OBJECTMAXPART; j++)
        {
            int rank = obj->GetObjectRank(j);
            if (rank == -1) continue;
            if (rank != objRank) continue;
            return target;
        }
    }
    return 0;
}

//! Indicates whether an object is selectable
bool CRobotMain::IsSelectable(CObject* obj)
{
    if (!obj->GetSelectable()) return false;

    ObjectType type = obj->GetType();
    if ( type == OBJECT_HUMAN    ||
         type == OBJECT_TOTO     ||
         type == OBJECT_MOBILEfa ||
         type == OBJECT_MOBILEta ||
         type == OBJECT_MOBILEwa ||
         type == OBJECT_MOBILEia ||
         type == OBJECT_MOBILEfc ||
         type == OBJECT_MOBILEtc ||
         type == OBJECT_MOBILEwc ||
         type == OBJECT_MOBILEic ||
         type == OBJECT_MOBILEfi ||
         type == OBJECT_MOBILEti ||
         type == OBJECT_MOBILEwi ||
         type == OBJECT_MOBILEii ||
         type == OBJECT_MOBILEfs ||
         type == OBJECT_MOBILEts ||
         type == OBJECT_MOBILEws ||
         type == OBJECT_MOBILEis ||
         type == OBJECT_MOBILErt ||
         type == OBJECT_MOBILErc ||
         type == OBJECT_MOBILErr ||
         type == OBJECT_MOBILErs ||
         type == OBJECT_MOBILEsa ||
         type == OBJECT_MOBILEft ||
         type == OBJECT_MOBILEtt ||
         type == OBJECT_MOBILEwt ||
         type == OBJECT_MOBILEit ||
         type == OBJECT_MOBILEdr ||
         type == OBJECT_APOLLO2  ||
         type == OBJECT_BASE     ||
         type == OBJECT_DERRICK  ||
         type == OBJECT_FACTORY  ||
         type == OBJECT_REPAIR   ||
         type == OBJECT_DESTROYER||
         type == OBJECT_STATION  ||
         type == OBJECT_CONVERT  ||
         type == OBJECT_TOWER    ||
         type == OBJECT_RESEARCH ||
         type == OBJECT_RADAR    ||
         type == OBJECT_INFO     ||
         type == OBJECT_ENERGY   ||
         type == OBJECT_LABO     ||
         type == OBJECT_NUCLEAR  ||
         type == OBJECT_PARA     ||
         type == OBJECT_SAFE     ||
         type == OBJECT_HUSTON   )
    {
        return true;
    }

    if (m_selectInsect)
    {
        if ( type == OBJECT_MOTHER   ||
             type == OBJECT_ANT      ||
             type == OBJECT_SPIDER   ||
             type == OBJECT_BEE      ||
             type == OBJECT_WORM     ||
             type == OBJECT_MOBILEtg )
        {
            return true;
        }
    }

    return false;
}


//! Deletes the selected object
bool CRobotMain::DeleteObject()
{
    CObject* obj = GetSelect();
    if (obj == nullptr) return false;

    m_engine->GetPyroManager()->Create(Gfx::PT_FRAGT, obj);

    obj->SetSelect(false);  // deselects the object
    m_camera->SetType(Gfx::CAM_TYPE_EXPLO);
    DeselectAll();
    RemoveFromSelectionHistory(obj);

    return true;
}


//! Removes setting evidence of the object with the mouse hovers over
void CRobotMain::HiliteClear()
{
    ClearTooltip();
    m_tooltipName.clear();  // really removes the tooltip

    if (!m_hilite) return;

    int rank = -1;
    m_engine->SetHighlightRank(&rank);  // nothing more selected

    for (CObject* obj : m_objMan->GetAllObjects())
    {
        obj->SetHighlight(false);
        m_map->SetHighlight(0);
        m_short->SetHighlight(0);
    }

    m_hilite = false;
}

//! Highlights the object with the mouse hovers over
void CRobotMain::HiliteObject(Math::Point pos)
{
    if (m_fixScene && m_phase != PHASE_APPERANCE) return;
    if (m_movieLock) return;
    if (m_movie->IsExist()) return;
    if (m_app->GetMouseMode() == MOUSE_NONE) return;

    ClearInterface();  // removes setting evidence and tooltip

    CObject* obj = m_short->DetectShort(pos);

    std::string interfaceTooltipName;
    if (m_settings->GetTooltips() && m_interface->GetTooltip(pos, interfaceTooltipName))
    {
        m_tooltipPos = pos;
        m_tooltipName = interfaceTooltipName;
        m_tooltipTime = 0.0f;
        if (obj == nullptr) return;
    }

    if (m_suspend) return;

    if (obj == nullptr)
    {
        bool inMap = false;
        obj = m_map->DetectMap(pos, inMap);
        if (obj == nullptr)
        {
            if (inMap)  return;

            obj = DetectObject(pos);

            if ((m_camera->GetType() == Gfx::CAM_TYPE_ONBOARD) &&
                (m_camera->GetControllingObject() == obj))
                return;
        }
    }

    if (obj != nullptr)
    {
        std::string objectTooltipName;
        if (m_settings->GetTooltips() && obj->GetTooltipName(objectTooltipName))
        {
            m_tooltipPos = pos;
            m_tooltipName = objectTooltipName;
            m_tooltipTime = 0.0f;
        }

        if (IsSelectable(obj))
        {
            obj->SetHighlight(true);
            m_map->SetHighlight(obj);
            m_short->SetHighlight(obj);
            m_hilite = true;
        }
    }
}

//! Highlights the object with the mouse hovers over
void CRobotMain::HiliteFrame(float rTime)
{
    if (m_fixScene && m_phase != PHASE_APPERANCE) return;
    if (m_movieLock) return;
    if (m_movie->IsExist()) return;

    m_tooltipTime += rTime;

    ClearTooltip();

    if (m_tooltipTime >= 0.2f && !m_tooltipName.empty())
    {
        CreateTooltip(m_tooltipPos, m_tooltipName);
    }
}

//! Creates a tooltip
void CRobotMain::CreateTooltip(Math::Point pos, const std::string& text)
{
    Math::Point corner;
    corner.x = pos.x+0.022f;
    corner.y = pos.y-0.052f;

    Math::Point start, end;

    m_engine->GetText()->SizeText(text, Gfx::FONT_COLOBOT, Gfx::FONT_SIZE_SMALL,
                                  corner, Gfx::TEXT_ALIGN_LEFT,
                                  start, end);

    start.x -= 0.010f;
    start.y -= 0.006f;
    end.x   += 0.010f;
    end.y   += 0.008f;  // small'ish margin

    pos.x = start.x;
    pos.y = start.y;

    Math::Point dim;
    dim.x = end.x-start.x;
    dim.y = end.y-start.y;

    Math::Point offset;
    offset.x = 0.0f;
    offset.y = 0.0f;
    if (pos.x+dim.x > 1.0f) offset.x = 1.0f-(pos.x+dim.x);
    if (pos.y       < 0.0f) offset.y = -pos.y;

    corner.x += offset.x;
    corner.y += offset.y;
    pos.x += offset.x;
    pos.y += offset.y;

    m_interface->CreateWindows(pos, dim, 1, EVENT_TOOLTIP);

    Ui::CWindow* pw = static_cast<Ui::CWindow*>(m_interface->SearchControl(EVENT_TOOLTIP));
    if (pw != nullptr)
    {
        pw->SetState(Ui::STATE_SHADOW);
        pw->SetTrashEvent(false);

        pos.y -= m_engine->GetText()->GetHeight(Gfx::FONT_COLOBOT, Gfx::FONT_SIZE_SMALL) / 2.0f;
        pw->CreateLabel(pos, dim, -1, EVENT_LABEL2, text);
    }
}

//! Clears the previous tooltip
void CRobotMain::ClearTooltip()
{
    m_interface->DeleteControl(EVENT_TOOLTIP);
}


//! Displays help for an object
void CRobotMain::HelpObject()
{
    CObject* obj = GetSelect();
    if (obj == nullptr) return;

    const char* filename = GetHelpFilename(obj->GetType()).c_str();
    if (filename[0] == 0) return;

    StartDisplayInfo(filename, -1);
}


//! Change the mode of the camera
void CRobotMain::ChangeCamera()
{
    for (CObject* obj : m_objMan->GetAllObjects())
    {
        if (obj->GetSelect())
        {
            if (obj->GetCameraLock()) return;

            ObjectType oType = obj->GetType();
            Gfx::CameraType type = obj->GetCameraType();

            if ( oType != OBJECT_MOBILEfa &&
                 oType != OBJECT_MOBILEta &&
                 oType != OBJECT_MOBILEwa &&
                 oType != OBJECT_MOBILEia &&
                 oType != OBJECT_MOBILEfc &&
                 oType != OBJECT_MOBILEtc &&
                 oType != OBJECT_MOBILEwc &&
                 oType != OBJECT_MOBILEic &&
                 oType != OBJECT_MOBILEfi &&
                 oType != OBJECT_MOBILEti &&
                 oType != OBJECT_MOBILEwi &&
                 oType != OBJECT_MOBILEii &&
                 oType != OBJECT_MOBILEfs &&
                 oType != OBJECT_MOBILEts &&
                 oType != OBJECT_MOBILEws &&
                 oType != OBJECT_MOBILEis &&
                 oType != OBJECT_MOBILErt &&
                 oType != OBJECT_MOBILErc &&
                 oType != OBJECT_MOBILErr &&
                 oType != OBJECT_MOBILErs &&
                 oType != OBJECT_MOBILEsa &&
                 oType != OBJECT_MOBILEtg &&
                 oType != OBJECT_MOBILEft &&
                 oType != OBJECT_MOBILEtt &&
                 oType != OBJECT_MOBILEwt &&
                 oType != OBJECT_MOBILEit &&
                 oType != OBJECT_MOBILEdr &&
                 oType != OBJECT_APOLLO2  )  return;

            if (oType == OBJECT_MOBILEdr)  // designer?
            {
                     if (type == Gfx::CAM_TYPE_PLANE  )  type = Gfx::CAM_TYPE_BACK;
                else if (type == Gfx::CAM_TYPE_BACK   )  type = Gfx::CAM_TYPE_PLANE;
            }
            else if (obj->GetTrainer())  // trainer?
            {
                     if (type == Gfx::CAM_TYPE_ONBOARD)  type = Gfx::CAM_TYPE_FIX;
                else if (type == Gfx::CAM_TYPE_FIX    )  type = Gfx::CAM_TYPE_PLANE;
                else if (type == Gfx::CAM_TYPE_PLANE  )  type = Gfx::CAM_TYPE_BACK;
                else if (type == Gfx::CAM_TYPE_BACK   )  type = Gfx::CAM_TYPE_ONBOARD;
            }
            else
            {
                     if (type == Gfx::CAM_TYPE_ONBOARD)  type = Gfx::CAM_TYPE_BACK;
                else if (type == Gfx::CAM_TYPE_BACK   )  type = Gfx::CAM_TYPE_ONBOARD;
            }

            obj->SetCameraType(type);
            m_camera->SetType(type);
        }
    }
}

//! Remote control the camera using the arrow keys
void CRobotMain::KeyCamera(EventType type, InputSlot key)
{
    if (type == EVENT_KEY_UP)
    {
        if (key == INPUT_SLOT_LEFT)
        {
            m_cameraPan = 0.0f;
        }

        if (key == INPUT_SLOT_RIGHT)
        {
            m_cameraPan = 0.0f;
        }

        if (key == INPUT_SLOT_UP)
        {
            m_cameraZoom = 0.0f;
        }

        if (key == INPUT_SLOT_DOWN)
        {
            m_cameraZoom = 0.0f;
        }
    }

    if (m_phase != PHASE_SIMUL) return;
    if (m_editLock) return;  // current edition?
    if (m_trainerPilot) return;

    CObject* obj = GetSelect();
    if (obj == nullptr) return;
    if (!obj->GetTrainer()) return;

    if (type == EVENT_KEY_DOWN)
    {
        if (key == INPUT_SLOT_LEFT)
        {
            m_cameraPan = -1.0f;
        }

        if (key == INPUT_SLOT_RIGHT)
        {
            m_cameraPan = 1.0f;
        }

        if (key == INPUT_SLOT_UP)
        {
            m_cameraZoom = -1.0f;
        }

        if (key == INPUT_SLOT_DOWN)
        {
            m_cameraZoom = 1.0f;
        }
    }
}

//! Panned with the camera if a button is pressed
void CRobotMain::RemoteCamera(float pan, float zoom, float rTime)
{
    if (pan != 0.0f)
    {
        float value = m_camera->GetRemotePan();
        value += pan*rTime*1.5f;
        m_camera->SetRemotePan(value);
    }

    if (zoom != 0.0f)
    {
        float value = m_camera->GetRemoteZoom();
        value += zoom*rTime*0.3f;
        m_camera->SetRemoteZoom(value);
    }
}



//! Cancels the current movie
void CRobotMain::AbortMovie()
{
    for (CObject* obj : m_objMan->GetAllObjects())
    {
        if (obj->Implements(ObjectInterfaceType::Old))
        {
            CAuto* automat = obj->GetAuto();
            if (automat != 0)
                automat->Abort();
        }
    }
}



//! Updates the text information
void CRobotMain::UpdateInfoText()
{
    if (m_phase == PHASE_SIMUL)
    {
        CObject* obj = GetSelect();
        if (obj != nullptr)
        {
            Math::Vector pos = obj->GetPosition();
            m_engine->SetStatisticPos(pos);
        }
    }
    m_engine->SetTimerDisplay(m_missionTimerEnabled && m_missionTimerStarted ? TimeFormat(m_missionTimer) : "");
}


//! Initializes the view
void CRobotMain::InitEye()
{
    if (m_phase == PHASE_SIMUL)
        m_camera->Init(Math::Vector( 0.0f, 10.0f, 0.0f),
                       Math::Vector(10.0f,  5.0f, 0.0f), 0.0f);
}

//! Advances the entire scene
bool CRobotMain::EventFrame(const Event &event)
{
    // TODO: For some reason we're getting one big event with event.rTime > 0.1f after loading before the movie starts?
    if (!m_immediatSatCom && !m_beginSatCom && !m_movieLock &&
         m_gameTime > 0.1f && m_phase == PHASE_SIMUL)
    {
        m_displayText->DisplayError(INFO_BEGINSATCOM, Math::Vector(0.0f,0.0f,0.0f));
        m_beginSatCom = true;  // message appears
    }

    m_time += event.rTime;
    if (!m_movieLock && m_pause->GetPause() == PAUSE_NONE)
    {
        m_gameTime += event.rTime;
        m_gameTimeAbsolute += m_app->GetRealRelTime() / 1e9f;
    }

    if (!m_movieLock && m_pause->GetPause() == PAUSE_NONE && m_missionTimerStarted)
        m_missionTimer += event.rTime;

    if (m_pause->GetPause() == PAUSE_NONE && m_autosave && m_gameTimeAbsolute >= m_autosaveLast+(m_autosaveInterval*60) && m_phase == PHASE_SIMUL)
    {
        if (m_levelCategory == LevelCategory::Missions    ||
            m_levelCategory == LevelCategory::FreeGame    ||
            m_levelCategory == LevelCategory::CustomLevels )
        {
            m_autosaveLast = m_gameTimeAbsolute;
            Autosave();
        }
    }

    m_water->EventProcess(event);
    m_cloud->EventProcess(event);
    m_lightning->EventProcess(event);
    m_planet->EventProcess(event);

    Ui::CMap* pm = nullptr;
    Ui::CWindow* pw = static_cast<Ui::CWindow*>(m_interface->SearchControl(EVENT_WINDOW1));
    if (pw == nullptr)
    {
        pm = nullptr;
    }
    else
    {
        pm = static_cast<Ui::CMap*>(pw->SearchControl(EVENT_OBJECT_MAP));
        if (pm != nullptr) pm->FlushObject();
    }

    CObject* toto = nullptr;
    if (!m_freePhoto)
    {
        // Advances all the robots, but not toto.
        for (CObject* obj : m_objMan->GetAllObjects())
        {
            if (pm != nullptr)
                pm->UpdateObject(obj);

            if (IsObjectBeingTransported(obj))
                continue;

            if (obj->GetType() == OBJECT_TOTO)
                toto = obj;
            else if (obj->Implements(ObjectInterfaceType::Interactive))
                dynamic_cast<CInteractiveObject*>(obj)->EventProcess(event);
        }
        // Advances all objects transported by robots.
        for (CObject* obj : m_objMan->GetAllObjects())
        {
            if (! IsObjectBeingTransported(obj))
                continue;

            if (obj->Implements(ObjectInterfaceType::Interactive))
                dynamic_cast<CInteractiveObject*>(obj)->EventProcess(event);
        }

        m_engine->GetPyroManager()->EventProcess(event);
    }

    // The camera follows the object, because its position
    // may depend on the selected object (Gfx::CAM_TYPE_ONBOARD or Gfx::CAM_TYPE_BACK).
    if (m_phase == PHASE_SIMUL && !m_editFull)
    {
        m_camera->EventProcess(event);

        if (m_engine->GetFog())
            m_camera->SetOverBaseColor(m_particle->GetFogColor(m_engine->GetEyePt()));
    }
    if (m_phase == PHASE_APPERANCE ||
        m_phase == PHASE_WIN   ||
        m_phase == PHASE_LOST)
    {
        m_camera->EventProcess(event);
    }

    // Advances toto following the camera, because its position depends on the camera.
    if (toto != nullptr)
        dynamic_cast<CInteractiveObject*>(toto)->EventProcess(event);

    HiliteFrame(event.rTime);

    // Moves the film indicator.
    if (m_movieLock && !m_editLock)  // movie in progress?
    {
        Ui::CControl* pc = m_interface->SearchControl(EVENT_OBJECT_MOVIELOCK);
        if (pc != nullptr)
        {
            Math::Point pos, dim;

            dim.x = 32.0f/640.0f;
            dim.y = 32.0f/480.0f;
            pos.x = 20.0f/640.0f;
            pos.y = (480.0f-24.0f)/480.0f;

            float zoom = 1.0f+sinf(m_time*6.0f)*0.1f;  // 0.9 .. 1.1
            dim.x *= zoom;
            dim.y *= zoom;
            pos.x -= dim.x/2.0f;
            pos.y -= dim.y/2.0f;

            pc->SetPos(pos);
            pc->SetDim(dim);
        }
    }

    // Moves edition indicator.
    if (m_editLock || m_pause->GetPause())  // edition in progress?
    {
        Ui::CControl* pc = m_interface->SearchControl(EVENT_OBJECT_EDITLOCK);
        if (pc != nullptr)
        {
            Math::Point pos, dim;

            if (m_editFull || m_editLock)
            {
                dim.x = 10.0f/640.0f;
                dim.y = 10.0f/480.0f;
                pos.x = -20.0f/640.0f;
                pos.y = -20.0f/480.0f;  // invisible!
            }
            else
            {
                dim.x = 32.0f/640.0f;
                dim.y = 32.0f/480.0f;
                pos.x = (640.0f-24.0f)/640.0f;
                pos.y = (480.0f-24.0f)/480.0f;

                float zoom = 1.0f+sinf(m_time*6.0f)*0.1f;  // 0.9 .. 1.1
                dim.x *= zoom;
                dim.y *= zoom;
                pos.x -= dim.x/2.0f;
                pos.y -= dim.y/2.0f;
            }
            pc->SetPos(pos);
            pc->SetDim(dim);
        }
    }

    Ui::CControl* pc = m_interface->SearchControl(EVENT_OBJECT_SAVING);
    if (pc != nullptr)
    {
        Math::Point pos, dim;

        if (m_shotSaving <= 0)
        {
            dim.x = 10.0f/640.0f;
            dim.y = 10.0f/480.0f;
            pos.x = -20.0f/640.0f;
            pos.y = -20.0f/480.0f;  // invisible!
        }
        else
        {
            dim.x = 32.0f/640.0f;
            dim.y = 32.0f/480.0f;
            pos.x = (640.0f-24.0f)/640.0f;
            pos.y = (480.0f-24.0f)/480.0f;

            float zoom = 1.0f+sinf(m_time*6.0f)*0.1f;  // 0.9 .. 1.1
            dim.x *= zoom;
            dim.y *= zoom;
            pos.x -= dim.x/2.0f;
            pos.y -= dim.y/2.0f;
        }
        pc->SetPos(pos);
        pc->SetDim(dim);
    }

    // Will move the arrow to visit.
    if (m_camera->GetType() == Gfx::CAM_TYPE_VISIT)
        FrameVisit(event.rTime);

    // Moves the boundaries.
    FrameShowLimit(event.rTime);

    if (m_phase == PHASE_SIMUL)
    {
        if (!m_editLock)
        {
            CheckEndMission(true);
            UpdateAudio(true);
        }

        if (m_winDelay > 0.0f && !m_editLock)
        {
            m_winDelay -= event.rTime;
            if (m_winDelay <= 0.0f)
            {
                if (m_movieLock)
                    m_winDelay = 1.0f;
                else
                    m_eventQueue->AddEvent(Event(EVENT_WIN));
            }
        }

        if (m_lostDelay > 0.0f && !m_editLock)
        {
            m_lostDelay -= event.rTime;
            if (m_lostDelay <= 0.0f)
            {
                if (m_movieLock)
                    m_winDelay = 1.0f;
                else
                    m_eventQueue->AddEvent(Event(EVENT_LOST));
            }
        }
    }

    if (GetMissionType() == MISSION_CODE_BATTLE)
    {
        if (!m_codeBattleInit)
        {
            // NOTE: It's important to do this AFTER the first update event finished processing
            //       because otherwise all robot parts are misplaced
            ChangePause(PAUSE_CODE_BATTLE_LOCK);
            m_sound->MuteAll(false); // Allow sound
            m_codeBattleInit = true; // Will start on resume
        }

        if (!m_codeBattleStarted && m_pause->GetPause() == PAUSE_NONE)
        {
            m_codeBattleStarted = true;
            m_eventQueue->AddEvent(Event(EVENT_UPDINTERFACE));
        }
    }

    return true;
}

void CRobotMain::ShowSaveIndicator(bool show)
{
    Ui::CControl* pc = m_interface->SearchControl(EVENT_OBJECT_SAVING);
    if (pc != nullptr)
    {
        Math::Point pos, dim;

        if (!show)
        {
            dim.x = 10.0f/640.0f;
            dim.y = 10.0f/480.0f;
            pos.x = -20.0f/640.0f;
            pos.y = -20.0f/480.0f;  // invisible!
        }
        else
        {
            dim.x = 32.0f/640.0f;
            dim.y = 32.0f/480.0f;
            pos.x = (640.0f-24.0f)/640.0f;
            pos.y = (480.0f-24.0f)/480.0f;

            pos.x -= dim.x/2.0f;
            pos.y -= dim.y/2.0f;
        }
        pc->SetPos(pos);
        pc->SetDim(dim);
    }
}

//! Makes the event for all robots
bool CRobotMain::EventObject(const Event &event)
{
    if (m_freePhoto) return true;

    m_resetCreate = false;

    for (CObject* obj : m_objMan->GetAllObjects())
    {
        if (obj->Implements(ObjectInterfaceType::Interactive))
        {
            dynamic_cast<CInteractiveObject*>(obj)->EventProcess(event);
        }
    }

    if (m_resetCreate)
        ResetCreate();

    return true;
}


//! Calculates the point of arrival of the camera
Math::Vector CRobotMain::LookatPoint(Math::Vector eye, float angleH, float angleV,
                                  float length)
{
    Math::Vector lookat = eye;
    lookat.z += length;

    RotatePoint(eye, angleH, angleV, lookat);
    return lookat;
}

//! Load the scene for the character
void CRobotMain::ScenePerso()
{
    DeleteAllObjects();  // removes all the current 3D Scene
    m_terrain->FlushRelief();
    m_engine->DeleteAllObjects();
    m_oldModelManager->DeleteAllModelCopies();
    m_terrain->FlushBuildingLevel();
    m_terrain->FlushFlyingLimit();
    m_lightMan->FlushLights();
    m_particle->FlushParticle();

    SetLevel(LevelCategory::Perso, 0, 0);
    try
    {
        CreateScene(false, true, false);  // sets scene
    }
    catch (const std::runtime_error& e)
    {
        LevelLoadingError("An error occured while trying to load apperance scene", e, PHASE_PLAYER_SELECT);
    }

    m_engine->SetDrawWorld(false);  // does not draw anything on the interface
    m_engine->SetDrawFront(true);  // draws on the human interface
    CObject* obj = SearchHuman();
    if (obj != nullptr)
    {
        obj->SetDrawFront(true);  // draws the interface

        CMotionHuman* mh = static_cast<CMotionHuman*>(obj->GetMotion());
        if (mh != nullptr)
            mh->StartDisplayPerso();
    }
}

//! Creates the whole scene
void CRobotMain::CreateScene(bool soluce, bool fixScene, bool resetObject)
{
    m_fixScene = fixScene;

    m_base = nullptr;

    if (!resetObject)
    {
        m_build = 0;
        m_researchDone.clear();  // no research done
        m_researchDone[0] = 0;
        m_researchEnable = 0;

        FlushDisplayInfo();
        m_terrain->FlushMaterials();
        m_audioTrack = "";
        m_audioRepeat = true;
        m_satcomTrack  = "";
        m_satcomRepeat = true;
        m_editorTrack  = "";
        m_editorRepeat = true;
        m_displayText->SetDelay(1.0f);
        m_displayText->SetEnable(true);
        m_immediatSatCom = false;
        m_lockedSatCom = false;
        m_endingWinRank   = 0;
        m_endingLostRank  = 0;
        m_audioChange.clear();
        m_endTake.clear();
        m_endTakeResearch = 0;
        m_endTakeWinDelay = 2.0f;
        m_endTakeLostDelay = 2.0f;
        m_globalMagnifyDamage = 1.0f;
        m_obligatoryTotal = 0;
        m_prohibitedTotal = 0;
        m_mapShow = true;
        m_mapImage = false;
        m_mapFilename[0] = 0;

        m_controller = nullptr;

        m_colorRefBot.r =  10.0f/256.0f;
        m_colorRefBot.g = 166.0f/256.0f;
        m_colorRefBot.b = 254.0f/256.0f;  // blue
        m_colorRefBot.a = 0.0f;
        m_colorNewBot.clear();
        m_colorNewBot[0] = m_colorRefBot;

        m_colorRefAlien.r = 135.0f/256.0f;
        m_colorRefAlien.g = 170.0f/256.0f;
        m_colorRefAlien.b =  13.0f/256.0f;  // green
        m_colorRefAlien.a = 0.0f;
        m_colorNewAlien = m_colorRefAlien;

        m_colorRefGreen.r = 135.0f/256.0f;
        m_colorRefGreen.g = 170.0f/256.0f;
        m_colorRefGreen.b =  13.0f/256.0f;  // green
        m_colorRefGreen.a = 0.0f;
        m_colorNewGreen = m_colorRefGreen;

        m_colorRefWater.r =  25.0f/256.0f;
        m_colorRefWater.g = 255.0f/256.0f;
        m_colorRefWater.b = 240.0f/256.0f;  // cyan
        m_colorRefWater.a = 0.0f;
        m_colorNewWater = m_colorRefWater;

        m_engine->SetAmbientColor(Gfx::Color(0.5f, 0.5f, 0.5f, 0.5f), 0);
        m_engine->SetAmbientColor(Gfx::Color(0.5f, 0.5f, 0.5f, 0.5f), 1);
        m_engine->SetFogColor(Gfx::Color(1.0f, 1.0f, 1.0f, 1.0f), 0);
        m_engine->SetFogColor(Gfx::Color(1.0f, 1.0f, 1.0f, 1.0f), 1);
        m_engine->SetDeepView(1000.0f, 0);
        m_engine->SetDeepView(1000.0f, 1);
        m_engine->SetFogStart(0.75f, 0);
        m_engine->SetFogStart(0.75f, 1);
        m_engine->SetSecondTexture("");
        m_engine->SetForegroundName("");

        sprintf(m_title,  "%s %d.%d", GetLevelCategoryDir(m_levelCategory).c_str(), m_levelChap, m_levelRank);
        sprintf(m_resume, "%s %d.%d", GetLevelCategoryDir(m_levelCategory).c_str(), m_levelChap, m_levelRank);
        std::string scriptNameStr;
        GetResource(RES_TEXT, RT_SCRIPT_NEW, scriptNameStr);
        strcpy(m_scriptName, scriptNameStr.c_str());
        m_scriptFile[0] = 0;

        m_missionType   = MISSION_NORMAL;
        m_codeBattleInit = false;
        m_codeBattleStarted = false;

        m_teamNames.clear();

        m_missionResult = ERR_MISSION_NOTERM;
    }

    //NOTE: Reset timer always, even when only resetting object positions
    m_missionTimerEnabled = false;
    m_missionTimerStarted = false;
    m_missionTimer = 0.0f;

    std::string backgroundPath = "";
    Gfx::Color backgroundUp = Gfx::Color(0.0f, 0.0f, 0.0f, 0.0f);
    Gfx::Color backgroundDown = Gfx::Color(0.0f, 0.0f, 0.0f, 0.0f);
    Gfx::Color backgroundCloudUp = Gfx::Color(0.0f, 0.0f, 0.0f, 0.0f);
    Gfx::Color backgroundCloudDown = Gfx::Color(0.0f, 0.0f, 0.0f, 0.0f);
    bool backgroundFull = false;

    try
    {
        m_ui->GetLoadingScreen()->SetProgress(0.05f, RT_LOADING_PROCESSING);
        CLevelParser levelParser(m_levelCategory, m_levelChap, m_levelRank);
        levelParser.Load();
        int numObjects = levelParser.CountLines("CreateObject");
        m_ui->GetLoadingScreen()->SetProgress(0.1f, RT_LOADING_LEVEL_SETTINGS);

        int rankObj = 0;
        int rankGadget = 0;
        CObject* sel = 0;

        /*
        * NOTE: Moving frequently used lines to the top
        *       may speed up loading
        */

        for (auto& line : levelParser.GetLines())
        {
            if (line->GetCommand() == "Title" && !resetObject)
            {
                strcpy(m_title, line->GetParam("text")->AsString().c_str());
                continue;
            }

            if (line->GetCommand() == "Resume" && !resetObject)
            {
                strcpy(m_resume, line->GetParam("text")->AsString().c_str());
                continue;
            }

            if (line->GetCommand() == "ScriptName" && !resetObject)
            {
                strcpy(m_scriptName, line->GetParam("text")->AsString().c_str());
                continue;
            }

            if (line->GetCommand() == "ScriptFile" && !resetObject)
            {
                strcpy(m_scriptFile, line->GetParam("name")->AsString().c_str());
                continue;
            }

            if (line->GetCommand() == "Instructions" && !resetObject)
            {
                strcpy(m_infoFilename[SATCOM_HUSTON], line->GetParam("name")->AsPath("help/%lng%").c_str());

                m_immediatSatCom = line->GetParam("immediat")->AsBool(false);
                m_beginSatCom = m_lockedSatCom = line->GetParam("lock")->AsBool(false);
                if (m_app->GetSceneTestMode()) m_immediatSatCom = false;
                continue;
            }

            if (line->GetCommand() == "Satellite" && !resetObject)
            {
                strcpy(m_infoFilename[SATCOM_SAT], line->GetParam("name")->AsPath("help/%lng%").c_str());
                continue;
            }

            if (line->GetCommand() == "Loading" && !resetObject)
            {
                strcpy(m_infoFilename[SATCOM_LOADING], line->GetParam("name")->AsPath("help/%lng%").c_str());
                continue;
            }

            if (line->GetCommand() == "HelpFile" && !resetObject)
            {
                strcpy(m_infoFilename[SATCOM_PROG], line->GetParam("name")->AsPath("help/%lng%").c_str());
                continue;
            }
            if (line->GetCommand() == "SoluceFile" && !resetObject)
            {
                strcpy(m_infoFilename[SATCOM_SOLUCE], line->GetParam("name")->AsPath("help/%lng%").c_str());
                continue;
            }

            if (line->GetCommand() == "EndingFile" && !resetObject)
            {
                // NOTE: The old default was 0, but I think -1 is more correct - 0 means "ending file 000", while -1 means "no ending file"
                m_endingWinRank  = line->GetParam("win")->AsInt(-1);
                m_endingLostRank = line->GetParam("lost")->AsInt(-1);
                continue;
            }

            if (line->GetCommand() == "MessageDelay" && !resetObject)
            {
                m_displayText->SetDelay(line->GetParam("factor")->AsFloat());
                continue;
            }

            if (line->GetCommand() == "MissionTimer")
            {
                m_missionTimerEnabled = line->GetParam("enabled")->AsBool();
                if (!line->GetParam("program")->AsBool(false))
                {
                    m_missionTimerStarted = true;
                }
                continue;
            }

            if (line->GetCommand() == "TeamName")
            {
                int team = line->GetParam("team")->AsInt();
                std::string name = line->GetParam("name")->AsString();
                m_teamNames[team] = name;
                continue;
            }

            if (line->GetCommand() == "CacheAudio" && !resetObject)
            {
                std::string filename = line->GetParam("filename")->AsPath("music");
                m_ui->GetLoadingScreen()->SetProgress(0.15f, RT_LOADING_MUSIC, filename);
                m_sound->CacheMusic(std::string("../")+filename);
                continue;
            }

            if (line->GetCommand() == "AudioChange" && !resetObject && m_controller == nullptr)
            {
                auto audioChange = MakeUnique<CAudioChangeCondition>();
                audioChange->Read(line.get());
                m_ui->GetLoadingScreen()->SetProgress(0.15f, RT_LOADING_MUSIC, CResourceManager::CleanPath("music/"+audioChange->music));
                m_sound->CacheMusic(audioChange->music);
                m_audioChange.push_back(std::move(audioChange));
                continue;
            }

            if (line->GetCommand() == "Audio" && !resetObject && m_controller == nullptr)
            {
                if (line->GetParam("track")->IsDefined())
                {
                    if (line->GetParam("filename")->IsDefined())
                        throw CLevelParserException("You can't use track and filename at the same time");

                    GetLogger()->Warn("Using track= is deprecated. Please replace this with filename=\n");
                    int trackid = line->GetParam("track")->AsInt();
                    if (trackid != 0)
                    {
                        std::stringstream filenameStr;
                        filenameStr << "music" << std::setfill('0') << std::setw(3) << trackid << ".ogg";
                        m_audioTrack = filenameStr.str();
                    }
                    else
                    {
                        m_audioTrack = "";
                    }
                }
                else
                {
                    if (line->GetParam("filename")->IsDefined())
                    {
                        m_audioTrack = "../"+line->GetParam("filename")->AsPath("music");
                    }
                    else
                    {
                        m_audioTrack = "";
                    }
                }
                if (!m_audioTrack.empty())
                {
                    m_audioRepeat = line->GetParam("repeat")->AsBool(true);
                }

                if (line->GetParam("satcom")->IsDefined())
                {
                    m_satcomTrack = "../"+line->GetParam("satcom")->AsPath("music");
                    m_satcomRepeat = line->GetParam("satcomRepeat")->AsBool(true);
                }
                else
                {
                    m_satcomTrack = "";
                }

                if (line->GetParam("editor")->IsDefined())
                {
                    m_editorTrack = "../"+line->GetParam("editor")->AsPath("music");
                    m_editorRepeat = line->GetParam("editorRepeat")->AsBool(true);
                }
                else
                {
                    m_editorTrack = "";
                }

                if (!m_audioTrack.empty())
                {
                    m_ui->GetLoadingScreen()->SetProgress(0.15f, RT_LOADING_MUSIC, CResourceManager::CleanPath("music/"+m_audioTrack));
                    m_sound->CacheMusic(m_audioTrack);
                }
                if (!m_satcomTrack.empty())
                {
                    m_sound->CacheMusic(m_satcomTrack);
                    m_ui->GetLoadingScreen()->SetProgress(0.15f, RT_LOADING_MUSIC, CResourceManager::CleanPath("music/"+m_satcomTrack));
                }
                if (!m_editorTrack.empty())
                {
                    m_sound->CacheMusic(m_editorTrack);
                    m_ui->GetLoadingScreen()->SetProgress(0.15f, RT_LOADING_MUSIC, CResourceManager::CleanPath("music/"+m_editorTrack));
                }
                continue;
            }

            if (line->GetCommand() == "AmbientColor" && !resetObject)
            {
                m_engine->SetAmbientColor(line->GetParam("air")->AsColor(Gfx::Color(0.533f, 0.533f, 0.533f, 0.533f)), 0);
                m_engine->SetAmbientColor(line->GetParam("water")->AsColor(Gfx::Color(0.533f, 0.533f, 0.533f, 0.533f)), 1);
                continue;
            }

            if (line->GetCommand() == "FogColor" && !resetObject)
            {
                m_engine->SetFogColor(line->GetParam("air")->AsColor(Gfx::Color(0.533f, 0.533f, 0.533f, 0.533f)), 0);
                m_engine->SetFogColor(line->GetParam("water")->AsColor(Gfx::Color(0.533f, 0.533f, 0.533f, 0.533f)), 1);
                continue;
            }

            if (line->GetCommand() == "VehicleColor" && !resetObject)
            {
                m_colorNewBot[line->GetParam("team")->AsInt(0)] = line->GetParam("color")->AsColor(Gfx::Color(0.533f, 0.533f, 0.533f, 0.533f));
                continue;
            }

            if (line->GetCommand() == "InsectColor" && !resetObject)
            {
                m_colorNewAlien = line->GetParam("color")->AsColor(Gfx::Color(0.533f, 0.533f, 0.533f, 0.533f));
                continue;
            }

            if (line->GetCommand() == "GreeneryColor" && !resetObject)
            {
                m_colorNewGreen = line->GetParam("color")->AsColor(Gfx::Color(0.533f, 0.533f, 0.533f, 0.533f));
                continue;
            }

            if (line->GetCommand() == "DeepView" && !resetObject)
            {
                m_engine->SetDeepView(line->GetParam("air")->AsFloat(500.0f)*g_unit, 0, true);
                m_engine->SetDeepView(line->GetParam("water")->AsFloat(100.0f)*g_unit, 1, true);
                continue;
            }

            if (line->GetCommand() == "FogStart" && !resetObject)
            {
                m_engine->SetFogStart(line->GetParam("air")->AsFloat(0.5f), 0);
                m_engine->SetFogStart(line->GetParam("water")->AsFloat(0.5f), 1);
                continue;
            }

            if (line->GetCommand() == "SecondTexture" && !resetObject)
            {
                if (line->GetParam("rank")->IsDefined())
                {
                    char tex[20] = { 0 };
                    sprintf(tex, "dirty%.2d.png", line->GetParam("rank")->AsInt());
                    m_engine->SetSecondTexture(tex);
                }
                else
                {
                    m_engine->SetSecondTexture("../" + line->GetParam("texture")->AsPath("textures"));
                }
                continue;
            }

            if (line->GetCommand() == "Background" && !resetObject)
            {
                if (line->GetParam("image")->IsDefined())
                    backgroundPath = line->GetParam("image")->AsPath("textures");
                backgroundUp = line->GetParam("up")->AsColor(backgroundUp);
                backgroundDown = line->GetParam("down")->AsColor(backgroundDown);
                backgroundCloudUp = line->GetParam("cloudUp")->AsColor(backgroundCloudUp);
                backgroundCloudDown = line->GetParam("cloudDown")->AsColor(backgroundCloudDown);
                backgroundFull = line->GetParam("full")->AsBool(backgroundFull);
                continue;
            }

            if (line->GetCommand() == "Planet" && !resetObject)
            {
                Math::Vector    ppos, uv1, uv2;

                ppos  = line->GetParam("pos")->AsPoint();
                uv1   = line->GetParam("uv1")->AsPoint();
                uv2   = line->GetParam("uv2")->AsPoint();
                m_planet->Create(line->GetParam("mode")->AsPlanetType(),
                                Math::Point(ppos.x, ppos.z),
                                line->GetParam("dim")->AsFloat(0.2f),
                                line->GetParam("speed")->AsFloat(0.0f),
                                line->GetParam("dir")->AsFloat(0.0f),
                                line->GetParam("image")->AsPath("textures"),
                                Math::Point(uv1.x, uv1.z),
                                Math::Point(uv2.x, uv2.z),
                                line->GetParam("image")->AsPath("textures").find("planet") != std::string::npos // TODO: add transparent op or modify textures
                );
                continue;
            }

            if (line->GetCommand() == "ForegroundName" && !resetObject)
            {
                m_engine->SetForegroundName(line->GetParam("image")->AsPath("textures"));
                continue;
            }

            if (line->GetCommand() == "Level" && !resetObject)
            {
                g_unit = line->GetParam("unitScale")->AsFloat(4.0f);
                m_engine->SetTracePrecision(line->GetParam("traceQuality")->AsFloat(1.0f));
                m_shortCut = line->GetParam("shortcut")->AsBool(true);

                m_missionType = line->GetParam("type")->AsMissionType(MISSION_NORMAL);
                m_globalMagnifyDamage = line->GetParam("magnifyDamage")->AsFloat(1.0f);

                continue;
            }

            if (line->GetCommand() == "TerrainGenerate" && !resetObject)
            {
                m_ui->GetLoadingScreen()->SetProgress(0.2f, RT_LOADING_TERRAIN);
                m_terrain->Generate(line->GetParam("mosaic")->AsInt(20),
                                    line->GetParam("brick")->AsInt(3),
                                    line->GetParam("size")->AsFloat(20.0f),
                                    line->GetParam("vision")->AsFloat(500.0f)*g_unit,
                                    line->GetParam("depth")->AsInt(2),
                                    line->GetParam("hard")->AsFloat(0.5f));
                continue;
            }

            if (line->GetCommand() == "TerrainWind" && !resetObject)
            {
                m_terrain->SetWind(line->GetParam("speed")->AsPoint());
                continue;
            }

            if (line->GetCommand() == "TerrainRelief" && !resetObject)
            {
                m_ui->GetLoadingScreen()->SetProgress(0.2f+(1.f/5.f)*0.05f, RT_LOADING_TERRAIN, RT_LOADING_TERRAIN_RELIEF);
                m_terrain->LoadRelief(
                    line->GetParam("image")->AsPath("textures"),
                    line->GetParam("factor")->AsFloat(1.0f),
                    line->GetParam("border")->AsBool(true));
                continue;
            }

            if (line->GetCommand() == "TerrainRandomRelief" && !resetObject)
            {
                m_ui->GetLoadingScreen()->SetProgress(0.2f+(1.f/5.f)*0.05f, RT_LOADING_TERRAIN, RT_LOADING_TERRAIN_RELIEF);
                m_terrain->RandomizeRelief();
                continue;
            }

            if (line->GetCommand() == "TerrainResource" && !resetObject)
            {
                m_ui->GetLoadingScreen()->SetProgress(0.2f+(2.f/5.f)*0.05f, RT_LOADING_TERRAIN, RT_LOADING_TERRAIN_RES);
                m_terrain->LoadResources(line->GetParam("image")->AsPath("textures"));
                continue;
            }

            if (line->GetCommand() == "TerrainWater" && !resetObject)
            {
                Math::Vector pos;
                pos.x = line->GetParam("moxeX")->AsFloat(0.0f);
                pos.y = line->GetParam("moxeY")->AsFloat(0.0f);
                pos.z = pos.x;
                m_water->Create(line->GetParam("air")->AsWaterType(Gfx::WATER_TT),
                                line->GetParam("water")->AsWaterType(Gfx::WATER_TT),
                                line->GetParam("image")->AsPath("textures"),
                                line->GetParam("diffuse")->AsColor(Gfx::Color(1.0f, 1.0f, 1.0f, 1.0f)),
                                line->GetParam("ambient")->AsColor(Gfx::Color(1.0f, 1.0f, 1.0f, 1.0f)),
                                line->GetParam("level")->AsFloat(100.0f)*g_unit,
                                line->GetParam("glint")->AsFloat(1.0f),
                                pos);
                m_colorNewWater = line->GetParam("color")->AsColor(m_colorRefWater);
                m_colorShiftWater = line->GetParam("brightness")->AsFloat(0.0f);
                continue;
            }

            if (line->GetCommand() == "TerrainLava" && !resetObject)
            {
                m_water->SetLava(line->GetParam("mode")->AsBool());
                continue;
            }

            if (line->GetCommand() == "TerrainCloud" && !resetObject)
            {
                std::string path = "";
                if (line->GetParam("image")->IsDefined())
                    path = line->GetParam("image")->AsPath("textures");
                m_cloud->Create(path,
                                line->GetParam("diffuse")->AsColor(Gfx::Color(1.0f, 1.0f, 1.0f, 1.0f)),
                                line->GetParam("ambient")->AsColor(Gfx::Color(1.0f, 1.0f, 1.0f, 1.0f)),
                                line->GetParam("level")->AsFloat(500.0f)*g_unit);
                continue;
            }

            if (line->GetCommand() == "TerrainBlitz" && !resetObject)
            {
                m_lightning->Create(line->GetParam("sleep")->AsFloat(0.0f),
                                    line->GetParam("delay")->AsFloat(3.0f),
                                    line->GetParam("magnetic")->AsFloat(50.0f)*g_unit);
                continue;
            }

            if (line->GetCommand() == "TerrainInitTextures" && !resetObject)
            {
                m_ui->GetLoadingScreen()->SetProgress(0.2f+(3.f/5.f)*0.05f, RT_LOADING_TERRAIN, RT_LOADING_TERRAIN_TEX);
                std::string name = "../" + line->GetParam("image")->AsPath("textures");
                if (name.find(".") == std::string::npos)
                    name += ".png";
                unsigned int dx = line->GetParam("dx")->AsInt(1);
                unsigned int dy = line->GetParam("dy")->AsInt(1);

                int tt[100]; //TODO: I have no idea how TerrainInitTextures works, but maybe we shuld remove the limit to 100?
                if (dx*dy > 100)
                    throw CLevelParserException("In TerrainInitTextures: dx*dy must be <100");
                if (line->GetParam("table")->IsDefined())
                {
                    auto& table = line->GetParam("table")->AsArray();

                    if (table.size() > dx*dy)
                        throw CLevelParserException("In TerrainInitTextures: table size must be dx*dy");

                    for (unsigned int i = 0; i < dx*dy; i++)
                    {
                        if (i >= table.size())
                        {
                            tt[i] = 0;
                        }
                        else
                        {
                            tt[i] = table[i]->AsInt();
                        }
                    }
                }
                else
                {
                    for (unsigned int i = 0; i < dx*dy; i++)
                    {
                        tt[i] = 0;
                    }
                }

                m_terrain->InitTextures(name.c_str(), tt, dx, dy);
                continue;
            }

            if (line->GetCommand() == "TerrainInit" && !resetObject)
            {
                m_terrain->InitMaterials(line->GetParam("id")->AsInt(1));
                continue;
            }

            if (line->GetCommand() == "TerrainMaterial" && !resetObject)
            {
                std::string name = line->GetParam("image")->AsPath("textures");
                if (name.find(".") == std::string::npos)
                    name += ".png";
                name = "../" + name;

                m_terrain->AddMaterial(line->GetParam("id")->AsInt(0),
                                    name.c_str(),
                                    Math::Point(line->GetParam("u")->AsFloat(),
                                                line->GetParam("v")->AsFloat()),
                                    line->GetParam("up")->AsInt(),
                                    line->GetParam("right")->AsInt(),
                                    line->GetParam("down")->AsInt(),
                                    line->GetParam("left")->AsInt(),
                                    line->GetParam("hard")->AsFloat(0.5f));
                continue;
            }

            if (line->GetCommand() == "TerrainLevel" && !resetObject)
            {
                m_ui->GetLoadingScreen()->SetProgress(0.2f+(3.f/5.f)*0.05f, RT_LOADING_TERRAIN, RT_LOADING_TERRAIN_TEX);
                int id[50]; //TODO: I have no idea how TerrainLevel works, but maybe we should remove the limit to 50?
                if (line->GetParam("id")->IsDefined())
                {
                    auto& idArray = line->GetParam("id")->AsArray();

                    if (idArray.size() > 50)
                        throw CLevelParserException("In TerrainLevel: id array size must be < 50");

                    unsigned int i = 0;
                    while (i < 50)
                    {
                        id[i] = idArray[i]->AsInt();
                        i++;
                        if (i >= idArray.size()) break;
                    }
                    id[i] = 0;
                }

                m_terrain->GenerateMaterials(id,
                                            line->GetParam("min")->AsFloat(0.0f)*g_unit,
                                            line->GetParam("max")->AsFloat(100.0f)*g_unit,
                                            line->GetParam("slope")->AsFloat(5.0f),
                                            line->GetParam("freq")->AsFloat(100.0f),
                                            line->GetParam("center")->AsPoint(Math::Vector(0.0f, 0.0f, 0.0f))*g_unit,
                                            line->GetParam("radius")->AsFloat(0.0f)*g_unit);
                continue;
            }

            if (line->GetCommand() == "TerrainCreate" && !resetObject)
            {
                m_ui->GetLoadingScreen()->SetProgress(0.2f+(4.f/5.f)*0.05f, RT_LOADING_TERRAIN, RT_LOADING_TERRAIN_GEN);
                m_terrain->CreateObjects();
                continue;
            }

            if (line->GetCommand() == "BeginObject")
            {
                InitEye();
                SetMovieLock(false);

                if (!m_sceneReadPath.empty())  // loading file ?
                {
                    m_ui->GetLoadingScreen()->SetProgress(0.25f, RT_LOADING_OBJECTS_SAVED);
                    sel = IOReadScene(m_sceneReadPath + "/data.sav", m_sceneReadPath + "/cbot.run");
                }
                else
                {
                    m_ui->GetLoadingScreen()->SetProgress(0.25f, RT_LOADING_OBJECTS);
                }

                continue;
            }

            if (line->GetCommand() == "LevelController" && m_sceneReadPath.empty())
            {
                m_controller = m_objMan->CreateObject(Math::Vector(0.0f, 0.0f, 0.0f), 0.0f, OBJECT_CONTROLLER, 100.0f);
                m_controller->SetMagnifyDamage(100.0f);
                m_controller->SetIgnoreBuildCheck(true);
                if (m_controller->Implements(ObjectInterfaceType::Programmable))
                {
                    CBrain* brain = dynamic_cast<CProgrammableObject*>(m_controller)->GetBrain();
                    if (line->GetParam("script")->IsDefined())
                    {
                        Program* program = brain->AddProgram();
                        program->filename = "../" + line->GetParam("script")->AsPath("ai");
                        program->readOnly = true;
                        brain->SetScriptRun(program);
                    }
                }
                continue;
            }

            if (line->GetCommand() == "CreateObject" && m_sceneReadPath.empty())
            {
                ObjectType type = line->GetParam("type")->AsObjectType();

                int gadget = line->GetParam("gadget")->AsInt(-1);
                if ( gadget == -1 )
                {
                    gadget = 0;
                    if ( type == OBJECT_TECH ||
                        (type >= OBJECT_PLANT0  &&
                        type <= OBJECT_PLANT19 ) ||
                        (type >= OBJECT_TREE0   &&
                        type <= OBJECT_TREE5   ) ||
                        (type >= OBJECT_QUARTZ0 &&
                        type <= OBJECT_QUARTZ3 ) ||
                        (type >= OBJECT_ROOT0   &&
                        type <= OBJECT_ROOT4   ) )  // not ROOT5!
                    {
                        gadget = 1;
                    }
                }
                if (gadget != 0)  // is this a gadget?
                {
                    if (!TestGadgetQuantity(rankGadget++)) continue;
                }


                float objectProgress = static_cast<float>(rankObj) / static_cast<float>(numObjects);
                std::string details = StrUtils::ToString<int>(rankObj+1)+" / "+StrUtils::ToString<int>(numObjects);
                #if DEV_BUILD
                // Object categories may spoil the level a bit, so hide them in release builds
                details += ": "+CLevelParserParam::FromObjectType(type);
                #endif
                m_ui->GetLoadingScreen()->SetProgress(0.25f+objectProgress*0.5f, RT_LOADING_OBJECTS, details);

                Math::Vector pos = line->GetParam("pos")->AsPoint()*g_unit;
                float dirAngle = line->GetParam("dir")->AsFloat(0.0f)*Math::PI;
                bool trainer;
                CObject* obj = nullptr;
                try
                {
                    obj = m_objMan->CreateObject(
                        pos, dirAngle,
                        type,
                        line->GetParam("power")->AsFloat(1.0f),
                        line->GetParam("z")->AsFloat(1.0f),
                        line->GetParam("h")->AsFloat(0.0f),
                        trainer = line->GetParam("trainer")->AsBool(false),
                        line->GetParam("toy")->AsBool(false),
                        line->GetParam("option")->AsInt(0),
                        line->GetParam("team")->AsInt(0)
                    );
                }
                catch (const CObjectCreateException& e)
                {
                    if (gadget)
                    {
                        GetLogger()->Warn("Error loading decorative object: %s\n", e.what());
                        continue;
                    }
                    else
                    {
                        GetLogger()->Error("Error loading level object: %s\n", e.what());
                        throw;
                    }
                }

                if (m_fixScene && type == OBJECT_HUMAN)
                {
                    CMotion* motion = obj->GetMotion();
                    if (m_phase == PHASE_WIN ) motion->SetAction(MHS_WIN,  0.4f);
                    if (m_phase == PHASE_LOST) motion->SetAction(MHS_LOST, 0.5f);
                }

                if (obj->Implements(ObjectInterfaceType::Old)) // TODO: temporary hack
                {
                    COldObject* oldObj = dynamic_cast<COldObject*>(obj);

                    oldObj->SetDefRank(rankObj); // TODO: do we really need this?

                    if (type == OBJECT_BASE)
                        m_base = oldObj;

                    if (line->GetParam("select")->AsBool(false))
                        sel = oldObj;

                    // TODO: everything below should go to CObject::Read() function
                    // In fact, we could give CLevelParserLine as parameter to CreateObject() in the first place

                    Gfx::CameraType cType = line->GetParam("camera")->AsCameraType(Gfx::CAM_TYPE_NULL);
                    if (cType != Gfx::CAM_TYPE_NULL)
                        oldObj->SetCameraType(cType);

                    oldObj->SetCameraDist(line->GetParam("cameraDist")->AsFloat(50.0f));
                    oldObj->SetCameraLock(line->GetParam("cameraLock")->AsBool(false));

                    Gfx::PyroType pType = line->GetParam("pyro")->AsPyroType(Gfx::PT_NULL);
                    if (pType != Gfx::PT_NULL)
                    {
                        m_engine->GetPyroManager()->Create(pType, oldObj);
                    }

                    if (type == OBJECT_INFO)
                    {
                        CExchangePost* exchangePost = static_cast<CExchangePost*>(oldObj);
                        exchangePost->ReadInfo(line.get());
                    }

                    // Sets the parameters of the command line.
                    if (line->GetParam("cmdline")->IsDefined())
                    {
                        const auto& cmdline = line->GetParam("cmdline")->AsArray();
                        for (unsigned int i = 0; i < cmdline.size(); i++)
                        {
                            oldObj->SetCmdLine(i, cmdline[i]->AsFloat());
                        }
                    }

                    bool selectable = line->GetParam("selectable")->AsBool(true);
                    oldObj->SetSelectable(selectable);
                    oldObj->SetIgnoreBuildCheck(line->GetParam("ignoreBuildCheck")->AsBool(false));
                    oldObj->SetEnable(line->GetParam("enable")->AsBool(true));
                    oldObj->SetProxyActivate(line->GetParam("proxyActivate")->AsBool(false));
                    oldObj->SetProxyDistance(line->GetParam("proxyDistance")->AsFloat(15.0f)*g_unit);
                    oldObj->SetRange(line->GetParam("range")->AsFloat(30.0f));
                    oldObj->SetShield(line->GetParam("shield")->AsFloat(1.0f));
                    oldObj->SetMagnifyDamage(line->GetParam("magnifyDamage")->AsFloat(1.0f));
                    oldObj->SetClip(line->GetParam("clip")->AsBool(true));
                    oldObj->SetCheckToken(!line->GetParam("checkToken")->IsDefined() ? trainer || !selectable : line->GetParam("checkToken")->AsBool(true));
                    // SetManual will affect bot speed
                    if (type == OBJECT_MOBILEdr)
                    {
                        oldObj->SetManual(!trainer);
                    }

                    Math::Vector zoom = line->GetParam("zoom")->AsPoint(Math::Vector(0.0f, 0.0f, 0.0f));
                    if (zoom.x != 0.0f || zoom.y != 0.0f || zoom.z != 0.0f)
                        oldObj->SetScale(zoom);

                    // only used in AlienWorm lines
                    if (type == OBJECT_WORM)
                    {
                        CMotion* motion = oldObj->GetMotion();
                        if (motion != nullptr && line->GetParam("param")->IsDefined())
                        {
                            const auto& p = line->GetParam("param")->AsArray();
                            for (unsigned int i = 0; i < 10 && i < p.size(); i++)
                            {
                                motion->SetParam(i, p[i]->AsFloat());
                            }
                        }
                    }

                    int run = -1;
                    std::map<int, Program*> loadedPrograms;
                    if (oldObj->Implements(ObjectInterfaceType::Programmable))
                    {
                        CBrain* brain = dynamic_cast<CProgrammableObject*>(oldObj)->GetBrain();

                        bool allFilled = true;
                        for (int i = 0; i < 10 || allFilled; i++)
                        {
                            std::string op = "script" + boost::lexical_cast<std::string>(i+1); // script1..script10
                            std::string opReadOnly = "scriptReadOnly" + boost::lexical_cast<std::string>(i+1); // scriptReadOnly1..scriptReadOnly10
                            std::string opRunnable = "scriptRunnable" + boost::lexical_cast<std::string>(i+1); // scriptRunnable1..scriptRunnable10
                            if (line->GetParam(op)->IsDefined())
                            {
                                Program* program = brain->AddProgram();
                                program->filename = "../" + line->GetParam(op)->AsPath("ai");
                                program->readOnly = line->GetParam(opReadOnly)->AsBool(true);
                                program->runnable = line->GetParam(opRunnable)->AsBool(true);
                                loadedPrograms[i] = program;
                            }
                            else
                            {
                                allFilled = false;
                            }
                        }

                        int i = line->GetParam("run")->AsInt(0);
                        if (i != 0)
                        {
                            run = i-1;
                            brain->SetScriptRun(loadedPrograms[run]);
                        }
                    }
                    CAuto* automat = oldObj->GetAuto();
                    if (automat != nullptr)
                    {
                        type = line->GetParam("autoType")->AsObjectType(OBJECT_NULL);
                        automat->SetType(type);
                        for (int i = 0; i < 5; i++)
                        {
                            std::string op = "autoValue" + boost::lexical_cast<std::string>(i+1); // autoValue1..autoValue5
                            automat->SetValue(i, line->GetParam(op)->AsFloat(0.0f));
                        }
                        automat->SetString(const_cast<char*>(line->GetParam("autoString")->AsPath("ai", "").c_str()));

                        int i = line->GetParam("run")->AsInt(-1);
                        if (i != -1)
                        {
                            if (i != PARAM_FIXSCENE &&
                                !m_settings->GetMovies()) i = 0;
                            automat->Start(i);  // starts the film
                        }
                    }

                    if (soluce && oldObj->Implements(ObjectInterfaceType::Programmable) && line->GetParam("soluce")->IsDefined())
                        dynamic_cast<CProgrammableObject*>(oldObj)->GetBrain()
                            ->SetSoluceName(const_cast<char*>(line->GetParam("soluce")->AsPath("ai").c_str()));

                    if (line->GetParam("reset")->AsBool(false))
                        oldObj->SetAnimateOnReset(true);
                }

                rankObj ++;
                continue;
            }

            if (line->GetCommand() == "CreateFog" && !resetObject)
            {
                Gfx::ParticleType type = static_cast<Gfx::ParticleType>(Gfx::PARTIFOG0+(line->GetParam("type")->AsInt()));
                Math::Vector pos = line->GetParam("pos")->AsPoint()*g_unit;
                float height = line->GetParam("height")->AsFloat(1.0f)*g_unit;
                float ddim = line->GetParam("dim")->AsFloat(50.0f)*g_unit;
                float delay = line->GetParam("delay")->AsFloat(2.0f);
                m_terrain->AdjustToFloor(pos);
                pos.y += height;
                Math::Point dim;
                dim.x = ddim;
                dim.y = dim.x;
                m_particle->CreateParticle(pos, Math::Vector(0.0f, 0.0f, 0.0f), dim, type, delay, 0.0f, 0.0f);
                continue;
            }

            if (line->GetCommand() == "CreateLight" && !resetObject)
            {
                Gfx::EngineObjectType  type;

                int lightRank = CreateLight(line->GetParam("dir")->AsPoint(),
                                            line->GetParam("color")->AsColor(Gfx::Color(0.5f, 0.5f, 0.5f, 1.0f)));

                type = line->GetParam("type")->AsTerrainType(Gfx::ENG_OBJTYPE_NULL);

                if (type == Gfx::ENG_OBJTYPE_TERRAIN)
                {
                    m_lightMan->SetLightPriority(lightRank, Gfx::LIGHT_PRI_HIGHEST);
                    m_lightMan->SetLightIncludeType(lightRank, Gfx::ENG_OBJTYPE_TERRAIN);
                }

                if (type == Gfx::ENG_OBJTYPE_QUARTZ)
                    m_lightMan->SetLightIncludeType(lightRank, Gfx::ENG_OBJTYPE_QUARTZ);

                if (type == Gfx::ENG_OBJTYPE_METAL)
                    m_lightMan->SetLightIncludeType(lightRank, Gfx::ENG_OBJTYPE_METAL);

                if (type == Gfx::ENG_OBJTYPE_FIX)
                    m_lightMan->SetLightExcludeType(lightRank, Gfx::ENG_OBJTYPE_TERRAIN);

                continue;
            }
            if (line->GetCommand() == "CreateSpot" && !resetObject)
            {
                Gfx::EngineObjectType  type;

                int rankLight = CreateSpot(line->GetParam("pos")->AsPoint()*g_unit,
                                        line->GetParam("color")->AsColor(Gfx::Color(0.5f, 0.5f, 0.5f, 1.0f)));

                type = line->GetParam("type")->AsTerrainType(Gfx::ENG_OBJTYPE_NULL);
                if (type == Gfx::ENG_OBJTYPE_TERRAIN)
                    m_lightMan->SetLightIncludeType(rankLight, Gfx::ENG_OBJTYPE_TERRAIN);

                if (type == Gfx::ENG_OBJTYPE_QUARTZ)
                    m_lightMan->SetLightIncludeType(rankLight, Gfx::ENG_OBJTYPE_QUARTZ);

                if (type == Gfx::ENG_OBJTYPE_METAL)
                    m_lightMan->SetLightIncludeType(rankLight, Gfx::ENG_OBJTYPE_METAL);

                if (type == Gfx::ENG_OBJTYPE_FIX)
                    m_lightMan->SetLightExcludeType(rankLight, Gfx::ENG_OBJTYPE_TERRAIN);

                continue;
            }

            if (line->GetCommand() == "GroundSpot" && !resetObject)
            {
                int rank = m_engine->CreateGroundSpot();
                if (rank != -1)
                {
                    m_engine->SetObjectGroundSpotPos(rank, line->GetParam("pos")->AsPoint(Math::Vector(0.0f, 0.0f, 0.0f))*g_unit);
                    m_engine->SetObjectGroundSpotRadius(rank, line->GetParam("radius")->AsFloat(10.0f)*g_unit);
                    m_engine->SetObjectGroundSpotColor(rank, line->GetParam("color")->AsColor(Gfx::Color(0.533f, 0.533f, 0.533f, 0.533f)));
                    m_engine->SetObjectGroundSpotSmooth(rank, line->GetParam("smooth")->AsFloat(1.0f));
                    m_engine->SetObjectGroundSpotMinMax(rank, line->GetParam("min")->AsFloat(0.0f)*g_unit,
                                                        line->GetParam("max")->AsFloat(0.0f)*g_unit);
                }
                continue;
            }

            if (line->GetCommand() == "WaterColor" && !resetObject)
            {
                m_engine->SetWaterAddColor(line->GetParam("color")->AsColor());
                continue;
            }

            if (line->GetCommand() == "MapColor" && !resetObject)
            {
                m_map->FloorColorMap(line->GetParam("floor")->AsColor(Gfx::Color(0.533f, 0.533f, 0.533f, 0.533f)),
                                    line->GetParam("water")->AsColor(Gfx::Color(0.533f, 0.533f, 0.533f, 0.533f)));
                m_mapShow = line->GetParam("show")->AsBool(true);
                m_map->SetToy(line->GetParam("toyIcon")->AsBool(false));
                m_mapImage = line->GetParam("image")->AsBool(false);
                if (m_mapImage)
                {
                    Math::Vector offset;
                    strcpy(m_mapFilename, line->GetParam("filename")->AsPath("textures").c_str());
                    offset = line->GetParam("offset")->AsPoint(Math::Vector(0.0f, 0.0f, 0.0f));
                    m_map->SetFixParam(line->GetParam("zoom")->AsFloat(1.0f),
                                    offset.x, offset.z,
                                    line->GetParam("angle")->AsFloat(0.0f)*Math::PI/180.0f,
                                    line->GetParam("mode")->AsInt(0),
                                    line->GetParam("debug")->AsBool(false));
                }
                continue;
            }

            if (line->GetCommand() == "MapZoom" && !resetObject)
            {
                m_map->ZoomMap(line->GetParam("factor")->AsFloat(2.0f));
                m_map->MapEnable(line->GetParam("enable")->AsBool(true));
                continue;
            }

            if (line->GetCommand() == "MaxFlyingHeight" && !resetObject)
            {
                m_terrain->SetFlyingMaxHeight(line->GetParam("max")->AsFloat(280.0f)*g_unit);
                continue;
            }

            if (line->GetCommand() == "AddFlyingHeight" && !resetObject)
            {
                m_terrain->AddFlyingLimit(line->GetParam("center")->AsPoint()*g_unit,
                                        line->GetParam("extRadius")->AsFloat(20.0f)*g_unit,
                                        line->GetParam("intRadius")->AsFloat(10.0f)*g_unit,
                                        line->GetParam("maxHeight")->AsFloat(200.0f));
                continue;
            }

            if (line->GetCommand() == "Camera")
            {
                m_camera->Init(line->GetParam("eye")->AsPoint(Math::Vector(0.0f, 0.0f, 0.0f))*g_unit,
                            line->GetParam("lookat")->AsPoint(Math::Vector(0.0f, 0.0f, 0.0f))*g_unit,
                            resetObject ? 0.0f : line->GetParam("delay")->AsFloat(0.0f));

                if (line->GetParam("fadeIn")->AsBool(false))
                    m_camera->StartOver(Gfx::CAM_OVER_EFFECT_FADEIN_WHITE, Math::Vector(0.0f, 0.0f, 0.0f), 1.0f);

                m_camera->SetFixDirection(line->GetParam("fixDirection")->AsFloat(0.25f)*Math::PI);
                continue;
            }

            if (line->GetCommand() == "EndMissionTake" && !resetObject && m_controller == nullptr)
            {
                auto endTake = MakeUnique<CSceneEndCondition>();
                endTake->Read(line.get());
                m_endTake.push_back(std::move(endTake));
                continue;
            }
            if (line->GetCommand() == "EndMissionDelay" && !resetObject && m_controller == nullptr)
            {
                m_endTakeWinDelay  = line->GetParam("win")->AsFloat(2.0f);
                m_endTakeLostDelay = line->GetParam("lost")->AsFloat(2.0f);
                continue;
            }
            if (line->GetCommand() == "EndMissionResearch" && !resetObject && m_controller == nullptr) //TODO: Is this used anywhere?
            {
                m_endTakeResearch |= line->GetParam("type")->AsResearchFlag();
                continue;
            }

            if (line->GetCommand() == "ObligatoryToken" && !resetObject) //NOTE: This was used only in CeeBot, maybe we should add this to some Colobot exercises?
            {
                int i = m_obligatoryTotal;
                if (i < 100) //TODO: remove the limit
                {
                    strcpy(m_obligatoryToken[i], line->GetParam("text")->AsString().c_str());
                    m_obligatoryTotal ++;
                }
                continue;
            }

            if (line->GetCommand() == "ProhibitedToken" && !resetObject) //NOTE: This was used only in CeeBot, maybe we should add this to some Colobot exercises?
            {
                int i = m_prohibitedTotal;
                if (i < 100) //TODO: remove the limit
                {
                    strcpy(m_prohibitedToken[i], line->GetParam("text")->AsString().c_str());
                    m_prohibitedTotal ++;
                }
                continue;
            }

            if (line->GetCommand() == "EnableBuild" && !resetObject)
            {
                m_build |= line->GetParam("type")->AsBuildFlag();
                continue;
            }

            if (line->GetCommand() == "EnableResearch" && !resetObject)
            {
                m_researchEnable |= line->GetParam("type")->AsResearchFlag();
                continue;
            }

            if (line->GetCommand() == "DoneResearch" && m_sceneReadPath.empty() && !resetObject) // not loading file?
            {
                m_researchDone[0] |= line->GetParam("type")->AsResearchFlag();
                continue;
            }

            if (line->GetCommand() == "NewScript" && !resetObject)
            {
                AddNewScriptName(line->GetParam("type")->AsObjectType(OBJECT_NULL), const_cast<char*>(line->GetParam("name")->AsPath("ai").c_str()));
                continue;
            }

            if (!m_sceneReadPath.empty()) continue; // ignore errors when loading saved game (TODO: don't report ones that are just not loaded when loading saved game)
            if (resetObject) continue; // ignore when reseting just objects (TODO: see above)

            throw CLevelParserException("Unknown command: '" + line->GetCommand() + "' in " + line->GetLevelFilename() + ":" + boost::lexical_cast<std::string>(line->GetLineNumber()));
        }

        if (m_sceneReadPath.empty())
            CompileScript(soluce);  // compiles all scripts

        m_ui->GetLoadingScreen()->SetProgress(1.0f, RT_LOADING_FINISHED);

        if (!resetObject)
        {
            m_engine->SetBackground(backgroundPath,
                                    backgroundUp,
                                    backgroundDown,
                                    backgroundCloudUp,
                                    backgroundCloudDown,
                                    backgroundFull);
        }

        if (m_levelCategory == LevelCategory::Missions && !resetObject)  // mission?
        {
            m_playerProfile->SetFreeGameResearchUnlock(m_playerProfile->GetFreeGameResearchUnlock() | m_researchDone[0]);
            m_playerProfile->SetFreeGameBuildUnlock(m_playerProfile->GetFreeGameBuildUnlock() | m_build);
        }

        if (m_levelCategory == LevelCategory::FreeGame && !resetObject)  // free play?
        {
            m_researchDone[0] = m_playerProfile->GetFreeGameResearchUnlock();

            m_build = m_playerProfile->GetFreeGameBuildUnlock();
            m_build &= ~BUILD_RESEARCH;
            m_build &= ~BUILD_LABO;
            m_build |= BUILD_FACTORY;
            m_build |= BUILD_GFLAT;
            m_build |= BUILD_FLAG;
        }

        if (!resetObject)
        {
            m_short->SetMode(false);  // vehicles?
        }

        m_map->ShowMap(m_mapShow);
        m_texturesNeedUpdate = true;
        // TODO: m_engine->TimeInit(); ??
        m_input->ResetKeyStates();
        m_time = 0.0f;
        m_gameTime = 0.0f;
        m_gameTimeAbsolute = 0.0f;
        m_autosaveLast = 0.0f;
        m_infoUsed = 0;

        m_selectObject = sel;

        if (m_base == nullptr &&  // no main base?
            !m_fixScene)    // interractive scene?
        {
            CObject* obj;
            if (sel == nullptr)
                obj = SearchHuman();
            else
                obj = sel;

            if (obj != nullptr)
            {
                SelectObject(obj);
                m_camera->SetControllingObject(obj);
                m_camera->SetType(obj->GetCameraType());
            }
        }

        if (m_fixScene)
            m_camera->SetType(Gfx::CAM_TYPE_SCRIPT);

        if (!m_sceneReadPath.empty() && sel != 0)  // loading file?
        {
            Math::Vector pos = sel->GetPosition();
            m_camera->Init(pos, pos, 0.0f);
            m_camera->FixCamera();

            SelectObject(sel);
            m_camera->SetControllingObject(sel);

            m_beginSatCom = true;  // message already displayed
        }
    }
    catch (...)
    {
        m_sceneReadPath = "";
        throw;
    }
    m_sceneReadPath = "";

    if (m_app->GetSceneTestMode())
        m_eventQueue->AddEvent(Event(EVENT_QUIT));

    m_ui->ShowLoadingScreen(false);
    CreateShortcuts();
}

void CRobotMain::LevelLoadingError(const std::string& error, const std::runtime_error& exception, Phase exitPhase)
{
    GetLogger()->Error("%s\n", error.c_str());
    GetLogger()->Error("%s\n", exception.what());
    ChangePhase(exitPhase);
    m_ui->GetDialog()->StartInformation("Loading error", error, exception.what(), true, false);
}

//! Creates a directional light
int CRobotMain::CreateLight(Math::Vector direction, Gfx::Color color)
{
    if (direction.x == 0.0f &&
        direction.y == 0.0f &&
        direction.z == 0.0f)
    {
        direction.y = -1.0f;
    }

    Gfx::Light light;
    light.type = Gfx::LIGHT_DIRECTIONAL;
    light.diffuse = color;
    light.ambient = color * 0.1f;
    light.direction  = direction;
    int obj = m_lightMan->CreateLight(Gfx::LIGHT_PRI_HIGH);
    m_lightMan->SetLight(obj, light);

    return obj;
}

//! Creates a light spot
int CRobotMain::CreateSpot(Math::Vector pos, Gfx::Color color)
{
    if (!m_engine->GetLightMode()) return -1;

    pos.y += m_terrain->GetFloorLevel(pos);

    Gfx::Light light;
    light.type          = Gfx::LIGHT_SPOT;
    light.diffuse       = color;
    light.ambient       = color * 0.1f;
    light.position      = pos;
    light.direction     = Math::Vector(0.0f, -1.0f, 0.0f);
    light.spotIntensity = 1.0f;
    light.spotAngle     = 90.0f*Math::PI/180.0f;
    light.attenuation0  = 2.0f;
    light.attenuation1  = 0.0f;
    light.attenuation2  = 0.0f;
    int obj = m_lightMan->CreateLight(Gfx::LIGHT_PRI_HIGH);
    m_lightMan->SetLight(obj, light);

    return obj;
}


//! Change the colors and textures
void CRobotMain::ChangeColor()
{
    if (m_phase != PHASE_SIMUL    &&
        m_phase != PHASE_SETUPds  &&
        m_phase != PHASE_SETUPgs  &&
        m_phase != PHASE_SETUPps  &&
        m_phase != PHASE_SETUPcs  &&
        m_phase != PHASE_SETUPss  &&
        m_phase != PHASE_WIN      &&
        m_phase != PHASE_LOST     &&
        m_phase != PHASE_APPERANCE ) return;

    // Player texture

    Math::Point ts = Math::Point(0.0f, 0.0f);
    Math::Point ti = Math::Point(1.0f, 1.0f);  // the entire image

    Gfx::Color colorRef1, colorNew1, colorRef2, colorNew2;

    colorRef1.a = 0.0f;
    colorRef2.a = 0.0f;

    colorRef1.r = 206.0f/256.0f;
    colorRef1.g = 206.0f/256.0f;
    colorRef1.b = 204.0f/256.0f;  // ~white
    colorNew1 = m_playerProfile->GetApperance().colorCombi;
    colorRef2.r = 255.0f/256.0f;
    colorRef2.g = 132.0f/256.0f;
    colorRef2.b =   1.0f/256.0f;  // orange
    colorNew2 = m_playerProfile->GetApperance().colorBand;

    Math::Point exclu[6];
    exclu[0] = Math::Point(192.0f/256.0f,   0.0f/256.0f);
    exclu[1] = Math::Point(256.0f/256.0f,  64.0f/256.0f);  // crystals + cylinders
    exclu[2] = Math::Point(208.0f/256.0f, 224.0f/256.0f);
    exclu[3] = Math::Point(256.0f/256.0f, 256.0f/256.0f);  // SatCom screen
    exclu[4] = Math::Point(0.0f, 0.0f);
    exclu[5] = Math::Point(0.0f, 0.0f);  // terminator
    m_engine->ChangeTextureColor("textures/objects/human.png", colorRef1, colorNew1, colorRef2, colorNew2, 0.30f, 0.01f, ts, ti, exclu);

    float tolerance;

    int face = GetGamerFace();
    if (face == 0)  // normal?
    {
        colorRef1.r =  90.0f/256.0f;
        colorRef1.g =  95.0f/256.0f;
        colorRef1.b =  85.0f/256.0f;  // black
        tolerance = 0.15f;
    }
    if (face == 1)  // bald?
    {
        colorRef1.r =  74.0f/256.0f;
        colorRef1.g =  58.0f/256.0f;
        colorRef1.b =  46.0f/256.0f;  // brown
        tolerance = 0.20f;
    }
    if (face == 2)  // carlos?
    {
        colorRef1.r =  70.0f/256.0f;
        colorRef1.g =  40.0f/256.0f;
        colorRef1.b =   8.0f/256.0f;  // brown
        tolerance = 0.30f;
    }
    if (face == 3)  // blonde?
    {
        colorRef1.r =  74.0f/256.0f;
        colorRef1.g =  16.0f/256.0f;
        colorRef1.b =   0.0f/256.0f;  // yellow
        tolerance = 0.20f;
    }
    colorNew1 = m_playerProfile->GetApperance().colorHair;
    colorRef2.r = 0.0f;
    colorRef2.g = 0.0f;
    colorRef2.b = 0.0f;
    colorNew2.r = 0.0f;
    colorNew2.g = 0.0f;
    colorNew2.b = 0.0f;

    char name[100];
    sprintf(name, "textures/objects/face%.2d.png", face+1);
    exclu[0] = Math::Point(105.0f/256.0f, 47.0f/166.0f);
    exclu[1] = Math::Point(153.0f/256.0f, 79.0f/166.0f);  // blue canister
    exclu[2] = Math::Point(0.0f, 0.0f);
    exclu[3] = Math::Point(0.0f, 0.0f);  // terminator
    m_engine->ChangeTextureColor(name, colorRef1, colorNew1, colorRef2, colorNew2, tolerance, 0.00f, ts, ti, exclu);

    colorRef2.r = 0.0f;
    colorRef2.g = 0.0f;
    colorRef2.b = 0.0f;
    colorNew2.r = 0.0f;
    colorNew2.g = 0.0f;
    colorNew2.b = 0.0f;

    // VehicleColor

    for(auto it : m_colorNewBot)
    {
        int team = it.first;
        Gfx::Color newColor = it.second;
        std::string teamStr = StrUtils::ToString<int>(team);
        if(team == 0) teamStr = "";

        m_engine->ChangeTextureColor("textures/objects/base1.png"+teamStr,   "textures/objects/base1.png",   m_colorRefBot, newColor, colorRef2, colorNew2, 0.10f, -1.0f, ts, ti, 0, 0, true);
        m_engine->ChangeTextureColor("textures/objects/convert.png"+teamStr, "textures/objects/convert.png", m_colorRefBot, newColor, colorRef2, colorNew2, 0.10f, -1.0f, ts, ti, 0, 0, true);
        m_engine->ChangeTextureColor("textures/objects/derrick.png"+teamStr, "textures/objects/derrick.png", m_colorRefBot, newColor, colorRef2, colorNew2, 0.10f, -1.0f, ts, ti, 0, 0, true);
        m_engine->ChangeTextureColor("textures/objects/factory.png"+teamStr, "textures/objects/factory.png", m_colorRefBot, newColor, colorRef2, colorNew2, 0.10f, -1.0f, ts, ti, 0, 0, true);
        m_engine->ChangeTextureColor("textures/objects/lemt.png"+teamStr,    "textures/objects/lemt.png",    m_colorRefBot, newColor, colorRef2, colorNew2, 0.10f, -1.0f, ts, ti, 0, 0, true);
        m_engine->ChangeTextureColor("textures/objects/roller.png"+teamStr,  "textures/objects/roller.png",  m_colorRefBot, newColor, colorRef2, colorNew2, 0.10f, -1.0f, ts, ti, 0, 0, true);
        m_engine->ChangeTextureColor("textures/objects/search.png"+teamStr,  "textures/objects/search.png",  m_colorRefBot, newColor, colorRef2, colorNew2, 0.10f, -1.0f, ts, ti, 0, 0, true);

        exclu[0] = Math::Point(  0.0f/256.0f, 160.0f/256.0f);
        exclu[1] = Math::Point(256.0f/256.0f, 256.0f/256.0f);  // pencils
        exclu[2] = Math::Point(0.0f, 0.0f);
        exclu[3] = Math::Point(0.0f, 0.0f);  // terminator
        m_engine->ChangeTextureColor("textures/objects/drawer.png"+teamStr, "textures/objects/drawer.png",  m_colorRefBot, newColor, colorRef2, colorNew2, 0.10f, -1.0f, ts, ti, exclu, 0, true);

        exclu[0] = Math::Point(237.0f/256.0f, 176.0f/256.0f);
        exclu[1] = Math::Point(256.0f/256.0f, 220.0f/256.0f);  // blue canister
        exclu[2] = Math::Point(106.0f/256.0f, 150.0f/256.0f);
        exclu[3] = Math::Point(130.0f/256.0f, 214.0f/256.0f);  // safe location
        exclu[4] = Math::Point(0.0f, 0.0f);
        exclu[5] = Math::Point(0.0f, 0.0f);  // terminator
        m_engine->ChangeTextureColor("textures/objects/subm.png"+teamStr,   "textures/objects/subm.png",    m_colorRefBot, newColor, colorRef2, colorNew2, 0.10f, -1.0f, ts, ti, exclu, 0, true);
    }

    // AlienColor

    exclu[0] = Math::Point(128.0f/256.0f, 160.0f/256.0f);
    exclu[1] = Math::Point(256.0f/256.0f, 256.0f/256.0f);  // SatCom
    exclu[2] = Math::Point(0.0f, 0.0f);
    exclu[3] = Math::Point(0.0f, 0.0f);  // terminator
    m_engine->ChangeTextureColor("textures/objects/ant.png",     m_colorRefAlien, m_colorNewAlien, colorRef2, colorNew2, 0.50f, -1.0f, ts, ti, exclu);
    m_engine->ChangeTextureColor("textures/objects/mother.png",  m_colorRefAlien, m_colorNewAlien, colorRef2, colorNew2, 0.50f, -1.0f, ts, ti);

    // GreeneryColor
    m_engine->ChangeTextureColor("textures/objects/plant.png",   m_colorRefGreen, m_colorNewGreen, colorRef2, colorNew2, 0.50f, -1.0f, ts, ti);

    // water color

    // PARTIPLOUF0 and PARTIDROP :
    ts = Math::Point(0.500f, 0.500f);
    ti = Math::Point(0.875f, 0.750f);
    m_engine->ChangeTextureColor("textures/effect00.png", m_colorRefWater, m_colorNewWater, colorRef2, colorNew2, 0.20f, -1.0f, ts, ti, 0, m_colorShiftWater, true);

    // PARTIFLIC :
    ts = Math::Point(0.00f, 0.75f);
    ti = Math::Point(0.25f, 1.00f);
    m_engine->ChangeTextureColor("textures/effect02.png", m_colorRefWater, m_colorNewWater, colorRef2, colorNew2, 0.20f, -1.0f, ts, ti, 0, m_colorShiftWater, true);

    // This loads the newly recolored textures to objects
    m_engine->LoadAllTextures();
}

//! Updates the number of unnecessary objects
bool CRobotMain::TestGadgetQuantity(int rank)
{
    static int table10[10] = {0,1,0,0,0,0,0,0,0,0};
    static int table20[10] = {0,1,0,0,0,1,0,0,0,0};
    static int table30[10] = {0,1,0,1,0,1,0,0,0,0};
    static int table40[10] = {0,1,0,1,0,1,0,1,0,0};
    static int table50[10] = {0,1,0,1,0,1,0,1,0,1};
    static int table60[10] = {0,1,0,1,1,1,0,1,0,1};
    static int table70[10] = {0,1,0,1,1,1,0,1,1,1};
    static int table80[10] = {0,1,1,1,1,1,0,1,1,1};
    static int table90[10] = {0,1,1,1,1,1,1,1,1,1};

    float percent = m_engine->GetGadgetQuantity();
    if (percent == 0.0f) return false;
    if (percent == 1.0f) return true;

    int *table;
         if (percent <= 0.15f) table = table10;
    else if (percent <= 0.25f) table = table20;
    else if (percent <= 0.35f) table = table30;
    else if (percent <= 0.45f) table = table40;
    else if (percent <= 0.55f) table = table50;
    else if (percent <= 0.65f) table = table60;
    else if (percent <= 0.75f) table = table70;
    else if (percent <= 0.85f) table = table80;
    else                       table = table90;

    return table[rank%10];
}



//! Calculates the distance to the nearest object
float CRobotMain::SearchNearestObject(Math::Vector center, CObject *exclu)
{
    float min = 100000.0f;
    for (CObject* obj : m_objMan->GetAllObjects())
    {
        if (!obj->GetActive()) continue;  // inactive?
        if (IsObjectBeingTransported(obj)) continue;

        if (obj == exclu)  continue;

        ObjectType type = obj->GetType();

        if (type == OBJECT_BASE)
        {
            Math::Vector oPos = obj->GetPosition();
            if (oPos.x != center.x ||
                oPos.z != center.z)
            {
                float dist = Math::Distance(center, oPos)-80.0f;
                if (dist < 0.0f) dist = 0.0f;
                min = Math::Min(min, dist);
                continue;
            }
        }

        if (type == OBJECT_STATION   ||
            type == OBJECT_REPAIR    ||
            type == OBJECT_DESTROYER)
        {
            Math::Vector oPos = obj->GetPosition();
            float dist = Math::Distance(center, oPos)-8.0f;
            if (dist < 0.0f) dist = 0.0f;
            min = Math::Min(min, dist);
        }

        for (const auto& crashSphere : obj->GetAllCrashSpheres())
        {
            Math::Vector oPos = crashSphere.sphere.pos;
            float oRadius = crashSphere.sphere.radius;

            float dist = Math::Distance(center, oPos)-oRadius;
            if (dist < 0.0f) dist = 0.0f;
            min = Math::Min(min, dist);
        }
    }
    return min;
}

//! Calculates a free space
bool CRobotMain::FreeSpace(Math::Vector &center, float minRadius, float maxRadius,
                           float space, CObject *exclu)
{
    if (minRadius < maxRadius)  // from internal to external?
    {
        for (float radius = minRadius; radius <= maxRadius; radius += space)
        {
            float ia = space/radius;
            for (float angle = 0.0f; angle < Math::PI*2.0f; angle += ia)
            {
                Math::Point p;
                p.x = center.x+radius;
                p.y = center.z;
                p = Math::RotatePoint(Math::Point(center.x, center.z), angle, p);
                Math::Vector pos;
                pos.x = p.x;
                pos.z = p.y;
                pos.y = 0.0f;
                m_terrain->AdjustToFloor(pos, true);
                float dist = SearchNearestObject(pos, exclu);
                if (dist >= space)
                {
                    float flat = m_terrain->GetFlatZoneRadius(pos, dist/2.0f);
                    if (flat >= dist/2.0f)
                    {
                        center = pos;
                        return true;
                    }
                }
            }
        }
    }
    else    // from external to internal?
    {
        for (float radius=maxRadius; radius >= minRadius; radius -= space)
        {
            float ia = space/radius;
            for (float angle=0.0f ; angle<Math::PI*2.0f ; angle+=ia )
            {
                Math::Point p;
                p.x = center.x+radius;
                p.y = center.z;
                p = Math::RotatePoint(Math::Point(center.x, center.z), angle, p);
                Math::Vector pos;
                pos.x = p.x;
                pos.z = p.y;
                pos.y = 0.0f;
                m_terrain->AdjustToFloor(pos, true);
                float dist = SearchNearestObject(pos, exclu);
                if (dist >= space)
                {
                    float flat = m_terrain->GetFlatZoneRadius(pos, dist/2.0f);
                    if (flat >= dist/2.0f)
                    {
                        center = pos;
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

//! Calculates a flat free space
bool CRobotMain::FlatFreeSpace(Math::Vector &center, float minFlat, float minRadius, float maxRadius,
                           float space, CObject *exclu)
{
    if (minRadius < maxRadius)  // from internal to external?
    {
        for (float radius = minRadius; radius <= maxRadius; radius += space)
        {
            float ia = space/radius;
            for (float angle = 0.0f; angle < Math::PI*2.0f; angle += ia)
            {
                Math::Point p;
                p.x = center.x+radius;
                p.y = center.z;
                p = Math::RotatePoint(Math::Point(center.x, center.z), angle, p);
                Math::Vector pos;
                pos.x = p.x;
                pos.z = p.y;
                pos.y = 0.0f;
                m_terrain->AdjustToFloor(pos, true);
                float dist = SearchNearestObject(pos, exclu);
                if (dist >= space)
                {
                    float flat = m_terrain->GetFlatZoneRadius(pos, dist/2.0f);
                    if (flat >= dist/2.0f)
                    {
                        flat = m_terrain->GetFlatZoneRadius(pos, minFlat);
                        if(flat >= minFlat)
                        {
                            center = pos;
                            return true;
                        }
                    }
                }
            }
        }
    }
    else    // from external to internal?
    {
        for (float radius=maxRadius; radius >= minRadius; radius -= space)
        {
            float ia = space/radius;
            for (float angle=0.0f ; angle<Math::PI*2.0f ; angle+=ia )
            {
                Math::Point p;
                p.x = center.x+radius;
                p.y = center.z;
                p = Math::RotatePoint(Math::Point(center.x, center.z), angle, p);
                Math::Vector pos;
                pos.x = p.x;
                pos.z = p.y;
                pos.y = 0.0f;
                m_terrain->AdjustToFloor(pos, true);
                float dist = SearchNearestObject(pos, exclu);
                if (dist >= space)
                {
                    float flat = m_terrain->GetFlatZoneRadius(pos, dist/2.0f);
                    if (flat >= dist/2.0f)
                    {
                        flat = m_terrain->GetFlatZoneRadius(pos, minFlat);
                        if(flat >= minFlat)
                        {
                            center = pos;
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

//! Calculates the maximum radius of a free space
float CRobotMain::GetFlatZoneRadius(Math::Vector center, float maxRadius,
                                    CObject *exclu)
{
    float dist = SearchNearestObject(center, exclu);
    if (dist == 0.0f) return 0.0f;
    if (dist < maxRadius)
        maxRadius = dist;

    return m_terrain->GetFlatZoneRadius(center, maxRadius);
}


//! Hides buildable area when a cube of metal is taken up
void CRobotMain::HideDropZone(CObject* metal)
{
    if (m_showLimit[1].used         &&
        m_showLimit[1].link == metal)
    {
        FlushShowLimit(1);
    }

    if (m_showLimit[2].used         &&
        m_showLimit[2].link == metal)
    {
        FlushShowLimit(2);
    }
}

//! Shows the buildable area when a cube of metal is deposited
void CRobotMain::ShowDropZone(CObject* metal, CObject* transporter)
{
    if (metal == nullptr) return;

    Math::Vector center = metal->GetPosition();

    // Calculates the maximum radius possible depending on other items.
    float oMax = 30.0f;  // radius to build the biggest building
    float tMax;
    for (CObject* obj : m_objMan->GetAllObjects())
    {
        if (!obj->GetActive()) continue;  // inactive?
        if (IsObjectBeingTransported(obj)) continue;

        if (obj == metal) continue;
        if (obj == transporter) continue;

        Math::Vector oPos;

        ObjectType type = obj->GetType();
        if (type == OBJECT_BASE)
        {
            oPos = obj->GetPosition();
            float dist = Math::Distance(center, oPos)-80.0f;
            oMax = Math::Min(oMax, dist);
        }
        else
        {
            for (const auto& crashSphere : obj->GetAllCrashSpheres())
            {
                float dist = Math::Distance(center, crashSphere.sphere.pos)-crashSphere.sphere.radius;
                oMax = Math::Min(oMax, dist);
            }
        }

        if ( type == OBJECT_DERRICK  ||
             type == OBJECT_FACTORY  ||
             type == OBJECT_STATION  ||
             type == OBJECT_CONVERT  ||
             type == OBJECT_REPAIR   ||
             type == OBJECT_DESTROYER||
             type == OBJECT_TOWER    ||
             type == OBJECT_RESEARCH ||
             type == OBJECT_RADAR    ||
             type == OBJECT_ENERGY   ||
             type == OBJECT_LABO     ||
             type == OBJECT_NUCLEAR  ||
             type == OBJECT_START    ||
             type == OBJECT_END      ||
             type == OBJECT_INFO     ||
             type == OBJECT_PARA     ||
             type == OBJECT_SAFE     ||
             type == OBJECT_HUSTON   )  // building?
        {
            for (const auto& crashSphere : obj->GetAllCrashSpheres())
            {
                float dist = Math::Distance(center, crashSphere.sphere.pos)-crashSphere.sphere.radius-BUILDMARGIN;
                oMax = Math::Min(oMax, dist);
            }
        }
    }

    // Calculates the maximum possible radius depending on terrain.
    if (oMax >= 2.0f)
        tMax = m_terrain->GetFlatZoneRadius(center, 30.0f);
    else
        tMax = 0.0f;

    float radius = Math::Min(oMax, tMax);
    if (radius >= 2.0f)
        SetShowLimit(1, Gfx::PARTILIMIT2, metal, center, radius, 10.0f);
}

//! Erases the boundaries shown
void CRobotMain::FlushShowLimit(int i)
{
    if (m_showLimit[i].link != 0)
    {
        m_showLimit[i].link->StopShowLimit();
    }

    for (int j = 0; j < m_showLimit[i].total; j++)
    {
        if (m_showLimit[i].parti[j] == 0) continue;

        m_particle->DeleteParticle(m_showLimit[i].parti[j]);
        m_showLimit[i].parti[j] = 0;
    }

    m_showLimit[i].total = 0;
    m_showLimit[i].link = 0;
    m_showLimit[i].used = false;
}

//! Specifies the boundaries to show
void CRobotMain::SetShowLimit(int i, Gfx::ParticleType parti, CObject *obj,
                              Math::Vector pos, float radius, float duration)
{
    FlushShowLimit(i);  // erases the current boundaries

    if (radius <= 0.0f) return;

    Math::Point dim;
    float dist;
    if (radius <= 50.0f)
    {
        dim = Math::Point(0.3f, 0.3f);
        dist = 2.5f;
    }
    else
    {
        dim = Math::Point(1.5f, 1.5f);
        dist = 10.0f;
    }

    m_showLimit[i].used = true;
    m_showLimit[i].link = obj;
    m_showLimit[i].pos = pos;
    m_showLimit[i].radius = radius;
    m_showLimit[i].duration = duration;
    m_showLimit[i].total = static_cast<int>((radius*2.0f*Math::PI)/dist);
    if (m_showLimit[i].total > MAXSHOWPARTI) m_showLimit[i].total = MAXSHOWPARTI;
    m_showLimit[i].time = 0.0f;

    for (int j = 0; j < m_showLimit[i].total; j++)
    {
        m_showLimit[i].parti[j] = m_particle->CreateParticle(pos, Math::Vector(0.0f, 0.0f, 0.0f), dim, parti, duration);
    }
}

//! Adjusts the boundaries to show
void CRobotMain::AdjustShowLimit(int i, Math::Vector pos)
{
    m_showLimit[i].pos = pos;
}

//! Mount the boundaries of the selected object
void CRobotMain::StartShowLimit()
{
    CObject* obj = GetSelect();
    if (obj == nullptr) return;

    obj->StartShowLimit();
}

//! Advances the boundaries shown
void CRobotMain::FrameShowLimit(float rTime)
{
    if (m_engine->GetPause()) return;

    for (int i = 0; i < MAXSHOWLIMIT; i++)
    {
        if (!m_showLimit[i].used) continue;

        m_showLimit[i].time += rTime;

        if (m_showLimit[i].time >= m_showLimit[i].duration)
        {
            FlushShowLimit(i);
            continue;
        }

        float factor;
        if (m_showLimit[i].time < 1.0f)
            factor = m_showLimit[i].time;
        else if (m_showLimit[i].time > m_showLimit[i].duration-1.0f)
            factor = m_showLimit[i].duration-m_showLimit[i].time;
        else
            factor = 1.0f;

        float speed = 0.4f-m_showLimit[i].radius*0.001f;
        if (speed < 0.1f) speed = 0.1f;
        float angle = m_showLimit[i].time*speed;

        for (int j = 0; j < m_showLimit[i].total; j++)
        {
            if (m_showLimit[i].parti[j] == 0) continue;

            Math::Point center;
            center.x = m_showLimit[i].pos.x;
            center.y = m_showLimit[i].pos.z;
            Math::Point rotate;
            rotate.x = center.x+m_showLimit[i].radius*factor;
            rotate.y = center.y;
            rotate = Math::RotatePoint(center, angle, rotate);

            Math::Vector pos;
            pos.x = rotate.x;
            pos.z = rotate.y;
            pos.y = 0.0f;
            m_terrain->AdjustToFloor(pos, true);
            if (m_showLimit[i].radius <= 50.0f) pos.y += 0.5f;
            else                                pos.y += 2.0f;
            m_particle->SetPosition(m_showLimit[i].parti[j], pos);

            angle += (2.0f*Math::PI)/m_showLimit[i].total;
        }
    }
}


//! Compiles all scripts of robots
void CRobotMain::CompileScript(bool soluce)
{
    m_ui->GetLoadingScreen()->SetProgress(0.75f, RT_LOADING_PROGRAMS);

    int numObjects = m_objMan->CountObjectsImplementing(ObjectInterfaceType::Programmable);
    int objCounter = 0;

    for (CObject* obj : m_objMan->GetAllObjects())
    {
        if (! obj->Implements(ObjectInterfaceType::Programmable)) continue;

        float objectProgress = static_cast<float>(objCounter) / static_cast<float>(numObjects);
        m_ui->GetLoadingScreen()->SetProgress(0.75f+objectProgress*0.25f, RT_LOADING_PROGRAMS, "for object "+StrUtils::ToString<int>(objCounter+1)+" / "+StrUtils::ToString<int>(numObjects));
        objCounter++;

        CBrain* brain = dynamic_cast<CProgrammableObject*>(obj)->GetBrain();
        for (auto& prog : brain->GetPrograms())
        {
            Program* program = prog.get();

            if (program->filename.empty()) continue;

            std::string name = "ai/" + program->filename;
            if (! brain->ReadProgram(program, const_cast<char*>(name.c_str())))
            {
                GetLogger()->Error("Unable to read script from file \"%s\"\n", name.c_str());
            }
            //? if (!brain->GetCompile(program)) nbError++;
        }

        if (soluce)
        {
            char* name = brain->GetSoluceName();
            if (name[0] != 0)
            {
                brain->ReadSoluce(name);  // load solution
            }
        }

        LoadOneScript(obj);
    }

    // Start all programs according to the command "run".
    for (CObject* obj : m_objMan->GetAllObjects())
    {
        if (! obj->Implements(ObjectInterfaceType::Programmable)) continue;

        CBrain* brain = dynamic_cast<CProgrammableObject*>(obj)->GetBrain();
        Program* program = brain->GetScriptRun();
        if (program != nullptr)
        {
            brain->RunProgram(program);  // starts the program
        }
    }
}

//! Load all programs of the robot
void CRobotMain::LoadOneScript(CObject *obj)
{
    if (! obj->Implements(ObjectInterfaceType::Programmable)) return;

    CBrain* brain = dynamic_cast<CProgrammableObject*>(obj)->GetBrain();

    if (!IsSelectable(obj)) return;

    ObjectType type = obj->GetType();
    if (type == OBJECT_HUMAN) return;

    int objRank = obj->GetDefRank();
    if (objRank == -1) return;

    char categoryChar = GetLevelCategoryDir(m_levelCategory)[0];
    for (unsigned int i = 0; i <= 999; i++)
    {
        char file[MAX_FNAME];
        sprintf(file, "%c%.3d%.3d%.3d%.3d.txt", categoryChar, m_levelChap, m_levelRank, objRank, i);
        std::string filename = m_playerProfile->GetSaveFile(file);

        if (CResourceManager::Exists(filename))
        {
            Program* program = brain->GetOrAddProgram(i);
            if(brain->GetCompile(program)) continue; // If already loaded (e.g. from level file), skip
            brain->ReadProgram(program, filename.c_str());
            //? if (!brain->GetCompile(program)) nbError++;
        }
    }
}

//! Load all programs of the robot
void CRobotMain::LoadFileScript(CObject *obj, const char* filename, int objRank)
{
    if (objRank == -1) return;

    if (! obj->Implements(ObjectInterfaceType::Programmable)) return;

    CBrain* brain = dynamic_cast<CProgrammableObject*>(obj)->GetBrain();

    ObjectType type = obj->GetType();
    if (type == OBJECT_HUMAN) return;

    std::string dirname = filename;
    dirname = dirname.substr(0, dirname.find_last_of("/"));

    char fn[MAX_FNAME]; //TODO: Refactor to std::string
    for (unsigned int i = 0; i <= 999; i++)
    {
        sprintf(fn, "%s/prog%.3d%.3d.txt", dirname.c_str(), objRank, i);
        if (CResourceManager::Exists(fn))
        {
            Program* program = brain->GetOrAddProgram(i);
            brain->ReadProgram(program, fn);
            //? if (!brain->GetCompile(program)) nbError++;
        }
    }
}

//! Saves all programs of all the robots
void CRobotMain::SaveAllScript()
{
    for (CObject* obj : m_objMan->GetAllObjects())
    {
        SaveOneScript(obj);
    }
}

//! Saves all programs of the robot.
//! If a program does not exist, the corresponding file is destroyed.
void CRobotMain::SaveOneScript(CObject *obj)
{
    if (! obj->Implements(ObjectInterfaceType::Programmable)) return;

    CBrain* brain = dynamic_cast<CProgrammableObject*>(obj)->GetBrain();

    if (!IsSelectable(obj)) return;

    ObjectType type = obj->GetType();
    if (type == OBJECT_HUMAN) return;

    int objRank = obj->GetDefRank();
    if (objRank == -1) return;

    char categoryChar = GetLevelCategoryDir(m_levelCategory)[0];
    auto& programs = brain->GetPrograms();
    // TODO: Find a better way to do that
    for (unsigned int i = 0; i <= 999; i++)
    {
        char file[MAX_FNAME];
        sprintf(file, "%c%.3d%.3d%.3d%.3d.txt", categoryChar, m_levelChap, m_levelRank, objRank, i);
        std::string filename = m_playerProfile->GetSaveFile(file);

        if (i < programs.size())
        {
            brain->WriteProgram(programs[i].get(), filename.c_str());
        }
        else
        {
            CResourceManager::Remove(filename);
        }
    }
}

//! Saves all programs of the robot.
//! If a program does not exist, the corresponding file is destroyed.
void CRobotMain::SaveFileScript(CObject *obj, const char* filename, int objRank)
{
    if (objRank == -1) return;

    if (! obj->Implements(ObjectInterfaceType::Programmable)) return;

    CBrain* brain = dynamic_cast<CProgrammableObject*>(obj)->GetBrain();

    ObjectType type = obj->GetType();
    if (type == OBJECT_HUMAN) return;

    std::string dirname = filename;
    dirname = dirname.substr(0, dirname.find_last_of("/"));

    char fn[MAX_FNAME]; //TODO: Refactor to std::string
    auto& programs = brain->GetPrograms();
    // TODO: Find a better way to do that
    for (unsigned int i = 0; i <= 999; i++)
    {
        sprintf(fn, "%s/prog%.3d%.3d.txt", dirname.c_str(), objRank, i);
        if (i < programs.size())
        {
            brain->WriteProgram(programs[i].get(), fn);
        }
        else
        {
            CResourceManager::Remove(fn);
        }
    }
}

//! Saves the stack of the program in execution of a robot
bool CRobotMain::SaveFileStack(CObject *obj, FILE *file, int objRank)
{
    if (objRank == -1) return true;

    if (! obj->Implements(ObjectInterfaceType::Programmable)) return true;

    CBrain* brain = dynamic_cast<CProgrammableObject*>(obj)->GetBrain();

    ObjectType type = obj->GetType();
    if (type == OBJECT_HUMAN) return true;

    return brain->WriteStack(file);
}

//! Resumes the execution stack of the program in a robot
bool CRobotMain::ReadFileStack(CObject *obj, FILE *file, int objRank)
{
    if (objRank == -1) return true;

    if (! obj->Implements(ObjectInterfaceType::Programmable)) return true;

    CBrain* brain = dynamic_cast<CProgrammableObject*>(obj)->GetBrain();

    ObjectType type = obj->GetType();
    if (type == OBJECT_HUMAN) return true;

    return brain->ReadStack(file);
}


//! Empty the list
bool CRobotMain::FlushNewScriptName()
{
    for (int i = 0; i < MAXNEWSCRIPTNAME; i++)
        m_newScriptName[i].used = false;

    return true;
}

//! Adds a script name
bool CRobotMain::AddNewScriptName(ObjectType type, char *name)
{
    for (int i = 0; i < MAXNEWSCRIPTNAME; i++)
    {
        if (!m_newScriptName[i].used)
        {
            m_newScriptName[i].used = true;
            m_newScriptName[i].type = type;
            strcpy(m_newScriptName[i].name, name);
            return true;
        }
    }
    return false;
}

//! Seeks a script name for a given type
char*  CRobotMain::GetNewScriptName(ObjectType type, int rank)
{
    for (int i = 0; i < MAXNEWSCRIPTNAME; i++)
    {
        if (m_newScriptName[i].used &&
            (m_newScriptName[i].type == type        ||
             m_newScriptName[i].type == OBJECT_NULL))
        {
            if (rank == 0) return m_newScriptName[i].name;
            else           rank --;
        }
    }

    return nullptr;
}


//! Seeks if an object occupies in a spot, to prevent a backup of the game
bool CRobotMain::IsBusy()
{
    if (CScriptFunctions::m_numberOfOpenFiles > 0) return true;

    for (CObject* obj : m_objMan->GetAllObjects())
    {
        if (! obj->Implements(ObjectInterfaceType::Programmable)) continue;

        CBrain* brain = dynamic_cast<CProgrammableObject*>(obj)->GetBrain();
        if (brain->IsBusy()) return true;
    }
    return false;
}

//! Writes an object into the backup file
void CRobotMain::IOWriteObject(CLevelParserLine* line, CObject* obj)
{
    if (obj->GetType() == OBJECT_FIX) return;

    line->AddParam("type", MakeUnique<CLevelParserParam>(obj->GetType()));
    line->AddParam("id", MakeUnique<CLevelParserParam>(obj->GetID()));
    line->AddParam("pos", MakeUnique<CLevelParserParam>(obj->GetPosition()/g_unit));
    line->AddParam("angle", MakeUnique<CLevelParserParam>(obj->GetRotation() * Math::RAD_TO_DEG));
    line->AddParam("zoom", MakeUnique<CLevelParserParam>(obj->GetScale()));

    if (obj->Implements(ObjectInterfaceType::Old))
    {
        COldObject* oldObj = dynamic_cast<COldObject*>(obj);

        for (int i = 1; i < OBJECTMAXPART; i++)
        {
            if (oldObj->GetObjectRank(i) == -1) continue;

            Math::Vector pos = oldObj->GetPartPosition(i);
            if (pos.x != 0.0f || pos.y != 0.0f || pos.z != 0.0f)
            {
                pos /= g_unit;
                line->AddParam("p" + boost::lexical_cast<std::string>(i), MakeUnique<CLevelParserParam>(pos));
            }

            Math::Vector rot = oldObj->GetPartRotation(i);
            if (rot.x != 0.0f || rot.y != 0.0f || rot.z != 0.0f)
            {
                rot /= (Math::PI/180.0f);
                line->AddParam("a" + boost::lexical_cast<std::string>(i), MakeUnique<CLevelParserParam>(rot));
            }

            Math::Vector scale = oldObj->GetPartScale(i);
            if (scale.x != 1.0f || scale.y != 1.0f || scale.z != 1.0f)
            {
                line->AddParam("z" + boost::lexical_cast<std::string>(i), MakeUnique<CLevelParserParam>(scale));
            }
        }

        line->AddParam("trainer", MakeUnique<CLevelParserParam>(obj->GetTrainer()));
        line->AddParam("option", MakeUnique<CLevelParserParam>(obj->GetOption()));
    }

    if (obj->GetSelect())
        line->AddParam("select", MakeUnique<CLevelParserParam>(true));

    obj->Write(line);

    if (obj->GetType() == OBJECT_BASE)
        line->AddParam("run", MakeUnique<CLevelParserParam>(3));  // stops and open (PARAM_FIXSCENE)

    if (obj->Implements(ObjectInterfaceType::Programmable))
    {
        CBrain* brain = dynamic_cast<CProgrammableObject*>(obj)->GetBrain();

        int run = brain->GetProgram();
        if (run != -1)
        {
            line->AddParam("run", MakeUnique<CLevelParserParam>(run+1));
        }

        auto& programs = brain->GetPrograms();
        for (unsigned int i = 0; i < programs.size(); i++)
        {
            if (programs[i]->readOnly)
            {
                line->AddParam("scriptReadOnly" + boost::lexical_cast<std::string>(i+1), MakeUnique<CLevelParserParam>(true));
            }
        }
    }
}

//! Saves the current game
bool CRobotMain::IOWriteScene(std::string filename, std::string filecbot, std::string filescreenshot, char *info)
{
    // Render the indicator to show that we are working
    ShowSaveIndicator(true);
    m_app->RenderNextFrame(); // update

    CLevelParser levelParser(filename);
    CLevelParserLineUPtr line;

    line = MakeUnique<CLevelParserLine>("Title");
    line->AddParam("text", MakeUnique<CLevelParserParam>(std::string(info)));
    levelParser.AddLine(std::move(line));


    //TODO: Do we need that? It's not used anyway
    line = MakeUnique<CLevelParserLine>("Version");
    line->AddParam("maj", MakeUnique<CLevelParserParam>(0));
    line->AddParam("min", MakeUnique<CLevelParserParam>(1));
    levelParser.AddLine(std::move(line));


    line = MakeUnique<CLevelParserLine>("Created");
    line->AddParam("date", MakeUnique<CLevelParserParam>(GetCurrentTimestamp()));
    levelParser.AddLine(std::move(line));

    line = MakeUnique<CLevelParserLine>("Mission");
    line->AddParam("base", MakeUnique<CLevelParserParam>(GetLevelCategoryDir(m_levelCategory)));
    if (m_levelCategory == LevelCategory::CustomLevels)
        line->AddParam("dir", MakeUnique<CLevelParserParam>(GetCustomLevelDir()));
    else
        line->AddParam("chap", MakeUnique<CLevelParserParam>(m_levelChap));
    line->AddParam("rank", MakeUnique<CLevelParserParam>(m_levelRank));
    levelParser.AddLine(std::move(line));

    line = MakeUnique<CLevelParserLine>("Map");
    line->AddParam("zoom", MakeUnique<CLevelParserParam>(m_map->GetZoomMap()));
    levelParser.AddLine(std::move(line));

    line = MakeUnique<CLevelParserLine>("DoneResearch");
    line->AddParam("bits", MakeUnique<CLevelParserParam>(static_cast<int>(m_researchDone[0])));
    levelParser.AddLine(std::move(line));

    float sleep, delay, magnetic, progress;
    if (m_lightning->GetStatus(sleep, delay, magnetic, progress))
    {
        line = MakeUnique<CLevelParserLine>("BlitzMode");
        line->AddParam("sleep", MakeUnique<CLevelParserParam>(sleep));
        line->AddParam("delay", MakeUnique<CLevelParserParam>(delay));
        line->AddParam("magnetic", MakeUnique<CLevelParserParam>(magnetic/g_unit));
        line->AddParam("progress", MakeUnique<CLevelParserParam>(progress));
        levelParser.AddLine(std::move(line));
    }


    int objRank = 0;
    for (CObject* obj : m_objMan->GetAllObjects())
    {
        if (obj->GetType() == OBJECT_TOTO) continue;
        if (obj->GetType() == OBJECT_FIX) continue;
        if (IsObjectBeingTransported(obj)) continue;
        if (obj->GetBurn()) continue;
        if (obj->GetDead()) continue;
        if (obj->IsExploding()) continue;

        if (obj->Implements(ObjectInterfaceType::Carrier))
        {
            CObject* cargo = dynamic_cast<CCarrierObject*>(obj)->GetCargo();
            if (cargo != nullptr)  // object transported?
            {
                line = MakeUnique<CLevelParserLine>("CreateFret");
                IOWriteObject(line.get(), cargo);
                levelParser.AddLine(std::move(line));
            }
        }

        if (obj->Implements(ObjectInterfaceType::Powered))
        {
            CObject* power = dynamic_cast<CPoweredObject*>(obj)->GetPower();
            if (power != nullptr) // battery transported?
            {
                line = MakeUnique<CLevelParserLine>("CreatePower");
                IOWriteObject(line.get(), power);
                levelParser.AddLine(std::move(line));
            }
        }


        line = MakeUnique<CLevelParserLine>("CreateObject");
        IOWriteObject(line.get(), obj);
        levelParser.AddLine(std::move(line));

        SaveFileScript(obj, filename.c_str(), objRank++);
    }
    try
    {
        levelParser.Save();
    }
    catch (CLevelParserException& e)
    {
        GetLogger()->Error("Failed to save level state - %s\n", e.what());
        return false;
    }

    // Writes the file of stacks of execution.
    FILE* file = fOpen((CResourceManager::GetSaveLocation() + "/" + filecbot).c_str(), "wb");
    if (file == NULL) return false;

    long version = 1;
    fWrite(&version, sizeof(long), 1, file);  // version of COLOBOT
    version = CBotProgram::GetVersion();
    fWrite(&version, sizeof(long), 1, file);  // version of CBOT

    objRank = 0;
    for (CObject* obj : m_objMan->GetAllObjects())
    {
        if (obj->GetType() == OBJECT_TOTO) continue;
        if (obj->GetType() == OBJECT_FIX) continue;
        if (IsObjectBeingTransported(obj)) continue;

        if (obj->GetBurn()) continue;
        if (obj->GetDead()) continue;

        if (!SaveFileStack(obj, file, objRank++))  break;
    }
    CBotClass::SaveStaticState(file);
    fClose(file);

    m_shotName = CResourceManager::GetSaveLocation() + "/" + filescreenshot; //TODO: Use PHYSFS?

    return true;
}

//! Notifies the user that scene write is finished
void CRobotMain::IOWriteSceneFinished()
{
    m_displayText->DisplayError(INFO_WRITEOK, Math::Vector(0.0f,0.0f,0.0f));
    m_shotSaving--;
}

//! Resumes the game
CObject* CRobotMain::IOReadObject(CLevelParserLine *line, const char* filename, const std::string& objCounterText, float objectProgress, int objRank)
{
    Math::Vector pos  = line->GetParam("pos")->AsPoint()*g_unit;
    Math::Vector dir  = line->GetParam("angle")->AsPoint()*(Math::PI/180.0f);
    Math::Vector zoom = line->GetParam("zoom")->AsPoint();

    ObjectType type = line->GetParam("type")->AsObjectType();
    int id = line->GetParam("id")->AsInt();

    std::string details = objCounterText;
    #if DEV_BUILD
    // Object categories may spoil the level a bit, so hide them in release builds
    details += ": "+CLevelParserParam::FromObjectType(type);
    #endif
    m_ui->GetLoadingScreen()->SetProgress(0.25f+objectProgress*0.5f, RT_LOADING_OBJECTS_SAVED, details);

    bool trainer = line->GetParam("trainer")->AsBool(false);
    bool toy = line->GetParam("toy")->AsBool(false);
    int option = line->GetParam("option")->AsInt(0);
    int team = line->GetParam("team")->AsInt(0);

    CObject* obj = m_objMan->CreateObject(pos, dir.y, type, 0.0f, 1.0f, 0.0f, trainer, toy, option, team, id);

    if (obj->Implements(ObjectInterfaceType::Old))
    {
        COldObject* oldObj = dynamic_cast<COldObject*>(obj);

        oldObj->SetDefRank(objRank);
        oldObj->SetPosition(pos);
        oldObj->SetRotation(dir);

        if (zoom.x != 0.0f || zoom.y != 0.0f || zoom.z != 0.0f)
            oldObj->SetScale(zoom);

        for (int i = 1; i < OBJECTMAXPART; i++)
        {
            if (oldObj->GetObjectRank(i) == -1) continue;

            pos = line->GetParam(std::string("p")+boost::lexical_cast<std::string>(i))->AsPoint(Math::Vector());
            if (pos.x != 0.0f || pos.y != 0.0f || pos.z != 0.0f)
            {
                oldObj->SetPartPosition(i, pos*g_unit);
            }

            dir = line->GetParam(std::string("a")+boost::lexical_cast<std::string>(i))->AsPoint(Math::Vector());
            if (dir.x != 0.0f || dir.y != 0.0f || dir.z != 0.0f)
            {
                oldObj->SetPartRotation(i, dir*(Math::PI/180.0f));
            }

            zoom = line->GetParam(std::string("z")+boost::lexical_cast<std::string>(i))->AsPoint(Math::Vector());
            if (zoom.x != 0.0f || zoom.y != 0.0f || zoom.z != 0.0f)
            {
                oldObj->SetPartScale(i, zoom);
            }
        }
    }

    if (type == OBJECT_BASE) m_base = obj;

    obj->Read(line);

    int run = line->GetParam("run")->AsInt(-1);
    if (run != -1)
    {
        CAuto* automat = obj->GetAuto();
        if (automat != nullptr)
            automat->Start(run);  // starts the film
    }

    if (obj->Implements(ObjectInterfaceType::Programmable))
    {
        CBrain* brain = dynamic_cast<CProgrammableObject*>(obj)->GetBrain();

        if (run != -1)
        {
            Program* program = brain->GetOrAddProgram(run-1);
            brain->SetScriptRun(program);  // marks the program to be started
        }

        for (unsigned int i = 0; i <= 999; i++)
        {
            if (line->GetParam("scriptReadOnly" + boost::lexical_cast<std::string>(i+1))->AsBool(false))
            {
                Program* prog = brain->GetOrAddProgram(i);
                prog->readOnly = true;
            }
        }
    }

    return obj;
}

//! Resumes some part of the game
CObject* CRobotMain::IOReadScene(std::string filename, std::string filecbot)
{
    CLevelParser levelParser(filename);
    levelParser.Load();
    int numObjects = levelParser.CountLines("CreateObject") + levelParser.CountLines("CreatePower") + levelParser.CountLines("CreateFret");

    m_base = nullptr;

    CObject* cargo   = nullptr;
    CObject* power  = nullptr;
    CObject* sel    = nullptr;
    int objRank = 0;
    int objCounter = 0;
    for (auto& line : levelParser.GetLines())
    {
        if (line->GetCommand() == "Map")
            m_map->ZoomMap(line->GetParam("zoom")->AsFloat());

        if (line->GetCommand() == "DoneResearch")
            m_researchDone[0] = line->GetParam("bits")->AsInt();

        if (line->GetCommand() == "BlitzMode")
        {
            float sleep = line->GetParam("sleep")->AsFloat();
            float delay = line->GetParam("delay")->AsFloat();
            float magnetic = line->GetParam("magnetic")->AsFloat()*g_unit;
            float progress = line->GetParam("progress")->AsFloat();
            m_lightning->SetStatus(sleep, delay, magnetic, progress);
        }

        if (line->GetCommand() == "CreateFret")
        {
            cargo = IOReadObject(line.get(), filename.c_str(), StrUtils::ToString<int>(objCounter+1)+" / "+StrUtils::ToString<int>(numObjects), static_cast<float>(objCounter) / static_cast<float>(numObjects), -1);
            objCounter++;
        }

        if (line->GetCommand() == "CreatePower")
        {
            power = IOReadObject(line.get(), filename.c_str(), StrUtils::ToString<int>(objCounter+1)+" / "+StrUtils::ToString<int>(numObjects), static_cast<float>(objCounter) / static_cast<float>(numObjects), -1);
            objCounter++;
        }

        if (line->GetCommand() == "CreateObject")
        {
            CObject* obj = IOReadObject(line.get(), filename.c_str(), StrUtils::ToString<int>(objCounter+1)+" / "+StrUtils::ToString<int>(numObjects), static_cast<float>(objCounter) / static_cast<float>(numObjects), objRank++);

            if (line->GetParam("select")->AsBool(false))
                sel = obj;

            if (cargo != nullptr)
            {
                assert(obj->Implements(ObjectInterfaceType::Carrier)); // TODO: exception?
                assert(obj->Implements(ObjectInterfaceType::Old));
                dynamic_cast<CCarrierObject*>(obj)->SetCargo(cargo);
                CTaskManip* task = new CTaskManip(dynamic_cast<COldObject*>(obj));
                task->Start(TMO_AUTO, TMA_GRAB);  // holds the object!
                delete task;
            }

            if (power != nullptr)
            {
                assert(obj->Implements(ObjectInterfaceType::Powered));
                dynamic_cast<CPoweredObject*>(obj)->SetPower(power);
                assert(power->Implements(ObjectInterfaceType::Transportable));
                dynamic_cast<CTransportableObject*>(power)->SetTransporter(obj);
            }
            cargo = nullptr;
            power = nullptr;

            objCounter++;
        }
    }

    // Compiles scripts.
    m_ui->GetLoadingScreen()->SetProgress(0.75f, RT_LOADING_PROGRAMS);
    numObjects = m_objMan->CountObjectsImplementing(ObjectInterfaceType::Programmable);
    objCounter = 0;

    for (CObject* obj : m_objMan->GetAllObjects())
    {
        if (! obj->Implements(ObjectInterfaceType::Programmable)) continue;

        float objectProgress = static_cast<float>(objCounter) / static_cast<float>(numObjects);
        m_ui->GetLoadingScreen()->SetProgress(0.75f+objectProgress*0.2f, RT_LOADING_PROGRAMS, "for object "+StrUtils::ToString<int>(objCounter+1)+" / "+StrUtils::ToString<int>(numObjects));
        objCounter++;

        if (IsObjectBeingTransported(obj)) continue; // TODO: WTF, programmable transportable objects?

        objRank = obj->GetDefRank();
        if (objRank == -1) continue;

        LoadFileScript(obj, filename.c_str(), objRank);
    }

    // Starts scripts
    for (CObject* obj : m_objMan->GetAllObjects())
    {
        if (! obj->Implements(ObjectInterfaceType::Programmable)) continue;
        if (obj->GetDefRank() == -1) continue;

        CBrain* brain = dynamic_cast<CProgrammableObject*>(obj)->GetBrain();

        Program* program = brain->GetScriptRun();
        if (program != nullptr)
        {
            brain->RunProgram(program);  // starts the program
        }
    }

    m_ui->GetLoadingScreen()->SetProgress(0.95f, RT_LOADING_CBOT_SAVE);

    // Reads the file of stacks of execution.
    FILE* file = fOpen((CResourceManager::GetSaveLocation() + "/" + filecbot).c_str(), "rb");
    if (file != NULL)
    {
        long version;
        fRead(&version, sizeof(long), 1, file);  // version of COLOBOT
        if (version == 1)
        {
            fRead(&version, sizeof(long), 1, file);  // version of CBOT
            if (version == CBotProgram::GetVersion())
            {
                objRank = 0;
                for (CObject* obj : m_objMan->GetAllObjects())
                {
                    if (obj->GetType() == OBJECT_TOTO) continue;
                    if (obj->GetType() == OBJECT_FIX) continue;
                    if (IsObjectBeingTransported(obj)) continue;
                    if (obj->GetBurn()) continue;
                    if (obj->GetDead()) continue;

                    if (!ReadFileStack(obj, file, objRank++)) break;
                }
            }
        }
        CBotClass::RestoreStaticState(file);
        fClose(file);
    }

    m_ui->GetLoadingScreen()->SetProgress(1.0f, RT_LOADING_FINISHED);

    return sel;
}


//! Changes current player
void CRobotMain::SelectPlayer(std::string playerName)
{
    assert(!playerName.empty());

    m_playerProfile = MakeUnique<CPlayerProfile>(playerName);
    SetGlobalGamerName(playerName);
}

CPlayerProfile* CRobotMain::GetPlayerProfile()
{
    return m_playerProfile.get();
}


//! Resets all objects to their original position
void CRobotMain::ResetObject()
{
    // schedule reset during next frame
    m_resetCreate = true;
}

//! Resets all objects to their original position
void CRobotMain::ResetCreate()
{
    SaveAllScript();

    // Removes all bullets in progress.
    m_particle->DeleteParticle(Gfx::PARTIGUN1);
    m_particle->DeleteParticle(Gfx::PARTIGUN2);
    m_particle->DeleteParticle(Gfx::PARTIGUN3);
    m_particle->DeleteParticle(Gfx::PARTIGUN4);

    DeselectAll();  // removes the control buttons
    DeleteAllObjects();  // removes all the current 3D Scene

    m_particle->FlushParticle();
    m_terrain->FlushBuildingLevel();

    m_camera->SetType(Gfx::CAM_TYPE_DIALOG);

    try
    {
        CreateScene(m_ui->GetSceneSoluce(), false, true);

        if (!GetNiceReset()) return;

        for (CObject* obj : m_objMan->GetAllObjects())
        {
            if (obj->GetAnimateOnReset())
            {
                m_engine->GetPyroManager()->Create(Gfx::PT_RESET, obj);
            }
        }
    }
    catch (const std::runtime_error& e)
    {
        LevelLoadingError("An error occured while trying to reset scene", e);
    }
}

//! Updates the audiotracks
void CRobotMain::UpdateAudio(bool frame)
{
    for(std::unique_ptr<CAudioChangeCondition>& audioChange : m_audioChange)
    {
        if (audioChange->changed) continue;

        if (audioChange->Check())
        {
            GetLogger()->Info("Changing music to \"%s\"\n", audioChange->music.c_str());
            m_sound->PlayMusic(audioChange->music, audioChange->repeat);
            audioChange->changed = true;
        }
    }
}

void CRobotMain::SetEndMission(Error result, float delay)
{
    if (m_controller != nullptr)
    {
        m_endTakeWinDelay = delay;
        m_endTakeLostDelay = delay;
        m_missionResult = result;
    }
}

Error CRobotMain::CheckEndMissionForGroup(std::vector<CSceneEndCondition*>& endTakes)
{
    Error finalResult = ERR_OK;
    bool hasWinningConditions = false;
    for (CSceneEndCondition* endTake : endTakes)
    {
        Error result = endTake->GetMissionResult();
        if (endTake->lost < 0)
            hasWinningConditions = true;

        if (result == ERR_OK && endTake->immediat)
        {
            hasWinningConditions = true;
            finalResult = result;
            break;
        }

        if (result != ERR_OK)
        {
            finalResult = result;
            break;
        }
    }
    if (finalResult == ERR_OK && !hasWinningConditions) finalResult = ERR_MISSION_NOTERM; // Never end mission without ending conditions
    return finalResult;
}

//! Checks if the mission is over
Error CRobotMain::CheckEndMission(bool frame)
{
    bool isImmediat = false;
    // Process EndMissionTake, unless we are using MissionController
    if (m_controller == nullptr)
    {
        // Sort end conditions by teams
        std::map<int, std::vector<CSceneEndCondition*>> teams;
        for (std::unique_ptr<CSceneEndCondition>& endTake : m_endTake)
        {
            teams[endTake->winTeam].push_back(endTake.get());
            if(endTake->immediat)
                isImmediat = true;
        }

        int teamCount = 0;
        bool usesTeamConditions = false;
        for (auto it : teams)
        {
            int team = it.first;
            if(team == 0) continue;
            usesTeamConditions = true;
            if(!m_objMan->TeamExists(team)) continue;
            teamCount++;
        }

        if (!usesTeamConditions)
        {
            m_missionResult = CheckEndMissionForGroup(teams[0]);
        }
        else
        {
            // Special handling for teams
            m_missionResult = ERR_MISSION_NOTERM;

            if (teamCount == 0)
            {
                GetLogger()->Info("All teams died, mission ended with failure\n");
                m_missionResult = INFO_LOST;
            }
            else
            {
                for (auto it : teams)
                {
                    int team = it.first;
                    if (team == 0) continue;
                    if (!m_objMan->TeamExists(team)) continue;

                    Error result = CheckEndMissionForGroup(it.second);
                    if (result == INFO_LOST || result == INFO_LOSTq)
                    {
                        GetLogger()->Info("Team %d lost\n", team);
                        m_displayText->DisplayText(("<<< Team "+boost::lexical_cast<std::string>(team)+" lost! >>>").c_str(), Math::Vector(0.0f,0.0f,0.0f), 15.0f, 60.0f, 10.0f, Ui::TT_ERROR);

                        m_displayText->SetEnable(false); // To prevent "bot destroyed" messages
                        m_objMan->DestroyTeam(team);
                        m_displayText->SetEnable(true);
                    }
                    else if(result == ERR_OK)
                    {
                        if (m_winDelay == 0.0f)
                        {
                            GetLogger()->Info("Team %d won\n", team);

                            m_displayText->DisplayText(("<<< Team "+boost::lexical_cast<std::string>(team)+" won the game >>>").c_str(), Math::Vector(0.0f,0.0f,0.0f));
                            if (m_missionTimerEnabled && m_missionTimerStarted)
                            {
                                GetLogger()->Info("Mission time: %s\n", TimeFormat(m_missionTimer).c_str());
                                m_displayText->DisplayText(("Time: " + TimeFormat(m_missionTimer)).c_str(), Math::Vector(0.0f,0.0f,0.0f));
                            }
                            m_missionTimerEnabled = m_missionTimerStarted = false;
                            m_winDelay  = m_endTakeWinDelay;  // wins in two seconds
                            m_lostDelay = 0.0f;
                            if (m_exitAfterMission)
                                m_eventQueue->AddEvent(Event(EVENT_QUIT));
                            m_displayText->SetEnable(false);
                        }
                        m_missionResult = ERR_OK;
                        return ERR_OK;
                    }
                }
            }
        }

        if (m_missionResult != INFO_LOST && m_missionResult != INFO_LOSTq)
        {
            if (m_endTakeResearch != 0)
            {
                if (m_endTakeResearch != (m_endTakeResearch&m_researchDone[0]))
                {
                    m_missionResult = ERR_MISSION_NOTERM;
                }
            }
        }
    }

    // Take action depending on m_missionResult

    if(m_missionResult == INFO_LOSTq)
    {
        if (m_lostDelay == 0.0f)
        {
            m_lostDelay = 0.1f;  // lost immediately
            m_winDelay  = 0.0f;
        }
        m_missionTimerEnabled = m_missionTimerStarted = false;
        m_displayText->SetEnable(false);
        if (m_exitAfterMission)
            m_eventQueue->AddEvent(Event(EVENT_QUIT));
        return INFO_LOSTq;
    }

    if(m_missionResult == INFO_LOST)
    {
        if (m_lostDelay == 0.0f)
        {
            m_displayText->DisplayError(INFO_LOST, Math::Vector(0.0f,0.0f,0.0f));
            m_lostDelay = m_endTakeLostDelay;  // lost in 6 seconds
            m_winDelay  = 0.0f;
        }
        m_missionTimerEnabled = m_missionTimerStarted = false;
        m_displayText->SetEnable(false);
        if (m_exitAfterMission)
            m_eventQueue->AddEvent(Event(EVENT_QUIT));
        return INFO_LOST;
    }

    if (m_missionResult == ERR_OK)
    {
        if (m_endTakeWinDelay == -1.0f)
        {
            m_winDelay  = 1.0f;  // wins in one second
            m_lostDelay = 0.0f;
            m_missionTimerEnabled = m_missionTimerStarted = false;
            m_displayText->SetEnable(false);
            if (m_exitAfterMission)
                m_eventQueue->AddEvent(Event(EVENT_QUIT));
            return ERR_OK;  // mission ended
        }

        if (frame && m_base != nullptr && m_base->GetSelectable() && !isImmediat) return ERR_MISSION_NOTERM;

        if (m_winDelay == 0.0f)
        {
            m_displayText->DisplayError(INFO_WIN, Math::Vector(0.0f,0.0f,0.0f));
            if (m_missionTimerEnabled && m_missionTimerStarted)
            {
                GetLogger()->Info("Mission time: %s\n", TimeFormat(m_missionTimer).c_str());
                m_displayText->DisplayText(("Time: " + TimeFormat(m_missionTimer)).c_str(), Math::Vector(0.0f,0.0f,0.0f));
            }
            m_missionTimerEnabled = m_missionTimerStarted = false;
            m_winDelay  = m_endTakeWinDelay;  // wins in two seconds
            m_lostDelay = 0.0f;
        }
        if (m_exitAfterMission)
            m_eventQueue->AddEvent(Event(EVENT_QUIT));
        m_displayText->SetEnable(false);
        return ERR_OK;  // mission ended
    }
    else
    {
        m_displayText->SetEnable(true);
        return ERR_MISSION_NOTERM;
    }
}


//! Returns the number of instructions required
int CRobotMain::GetObligatoryToken()
{
    return m_obligatoryTotal;
}

//! Returns the name of a required instruction
char* CRobotMain::GetObligatoryToken(int i)
{
    return m_obligatoryToken[i];
}

//! Checks if an instruction is part of the obligatory list
int CRobotMain::IsObligatoryToken(const char* token)
{
    for (int i = 0; i < m_obligatoryTotal; i++)
    {
        if (strcmp(token, m_obligatoryToken[i]) == 0)
            return i;
    }
    return -1;
}

//! Checks if an instruction is not part of the banned list
bool CRobotMain::IsProhibitedToken(const char* token)
{
    for (int i = 0; i < m_prohibitedTotal; i++)
    {
        if (strcmp(token, m_prohibitedToken[i]) == 0)
            return false;
    }
    return true;
}


//! Indicates whether it is possible to control a driving robot
bool CRobotMain::GetTrainerPilot()
{
    return m_trainerPilot;
}

//! Indicates whether the scene is fixed, without interaction
bool CRobotMain::GetFixScene()
{
    return m_fixScene;
}


char* CRobotMain::GetTitle()
{
    return m_title;
}

char* CRobotMain::GetResume()
{
    return m_resume;
}

char* CRobotMain::GetScriptName()
{
    return m_scriptName;
}

char* CRobotMain::GetScriptFile()
{
    return m_scriptFile;
}


bool CRobotMain::GetInterfaceGlint()
{
    return m_settings->GetInterfaceGlint();
}

bool CRobotMain::GetSoluce4()
{
    return m_settings->GetSoluce4();
}

bool CRobotMain::GetMovies()
{
    return m_settings->GetMovies();
}

bool CRobotMain::GetNiceReset()
{
    return m_settings->GetNiceReset();
}

bool CRobotMain::GetHimselfDamage()
{
    return m_settings->GetHimselfDamage();
}

bool CRobotMain::GetShowSoluce()
{
    return m_showSoluce;
}

bool CRobotMain::GetSceneSoluce()
{
    if (m_infoFilename[SATCOM_SOLUCE][0] == 0) return false;
    return m_ui->GetSceneSoluce();
}

bool CRobotMain::GetShowAll()
{
    return m_showAll;
}

bool CRobotMain::GetRadar()
{
    if (m_cheatRadar)
        return true;

    for (CObject* obj : m_objMan->GetAllObjects())
    {
        ObjectType type = obj->GetType();
        if (type == OBJECT_RADAR && !obj->GetLock())
            return true;
    }
    return false;
}

MissionType CRobotMain::GetMissionType()
{
    return m_missionType;
}


//! Returns the representation to use for the player
int CRobotMain::GetGamerFace()
{
    return m_playerProfile->GetApperance().face;
}

//! Returns the representation to use for the player
int CRobotMain::GetGamerGlasses()
{
    return m_playerProfile->GetApperance().glasses;
}

//! Returns the mode with just the head
bool CRobotMain::GetGamerOnlyHead()
{
    return m_ui->GetGamerOnlyHead();
}

//! Returns the angle of presentation
float CRobotMain::GetPersoAngle()
{
    return m_ui->GetPersoAngle();
}

void CRobotMain::SetLevel(LevelCategory cat, int chap, int rank)
{
    m_levelCategory = cat;
    m_levelChap = chap;
    m_levelRank = rank;
}

LevelCategory CRobotMain::GetLevelCategory()
{
    return m_levelCategory;
}

int CRobotMain::GetLevelChap()
{
    return m_levelChap;
}

int CRobotMain::GetLevelRank()
{
    return m_levelRank;
}

//! Returns folder name of the scene that user selected to play.
std::string CRobotMain::GetCustomLevelDir()
{
    assert(m_levelCategory == LevelCategory::CustomLevels);
    return m_ui->GetCustomLevelName(m_levelChap);
}

void CRobotMain::SetReadScene(std::string path)
{
    m_sceneReadPath = path;
}

void CRobotMain::UpdateChapterPassed()
{
    return m_ui->UpdateChapterPassed();
}


//! Changes on the pause mode
void CRobotMain::ChangePause(PauseType pause)
{
    if (pause != PAUSE_NONE)
        m_pause->SetPause(pause);
    else
        m_pause->ClearPause();

    m_sound->MuteAll(m_pause->GetPause());
    CreateShortcuts();
    if (m_pause->GetPause()) HiliteClear();
}


//! Changes game speed
void CRobotMain::SetSpeed(float speed)
{
    m_app->SetSimulationSpeed(speed);
    UpdateSpeedLabel();
}

float CRobotMain::GetSpeed()
{
    return m_app->GetSimulationSpeed();
}

void CRobotMain::UpdateSpeedLabel()
{
    Ui::CButton* pb = static_cast<Ui::CButton*>(m_interface->SearchControl(EVENT_SPEED));
    float speed = m_app->GetSimulationSpeed();

    if (pb != nullptr)
    {
        if (speed == 1.0f)
        {
            pb->ClearState(Ui::STATE_VISIBLE);
        }
        else
        {
            char text[10];
            sprintf(text, "x%.1f", speed);
            pb->SetName(text);
            pb->SetState(Ui::STATE_VISIBLE);
        }
    }

}


//! Creates interface shortcuts to the units
bool CRobotMain::CreateShortcuts()
{
    if (m_phase != PHASE_SIMUL) return false;
    if (m_ui->GetLoadingScreen()->IsVisible()) return false;
    if (!m_shortCut) return false;
    return m_short->CreateShortcuts();
}

//! Updates the map
void CRobotMain::UpdateMap()
{
    m_map->UpdateMap();
}

//! Indicates whether the mini-map is visible
bool CRobotMain::GetShowMap()
{
    return m_mapShow;
}


//! Management of the lock mode for movies
void CRobotMain::SetMovieLock(bool lock)
{
    m_movieLock = lock;

    CreateShortcuts();
    m_map->ShowMap(!m_movieLock && m_mapShow && !m_ui->GetLoadingScreen()->IsVisible());
    if (m_movieLock) HiliteClear();
}

bool CRobotMain::GetMovieLock()
{
    return m_movieLock;
}

bool CRobotMain::GetInfoLock()
{
    return m_displayInfo != nullptr;  // info in progress?
}

//! Management of the blocking of the call of SatCom
void CRobotMain::SetSatComLock(bool lock)
{
    m_satComLock = lock;
}

bool CRobotMain::GetSatComLock()
{
    return m_satComLock;
}

//! Management of the lock mode for the edition
void CRobotMain::SetEditLock(bool lock, bool edit)
{
    m_editLock = lock;

    CreateShortcuts();

    // Do not remove the card if it contains a still image.
    if (!lock || !m_map->GetFixImage())
        m_map->ShowMap(!m_editLock && m_mapShow);

    m_displayText->HideText(lock);
    m_input->ResetKeyStates();

    if (m_editLock)
        HiliteClear();
    else
        m_editFull = false;
}

bool CRobotMain::GetEditLock()
{
    return m_editLock;
}

//! Management of the fullscreen mode during editing
void CRobotMain::SetEditFull(bool full)
{
    m_editFull = full;
}

bool CRobotMain::GetEditFull()
{
    return m_editFull;
}


bool CRobotMain::GetFreePhoto()
{
    return m_freePhoto;
}


//! Indicates whether mouse is on an friend object, on which we should not shoot
void CRobotMain::SetFriendAim(bool friendAim)
{
    m_friendAim = friendAim;
}

bool CRobotMain::GetFriendAim()
{
    return m_friendAim;
}


//! Management of the precision of drawing the ground
void CRobotMain::SetTracePrecision(float factor)
{
    m_engine->SetTracePrecision(factor);
}

float CRobotMain::GetTracePrecision()
{
    return m_engine->GetTracePrecision();
}


//! Starts music with a mission
void CRobotMain::StartMusic()
{
    GetLogger()->Debug("Starting music...\n");
    if (m_audioTrack != "")
    {
        m_sound->PlayMusic(m_audioTrack, m_audioRepeat, 0.0f);
    }
}

//! Starts pause music
void CRobotMain::StartPauseMusic(PauseType pause)
{
    switch(pause)
    {
        case PAUSE_EDITOR:
            if (m_editorTrack != "")
                m_sound->PlayPauseMusic(m_editorTrack, m_editorRepeat);
            break;

        case PAUSE_SATCOM:
            if (m_satcomTrack != "")
                m_sound->PlayPauseMusic(m_satcomTrack, m_satcomRepeat);
            break;

        default:
            // Don't change music
            break;
    }
}

//! Removes hilite and tooltip
void CRobotMain::ClearInterface()
{
    HiliteClear();  // removes setting evidence
    m_tooltipName.clear();  // really removes the tooltip
}

void CRobotMain::DisplayError(Error err, CObject* pObj, float time)
{
    m_displayText->DisplayError(err, pObj, time);
}

void CRobotMain::DisplayError(Error err, Math::Vector goal, float height, float dist, float time)
{
    m_displayText->DisplayError(err, goal, height, dist, time);
}

void CRobotMain::UpdateCustomLevelList()
{
    m_ui->UpdateCustomLevelList();
}

std::string CRobotMain::GetCustomLevelName(int id)
{
    return m_ui->GetCustomLevelName(id);
}

const std::vector<std::string>& CRobotMain::GetCustomLevelList()
{
    return m_ui->GetCustomLevelList();
}

void CRobotMain::StartMissionTimer()
{
    if (m_missionTimerEnabled && !m_missionTimerStarted)
    {
        GetLogger()->Info("Starting mission timer...\n");
        m_missionTimerStarted = true;
    }
}

void CRobotMain::SetAutosave(bool enable)
{
    if (m_autosave == enable) return;

    m_autosave = enable;
    m_autosaveLast = m_gameTimeAbsolute;
    AutosaveRotate(false);
}

bool CRobotMain::GetAutosave()
{
    return m_autosave;
}

void CRobotMain::SetAutosaveInterval(int interval)
{
    if (m_autosaveInterval == interval) return;

    m_autosaveInterval = interval;
    m_autosaveLast = m_gameTimeAbsolute;
}

int CRobotMain::GetAutosaveInterval()
{
    return m_autosaveInterval;
}

void CRobotMain::SetAutosaveSlots(int slots)
{
    if (m_autosaveSlots == slots) return;

    m_autosaveSlots = slots;
    AutosaveRotate(false);
}

int CRobotMain::GetAutosaveSlots()
{
    return m_autosaveSlots;
}

int CRobotMain::AutosaveRotate(bool freeOne)
{
    GetLogger()->Debug("Rotate autosaves...\n");
    // Find autosave dirs
    auto saveDirs = CResourceManager::ListDirectories(m_playerProfile->GetSaveDir());
    std::map<int, std::string> autosaveDirs;
    for (auto& dir : saveDirs)
    {
        try
        {
            const std::string autosavePrefix = "autosave";
            if (dir.substr(0, autosavePrefix.length()) == "autosave")
            {
                int id = boost::lexical_cast<int>(dir.substr(autosavePrefix.length()));
                autosaveDirs[id] = m_playerProfile->GetSaveFile(dir);
            }
        }
        catch (...)
        {
            GetLogger()->Info("Bad autosave found: %s\n", dir.c_str());
            // skip
        }
    }
    if (autosaveDirs.size() == 0) return 1;

    // Remove all but last m_autosaveSlots
    std::map<int, std::string> autosavesToKeep;
    int last_id = autosaveDirs.rbegin()->first;
    int count = 0;
    int to_keep = m_autosaveSlots-(freeOne ? 1 : 0);
    int new_last_id = Math::Min(autosaveDirs.size(), to_keep);
    bool rotate = false;
    for (int i = last_id; i > 0; i--)
    {
        if (autosaveDirs.count(i) > 0)
        {
            count++;
            if (count > m_autosaveSlots-(freeOne ? 1 : 0) || !m_autosave)
            {
                GetLogger()->Trace("Remove %s\n", autosaveDirs[i].c_str());
                CResourceManager::RemoveDirectory(autosaveDirs[i]);
                rotate = true;
            }
            else
            {
                GetLogger()->Trace("Keep %s\n", autosaveDirs[i].c_str());
                autosavesToKeep[new_last_id-count+1] = autosaveDirs[i];
            }
        }
    }

    // Rename autosaves that we kept
    if (rotate)
    {
        for (auto& save : autosavesToKeep)
        {
            std::string newDir = m_playerProfile->GetSaveFile("autosave" + boost::lexical_cast<std::string>(save.first));
            GetLogger()->Trace("Rename %s -> %s\n", save.second.c_str(), newDir.c_str());
            CResourceManager::Move(save.second, newDir);
        }
    }

    return rotate ? count : count+1;
}

void CRobotMain::Autosave()
{
    int id = AutosaveRotate(true);
    GetLogger()->Info("Autosave!\n");

    std::string dir = m_playerProfile->GetSaveFile("autosave" + boost::lexical_cast<std::string>(id));

    char timestr[100];
    TimeToAscii(time(NULL), timestr);
    std::string info = std::string("[AUTOSAVE] ")+timestr;

    m_playerProfile->SaveScene(dir, info);
}

void CRobotMain::SetExitAfterMission(bool exit)
{
    m_exitAfterMission = exit;
}

bool CRobotMain::CanPlayerInteract()
{
    if(GetMissionType() == MISSION_CODE_BATTLE)
    {
        return !m_codeBattleStarted;
    }
    return true;
}

const std::string NO_TEAM_NAME = "Team";
const std::string& CRobotMain::GetTeamName(int id)
{
    if(m_teamNames.count(id) == 0) return NO_TEAM_NAME;
    return m_teamNames[id];
}

bool CRobotMain::IsTeamColorDefined(int id)
{
    if(id == 0) return false; // Always use default for team 0

    return m_colorNewBot.find(id) != m_colorNewBot.end();
}


int CRobotMain::GetEnableBuild()
{
    return m_build;
}

void CRobotMain::SetEnableBuild(int enableBuild)
{
    m_build = enableBuild;
}

int CRobotMain::GetEnableResearch()
{
    return m_researchEnable;
}

void CRobotMain::SetEnableResearch(int enableResearch)
{
    m_researchEnable = enableResearch;
}

int CRobotMain::GetDoneResearch(int team)
{
    return m_researchDone[team];
}

void CRobotMain::SetDoneResearch(int doneResearch, int team)
{
    m_researchDone[team] = doneResearch;
}


bool CRobotMain::IsBuildingEnabled(BuildType type)
{
    return (m_build & type) != 0;
}

bool CRobotMain::IsBuildingEnabled(ObjectType type)
{
    if(type == OBJECT_DERRICK) return IsBuildingEnabled(BUILD_DERRICK);
    if(type == OBJECT_FACTORY) return IsBuildingEnabled(BUILD_FACTORY);
    if(type == OBJECT_STATION) return IsBuildingEnabled(BUILD_STATION);
    if(type == OBJECT_CONVERT) return IsBuildingEnabled(BUILD_CONVERT);
    if(type == OBJECT_REPAIR) return IsBuildingEnabled(BUILD_REPAIR);
    if(type == OBJECT_TOWER) return IsBuildingEnabled(BUILD_TOWER);
    if(type == OBJECT_RESEARCH) return IsBuildingEnabled(BUILD_RESEARCH);
    if(type == OBJECT_RADAR) return IsBuildingEnabled(BUILD_RADAR);
    if(type == OBJECT_ENERGY) return IsBuildingEnabled(BUILD_ENERGY);
    if(type == OBJECT_LABO) return IsBuildingEnabled(BUILD_LABO);
    if(type == OBJECT_NUCLEAR) return IsBuildingEnabled(BUILD_NUCLEAR);
    if(type == OBJECT_INFO) return IsBuildingEnabled(BUILD_INFO);
    if(type == OBJECT_PARA) return IsBuildingEnabled(BUILD_PARA);
    if(type == OBJECT_DESTROYER) return IsBuildingEnabled(BUILD_DESTROYER);

    return true;
}

bool CRobotMain::IsResearchEnabled(ResearchType type)
{
    return (m_researchEnable & type) != 0;
}

bool CRobotMain::IsResearchDone(ResearchType type, int team)
{
    if(team != 0 && m_researchDone.count(team) == 0)
    {
        // Initialize with defaults
        m_researchDone[team] = m_researchDone[0];
    }

    return (m_researchDone[team] & type) != 0;
}

void CRobotMain::MarkResearchDone(ResearchType type, int team)
{
    if(team != 0 && m_researchDone.count(team) == 0)
    {
        // Initialize with defaults
        m_researchDone[team] = m_researchDone[0];
    }

    m_researchDone[team] |= type;

    if(team == 0)
    {
        m_playerProfile->SetFreeGameResearchUnlock(m_playerProfile->GetFreeGameResearchUnlock() | m_researchDone[0]);
    }
}

Error CRobotMain::CanBuildError(ObjectType type, int team)
{
    if(!IsBuildingEnabled(type)) return ERR_BUILD_DISABLED;

    if(type == OBJECT_TOWER  && !IsResearchDone(RESEARCH_TOWER,  team)) return ERR_BUILD_RESEARCH;
    if(type == OBJECT_ATOMIC && !IsResearchDone(RESEARCH_ATOMIC, team)) return ERR_BUILD_RESEARCH;

    return ERR_OK;
}

bool CRobotMain::CanBuild(ObjectType type, int team)
{
    return CanBuildError(type, team) == ERR_OK;
}

Error CRobotMain::CanFactoryError(ObjectType type, int team)
{
    ToolType tool = GetToolFromObject(type);
    DriveType drive = GetDriveFromObject(type);

    if (tool == ToolType::Sniffer        && !IsResearchDone(RESEARCH_SNIFFER,  team)) return ERR_BUILD_RESEARCH;
    if (tool == ToolType::Shooter        && !IsResearchDone(RESEARCH_CANON,    team)) return ERR_BUILD_RESEARCH;
    if (tool == ToolType::OrganicShooter && !IsResearchDone(RESEARCH_iGUN,     team)) return ERR_BUILD_RESEARCH;

    if (drive == DriveType::Tracked      && !IsResearchDone(RESEARCH_TANK,     team)) return ERR_BUILD_RESEARCH;
    if (drive == DriveType::Winged       && !IsResearchDone(RESEARCH_FLY,      team)) return ERR_BUILD_RESEARCH;
    if (drive == DriveType::Legged       && !IsResearchDone(RESEARCH_iPAW,     team)) return ERR_BUILD_RESEARCH;
    if (drive == DriveType::BigTracked   && !IsResearchDone(RESEARCH_TANK,     team)) return ERR_BUILD_RESEARCH; // NOTE: Subber is not BigTracked! It currently counts as Other

    if (type == OBJECT_MOBILErt          && !IsResearchDone(RESEARCH_THUMP,    team)) return ERR_BUILD_RESEARCH;
    if (type == OBJECT_MOBILErc          && !IsResearchDone(RESEARCH_PHAZER,   team)) return ERR_BUILD_RESEARCH;
    if (type == OBJECT_MOBILErr          && !IsResearchDone(RESEARCH_RECYCLER, team)) return ERR_BUILD_RESEARCH;
    if (type == OBJECT_MOBILErs          && !IsResearchDone(RESEARCH_SHIELD,   team)) return ERR_BUILD_RESEARCH;
    if (type == OBJECT_MOBILEsa          && !IsResearchDone(RESEARCH_SUBM,     team)) return ERR_BUILD_DISABLED; // Can be only researched manually in Scene file

    return ERR_OK;
}

bool CRobotMain::CanFactory(ObjectType type, int team)
{
    return CanFactoryError(type, team) == ERR_OK;
}

void CRobotMain::PushToSelectionHistory(CObject* obj)
{
    if (!m_selectionHistory.empty() && m_selectionHistory.back() == obj)
        return; // already in history

    m_selectionHistory.push_back(obj);

    if (m_selectionHistory.size() > 50) // to avoid infinite growth
        m_selectionHistory.pop_front();
}

CObject* CRobotMain::PopFromSelectionHistory()
{
    if (m_selectionHistory.empty())
        return nullptr;

    CObject* obj = m_selectionHistory.back();
    m_selectionHistory.pop_back();
    return obj;
}

void CRobotMain::RemoveFromSelectionHistory(CObject* object)
{
    auto it = std::remove_if(m_selectionHistory.begin(), m_selectionHistory.end(),
                             [object](const CObject* obj) { return obj == object; });
    m_selectionHistory.erase(it, m_selectionHistory.end());
}

float CRobotMain::GetGlobalMagnifyDamage()
{
    return m_globalMagnifyDamage;
}
