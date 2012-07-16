/*
 *      Copyright (C) 2010-2012 Hendrik Leppkes
 *      http://www.1f0.de
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "stdafx.h"

#include <assert.h>
#include <Commctrl.h>

#include "SettingsProp.h"

#include "DShowUtil.h"


#define LANG_BUFFER_SIZE 256

CLAVSplitterSettingsProp::CLAVSplitterSettingsProp(LPUNKNOWN pUnk, HRESULT* phr)
  : CBaseDSPropPage(NAME("LAVF Settings"), pUnk, IDD_PROPPAGE_LAVFSETTINGS, IDS_PAGE_TITLE)
  , m_pLAVF(NULL)
  , m_pszPrefLang(NULL)
  , m_pszPrefSubLang(NULL)
  , m_pszAdvSubConfig(NULL)
{
}

CLAVSplitterSettingsProp::~CLAVSplitterSettingsProp(void)
{
  SAFE_CO_FREE(m_pszPrefLang);
  SAFE_CO_FREE(m_pszPrefSubLang);
  SAFE_CO_FREE(m_pszAdvSubConfig);
  SafeRelease(&m_pLAVF);
}

HRESULT CLAVSplitterSettingsProp::OnConnect(IUnknown *pUnk)
{
  if (pUnk == NULL) {
    return E_POINTER;
  }
  ASSERT(m_pLAVF == NULL);
  return pUnk->QueryInterface(&m_pLAVF);
}

HRESULT CLAVSplitterSettingsProp::OnDisconnect()
{
  SafeRelease(&m_pLAVF);
  return S_OK;
}

HRESULT CLAVSplitterSettingsProp::OnApplyChanges()
{
  ASSERT(m_pLAVF != NULL);
  HRESULT hr = S_OK;
  DWORD dwVal;
  BOOL bFlag;

  WCHAR buffer[LANG_BUFFER_SIZE];
  // Save audio language
  SendDlgItemMessage(m_Dlg, IDC_PREF_LANG, WM_GETTEXT, LANG_BUFFER_SIZE, (LPARAM)&buffer);
  CHECK_HR(hr = m_pLAVF->SetPreferredLanguages(buffer));

  // Save subtitle language
  SendDlgItemMessage(m_Dlg, IDC_PREF_LANG_SUBS, WM_GETTEXT, LANG_BUFFER_SIZE, (LPARAM)&buffer);

  if (m_selectedSubMode == LAVSubtitleMode_Advanced) {
    CHECK_HR(hr = m_pLAVF->SetPreferredSubtitleLanguages(m_subLangBuffer));
    CHECK_HR(hr = m_pLAVF->SetAdvancedSubtitleConfig(buffer));
  } else {
    CHECK_HR(hr = m_pLAVF->SetPreferredSubtitleLanguages(buffer));
    CHECK_HR(hr = m_pLAVF->SetAdvancedSubtitleConfig(m_advSubBuffer));
  }

  // Save subtitle mode
  dwVal = (DWORD)SendDlgItemMessage(m_Dlg, IDC_SUBTITLE_MODE, CB_GETCURSEL, 0, 0);
  CHECK_HR(hr = m_pLAVF->SetSubtitleMode((LAVSubtitleMode)dwVal));

  bFlag = (BOOL)SendDlgItemMessage(m_Dlg, IDC_BD_SEPARATE_FORCED_SUBS, BM_GETCHECK, 0, 0);
  CHECK_HR(hr = m_pLAVF->SetPGSForcedStream(bFlag));

  bFlag = (BOOL)SendDlgItemMessage(m_Dlg, IDC_BD_ONLY_FORCED_SUBS, BM_GETCHECK, 0, 0);
  CHECK_HR(hr = m_pLAVF->SetPGSOnlyForced(bFlag));

  int vc1flag = (int)SendDlgItemMessage(m_Dlg, IDC_VC1TIMESTAMP, BM_GETCHECK, 0, 0);
  CHECK_HR(hr = m_pLAVF->SetVC1TimestampMode(vc1flag));

  bFlag = (BOOL)SendDlgItemMessage(m_Dlg, IDC_SUBSTREAMS, BM_GETCHECK, 0, 0);
  CHECK_HR(hr = m_pLAVF->SetSubstreamsEnabled(bFlag));

  bFlag = (BOOL)SendDlgItemMessage(m_Dlg, IDC_VIDEOPARSING, BM_GETCHECK, 0, 0);
  CHECK_HR(hr = m_pLAVF->SetVideoParsingEnabled(bFlag));

  bFlag = (BOOL)SendDlgItemMessage(m_Dlg, IDC_STREAM_SWITCH_REMOVE_AUDIO, BM_GETCHECK, 0, 0);
  CHECK_HR(hr = m_pLAVF->SetStreamSwitchRemoveAudio(bFlag));

  bFlag = (BOOL)SendDlgItemMessage(m_Dlg, IDC_IMPAIRED_AUDIO, BM_GETCHECK, 0, 0);
  CHECK_HR(hr = m_pLAVF->SetUseAudioForHearingVisuallyImpaired(bFlag));

  LoadData();

done:    
  return hr;
}

void CLAVSplitterSettingsProp::UpdateSubtitleMode(LAVSubtitleMode mode)
{
  if (mode == LAVSubtitleMode_NoSubs) {
    WCHAR *note = L"No subtitles: Subtitles are disabled and will not be loaded by default.";
    SendDlgItemMessage(m_Dlg, IDC_SUBTITLE_NOTE, WM_SETTEXT, 0, (LPARAM)note);
  } else if (mode == LAVSubtitleMode_ForcedOnly) {
    WCHAR *note = L"Only Forced Subtitles: Only subtitles marked as \"forced\" will be loaded.";
    SendDlgItemMessage(m_Dlg, IDC_SUBTITLE_NOTE, WM_SETTEXT, 0, (LPARAM)note);
  } else if (mode == LAVSubtitleMode_Default) {
    WCHAR *note = L"Default Mode: Subtitles matching the preferred languages, as well as \"default\" and \"forced\" subtitles will be loaded.";
    SendDlgItemMessage(m_Dlg, IDC_SUBTITLE_NOTE, WM_SETTEXT, 0, (LPARAM)note);
  } else if (mode == LAVSubtitleMode_Advanced) {
    WCHAR *note = L"Advanced Mode: Refer to the README or the documention on http://1f0.de for details.";
    SendDlgItemMessage(m_Dlg, IDC_SUBTITLE_NOTE, WM_SETTEXT, 0, (LPARAM)note);
  } else {
    WCHAR *empty = L"";
    SendDlgItemMessage(m_Dlg, IDC_SUBTITLE_NOTE, WM_SETTEXT, 0, (LPARAM)empty);
  }

  LAVSubtitleMode oldMode = m_selectedSubMode;
  m_selectedSubMode = mode;
  // Switch away from advanced
  if (oldMode != mode && oldMode == LAVSubtitleMode_Advanced) {
    SendDlgItemMessage(m_Dlg, IDC_PREF_LANG_SUBS, WM_GETTEXT, LANG_BUFFER_SIZE, (LPARAM)&m_advSubBuffer);
    SendDlgItemMessage(m_Dlg, IDC_PREF_LANG_SUBS, WM_SETTEXT, 0, (LPARAM)&m_subLangBuffer);
  // Switch to advanced
  } else if (oldMode != mode && mode == LAVSubtitleMode_Advanced) {
    SendDlgItemMessage(m_Dlg, IDC_PREF_LANG_SUBS, WM_GETTEXT, LANG_BUFFER_SIZE, (LPARAM)&m_subLangBuffer);
    SendDlgItemMessage(m_Dlg, IDC_PREF_LANG_SUBS, WM_SETTEXT, 0, (LPARAM)&m_advSubBuffer);
  }
}

HRESULT CLAVSplitterSettingsProp::OnActivate()
{
  HRESULT hr = S_OK;
  INITCOMMONCONTROLSEX icc;
  icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icc.dwICC = ICC_BAR_CLASSES | ICC_STANDARD_CLASSES;
  if (InitCommonControlsEx(&icc) == FALSE)
  {
    return E_FAIL;
  }
  ASSERT(m_pLAVF != NULL);

  const WCHAR *version = TEXT(LAV_SPLITTER) L" " TEXT(LAV_VERSION_STR);
  SendDlgItemMessage(m_Dlg, IDC_SPLITTER_FOOTER, WM_SETTEXT, 0, (LPARAM)version);

  hr = LoadData();
  memset(m_subLangBuffer, 0, sizeof(m_advSubBuffer));
  memset(m_advSubBuffer, 0, sizeof(m_advSubBuffer));

  m_selectedSubMode = LAVSubtitleMode_Default;
  if (m_pszAdvSubConfig)
    wcsncpy_s(m_advSubBuffer, m_pszAdvSubConfig, _TRUNCATE);

  // Notify the UI about those settings
  SendDlgItemMessage(m_Dlg, IDC_PREF_LANG, WM_SETTEXT, 0, (LPARAM)m_pszPrefLang);
  SendDlgItemMessage(m_Dlg, IDC_PREF_LANG_SUBS, WM_SETTEXT, 0, (LPARAM)m_pszPrefSubLang);

  // Init the Combo Box
  SendDlgItemMessage(m_Dlg, IDC_SUBTITLE_MODE, CB_RESETCONTENT, 0, 0);
  WideStringFromResource(stringBuffer, IDS_SUBMODE_NO_SUBS);
  SendDlgItemMessage(m_Dlg, IDC_SUBTITLE_MODE, CB_ADDSTRING, 0, (LPARAM)stringBuffer);
  WideStringFromResource(stringBuffer, IDS_SUBMODE_FORCED_SUBS);
  SendDlgItemMessage(m_Dlg, IDC_SUBTITLE_MODE, CB_ADDSTRING, 0, (LPARAM)stringBuffer);
  WideStringFromResource(stringBuffer, IDS_SUBMODE_DEFAULT);
  SendDlgItemMessage(m_Dlg, IDC_SUBTITLE_MODE, CB_ADDSTRING, 0, (LPARAM)stringBuffer);
  WideStringFromResource(stringBuffer, IDS_SUBMODE_ADVANCED);
  SendDlgItemMessage(m_Dlg, IDC_SUBTITLE_MODE, CB_ADDSTRING, 0, (LPARAM)stringBuffer);

  SendDlgItemMessage(m_Dlg, IDC_SUBTITLE_MODE, CB_SETCURSEL, m_subtitleMode, 0);
  addHint(IDC_SUBTITLE_MODE, L"Configure how subtitles are selected.");

  SendDlgItemMessage(m_Dlg, IDC_BD_SEPARATE_FORCED_SUBS, BM_SETCHECK, m_PGSForcedStream, 0);
  addHint(IDC_BD_SEPARATE_FORCED_SUBS, L"Enabling this causes the creation of a new \"Forced Subtitles\" stream, which will try to always display forced subtitles matching your selected audio language.\n\nNOTE: This option may not work on all Blu-ray discs.\nRequires restart to take effect.");

  SendDlgItemMessage(m_Dlg, IDC_BD_ONLY_FORCED_SUBS, BM_SETCHECK, m_PGSOnlyForced, 0);
  addHint(IDC_BD_ONLY_FORCED_SUBS, L"When enabled, all Blu-ray (PGS) subtitles will be filtered, and only forced subtitles will be sent to the renderer.\n\nNOTE: When this option is active, you will not be able to get the \"full\" subtitles.");

  SendDlgItemMessage(m_Dlg, IDC_VC1TIMESTAMP, BM_SETCHECK, m_VC1Mode, 0);
  addHint(IDC_VC1TIMESTAMP, L"Checked - Frame timings will be corrected.\nUnchecked - Frame timings will be sent untouched.\nIndeterminate (Auto) - Only enabled for decoders that rely on the splitter doing the corrections.\n\nNOTE: Only for debugging, if unsure, set to \"Auto\".");

  SendDlgItemMessage(m_Dlg, IDC_SUBSTREAMS, BM_SETCHECK, m_substreams, 0);
  addHint(IDC_SUBSTREAMS, L"Controls if sub-streams should be exposed as a separate stream.\nSub-streams are typically streams for backwards compatibility, for example the AC3 part of TrueHD streams on Blu-rays.");

  SendDlgItemMessage(m_Dlg, IDC_VIDEOPARSING, BM_SETCHECK, m_videoParsing, 0);
  addHint(IDC_VIDEOPARSING, L"Enables parsing and repacking of video streams.\n\nNOTE: Only for debugging, if unsure, set to ON.");

  SendDlgItemMessage(m_Dlg, IDC_STREAM_SWITCH_REMOVE_AUDIO, BM_SETCHECK, m_StreamSwitchRemoveAudio, 0);
  addHint(IDC_STREAM_SWITCH_REMOVE_AUDIO, L"Remove the old Audio Decoder from the Playback Chain before switching the audio stream, forcing DirectShow to select a new one.\n\nThis option ensures that the preferred decoder is always used, however it does not work properly with all players.");

  SendDlgItemMessage(m_Dlg, IDC_IMPAIRED_AUDIO, BM_SETCHECK, m_ImpairedAudio, 0);

  UpdateSubtitleMode(m_subtitleMode);

  return hr;
}
HRESULT CLAVSplitterSettingsProp::LoadData()
{
  HRESULT hr = S_OK;

  // Free old strings
  SAFE_CO_FREE(m_pszPrefLang);
  SAFE_CO_FREE(m_pszPrefSubLang);
  SAFE_CO_FREE(m_pszAdvSubConfig);

  // Query current settings
  CHECK_HR(hr = m_pLAVF->GetPreferredLanguages(&m_pszPrefLang));
  CHECK_HR(hr = m_pLAVF->GetPreferredSubtitleLanguages(&m_pszPrefSubLang));
  CHECK_HR(hr = m_pLAVF->GetAdvancedSubtitleConfig(&m_pszAdvSubConfig));
  m_subtitleMode = m_pLAVF->GetSubtitleMode();
  m_PGSForcedStream = m_pLAVF->GetPGSForcedStream();
  m_PGSOnlyForced = m_pLAVF->GetPGSOnlyForced();
  m_VC1Mode = m_pLAVF->GetVC1TimestampMode();
  m_substreams = m_pLAVF->GetSubstreamsEnabled();

  m_videoParsing = m_pLAVF->GetVideoParsingEnabled();
  m_StreamSwitchRemoveAudio = m_pLAVF->GetStreamSwitchRemoveAudio();
  m_ImpairedAudio = m_pLAVF->GetUseAudioForHearingVisuallyImpaired();

done:
  return hr;
}

INT_PTR CLAVSplitterSettingsProp::OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
  case WM_COMMAND:
    // Mark the page dirty if the text changed
    if(HIWORD(wParam) == EN_CHANGE
      && (LOWORD(wParam) == IDC_PREF_LANG || LOWORD(wParam) == IDC_PREF_LANG_SUBS)) {

        WCHAR buffer[LANG_BUFFER_SIZE];
        SendDlgItemMessage(m_Dlg, LOWORD(wParam), WM_GETTEXT, LANG_BUFFER_SIZE, (LPARAM)&buffer);

        int dirty = 0;
        WCHAR *source = NULL;
        if(LOWORD(wParam) == IDC_PREF_LANG) {
          source = m_pszPrefLang;
        } else {
          source = (m_selectedSubMode == LAVSubtitleMode_Advanced) ? m_pszAdvSubConfig : m_pszPrefSubLang;
        }

        if (source) {
          dirty = _wcsicmp(buffer, source);
        } else {
          dirty = (int)wcslen(buffer);
        }

        if(dirty != 0) {
          SetDirty();
        }
    } else if (HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == IDC_SUBTITLE_MODE) {
      DWORD dwVal = (DWORD)SendDlgItemMessage(m_Dlg, IDC_SUBTITLE_MODE, CB_GETCURSEL, 0, 0);
      UpdateSubtitleMode((LAVSubtitleMode)dwVal);
      if (dwVal != m_subtitleMode) {
        SetDirty();
      }
    } else if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_BD_SEPARATE_FORCED_SUBS) {
      BOOL bFlag = (BOOL)SendDlgItemMessage(m_Dlg, IDC_BD_SEPARATE_FORCED_SUBS, BM_GETCHECK, 0, 0);
      if (bFlag != m_PGSForcedStream) {
        SetDirty();
      }
    } else if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_BD_ONLY_FORCED_SUBS) {
      BOOL bFlag = (BOOL)SendDlgItemMessage(m_Dlg, IDC_BD_ONLY_FORCED_SUBS, BM_GETCHECK, 0, 0);
      if (bFlag != m_PGSOnlyForced) {
        SetDirty();
      }
    } else if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_VC1TIMESTAMP) {
      int iFlag = (int)SendDlgItemMessage(m_Dlg, IDC_VC1TIMESTAMP, BM_GETCHECK, 0, 0);
      if (iFlag != m_VC1Mode) {
        SetDirty();
      }
    } else if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_SUBSTREAMS) {
      BOOL bFlag = (BOOL)SendDlgItemMessage(m_Dlg, IDC_SUBSTREAMS, BM_GETCHECK, 0, 0);
      if (bFlag != m_substreams) {
        SetDirty();
      }
    } else if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_VIDEOPARSING) {
      BOOL bFlag = (BOOL)SendDlgItemMessage(m_Dlg, IDC_VIDEOPARSING, BM_GETCHECK, 0, 0);
      if (bFlag != m_videoParsing) {
        SetDirty();
      }
    } else if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_STREAM_SWITCH_REMOVE_AUDIO) {
      BOOL bFlag = (BOOL)SendDlgItemMessage(m_Dlg, IDC_STREAM_SWITCH_REMOVE_AUDIO, BM_GETCHECK, 0, 0);
      if (bFlag != m_StreamSwitchRemoveAudio) {
        SetDirty();
      }
    }  else if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_IMPAIRED_AUDIO) {
      BOOL bFlag = (BOOL)SendDlgItemMessage(m_Dlg, IDC_IMPAIRED_AUDIO, BM_GETCHECK, 0, 0);
      if (bFlag != m_ImpairedAudio) {
        SetDirty();
      }
    }
    break;
  }
  // Let the parent class handle the message.
  return __super::OnReceiveMessage(hwnd, uMsg, wParam, lParam);
}


CLAVSplitterFormatsProp::CLAVSplitterFormatsProp(LPUNKNOWN pUnk, HRESULT* phr)
  : CBaseDSPropPage(NAME("LAVF Settings"), pUnk, IDD_PROPPAGE_FORMATS, IDS_INPUT_FORMATS)
  , m_pLAVF(NULL), m_bFormats(NULL)
{
}

CLAVSplitterFormatsProp::~CLAVSplitterFormatsProp(void)
{
  SAFE_CO_FREE(m_bFormats);
  SafeRelease(&m_pLAVF);
}

HRESULT CLAVSplitterFormatsProp::OnConnect(IUnknown *pUnk)
{
  if (pUnk == NULL) {
    return E_POINTER;
  }
  ASSERT(m_pLAVF == NULL);
  return pUnk->QueryInterface(&m_pLAVF);
}

HRESULT CLAVSplitterFormatsProp::OnDisconnect()
{
  SafeRelease(&m_pLAVF);
  return S_OK;
}

HRESULT CLAVSplitterFormatsProp::OnApplyChanges()
{
  HRESULT hr = S_OK;
  ASSERT(m_pLAVF != NULL);

  HWND hlv = GetDlgItem(m_Dlg, IDC_FORMATS);

  int nItem = 0;
  std::set<FormatInfo>::const_iterator it;
  for (it = m_Formats.begin(); it != m_Formats.end(); ++it) {
    m_bFormats[nItem] = ListView_GetCheckState(hlv, nItem);
    m_pLAVF->SetFormatEnabled(it->strName, m_bFormats[nItem]);

    nItem++;
  }

  return hr;
}

HRESULT CLAVSplitterFormatsProp::OnActivate()
{
  HRESULT hr = S_OK;
  INITCOMMONCONTROLSEX icc;
  icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icc.dwICC = ICC_BAR_CLASSES | ICC_STANDARD_CLASSES;
  if (InitCommonControlsEx(&icc) == FALSE)
  {
    return E_FAIL;
  }
  ASSERT(m_pLAVF != NULL);

  memset(stringBuffer, 0, sizeof(stringBuffer));

  const char *pszInput = m_pLAVF->GetInputFormat();
  if (pszInput) {
    _snwprintf_s(stringBuffer, _TRUNCATE, L"%S", pszInput);
  }
  SendDlgItemMessage(m_Dlg, IDC_CUR_INPUT, WM_SETTEXT, 0, (LPARAM)stringBuffer);

  m_Formats = m_pLAVF->GetInputFormats();

  // Setup ListView control for format configuration
  HWND hlv = GetDlgItem(m_Dlg, IDC_FORMATS);
  ListView_SetExtendedListViewStyle(hlv, LVS_EX_CHECKBOXES|LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);

  int nCol = 1;
  LVCOLUMN lvc = {LVCF_WIDTH, 0, 20, 0};
  ListView_InsertColumn(hlv, 0, &lvc);
  ListView_AddCol(hlv, nCol,  75, L"Format", false);
  ListView_AddCol(hlv, nCol, 210, L"Description", false);

  ListView_DeleteAllItems(hlv);
  ListView_SetItemCount(hlv, m_Formats.size());

  SAFE_CO_FREE(m_bFormats);
  m_bFormats = (BOOL *)CoTaskMemAlloc(sizeof(BOOL) * m_Formats.size());
  memset(m_bFormats, 0, sizeof(BOOL) * m_Formats.size());

  // Create entrys for the formats
  LVITEM lvi;
  memset(&lvi, 0, sizeof(lvi));
  lvi.mask = LVIF_TEXT|LVIF_PARAM;

  int nItem = 0;
  std::set<FormatInfo>::const_iterator it;
  for (it = m_Formats.begin(); it != m_Formats.end(); ++it) {
    // Create main entry
    lvi.iItem = nItem + 1;
    ListView_InsertItem(hlv, &lvi);

    // Set sub item texts
    _snwprintf_s(stringBuffer, _TRUNCATE, L"%S", it->strName);
    ListView_SetItemText(hlv, nItem, 1, (LPWSTR)stringBuffer);

    _snwprintf_s(stringBuffer, _TRUNCATE, L"%S", it->strDescription);
    ListView_SetItemText(hlv, nItem, 2, (LPWSTR)stringBuffer);

    m_bFormats[nItem] =  m_pLAVF->IsFormatEnabled(it->strName);
    ListView_SetCheckState(hlv, nItem, m_bFormats[nItem]);

    nItem++;
  }

  return hr;
}

INT_PTR CLAVSplitterFormatsProp::OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
  case WM_NOTIFY:
    NMHDR *hdr = (LPNMHDR)lParam;
    if (hdr->idFrom == IDC_FORMATS) {
      switch (hdr->code) {
      case LVN_ITEMCHANGED:
        LPNMLISTVIEW nmlv = (LPNMLISTVIEW)lParam;
        BOOL check = ListView_GetCheckState(hdr->hwndFrom, nmlv->iItem);
        if (check != m_bFormats[nmlv->iItem]) {
          SetDirty();
        }
        return TRUE;
      }
    }
    break;
  }
  // Let the parent class handle the message.
  return __super::OnReceiveMessage(hwnd, uMsg, wParam, lParam);
}
