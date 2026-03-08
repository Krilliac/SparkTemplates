/**
 * @file SparkEngine.h
 * @brief SparkEngine executable — the runtime host for game modules
 *
 * SparkEngine is the executable that hosts the runtime. It initialises
 * core systems (graphics, input, audio) and loads game modules dynamically.
 * All game-specific logic lives in separately compiled modules (DLLs / .so).
 *
 * DEPRECATION NOTICE
 * ------------------
 * The global extern pointers declared below are provided only for backward
 * compatibility. New code should use EngineContext / IEngineContext instead
 * to access engine subsystems.
 */

#pragma once

#include "Core/Platform.h"

// Forward-declare engine subsystems
class GraphicsEngine;
class InputManager;
class Timer;
class GameModuleLoader;

#ifdef SPARK_PLATFORM_WINDOWS
extern HINSTANCE g_hInst;
#endif

// ---------------------------------------------------------------------------
// Deprecated global accessors — use EngineContext instead
// ---------------------------------------------------------------------------

/// @deprecated Use EngineContext::GetGraphics()
[[deprecated("Use EngineContext / IEngineContext instead")]]
inline GraphicsEngine* SparkGetGlobalGraphics();

/// @deprecated Use EngineContext::GetInput()
[[deprecated("Use EngineContext / IEngineContext instead")]]
inline InputManager* SparkGetGlobalInput();

/// @deprecated Use EngineContext::GetTimer()
[[deprecated("Use EngineContext / IEngineContext instead")]]
inline Timer* SparkGetGlobalTimer();

// The actual globals (still needed for InitInstance / WndProc linkage)
extern std::unique_ptr<GraphicsEngine> g_graphics;
extern std::unique_ptr<InputManager>   g_input;
extern std::unique_ptr<Timer>          g_timer;
extern std::unique_ptr<GameModuleLoader> g_moduleLoader;

#ifdef SPARK_HEADLESS_SUPPORT
/// True when the engine was launched with -headless or -dedicated
extern bool g_headlessMode;
#endif
