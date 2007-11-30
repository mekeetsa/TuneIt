#if !defined(AFX_FLATBUTTON_H__C27CF0B4_00C8_4E2E_8CD6_4E23097E8060__INCLUDED_)
#define AFX_FLATBUTTON_H__C27CF0B4_00C8_4E2E_8CD6_4E23097E8060__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// FlatButton.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFlatButton window

class CFlatButton : public CButton
{
// Construction
public:
	CFlatButton();

// Attributes
public:
    CString szTitle;
    CFont fontText;
    BOOL fCapture;
    BOOL fPressed;
    BOOL fHighlighted;

// Operations
public:
    void DrawButton( CDC& dc );

    void Create( LPCTSTR title, CWnd* parent, int x, int y, int cx, int cy, UINT nID )
    {
        szTitle = title;
        CButton::Create( title, WS_VISIBLE | WS_CHILD, CRect( x, y, x + cx, y + cy ), parent, nID );
        }

    void SetHighlighted( BOOL on, LPCSTR szNewTitle = NULL )
    {
        fHighlighted = on;
        if ( szNewTitle ) 
            szTitle = szNewTitle;
        InvalidateRect( NULL, FALSE );
        }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFlatButton)
	public:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CFlatButton();

	// Generated message map functions
protected:
	//{{AFX_MSG(CFlatButton)
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnClicked();
	afx_msg void OnDoubleclicked();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnCaptureChanged(CWnd *pWnd);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FLATBUTTON_H__C27CF0B4_00C8_4E2E_8CD6_4E23097E8060__INCLUDED_)
