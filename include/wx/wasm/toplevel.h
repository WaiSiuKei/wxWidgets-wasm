/////////////////////////////////////////////////////////////////////////////
// Name:        wx/wasm/toplevel.h
// Purpose:
// Author:      Adam Hilss
// Copyright:   (c) 2019 Adam Hilss
// Licence:     LGPL v2
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_WASM_TOPLEVEL_H_
#define _WX_WASM_TOPLEVEL_H_

//-----------------------------------------------------------------------------
// wxTopLevelWindowWasm
//-----------------------------------------------------------------------------

class WXDLLIMPEXP_CORE wxTopLevelWindowWasm : public wxTopLevelWindowBase
{
    typedef wxTopLevelWindowBase base_type;
public:
    // construction
    wxTopLevelWindowWasm() { Init(); }
    wxTopLevelWindowWasm(wxWindow *parent,
                         wxWindowID id,
                         const wxString& title,
                         const wxPoint& pos = wxDefaultPosition,
                         const wxSize& size = wxDefaultSize,
                         long style = wxDEFAULT_FRAME_STYLE,
                         const wxString& name = wxFrameNameStr)
    {
        Init();
        Create(parent, id, title, pos, size, style, name);
    }

    bool Create(wxWindow *parent,
                wxWindowID id,
                const wxString& title,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxDEFAULT_FRAME_STYLE,
                const wxString& name = wxFrameNameStr);

    virtual ~wxTopLevelWindowWasm() { }

    virtual wxPoint GetClientAreaOrigin() const override;

    // implement base class pure virtuals
    virtual void Maximize(bool WXUNUSED(maximize) = true) override { }
    virtual bool IsMaximized() const override { return false; }
    virtual bool IsAlwaysMaximized() const override { return IsMainFrame(); }
    virtual void Iconize(bool WXUNUSED(iconize) = true) override { }
    virtual bool IsIconized() const override { return false; }
    virtual void Restore() override { }

    virtual void SetIcons(const wxIconBundle& icons) override;

    virtual void ShowWithoutActivating() override;
    virtual bool ShowFullScreen(bool show, long style = wxFULLSCREEN_ALL) override;
    virtual bool IsFullScreen() const override;

    virtual bool IsActive() override { return m_isActive; }

    virtual void SetTitle(const wxString &title) override;
    virtual wxString GetTitle() const override { return m_title; }

    virtual wxString GetCSSClassList() const override {
      return wxNonOwnedWindow::GetCSSClassList() + " toplevel";
    }

protected:
    virtual void DoGetClientSize(int *width, int *height) const override;
    virtual void DoSetClientSize(int width, int height) override;

    virtual void DoScreenToClient(int *x, int *y) const override;
    virtual void DoClientToScreen(int *x, int *y) const override;

    virtual bool HasTitleBar() const;

private:
    void Init();

    void SetActive(bool active) { m_isActive = active; }

    void DrawTitleText(wxDC& dc, const wxRect& rect);
    void DrawMinimizeButton(wxDC& dc, const wxRect& rect);

    void StartDrag(const wxPoint& pos);
    void EndDrag();
    void DragMove(const wxPoint& pos);

    void OnNcPaint(wxNcPaintEvent& event);
    void OnMouseDown(wxMouseEvent& event);
    void OnMouseUp(wxMouseEvent& event);
    void OnMotion(wxMouseEvent& event);

    bool m_isActive;
    wxString m_title;

    wxRect m_minimizeButtonRect;

    bool m_isDragging;
    wxPoint m_dragOffset;

    friend class wxApp;

    wxDECLARE_EVENT_TABLE();
};

#endif // _WX_WASM_TOPLEVEL_H_
