/* 
 *	Copyright (C) 2007-2009 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "resource.h"
#include "baseclasses/streams.h"

// GSCaptureDlg dialog

class GSCaptureDlg : public CDialog
{
	DECLARE_DYNAMIC(GSCaptureDlg)

private:
	struct Codec
	{
		CComPtr<IMoniker> moniker;
		CComPtr<IBaseFilter> filter;
		CString FriendlyName;
		CComBSTR DisplayName;
	};

	CAtlList<Codec> m_codecs;

	int GetSelCodec(Codec& c);

public:
	GSCaptureDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~GSCaptureDlg();

	CComPtr<IBaseFilter> m_enc;

// Dialog Data
	enum { IDD = IDD_CAPTURE };
	CString m_filename;
	CComboBox m_codeclist;

protected:
	virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnKickIdle();
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnUpdateButton2(CCmdUI* pCmdUI);
	afx_msg void OnBnClickedOk();
	afx_msg void OnUpdateOK(CCmdUI* pCmdUI);
	int m_width;
	int m_height;
};
