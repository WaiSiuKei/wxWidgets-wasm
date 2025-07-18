/////////////////////////////////////////////////////////////////////////////
// Name:        wx/wasm/window.h
// Purpose:     wxWindowWasm
// Author:      Adam Hilss
// Copyright:   (c) 2019 Adam Hilss
// Licence:     LGPL v2
/////////////////////////////////////////////////////////////////////////////

#ifndef __WX_WASM_WINDOW_H__
#define __WX_WASM_WINDOW_H__

class wxNonOwnedWindow;

class WXDLLIMPEXP_CORE wxWindowWasm : public wxWindowBase
{
public:
    // creating the window
    // -------------------
    wxWindowWasm();
    wxWindowWasm(wxWindow *parent,
                 wxWindowID id,
                 const wxPoint& pos = wxDefaultPosition,
                 const wxSize& size = wxDefaultSize,
                 long style = 0,
                 const wxString& name = wxPanelNameStr);
    virtual ~wxWindowWasm();

    bool Create(wxWindow *parent,
                wxWindowID id,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = 0,
                const wxString& name = wxPanelNameStr);

    // implement base class pure virtuals
    virtual void SetLabel(const wxString& label) override { m_label = label; }
    virtual wxString GetLabel() const override { return m_label; }

    virtual void Raise() override;
    virtual void Lower() override;

    virtual bool Show(bool show = true) override;

    virtual void SetFocus() override;

    virtual void WarpPointer(int x, int y) override;

    virtual void Refresh(bool eraseBackground = true,
                         const wxRect *rect = (const wxRect *) nullptr) override;

    virtual bool SetFont(const wxFont& font) override;

    virtual bool SetCursor(const wxCursor &cursor) override;

    virtual int GetCharHeight() const override;
    virtual int GetCharWidth() const override;

    virtual double GetContentScaleFactor() const override;
    virtual double GetDPIScaleFactor() const override;

#if wxUSE_DRAG_AND_DROP
    virtual void SetDropTarget(wxDropTarget *dropTarget) override;
#endif // wxUSE_DRAG_AND_DROP

    virtual bool IsDoubleBuffered() const override { return true; }

    virtual WXWidget GetHandle() const override { return nullptr; }

    virtual bool HasTransparentBackground() override;

    wxNonOwnedWindow* GetTopLevelWindow();

    bool NeedsPaint() const { return m_childNeedsPaint; }
    bool SelfNeedsPaint() const { return m_selfNeedsPaint; }
    void Invalidate(bool needsPaint);

protected:
    virtual void DoGetTextExtent(const wxString& string,
                                 int *x, int *y,
                                 int *descent = nullptr,
                                 int *externalLeading = nullptr,
                                 const wxFont *theFont = nullptr) const override;

    virtual void DoClientToScreen(int *x, int *y) const override;
    virtual void DoScreenToClient(int *x, int *y) const override;

    virtual void DoGetPosition(int *x, int *y) const override;
    virtual void DoGetSize(int *width, int *height) const override;
    virtual void DoGetClientSize(int *width, int *height) const override;
    virtual void DoSetSize(int x, int y,
                           int width, int height,
                           int sizeFlags = wxSIZE_AUTO) override;
    virtual void DoSetClientSize(int width, int height) override;

    virtual void DoMoveWindow(int x, int y, int width, int height) override;
    virtual void DoEnable(bool enable) override;

    virtual void DoCaptureMouse() override;
    virtual void DoReleaseMouse() override;

    virtual void DoFreeze() override { }
    virtual void DoThaw() override;

    // implementation
    void KillFocus();

    void EraseBackgroundWindow();
    void PaintSelf();
    void PaintChildren(bool selfWasPainted);
    void DoPaint(bool parentWasPainted);

private:
    void Init();

    int m_x, m_y;          // window position
    int m_width, m_height; // window size

    wxString m_label;

    bool m_childNeedsPaint;
    bool m_selfNeedsPaint;

    wxDECLARE_DYNAMIC_CLASS(wxWindowWasm);
    wxDECLARE_NO_COPY_CLASS(wxWindowWasm);
};

extern wxWindow *g_mouseWindow;

#endif // __WX_WASM_WINDOW_H__
