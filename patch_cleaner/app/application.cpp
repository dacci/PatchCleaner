// Copyright (c) 2016 dacci.org

#include "app/application.h"

#include "ui/main_frame.h"

namespace patch_cleaner {
namespace app {

Application::Application() : message_loop_(nullptr), frame_(nullptr) {}

Application::~Application() {}

bool Application::ParseCommandLine(LPCTSTR /*command_line*/,
                                   HRESULT* result) throw() {
  *result = S_OK;

  return true;
}

HRESULT Application::PreMessageLoop(int show_mode) throw() {
  auto result = CAtlExeModuleT::PreMessageLoop(show_mode);
  if (FAILED(result)) {
    return result;
  }

  if (!AtlInitCommonControls(0xFFFF)) {  // all classes
    return S_FALSE;
  }

  message_loop_ = new CMessageLoop();
  if (message_loop_ == nullptr) {
    return S_FALSE;
  }

  frame_ = new ui::MainFrame();
  if (frame_ == nullptr) {
    return S_FALSE;
  }

  if (frame_->CreateEx() == NULL) {
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
