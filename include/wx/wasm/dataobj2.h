///////////////////////////////////////////////////////////////////////////////
// Name:        wx/wasm/dataobj2.h
// Purpose:     wxDataObject derived classes
// Author:      Adam Hilss
// Copyright:   (c) 2019 Adam Hilss
// Licence:     LGPL v2
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_WASM_DATAOBJ2_H_
#define _WX_WASM_DATAOBJ2_H_

// ----------------------------------------------------------------------------
// wxBitmapDataObject is a specialization of wxDataObject for bitmaps
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_CORE wxBitmapDataObject : public wxBitmapDataObjectBase
{
public:
    // ctors
    wxBitmapDataObject();
    wxBitmapDataObject(const wxBitmap& bitmap);

    // destr
    virtual ~wxBitmapDataObject();

    // override base class virtual to update PNG data too
    virtual void SetBitmap(const wxBitmap& bitmap) override;

    // implement base class pure virtuals
    // ----------------------------------

    virtual size_t GetDataSize() const override { return m_pngSize; }
    virtual bool GetDataHere(void *buf) const override;
    virtual bool SetData(size_t len, const void *buf) override;
    // Must provide overloads to avoid hiding them (and warnings about it)
    virtual size_t GetDataSize(const wxDataFormat&) const override
    {
        return GetDataSize();
    }
    virtual bool GetDataHere(const wxDataFormat&, void *buf) const override
    {
        return GetDataHere(buf);
    }
    virtual bool SetData(const wxDataFormat&, size_t len, const void *buf) override
    {
        return SetData(len, buf);
    }

protected:
    void Clear() { delete [] m_pngData; }
    void ClearAll() { Clear(); Init(); }

    size_t      m_pngSize;
    char       *m_pngData;

    void DoConvertToPng();

private:
    void Init() { m_pngData = nullptr; m_pngSize = 0; }
};

// ----------------------------------------------------------------------------
// wxFileDataObject is a specialization of wxDataObject for file names
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_CORE wxFileDataObject : public wxFileDataObjectBase
{
public:
    // implement base class pure virtuals
    // ----------------------------------

    void AddFile(const wxString &filename);

    virtual size_t GetDataSize() const;
    virtual bool GetDataHere(void *buf) const;
    virtual bool SetData(size_t len, const void *buf);

private:
    // Must provide overloads to avoid hiding them (and warnings about it)
    virtual size_t GetDataSize(const wxDataFormat&) const
    {
        return GetDataSize();
    }
    virtual bool GetDataHere(const wxDataFormat&, void *buf) const
    {
        return GetDataHere(buf);
    }
    virtual bool SetData(const wxDataFormat&, size_t len, const void *buf)
    {
        return SetData(len, buf);
    }
};

#endif // _WX_WASM_DATAOBJ2_H_

