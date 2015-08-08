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
 * \file app/app.h
 * \brief CApplication class
 */

#pragma once

#include "common/config_file.h"
#include "common/event.h"
#include "common/global.h"
#include "common/singleton.h"

#include "graphics/core/device.h"

#include "object/level_category.h"


#include <string>
#include <vector>


class CEventQueue;
class CController;
class CSoundInterface;
class CInput;
class CPathManager;
class CSystemUtils;
struct SystemTimeStamp;
class CSDLMutexWrapper;
class CSignal;
class CPerformanceCounters;

namespace Gfx
{
class CEngine;
}

/**
 * \struct JoystickDevice
 * \brief Information about a joystick device
 */
struct JoystickDevice
{
    //! Device index (-1 = invalid device)
    int index;
    //! Device name
    std::string name;
    //! Number of axes (only available after joystick opened)
    int axisCount;
    //! Number of buttons (only available after joystick opened)
    int buttonCount;

    JoystickDevice()
        : index(-1), axisCount(0), buttonCount(0) {}
};

/**
 * \enum VideoQueryResult
 * \brief Result of querying for available video resolutions
 */
enum VideoQueryResult
{
    VIDEO_QUERY_ERROR,
    VIDEO_QUERY_NONE,
    VIDEO_QUERY_ALL,
    VIDEO_QUERY_OK
};

/**
 * \enum ParseArgsStatus
 * \brief State of parsing commandline arguments
 */
enum ParseArgsStatus
{
    PARSE_ARGS_OK   = 1, //! < all ok
    PARSE_ARGS_FAIL = 2, //! < invalid syntax
    PARSE_ARGS_HELP = 3  //! < -help requested
};

/**
 * \enum MouseMode
 * \brief Mode of mouse cursor
 */
enum MouseMode
{
    MOUSE_SYSTEM, //! < system cursor visible; in-game cursor hidden
    MOUSE_ENGINE, //! < in-game cursor visible; system cursor hidden
    MOUSE_BOTH,   //! < both cursors visible (only for debug)
    MOUSE_NONE,   //! < no cursor visible
};

enum DebugMode
{
    DEBUG_SYS_EVENTS = 1 << 0,
    DEBUG_APP_EVENTS = 1 << 1,
    DEBUG_EVENTS     = DEBUG_SYS_EVENTS | DEBUG_APP_EVENTS,
    DEBUG_MODELS     = 1 << 2,
    DEBUG_ALL        = DEBUG_SYS_EVENTS | DEBUG_APP_EVENTS | DEBUG_MODELS
};

struct ApplicationPrivate;

/**
 * \class CApplication
 * \brief Main application
 *
 * This class is responsible for main application execution, including creating
 * and handling main application window, receiving events, etc.
 *
 * It is a singleton class with only one instance that can be created.
 *
 * \section Creation Creation of other main objects
 *
 * The class creates the only instance of CEventQueue, CEngine,
 * CRobotMain and CSoundInterface classes.
 *
 * \section Window Window management
 *
 * The class is responsible for creating app window, setting and changing the video mode,
 * joystick management, grabbing input and changing the system mouse cursor
 * position and visibility.
 * ("System mouse cursor" means the cursor displayed by the OS in constrast to the cursor
 * displayed by CEngine).
 *
 * \section Events Events
 *
 * Events are taken from SDL event queue, translated to common events from src/common.h
 * and pushed to global event queue CEventQueue.
 *
 * Joystick events are generated somewhat differently, by running a separate timer,
 * polling the device for changes and synthesising events on change. It avoids flooding
 * the event queue with too many joystick events and the granularity of the timer can be
 * adjusted.
 *
 * The events are passed to ProcessEvent() of classes in this order: CApplication, CEngine
 * and CRobotMain. CApplication and CEngine's ProcessEvent() functions return bool, which
 * means whether to pass the event on, or stop the chain. This is to enable handling some
 * events which are internal to CApplication or CEngine.
 *
 * \section Portability Portability
 *
 * Currently, the class only handles OpenGL devices. SDL can be used with DirectX, but
 * for that to work, video initialization and video setting must be done differently.
 *
 */
class CApplication : public CSingleton<CApplication>
{
public:
    //! Constructor (can only be called once!)
    CApplication(CSystemUtils* systemUtils);
    //! Destructor
    ~CApplication();

    //! Returns the application's event queue
    CEventQueue* GetEventQueue();
    //! Returns the sound subsystem
    CSoundInterface* GetSound();
    //! Returns the performance counters
    CPerformanceCounters* GetPerformanceCounters();

public:
    //! Parses commandline arguments
    ParseArgsStatus ParseArguments(int argc, char *argv[]);
    //! Initializes the application
    bool        Create();
    //! Main event loop
    int         Run();
    //! Returns the code to be returned at main() exit
    int         GetExitCode() const;

    //! Returns the message of error (set to something if exit code is not 0)
    const std::string& GetErrorMessage() const;

    //! Restart
    void        Restart();
    //! Should we restart after app quits?
    bool        IsRestarting();

    //! Returns a list of possible video modes
    VideoQueryResult GetVideoResolutionList(std::vector<Math::IntPoint> &resolutions,
                                            bool fullScreen, bool resizeable) const;

    //! Returns the current video mode
    Gfx::DeviceConfig GetVideoConfig() const;

    //! Change the video mode to given mode
    bool        ChangeVideoConfig(const Gfx::DeviceConfig &newConfig);

    //! Allows next frame to be rendered. This is to be called from the update thread.
    void        RenderNextFrame();

    //! Suspends animation (time will not be updated)
    void        SuspendSimulation();
    //! Resumes animation
    void        ResumeSimulation();
    //! Returns whether simulation is suspended
    bool        GetSimulationSuspended() const;

    //! Resets time counters to account for time spent loading game
    void        ResetTimeAfterLoading();

    //@{
    //! Management of simulation speed
    void            SetSimulationSpeed(float speed);
    float           GetSimulationSpeed() const;
    //@}

    //! Returns the absolute time counter [seconds]
    float       GetAbsTime() const;
    //! Returns the exact absolute time counter [nanoseconds]
    long long   GetExactAbsTime() const;

    //! Returns the exact absolute time counter disregarding speed setting [nanoseconds]
    long long   GetRealAbsTime() const;

    //! Returns the relative time since last update [seconds]
    float       GetRelTime() const;
    //! Returns the exact realative time since last update [nanoseconds]
    long long   GetExactRelTime() const;

    //! Returns the exact relative time since last update disregarding speed setting [nanoseconds]
    long long   GetRealRelTime() const;

    //! Returns a list of available joystick devices
    std::vector<JoystickDevice> GetJoystickList() const;

    //! Returns info about the current joystick
    JoystickDevice GetJoystick() const;

    //! Change the current joystick device
    bool        ChangeJoystick(const JoystickDevice &newJoystick);

    //! Management of joystick enable state
    //@{
    void        SetJoystickEnabled(bool enable);
    bool        GetJoystickEnabled() const;
    //@}

    //! Polls the state of joystick axes and buttons
    void        UpdateJoystick();

    //! Updates the mouse position explicitly
    void        UpdateMouse();

    //! Management of the grab mode for input (keyboard & mouse)
    //@{
    void        SetGrabInput(bool grab);
    bool        GetGrabInput() const;
    //@}

    //! Management of mouse mode
    //@{
    void        SetMouseMode(MouseMode mode);
    MouseMode   GetMouseMode() const;
    //@}

    //! Moves (warps) the mouse cursor to the specified position (in interface coords)
    void        MoveMouse(Math::Point pos);

    //! Management of debug modes (printing more info in logger)
    //@{
    void        SetDebugModeActive(DebugMode mode, bool active);
    bool        IsDebugModeActive(DebugMode mode) const;
    static bool ParseDebugModes(const std::string& str, int& debugModes);
    //@}

    //! Management of language
    //@{
    Language    GetLanguage() const;
    char        GetLanguageChar() const;
    void        SetLanguage(Language language);
    static bool ParseLanguage(const std::string& str, Language& language);
    //@}

    //! Management of sleep in main loop (lowers CPU usage)
    //@{
    void        SetLowCPU(bool low);
    bool        GetLowCPU() const;
    //@}

    bool        GetSceneTestMode();

protected:
    //! Creates the window's SDL_Surface
    bool CreateVideoSurface();

    //! Update thread function
    void RunUpdateThread();

    //! Execute one update
    void ExecuteUpdate();

    //! Processes the captured SDL event to Event struct
    Event       ProcessSystemEvent();
    //! If applicable, creates a virtual event to match the changed state as of new event
    Event       CreateVirtualEvent(const Event& sourceEvent);
    //! Prepares a simulation update event
    TEST_VIRTUAL Event CreateUpdateEvent();
    //! Logs debug data for event
    void        LogEvent(const Event& event);

    //! Opens the joystick device
    bool OpenJoystick();
    //! Closes the joystick device
    void CloseJoystick();

    //! Internal procedure to reset time counters
    void InternalResumeSimulation();

protected:
    //! System utils instance
    CSystemUtils* m_systemUtils;
    //! Private (SDL-dependent data)
    std::unique_ptr<ApplicationPrivate> m_private;
    //! Global event queue
    std::unique_ptr<CEventQueue> m_eventQueue;
    //! Graphics engine
    std::unique_ptr<Gfx::CEngine> m_engine;
    //! Graphics device
    std::unique_ptr<Gfx::CDevice> m_device;
    //! Sound subsystem
    std::unique_ptr<CSoundInterface> m_sound;
    //! Game controller - game engine and UI
    std::unique_ptr<CController> m_controller;
    //! Profile (INI) reader/writer
    std::unique_ptr<CConfigFile> m_configFile;
    //! Input manager
    std::unique_ptr<CInput> m_input;
    //! Path manager
    std::unique_ptr<CPathManager> m_pathManager;
    //! Performance counters
    std::unique_ptr<CPerformanceCounters> m_performanceCounters;

    //! Code to return at exit
    int             m_exitCode;
    //! Whether application window is active
    bool            m_active;
    //! Bit array of active debug modes
    long            m_debugModes;
    //! If we are restarting the app
    bool            m_restart;
    //! false on exit
    bool            m_running;
    //! Update thread mutex
    std::unique_ptr<CSDLMutexWrapper> m_updateMutex;
    //! Start frame signal
    std::unique_ptr<CSignal> m_startFrameSignal;
    //! Finished frame signal
    std::unique_ptr<CSignal> m_finishedFrameSignal;

    //! Message to be displayed as error to the user
    std::string     m_errorMessage;

    //! Current configuration of OpenGL display device
    Gfx::DeviceConfig m_deviceConfig;
    //! Previous configuration of OpenGL display device
    Gfx::DeviceConfig m_lastDeviceConfig;

    //! Text set as window title
    std::string     m_windowTitle;

    //! Animation time stamps, etc.
    //@{
    SystemTimeStamp* m_baseTimeStamp;
    SystemTimeStamp* m_lastTimeStamp;
    SystemTimeStamp* m_curTimeStamp;

    long long       m_realAbsTimeBase;
    long long       m_realAbsTime;
    long long       m_realRelTime;

    long long       m_absTimeBase;
    long long       m_exactAbsTime;
    long long       m_exactRelTime;

    float           m_absTime;
    float           m_relTime;

    float           m_simulationSpeed;
    bool            m_simulationSuspended;
    //@}

    //! Graphics device to use
    std::string     m_graphics;

    //! Current mode of mouse
    MouseMode       m_mouseMode;

    //! Info about current joystick device
    JoystickDevice  m_joystick;
    //! Whether joystick is enabled
    bool            m_joystickEnabled;
    //! Current state of joystick axes; may be updated from another thread
    std::vector<int> m_joyAxeState;
    //! Current state of joystick buttons; may be updated from another thread
    std::vector<bool> m_joyButtonState;

    //@{
    //! Scene to run on startup
    LevelCategory   m_runSceneCategory;
    int             m_runSceneRank;
    //@}

    //! Scene test mode
    bool            m_sceneTest;

    //! Application language
    Language        m_language;

    //! Low cpu mode
    bool            m_lowCPU;

    //! Screen resoultion overriden by commandline
    bool            m_resolutionOverride;

    //! Headles mode
    bool            m_headless;

    //! Static buffer for putenv locale
    static char m_languageLocale[50];
};
