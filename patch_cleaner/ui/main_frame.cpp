// Copyright (c) 2016 dacci.org

#include "ui/main_frame.h"

#include <atlstr.h>

#include <msi.h>
#include <shlobj.h>

#include <base/logging.h>
#include <base/win/scoped_co_mem.h>

#include <map>
#include <set>
#include <string>

#include "app/application.h"

namespace patch_cleaner {
namespace ui {

namespace {

typedef CWinTraitsOR<TBSTYLE_TOOLTIPS | TBSTYLE_LIST, TBSTYLE_EX_MIXEDBUTTONS>
    ToolBarWinTraits;
typedef CWinTraitsOR<SBARS_SIZEGRIP> StatusBarWinTraits;
typedef CWinTraitsOR<LVS_REPORT | LVS_SHOWSELALWAYS> FileListWinTraits;

struct StringLessIgnoreCase {
  bool operator()(const std::wstring& a, const std::wstring& b) const {
    return _wcsicmp(a.c_str(), b.c_str()) < 0;
  }
};

typedef std::map<std::wstring, uint64_t, StringLessIgnoreCase> FileSizeMap;

void EnumFiles(const std::wstring& base_path, const wchar_t* pattern,
               FileSizeMap* output) {
  auto query = base_path + pattern;
  WIN32_FIND_DATA find_data;
  auto find = FindFirstFile(query.c_str(), &find_data);
  if (find != INVALID_HANDLE_VALUE) {
    do {
      auto path = base_path + find_data.cFileName;

      ULARGE_INTEGER size;
      size.LowPart = find_data.nFileSizeLow;
      size.HighPart = find_data.nFileSizeHigh;

      output->insert({path, size.QuadPart});
    } while (FindNextFile(find, &find_data));

    FindClose(find);
  }
}

template <size_t length>
int FormatSize(uint64_t size, wchar_t (&buffer)[length]) {
  static const wchar_t* kUnits[]{L"B", L"KB", L"MB", L"GB", L"TB"};

  auto double_size = static_cast<double>(size);
  auto index = 0;
  for (; double_size > 1024.0; ++index)
    double_size /= 1024.0;

  return swprintf_s(buffer, L"%.1lf %s", double_size, kUnits[index]);
}

}  // namespace

MainFrame::MainFrame() : selected_size_(0) {}

BOOL MainFrame::PreTranslateMessage(MSG* message) {
  if (CFrameWindowImpl::PreTranslateMessage(message))
    return TRUE;

  return FALSE;
}

int MainFrame::OnCreate(CREATESTRUCT* /*create*/) {
  if (!app::GetApplication()->GetMessageLoop()->AddMessageFilter(this)) {
    LOG(ERROR) << "Failed to add message filter.";
    return -1;
  }

  CBitmap tool_bar_bitmap;
  tool_bar_bitmap = AtlLoadBitmapImage(IDR_MAIN, LR_CREATEDIBSECTION);
  if (tool_bar_bitmap.IsNull()) {
    LOG(ERROR) << "Failed to load tool bar bitmap: " << GetLastError();
    return -1;
  }

  if (!tool_bar_image_.Create(16, 16, ILC_COLOR32, 0, 2)) {
    LOG(ERROR) << "Failed to create tool bar image.";
    return -1;
  }

  if (tool_bar_image_.Add(tool_bar_bitmap) == -1) {
    LOG(ERROR) << "Failed to add tool bar image.";
    return -1;
  }

  m_hWndToolBar = tool_bar_.Create(
      m_hWnd, nullptr, nullptr, ToolBarWinTraits::GetWndStyle(0),
      ToolBarWinTraits::GetWndExStyle(0), ATL_IDW_TOOLBAR);
  if (tool_bar_.IsWindow()) {
    tool_bar_.SetImageList(tool_bar_image_);
    tool_bar_.AddString(ID_FILE_UPDATE);
    tool_bar_.AddString(ID_EDIT_DELETE);
  } else {
    LOG(ERROR) << "Failed to create tool bar.";
    return -1;
  }

  tool_bar_.AddButton(ID_FILE_UPDATE, BTNS_AUTOSIZE | BTNS_SHOWTEXT,
                      TBSTATE_ENABLED, 0, MAKEINTRESOURCE(0), 0);
  tool_bar_.AddButton(ID_EDIT_DELETE, BTNS_AUTOSIZE | BTNS_SHOWTEXT,
                      TBSTATE_ENABLED, 1, MAKEINTRESOURCE(1), 0);

  m_hWndStatusBar = status_bar_.Create(m_hWnd, nullptr, nullptr,
                                       StatusBarWinTraits::GetWndStyle(0),
                                       StatusBarWinTraits::GetWndExStyle(0));
  if (!status_bar_.IsWindow()) {
    LOG(ERROR) << "Failed to create status bar.";
    return -1;
  }

  m_hWndClient = file_list_.Create(
      m_hWnd, nullptr, nullptr, FileListWinTraits::GetWndStyle(0),
      FileListWinTraits::GetWndExStyle(0), IDC_FILE_LIST);
  if (file_list_.IsWindow()) {
    file_list_.SetExtendedListViewStyle(
        LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER |
        LVS_EX_SIMPLESELECT | LVS_EX_AUTOSIZECOLUMNS);

    LVCOLUMN column{LVCF_FMT | LVCF_WIDTH | LVCF_TEXT};
    column.fmt |= LVCFMT_LEFT;
    column.cx = 250;
    column.pszText = L"Path";
    file_list_.InsertColumn(0, &column);

    column.fmt |= LVCFMT_RIGHT;
    column.cx = 80;
    column.pszText = L"Size";
    file_list_.InsertColumn(1, &column);
  } else {
    LOG(ERROR) << "Failed to create file list.";
    return -1;
  }

  return 0;
}

void MainFrame::OnDestroy() {
  SetMsgHandled(FALSE);

  if (!app::GetApplication()->GetMessageLoop()->RemoveMessageFilter(this))
    LOG(WARNING) << "Failed to remove message filter.";
}

LRESULT MainFrame::OnItemChanged(NMHDR* header) {
  auto data = reinterpret_cast<NMLISTVIEW*>(header);

  if (data->uChanged & LVIF_STATE) {
    auto old_checked =
        (data->uOldState & LVIS_STATEIMAGEMASK) == INDEXTOSTATEIMAGEMASK(2);
    auto new_checked =
        (data->uNewState & LVIS_STATEIMAGEMASK) == INDEXTOSTATEIMAGEMASK(2);

    if (old_checked != new_checked) {
      if (new_checked)
        selected_size_ += *reinterpret_cast<uint64_t*>(data->lParam);
      else
        selected_size_ -= *reinterpret_cast<uint64_t*>(data->lParam);

      wchar_t size_text[32];
      FormatSize(selected_size_, size_text);
      // DLOG(INFO) << size_text;
      status_bar_.SetText(0, size_text);
    }
  }

  return 0;
}

LRESULT MainFrame::OnDeleteItem(NMHDR* header) {
  auto data = reinterpret_cast<NMLISTVIEW*>(header);
  delete reinterpret_cast<uint64_t*>(data->lParam);

  return 0;
}

void MainFrame::OnFileUpdate(UINT /*notify_code*/, int /*id*/,
                             CWindow /*control*/) {
  std::wstring cache_path;
  {
    base::win::ScopedCoMem<wchar_t> windir;
    auto result =
        SHGetKnownFolderPath(FOLDERID_Windows, KF_FLAG_DEFAULT, NULL, &windir);
    if (FAILED(result)) {
      LOG(ERROR) << "SHGetKnownFolderPath() failed: 0x" << std::hex << result;
      return;
    }

    cache_path.assign(windir).append(L"\\Installer\\");
  }

  FileSizeMap files;
  EnumFiles(cache_path, L"*.msi", &files);
  EnumFiles(cache_path, L"*.msp", &files);

  DWORD length;
  std::wstring path;
  path.reserve(MAX_PATH - 1);

  for (DWORD i = 0;; ++i) {
    wchar_t product[40];
    auto error = MsiEnumProducts(i, product);
    if (error != ERROR_SUCCESS) {
      LOG_IF(ERROR, error != ERROR_NO_MORE_ITEMS)
          << "MsiEnumProducts() failed: " << error;
      break;
    }

    length = MAX_PATH;
    path.resize(length - 1);
    error = MsiGetProductInfo(product, INSTALLPROPERTY_LOCALPACKAGE, &path[0],
                              &length);
    if (error == ERROR_SUCCESS) {
      path.resize(length);
      files.erase(path);
    }

    for (DWORD j = 0;; ++j) {
      wchar_t patch[40], transform[128];
      length = _countof(transform);
      error = MsiEnumPatches(product, j, patch, transform, &length);
      if (error != ERROR_SUCCESS) {
        LOG_IF(ERROR, error != ERROR_NO_MORE_ITEMS)
            << "MsiEnumPatches() failed: " << error;
        break;
      }

      length = MAX_PATH;
      path.resize(length - 1);
      error = MsiGetPatchInfo(patch, INSTALLPROPERTY_LOCALPACKAGE, &path[0],
                              &length);
      if (error == ERROR_SUCCESS) {
        path.resize(length);
        files.erase(path);
      }
    }
  }

  file_list_.SetRedraw(FALSE);
  file_list_.DeleteAllItems();

  auto count = 0;
  LVITEM item{};
  wchar_t size_text[32];
  for (auto& pair : files) {
    item.mask = LVIF_TEXT | LVIF_PARAM;
    item.iItem = count;
    item.iSubItem = 0;
    item.pszText = const_cast<wchar_t*>(pair.first.c_str());
    item.lParam = reinterpret_cast<LPARAM>(new uint64_t{pair.second});
    item.iItem = file_list_.InsertItem(&item);
    if (item.iItem == -1)
      break;

    FormatSize(pair.second, size_text);

    item.mask = LVIF_TEXT;
    item.iSubItem = 1;
    item.pszText = size_text;
    if (file_list_.SetItem(&item) == -1)
      break;

    ++count;
  }

  file_list_.SetRedraw(TRUE);
  file_list_.RedrawWindow(
      NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_FRAME);
}

void MainFrame::OnEditSelectAll(UINT /*notify_code*/, int /*id*/,
                                CWindow /*control*/) {
  for (auto i = 0, ix = file_list_.GetItemCount(); i < ix; ++i)
    file_list_.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
}

void MainFrame::OnEditDelete(UINT /*notify_code*/, int /*id*/,
                             CWindow /*control*/) {
  wchar_t path[MAX_PATH];
  for (auto i = 0, ix = file_list_.GetItemCount(); i < ix; ++i) {
    auto state = file_list_.GetItemState(i, LVIS_STATEIMAGEMASK);
    if ((state & INDEXTOSTATEIMAGEMASK(2)) == 0)
      continue;

    file_list_.GetItemText(i, 0, path, _countof(path));
    if (!DeleteFile(path))
      LOG(ERROR) << "Failed to delete " << path << ": " << GetLastError();
  }

  PostMessage(WM_COMMAND, MAKEWPARAM(ID_FILE_UPDATE, 0));
}

}  // namespace ui
}  // namespace patch_cleaner
