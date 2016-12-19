// Copyright (c) 2016 dacci.org

#ifndef PATCH_CLEANER_UI_MAIN_FRAME_H_
#define PATCH_CLEANER_UI_MAIN_FRAME_H_

#include <stdint.h>

#include <atlbase.h>
#include <atlwin.h>

#include <atlapp.h>
#include <atlcrack.h>
#include <atlctrls.h>
#include <atlframe.h>

#include "res/resource.h"

namespace patch_cleaner {
namespace ui {

class MainFrame : public CFrameWindowImpl<MainFrame>, private CMessageFilter {
 public:
  MainFrame();

  DECLARE_FRAME_WND_CLASS(L"MainFrame", IDR_MAIN)

  BEGIN_MSG_MAP(MainFrame)
    MSG_WM_CREATE(OnCreate)
    MSG_WM_DESTROY(OnDestroy)

    NOTIFY_HANDLER_EX(IDC_FILE_LIST, LVN_ITEMCHANGED, OnItemChanged)
    NOTIFY_HANDLER_EX(IDC_FILE_LIST, LVN_DELETEITEM, OnDeleteItem)

    COMMAND_ID_HANDLER_EX(ID_FILE_UPDATE, OnFileUpdate)
    COMMAND_ID_HANDLER_EX(ID_EDIT_SELECT_ALL, OnEditSelectAll)
    COMMAND_ID_HANDLER_EX(ID_EDIT_DELETE, OnEditDelete)

    CHAIN_MSG_MAP(CFrameWindowImpl)
  END_MSG_MAP()

 private:
  enum Controls {
    IDC_FILE_LIST = 1,
  };

  BOOL PreTranslateMessage(MSG* message) override;

  int OnCreate(CREATESTRUCT* create);
  void OnDestroy();

  LRESULT OnItemChanged(NMHDR* header);
  LRESULT OnDeleteItem(NMHDR* header);

  void OnFileUpdate(UINT notify_code, int id, CWindow control);
  void OnEditSelectAll(UINT notify_code, int id, CWindow control);
  void OnEditDelete(UINT notify_code, int id, CWindow control);

  uint64_t selected_size_;
  CImageList tool_bar_image_;
  CToolBarCtrl tool_bar_;
  CStatusBarCtrl status_bar_;
  CListViewCtrl file_list_;

  MainFrame(const MainFrame&) = delete;
  MainFrame& operator=(const MainFrame&) = delete;
};

}  // namespace ui
}  // namespace patch_cleaner

#endif  // PATCH_CLEANER_UI_MAIN_FRAME_H_
