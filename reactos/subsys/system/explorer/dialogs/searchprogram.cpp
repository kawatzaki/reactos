/*
 * Copyright 2003 Martin Fuchs
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


 //
 // Explorer clone
 //
 // searchprogram.cpp
 //
 // Explorer dialogs
 //
 // Martin Fuchs, 02.10.2003
 //


#include "../utility/utility.h"

#include "../explorer.h"
#include "../globals.h"
#include "../explorer_intres.h"

#include "searchprogram.h"


int CollectProgramsThread::Run()
{
	try {
		collect_programs(SpecialFolderPath(CSIDL_COMMON_PROGRAMS, _hwnd));
	} catch(COMException&) {
	}

	try {
		collect_programs(SpecialFolderPath(CSIDL_PROGRAMS, _hwnd));
	} catch(COMException&) {
	}

	if (_alive)
		_cache_valid = true;

	return 0;
}

void CollectProgramsThread::collect_programs(const ShellPath& path)
{
	ShellDirectory* dir = new ShellDirectory(Desktop(), path, 0);
	_dirs.push(dir);

	dir->smart_scan();

	for(Entry*entry=dir->_down; entry; entry=entry->_next) {
		if (!_alive)
			break;

		if (entry->_shell_attribs & SFGAO_HIDDEN)
			continue;

		ShellEntry* shell_entry = static_cast<ShellEntry*>(entry);

		if (entry->_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			collect_programs(shell_entry->create_absolute_pidl());
		else if (entry->_shell_attribs & SFGAO_LINK)
			if (_alive)
				_callback(dir->_folder, shell_entry, _para);
	}
}

void CollectProgramsThread::free_dirs()
{
	while(!_dirs.empty()) {
		ShellDirectory* dir = _dirs.top();
		dir->free_subentries();
		_dirs.pop();
	}
}


#pragma warning(disable: 4355)

FindProgramDlg::FindProgramDlg(HWND hwnd)
 :	super(hwnd),
	_list_ctrl(GetDlgItem(hwnd, IDC_MAILS_FOUND)),
	_himl(ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_COLOR32, 0, 0)),
	_thread(collect_programs_callback, hwnd, this),
	_sort(_list_ctrl, CompareFunc/*, (LPARAM)this*/)
{
	SetWindowIcon(hwnd, IDI_REACTOS/*IDI_SEARCH*/);

	_resize_mgr.Add(IDC_TOPIC,		RESIZE_X);
	_resize_mgr.Add(IDC_MAILS_FOUND,RESIZE);

	_resize_mgr.Resize(+520, +300);

	_haccel = LoadAccelerators(g_Globals._hInstance, MAKEINTRESOURCE(IDA_SEARCH_PROGRAM));

	ListView_SetImageList(_list_ctrl, _himl, LVSIL_SMALL);
	_idxNoIcon = ImageList_AddIcon(_himl, SmallIcon(IDI_APPICON));

	LV_COLUMN column = {LVCF_FMT|LVCF_WIDTH|LVCF_TEXT, LVCFMT_LEFT, 250};

	column.pszText = _T("Name");
	ListView_InsertColumn(_list_ctrl, 0, &column);

	column.cx = 300;
	column.pszText = _T("Path");
	ListView_InsertColumn(_list_ctrl, 1, &column);

	column.cx = 400;
	column.pszText = _T("Menu Path");
	ListView_InsertColumn(_list_ctrl, 2, &column);

	ListView_SetExtendedListViewStyleEx(_list_ctrl, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	_common_programs = SpecialFolderFSPath(CSIDL_COMMON_PROGRAMS, hwnd);
	if (!_common_programs.empty())
		_common_programs.append(_T("\\"));

	_user_programs = SpecialFolderFSPath(CSIDL_PROGRAMS, hwnd);
	if (!_user_programs.empty())
		_user_programs.append(_T("\\"));

	CenterWindow(hwnd);

	Refresh();
}

FindProgramDlg::~FindProgramDlg()
{
	ImageList_Destroy(_himl);
}


void FindProgramDlg::Refresh(bool delete_cache)
{
	WaitCursor wait;

	_thread.Stop();

	TCHAR buffer[1024];
	GetWindowText(GetDlgItem(_hwnd, IDC_TOPIC), buffer, 1024);
#ifndef __WINE__ //TODO
	_tcslwr(buffer);
#endif
	_lwr_filter = buffer;

	HiddenWindow hide_listctrl(_list_ctrl);

	ListView_DeleteAllItems(_list_ctrl);

	if (delete_cache || !_thread._cache_valid) {
		_thread.free_dirs();
		_thread.Start();
	} else {
		for(FPDCache::const_iterator it=_cache.begin(); it!=_cache.end(); ++it)
			add_entry(*it);
	}
}

void FindProgramDlg::collect_programs_callback(ShellFolder& folder, ShellEntry* shell_entry, void* param)
{
	FindProgramDlg* pThis = (FindProgramDlg*) param;
	LPCITEMIDLIST pidl = shell_entry->_pidl;

	IShellLink* pShellLink;
	HRESULT hr = folder->GetUIObjectOf(NULL, 1, &pidl, IID_IShellLink, NULL, (LPVOID*)&pShellLink);
	if (SUCCEEDED(hr)) {
		ShellLinkPtr shell_link(pShellLink);

		/*hr = pShellLink->Resolve(pThis->_hwnd, SLR_NO_UI);
		if (SUCCEEDED(hr))*/ {
			WIN32_FIND_DATA wfd;
			TCHAR path[MAX_PATH];

			hr = pShellLink->GetPath(path, MAX_PATH-1, (WIN32_FIND_DATA*)&wfd, SLGP_UNCPRIORITY);

			if (SUCCEEDED(hr)) {
				FileSysShellPath entry_path(shell_entry->create_absolute_pidl());
				String menu_path;

				int len = pThis->_common_programs.size();
				if (len && !_tcsnicmp(entry_path, pThis->_common_programs, len))
					menu_path = ResString(IDS_ALL_USERS) + (String(entry_path)+len);
				else if ((len=pThis->_user_programs.size()) && !_tcsnicmp(entry_path, pThis->_user_programs, len))
					menu_path = String(entry_path)+len;

				 // store info in cache
				FPDEntry new_entry;

				new_entry._shell_entry = shell_entry;
				new_entry._menu_path = menu_path;
				new_entry._path = path;

				if (shell_entry->_hIcon != (HICON)-1)
					new_entry._idxIcon = ImageList_AddIcon(pThis->_himl, shell_entry->_hIcon);
				else
					new_entry._idxIcon = pThis->_idxNoIcon;

				pThis->_cache.push_front(new_entry);
				FPDEntry& cache_entry = pThis->_cache.front();

				Lock lock(pThis->_thread._crit_sect);

				 // resolve deadlocks while executing Thread::Stop()
				if (!pThis->_thread.is_alive())
					return;

				pThis->add_entry(cache_entry);
			}
		}
	}
}

void FindProgramDlg::add_entry(const FPDEntry& cache_entry)
{
	String lwr_path = cache_entry._path;
	String lwr_name = cache_entry._shell_entry->_display_name;

#ifndef __WINE__ //TODO
	_tcslwr((LPTSTR)lwr_path.c_str());
	_tcslwr((LPTSTR)lwr_name.c_str());
#endif

	if (_lwr_filter.empty())
		if (_tcsstr(lwr_name, _T("uninstal")) || _tcsstr(lwr_name, _T("deinstal")))	// filter out deinstallation links
			return;

	if (!_tcsstr(lwr_path, _lwr_filter) && !_tcsstr(lwr_name, _lwr_filter))
		return;

	LV_ITEM item = {LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM, INT_MAX};

	item.pszText = cache_entry._shell_entry->_display_name;
	item.iImage = cache_entry._idxIcon;
	item.lParam = (LPARAM) &cache_entry;
	item.iItem = ListView_InsertItem(_list_ctrl, &item);	// We could use the information in _sort to enable manual sorting while populating the list.

	item.mask = LVIF_TEXT;
	item.iSubItem = 1;
	item.pszText = (LPTSTR)(LPCTSTR)cache_entry._path;
	ListView_SetItem(_list_ctrl, &item);

	item.iSubItem = 2;
	item.pszText = (LPTSTR)(LPCTSTR)cache_entry._menu_path;
	ListView_SetItem(_list_ctrl, &item);
}

LRESULT FindProgramDlg::WndProc(UINT message, WPARAM wparam, LPARAM lparam)
{
	switch(message) {
	  default:
		return super::WndProc(message, wparam, lparam);
	}

	return FALSE;
}

int FindProgramDlg::Command(int id, int code)
{
	if (code == BN_CLICKED)
		switch(id) {
		  case ID_REFRESH:
			Refresh(true);
			break;

		  case IDOK:
			LaunchSelected();
			break;

		  default:
			return super::Command(id, code);
		}
	else if (code == EN_CHANGE)
		switch(id) {
		  case IDC_TOPIC:
			Refresh();
		}

	return TRUE;
}

void FindProgramDlg::LaunchSelected()
{
	Lock lock(_thread._crit_sect);

	int count = ListView_GetSelectedCount(_list_ctrl);
	//TODO: ask user if there are many selected items

	for(int idx=-1; (idx=ListView_GetNextItem(_list_ctrl, idx, LVNI_SELECTED))!=-1; ) {
		LPARAM lparam = ListView_GetItemData(_list_ctrl, idx);

		if (lparam) {
			FPDEntry& cache_entry = *(FPDEntry*)lparam;
			cache_entry._shell_entry->launch_entry(_hwnd);
		}
	}
}

int FindProgramDlg::Notify(int id, NMHDR* pnmh)
{
	switch(pnmh->code) {
	  case LVN_GETDISPINFO: {/*
		LV_DISPINFO* pDispInfo = (LV_DISPINFO*) pnmh;

		if (pnmh->hwndFrom == _list_ctrl) {
			if (pDispInfo->item.mask & LVIF_IMAGE) {
				int icon;
				HRESULT hr = pShellLink->GetIconLocation(path, MAX_PATH-1, &icon);

				HICON hIcon = ExtractIcon();
				pDispInfo->item.iImage = ImageList_AddIcon(_himl, hIcon);

				pDispInfo->item.mask |= LVIF_DI_SETITEM;

				return 1;
			}
		}*/}
		break;

	  case NM_DBLCLK:
		if (pnmh->hwndFrom == _list_ctrl)
			LaunchSelected();
		/*{
			Lock lock(_thread._crit_sect);

			LPNMLISTVIEW pnmv = (LPNMLISTVIEW) pnmh;
			LPARAM lparam = ListView_GetItemData(pnmh->hwndFrom, pnmv->iItem);

			if (lparam) {
				FPDEntry& cache_entry = *(FPDEntry*)lparam;
				cache_entry._shell_entry->launch_entry(_hwnd);
			}
		}*/
		break;

	  case HDN_ITEMCLICK: {
		WaitCursor wait;
		NMHEADER* phdr = (NMHEADER*)pnmh;

		if (GetParent(pnmh->hwndFrom) == _list_ctrl) {
			if (_thread._cache_valid) {	// disable manual sorting while populating the list
				_sort.toggle_sort(phdr->iItem);
				_sort.sort();
			}
		}
		break;}
	}

	return 0;
}

int CALLBACK FindProgramDlg::CompareFunc(LPARAM lparam1, LPARAM lparam2, LPARAM lparamSort)
{
	ListSort* sort = (ListSort*)lparamSort;

	FPDEntry& a = *(FPDEntry*)lparam1;
	FPDEntry& b = *(FPDEntry*)lparam2;

	int cmp = 0;

	switch(sort->_sort_crit) {
	  case 0:
		cmp = _tcsicoll(a._shell_entry->_display_name, b._shell_entry->_display_name);
		break;

	  case 1:
		cmp = _tcsicoll(a._path, b._path);
		break;

	  case 2:
		cmp = _tcsicoll(a._menu_path, b._menu_path);
	}

	return sort->_direction? -cmp: cmp;
}
