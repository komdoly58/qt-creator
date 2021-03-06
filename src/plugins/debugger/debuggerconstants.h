/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef DEBUGGERCONSTANTS_H
#define DEBUGGERCONSTANTS_H

#include <QtCore/QFlags>

namespace Debugger {
namespace Constants {

// Debug mode
const char * const MODE_DEBUG           = "Debugger.Mode.Debug";

// Contexts
const char * const C_DEBUGMODE          = "Debugger.DebugMode";
const char * const C_CPPDEBUGGER        = "Gdb Debugger";
const char * const C_QMLDEBUGGER        = "Qml/JavaScript Debugger";

// Project Explorer run mode (RUN/DEBUG)
const char * const DEBUGMODE            = "Debugger.DebugMode";
const char * const DEBUGMODE2           = "Debugger.DebugMode2";

// Common actions (accessed by QML inspector)
const char * const INTERRUPT            = "Debugger.Interrupt";
const char * const CONTINUE             = "Debugger.Continue";
const char * const STOP                 = "Debugger.Stop";
const char * const RESET                = "Debugger.Reset";
const char * const STEP                 = "Debugger.StepLine";
const char * const STEPOUT              = "Debugger.StepOut";
const char * const NEXT                 = "Debugger.NextLine";
const char * const REVERSE              = "Debugger.ReverseDirection";
const char * const OPERATE_BY_INSTRUCTION   = "Debugger.OperateByInstruction";

// DebuggerMainWindow dock widget names
const char * const DOCKWIDGET_BREAK      = "Debugger.Docks.Break";
const char * const DOCKWIDGET_MODULES    = "Debugger.Docks.Modules";
const char * const DOCKWIDGET_REGISTER   = "Debugger.Docks.Register";
const char * const DOCKWIDGET_OUTPUT     = "Debugger.Docks.Output";
const char * const DOCKWIDGET_SNAPSHOTS  = "Debugger.Docks.Snapshots";
const char * const DOCKWIDGET_STACK      = "Debugger.Docks.Stack";
const char * const DOCKWIDGET_SOURCE_FILES = "Debugger.Docks.SourceFiles";
const char * const DOCKWIDGET_THREADS    = "Debugger.Docks.Threads";
const char * const DOCKWIDGET_WATCHERS   = "Debugger.Docks.LocalsAndWatchers";

const char * const DOCKWIDGET_QML_INSPECTOR = "Debugger.Docks.QmlInspector";
const char * const DOCKWIDGET_QML_SCRIPTCONSOLE = "Debugger.Docks.ScriptConsole";
const char * const DOCKWIDGET_DEFAULT_AREA = "Debugger.Docks.DefaultArea";

} // namespace Constants

enum DebuggerState
{
    DebuggerNotReady,          // Debugger not started

    EngineSetupRequested,      // Engine starts
    EngineSetupFailed,
    EngineSetupOk,

    InferiorSetupRequested,
    InferiorSetupFailed,
    InferiorSetupOk,

    EngineRunRequested,
    EngineRunFailed,

    InferiorUnrunnable,        // Used in the core dump adapter

    InferiorRunRequested,      // Debuggee requested to run
    InferiorRunOk,             // Debuggee running
    InferiorRunFailed,         // Debuggee running

    InferiorStopRequested,     // Debuggee running, stop requested
    InferiorStopOk,            // Debuggee stopped
    InferiorStopFailed,        // Debuggee not stopped, will kill debugger

    InferiorExitOk,

    InferiorShutdownRequested,
    InferiorShutdownFailed,
    InferiorShutdownOk,

    EngineShutdownRequested,
    EngineShutdownFailed,
    EngineShutdownOk,

    DebuggerFinished
};

enum DebuggerStartMode
{
    NoStartMode,
    StartInternal,         // Start current start project's binary
    StartExternal,         // Start binary found in file system
    AttachExternal,        // Attach to running process by process id
    AttachCrashedExternal, // Attach to crashed process by process id
    AttachCore,            // Attach to a core file
    AttachToRemote,        // Start and attach to a remote process
    StartRemoteGdb,        // Start gdb itself remotely
    StartRemoteEngine      // Start ipc guest engine on other machine
};

enum DebuggerCapabilities
{
    ReverseSteppingCapability = 0x1,
    SnapshotCapability = 0x2,
    AutoDerefPointersCapability = 0x4,
    DisassemblerCapability = 0x8,
    RegisterCapability = 0x10,
    ShowMemoryCapability = 0x20,
    JumpToLineCapability = 0x40,
    ReloadModuleCapability = 0x80,
    ReloadModuleSymbolsCapability = 0x100,
    BreakOnThrowAndCatchCapability = 0x200,
    BreakConditionCapability = 0x400, //!< Conditional Breakpoints
    BreakModuleCapability = 0x800, //!< Breakpoint specification includes module
    TracePointCapability = 0x1000,
    ReturnFromFunctionCapability = 0x2000,
    CreateFullBacktraceCapability = 0x4000,
    AddWatcherCapability = 0x8000,
    WatchpointByAddressCapability = 0x10000,
    WatchpointByExpressionCapability = 0x20000,
    ShowModuleSymbolsCapability = 0x40000,
    CatchCapability = 0x80000, //!< fork, vfork, syscall
    OperateByInstructionCapability = 0x100000,
    RunToLineCapability = 0x200000,
    AllDebuggerCapabilities = 0xFFFFFFFF
};

enum LogChannel
{
    LogInput,                // Used for user input
    LogMiscInput,            // Used for misc stuff in the input pane
    LogOutput,
    LogWarning,
    LogError,
    LogStatus,               // Used for status changed messages
    LogTime,                 // Used for time stamp messages
    LogDebug,
    LogMisc,
    AppOutput,               // stdout
    AppError,                // stderr
    AppStuff,                // (possibly) windows debug channel
    StatusBar,                // LogStatus and also put to the status bar
    ScriptConsoleOutput
};

enum DebuggerEngineType
{
    NoEngineType      = 0,
    GdbEngineType     = 0x01,
    ScriptEngineType  = 0x02,
    CdbEngineType     = 0x04,
    PdbEngineType     = 0x08,
    QmlEngineType     = 0x20,
    QmlCppEngineType  = 0x40,
    LldbEngineType  = 0x80,
    AllEngineTypes = GdbEngineType
        | ScriptEngineType
        | CdbEngineType
        | PdbEngineType
        | QmlEngineType
        | QmlCppEngineType
        | LldbEngineType
};

enum DebuggerLanguage
{
    AnyLanguage       = 0x0,
    CppLanguage      = 0x1,
    QmlLanguage      = 0x2
};
Q_DECLARE_FLAGS(DebuggerLanguages, DebuggerLanguage)

} // namespace Debugger

#endif // DEBUGGERCONSTANTS_H
