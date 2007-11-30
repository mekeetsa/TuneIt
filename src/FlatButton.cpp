// FlatButton.cpp : implementation file
//

#include "stdafx.h"
#include "tuneit.h"
#include "FlatButton.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFlatButton

CFlatButton::CFlatButton()
{
    fCapture = FALSE;
    fPressed = FALSE;
    fHighlighted = FALSE;
}

CFlatButton::~CFlatButton()
{
}

BEGIN_MESSAGE_MAP(CFlatButton, CButton)
	//{{AFX_MSG_MAP(CFlatButton)
	ON_WM_MOUSEMOVE()
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_CONTROL_REFLECT(BN_CLICKED, OnClicked)
	ON_CONTROL_REFLECT(BN_DOUBLECLICKED, OnDoubleclicked)
	ON_WM_ERASEBKGND()
	ON_WM_CAPTURECHANGED()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFlatButton message handlers

BOOL CFlatButton::PreCreateWindow(CREATESTRUCT& cs) 
{
    cs.style |= BS_OWNERDRAW | BS_CHECKBOX;
	return CButton::PreCreateWindow(cs);
}

int CFlatButton::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
    LOGFONT lf = { 0 };
    lf.lfHeight = 16;
    strcpy(lf.lfFaceName, "MS Sans Serif");
    VERIFY(fontText.CreateFontIndirect(&lf));

	if (CButton::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	// TODO: Add your specialized creation code here

	return 0;
}

void CFlatButton::DrawButton( CDC& dc )
{
    // Get the button's text.
    //
    CRect rect;
    GetClientRect( &rect );

    if ( fHighlighted )
        dc.FillSolidRect( rect, RGB( 0x00, 0x40, 0x30 ) );
    else
        dc.FillSolidRect( rect, RGB( 0x00, 0x00, 0x30 ) );
   
    if ( fCapture && ! fPressed )
        dc.FrameRect( rect, &CBrush( RGB( 0x00, 0xFF, 0xFF ) ) );
    else
        dc.FrameRect( rect, &CBrush( RGB( 0x00, 0x60, 0x60 ) ) );

    // Draw the button text using the text color
    //
    COLORREF crOldColor = dc.SetTextColor(RGB(0x80,0x80,0x00));

    dc.SelectObject( &fontText );

    dc.DrawText( szTitle, szTitle.GetLength(), 
        &rect, DT_SINGLELINE|DT_VCENTER|DT_CENTER);

    dc.SetBkMode( TRANSPARENT );

    if ( fPressed )
    {
        dc.SetTextColor( RGB(0x80,0x80,0x00));
        dc.DrawText( szTitle, szTitle.GetLength(), 
            &rect, DT_SINGLELINE|DT_VCENTER|DT_CENTER);
        rect.top += 2; rect.left += 2;
        dc.SetTextColor( RGB(0xFF,0xFF,0x00));
        dc.DrawText( szTitle, &rect, DT_SINGLELINE|DT_VCENTER|DT_CENTER);
        rect.top -= 1; rect.left -= 1;
        dc.FrameRect( rect, &CBrush( RGB( 0x00, 0xFF, 0xFF ) ) );
        }
    else if ( fCapture )
    {
        dc.SetTextColor( RGB(0xE0,0xE0,0x00));
        dc.DrawText( szTitle, &rect, DT_SINGLELINE|DT_VCENTER|DT_CENTER);
        }
    else if ( fHighlighted )
    {
        dc.SetTextColor( RGB(0xE0,0xE0,0xE0));
        dc.DrawText( szTitle, &rect, DT_SINGLELINE|DT_VCENTER|DT_CENTER);
        }
    else
    {
        dc.SetTextColor( RGB(0xA0,0xA0,0x00));
        dc.DrawText( szTitle, &rect, DT_SINGLELINE|DT_VCENTER|DT_CENTER);
        }
    dc.SetTextColor( crOldColor );
    }

void CFlatButton::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
    DrawButton( dc );
}

void CFlatButton::OnMouseMove(UINT nFlags, CPoint point) 
{
    CRect rect;
    GetClientRect( &rect );

    if ( rect.PtInRect( point ) )
    {
        if ( ! fCapture )
        {
            fCapture = TRUE;
            SetCapture ();
            InvalidateRect( NULL, FALSE );
            }
        }
    else
    {
        if ( fCapture )
            ReleaseCapture ();

        fCapture = FALSE;
        fPressed = FALSE;
        InvalidateRect( NULL, FALSE );
        }

	CButton::OnMouseMove(nFlags, point);
}

void CFlatButton::OnCaptureChanged(CWnd *pWnd) 
{
    if ( fCapture )
        ReleaseCapture ();

    fCapture = FALSE;
    fPressed = FALSE;
    InvalidateRect( NULL, FALSE );

	CButton::OnCaptureChanged(pWnd);
}

void CFlatButton::OnLButtonDown(UINT nFlags, CPoint point) 
{
    fPressed = TRUE;
    InvalidateRect( NULL, FALSE );

    CButton::OnLButtonDown( nFlags, point );
}

void CFlatButton::OnLButtonUp(UINT nFlags, CPoint point) 
{
    CButton::OnLButtonUp( nFlags, point );

    if ( fCapture )
        ReleaseCapture ();

    fPressed = FALSE;
    fCapture = FALSE;
    InvalidateRect( NULL, FALSE );
}

void CFlatButton::OnClicked() 
{
    if ( GetParent() && GetParent()->IsWindowVisible () )
        GetParent()->SendMessage( WM_COMMAND, GetDlgCtrlID (), 0 );
}

void CFlatButton::OnDoubleclicked() 
{
    // MessageBox( "x" );
}

BOOL CFlatButton::OnEraseBkgnd(CDC* pDC) 
{
	return TRUE;
}

void CFlatButton::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
    // This code only works with buttons.
    ASSERT(lpDrawItemStruct->CtlType == ODT_BUTTON);

    CDC& dc = *CDC::FromHandle(lpDrawItemStruct->hDC);

    DrawButton( dc );
}
