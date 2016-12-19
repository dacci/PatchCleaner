// Copyright (c) 2016 dacci.org

#include "app/application.h"

#include <base/command_line.h>
#include <base/logging.h>

#include "ui/main_frame.h"

namespace patch_cleaner {
namespace app {

Application::Application() : message_loop_(nullptr), frame_(nullptr) {}

Application::~Application() {
  base::CommandLine::Reset();
}

bool Application::ParseCommandLine(LPCTSTR /*command_line*/,
                                   HRESULT* result) throw() {
  *result = S_OK;

  auto succeeded = base::CommandLine::Init(0, nullptr);
  ATLASSERT(succeeded);
  if (!succeeded) {
    *result = E_FAIL;
    OutputDebugString(L"Failed to initialize command line.\n");
    return false;
  }

  logging::LoggingSettings logging_settings;
  logging_settings.logging_dest = logging::LOG_TO_SYSTEM_DEBUG_LOG;
  succeeded = logging::InitLogging(logging_settings);
  ATLASSERT(succeeded);
  if (!succeeded) {
    *result = E_FAIL;
    OutputDebugString(L"Failed to initialize logging.\n");
    return false;
  }

  return true;
}

HRESULT Application::PreMessageLoop(int show_mode) throw() {
  auto result = CAtlExeModuleT::PreMessageLoop(show_mode);
  if (FAILED(result)) {
    LOG(ERROR) << "CAtlExeModuleT::PreMessageLoop() returned: 0x" << std::hex
               << result;
    return result;
  }

  if (!AtlInitCommonControls(0xFFFF)) {  // all classes
    LOG(ERROR) << "Failed to initialize common controls.";
    return S_FALSE;
  }

  message_loop_ = new CMessageLoop();
  if (message_loop_ == nullptr) {
    LOG(ERROR) << "Failed to allocate message loop.";
    return S_FALSE;
  }

  frame_ = new ui::MainFrame();
  if (frame_ == nullptr) {
    LOG(ERROR) << "Failed to allocate main frame.";
    return S_FALSE;
  }

  if (frame_->CreateEx() == NULL) {
    LOG(ERROR) << "Failed to create main frame.";
    return S_FALSE;
  }

  frame_->ResizeClient(370, 280, FALSE);
  frame_->CenterWindow();
  frame_->ShowWindow(show_mode);
  frame_->UpdateWindow();

  return S_OK;
}

HRESULT Application::PostMessageLoop() throw() {
  if (frame_ != nullptr) {
    delete frame_;
    frame_ = nullptr;
  }

  if (message_loop_ != nullptr) {
    delete message_loop_;
    message_loop_ = nullptr;
  }

  return CAtlExeModuleT::PostMessageLoop();
}

void Application::RunMessageLoop() throw() {
  CHECK(message_loop_ != nullptr) << "Something wrong.";
  message_loop_->Run();
}

}  // namespace app
}  // namespace patch_cleaner

int __stdcall wWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/,
                       wchar_t* /*command_line*/, int show_mode) {
  HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, nullptr, 0);

  SetSearchPathMode(BASE_SEARCH_PATH_ENABLE_SAFE_SEARCHMODE |
                    BASE_SEARCH_PATH_PERMANENT);
  SetDllDirectory(L"");

#ifdef _DEBUG
  {
    auto flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    flags |=
        _CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF;
    _CrtSetDbgFlag(flags);
  }
#endif

  return patch_cleaner::app::Application().WinMain(show_mode);
}
