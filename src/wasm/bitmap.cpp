/////////////////////////////////////////////////////////////////////////////
// Name:        wx/wasm/bitmap.cpp
// Purpose:     wxBitmap implementation
// Author:      Adam Hilss
// Copyright:   (c) 2022 Adam Hilss
// Licence:     LGPL v2
/////////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

#include "wx/bitmap.h"
#include "wx/log.h"

#ifndef WX_PRECOMP
#include "wx/icon.h"
#include "wx/image.h"
#include "wx/colour.h"
#endif

#include "wx/dcmemory.h"
#include "wx/rawbmp.h"
#include "wx/tokenzr.h"
#include "wx/wasm/dc.h"
#include "wx/wasm/private.h"

#include <emscripten.h>

enum BitmapDataSource
{
    BITMAP_DATA_SOURCE_NONE,
    BITMAP_DATA_SOURCE_JS,
    BITMAP_DATA_SOURCE_CPP
};

// ========================================================================
// wxBitmapRefData
// ========================================================================

class wxBitmapRefData: public wxGDIRefData
{
    friend class wxBitmap;
public:
    wxBitmapRefData(int width, int height, int depth, double scale);
    virtual ~wxBitmapRefData();

    inline double GetScaleFactor() const { return m_scaleFactor; }

    inline int GetScaledWidth() const { return m_width * m_scaleFactor; }
    inline int GetScaledHeight() const { return m_height * m_scaleFactor; }

    inline int GetBytesPerPixel() const { return 4; }
    inline int GetBytesPerRow() const { return GetBytesPerPixel() * m_dataWidth; }
    int GetDataSize() const { return GetBytesPerPixel() * m_dataWidth * m_dataHeight; }

    inline BitmapDataSource GetDataSource() const { return m_dataSource; }
    unsigned char *GetData() const { return m_bitmap; }

    inline bool HasMask() const { return m_mask != nullptr; }

    void SyncToCpp();
    void SyncToJs();

    int GetJavascriptId() const;

protected:
    void AllocateData();
    unsigned char* CreateComposite();

protected:
    mutable int m_jsId;
    unsigned char *m_bitmap;
    wxMask *m_mask;
    int m_width;
    int m_height;
    int m_depth;
    double m_scaleFactor;
    int m_dataWidth;
    int m_dataHeight;
    mutable BitmapDataSource m_dataSource;

    wxDECLARE_NO_COPY_CLASS(wxBitmapRefData);
};

wxBitmapRefData::wxBitmapRefData(int width, int height, int depth, double scale)
{
    m_jsId = -1;
    m_bitmap = nullptr;
    m_mask = nullptr;
    m_width = width;
    m_height = height;
    m_depth = depth;
    m_scaleFactor = scale;
    m_dataWidth = width * m_scaleFactor;
    m_dataHeight = height * m_scaleFactor;
    m_dataSource = BITMAP_DATA_SOURCE_NONE;
}

wxBitmapRefData::~wxBitmapRefData()
{
    if (m_jsId != -1)
    {
        EM_ASM({
            destroyBitmap($0);
        }, m_jsId);
    }

    delete [] m_bitmap;
    delete m_mask;
}

void wxBitmapRefData::AllocateData()
{
    if (m_bitmap == nullptr)
    {
        int size = GetDataSize();
        m_bitmap = new unsigned char[size];
        if (m_dataSource == BITMAP_DATA_SOURCE_NONE)
        {
            memset(m_bitmap, 0, size);
        }
    }
}

unsigned char* wxBitmapRefData::CreateComposite()
{
    // Initialize composite with original bitmap.
    int size = GetDataSize();
    unsigned char* compositeData = new unsigned char[size];

    int rowPixels = GetBytesPerRow() / sizeof(uint32_t);;

    uint32_t *bitmapPtr = reinterpret_cast<uint32_t*>(m_bitmap);
    uint32_t *maskPtr = m_mask->GetData();
    uint32_t *compositePtr = reinterpret_cast<uint32_t*>(compositeData);

    int width = GetScaledWidth();
    int height = GetScaledHeight();

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            *compositePtr = *maskPtr & bitmapPtr[x];

            maskPtr++;
            compositePtr++;
        }
        bitmapPtr += rowPixels;
    }

    return compositeData;
}

void wxBitmapRefData::SyncToCpp()
{
    switch (m_dataSource)
    {
        case BITMAP_DATA_SOURCE_NONE:
            AllocateData();
            break;
        case BITMAP_DATA_SOURCE_JS:
            AllocateData();
            wxASSERT_MSG(m_bitmap != nullptr, wxT("Data not allocated"));

            if (m_jsId != -1)
            {
                EM_ASM({
                    getBitmapData($0, $1);
                }, m_jsId, m_bitmap);
            }
            break;
        case BITMAP_DATA_SOURCE_CPP:
            break;
    }

    m_dataSource = BITMAP_DATA_SOURCE_CPP;
}

void wxBitmapRefData::SyncToJs()
{
    switch (m_dataSource)
    {
        case BITMAP_DATA_SOURCE_NONE:
            AllocateData();
        case BITMAP_DATA_SOURCE_CPP:
        {
            unsigned char* compositeData;
            unsigned char* data;

            if (HasMask())
            {
                compositeData = CreateComposite();
                data = compositeData;
            }
            else
            {
                compositeData = nullptr;
                data = m_bitmap;
            }

            if (m_jsId == -1)
            {
                m_jsId = EM_ASM_INT({
                    return createBitmap($0, $1, $2, $3);
                }, m_dataWidth, m_dataHeight, data, m_scaleFactor);
            }
            else
            {
                EM_ASM({
                    setBitmapData($0, $1, $2, $3, $4);
                }, m_jsId, m_dataWidth, m_dataHeight, data, m_scaleFactor);
            }
            m_dataSource = BITMAP_DATA_SOURCE_JS;

            delete [] compositeData;
        }
        break;
        case BITMAP_DATA_SOURCE_JS:
            break;
    }
}

int wxBitmapRefData::GetJavascriptId() const
{
    return m_jsId;
}

//-----------------------------------------------------------------------------
// wxMask
//-----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxMask, wxObject)

wxMask::wxMask()
    : m_dataSize(0),
      m_data(nullptr)
{
}

wxMask::wxMask(const wxMask& mask)
{
    m_dataSize = mask.m_dataSize;

    if (m_dataSize > 0)
    {
        m_data = new uint32_t[m_dataSize];
        memcpy(m_data, mask.m_data, m_dataSize * sizeof(uint32_t));
    }
    else
    {
        m_data = nullptr;
    }

    m_bitmap = mask.m_bitmap;
}

wxMask::wxMask(const wxBitmap& bitmap, const wxColour& colour)
    : m_dataSize(0),
      m_data(nullptr)
{
    Create(bitmap, colour);
}

#if wxUSE_PALETTE
wxMask::wxMask(const wxBitmap& bitmap, int paletteIndex)
    : m_dataSize(0),
      m_data(nullptr)
{
    Create(bitmap, paletteIndex);
}
#endif // wxUSE_PALETTE

wxMask::wxMask(const wxBitmap& bitmap)
    : m_dataSize(0),
      m_data(nullptr)
{
    Create(bitmap);
}

wxMask::~wxMask()
{
    FreeData();
}

wxBitmap wxMask::GetBitmap() const
{
    return m_bitmap;
}

uint32_t* wxMask::GetData() const
{
    return m_data;
}

void wxMask::FreeData()
{
    delete [] m_data;
    m_data = nullptr;
}

bool wxMask::InitFromColour(const wxBitmap& bitmap, const wxColour& colour)
{
    bitmap.SyncToCpp();

    int width = bitmap.GetScaledWidth();
    int height = bitmap.GetScaledHeight();

    m_dataSize = width * height;
    m_data = new uint32_t[m_dataSize];

    const uint32_t maskOn = 0xffffffff;
    const uint32_t maskOff = 0x00000000;

    // Creates mask to filters out the alpha channel.
    const uint32_t colorMask = wxColour(0xff, 0xff, 0xff, 0x00).GetRGBA();
    uint32_t maskColor = colour.GetRGBA() & colorMask;

    int rowPixels = bitmap.GetBytesPerRow() / sizeof(uint32_t);;

    wxBitmapRefData* maskBitmapRef = static_cast<wxBitmapRefData*>(bitmap.GetRefData());
    uint32_t* bitmapPtr = reinterpret_cast<uint32_t*>(maskBitmapRef->GetData());
    uint32_t* dataPtr = m_data;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            *dataPtr = (bitmapPtr[x] & colorMask) == maskColor ? maskOff : maskOn;
            ++dataPtr;
        }

        bitmapPtr += rowPixels;
    }
    bitmapPtr = reinterpret_cast<uint32_t*>(maskBitmapRef->GetData());
    dataPtr = m_data;

    return true;
}

bool wxMask::InitFromMonoBitmap(const wxBitmap& bitmap)
{
    // TODO: implement
    wxFAIL_MSG(wxT("InitFromMonoBitmap is not implemented"));
    m_bitmap = bitmap;
    return true;
}

//-----------------------------------------------------------------------------
// wxBitmap
//-----------------------------------------------------------------------------

#define M_BITMAPDATA static_cast<wxBitmapRefData*>(m_refData)

IMPLEMENT_DYNAMIC_CLASS(wxBitmap, wxGDIObject)

wxBitmap::wxBitmap()
{
}

wxBitmap::wxBitmap(int width, int height, int depth)
{
    bool retval = Create(width, height, depth);
    wxASSERT_MSG(retval, wxT("error creating bitmap"));
}

wxBitmap::wxBitmap(const wxSize& sz, int depth)
{
    bool retval = Create(sz, depth);
    wxASSERT_MSG(retval, wxT("error creating bitmap"));
}

wxBitmap::wxBitmap(const char bits[], int width, int height, int depth)
{
    bool retval = Create(bits, width, height, depth);
    wxASSERT_MSG(retval, wxT("error creating bitmap"));
}

wxBitmap::wxBitmap(const wxString &filename, wxBitmapType type)
{
    bool retval = LoadFile(filename, type);
    wxASSERT_MSG(retval,
                 wxString::Format(wxT("error creating bitmap from file %s"), filename));
}

wxBitmap::wxBitmap(const wxImage& image, int depth, double scale)
{
    bool retval = Create(image, depth, scale);
    wxASSERT_MSG(retval, wxT("error creating bitmap"));
}

wxBitmap::wxBitmap(const wxImage& image, const wxDC& dc)
{
    bool retval = Create(image, -1, dc.GetContentScaleFactor());
    wxASSERT_MSG(retval, wxT("error creating bitmap"));
}

bool wxBitmap::Create(int width, int height, int depth)
{
    return CreateScaled(width, height, depth, 1.0);
}

bool wxBitmap::Create(const wxSize& sz, int depth)
{
    return Create(sz.GetWidth(), sz.GetHeight(), depth);
}

bool wxBitmap::Create(int width, int height, const wxDC& dc)
{
    return CreateScaled(width, height, dc.GetDepth(), dc.GetContentScaleFactor());
}

bool wxBitmap::Create(const char bits[], int width, int height, int depth)
{
    if (!Create(width, height, depth))
    {
        return false;
    }

    int srcBytesPerRow = static_cast<int>(ceil(width * depth / 8.0));
    int dstBytesPerRow = GetBytesPerRow();

    unsigned char *data = static_cast<unsigned char*>(BeginRawAccess());
    wxASSERT_MSG(data != nullptr, wxT("bitmap not allocated"));

    const char *srcPtr = bits;
    unsigned char *dstPtr = data;

    for (int y = 0; y < height; y++)
    {
        memcpy(dstPtr, srcPtr, srcBytesPerRow);

        srcPtr += srcBytesPerRow;
        dstPtr += dstBytesPerRow;
    }

    EndRawAccess();

    return true;
}

bool wxBitmap::CreateScaled(int width, int height, int depth, double scale)
{
    if (depth == wxBITMAP_SCREEN_DEPTH)
    {
        depth = wxDisplayDepth();
    }

    UnRef();

    wxCHECK_MSG(depth == 32 || depth == 24, false,
                wxString::Format("unsupported bitmap depth: %d", depth));
    wxCHECK_MSG(width >= 0 && height >= 0, false, wxT("invalid bitmap size"));

    m_refData = new wxBitmapRefData(width, height, depth, scale);

    return true;
}

#if wxUSE_IMAGE

bool wxBitmap::Create(const wxImage& image, int depth, double scale)
{
    bool hasAlpha = image.HasAlpha();

    if (depth == wxBITMAP_SCREEN_DEPTH)
    {
        depth = wxDisplayDepth();
    }

    wxCHECK_MSG(depth == 32 || (depth == 24 && !hasAlpha), false,
                wxString::Format("unsupported bitmap depth: %d", depth));

    int scaledWidth = image.GetWidth() / scale;
    int scaledHeight = image.GetHeight() / scale;

    const int width = scaledWidth * scale;
    const int height = scaledHeight * scale;

    if (!CreateScaled(scaledWidth, scaledHeight, depth, scale))
    {
        return false;
    }

    int bytesPerRow = GetBytesPerRow();
    if (bytesPerRow < 0)
    {
        return false;
    }

    unsigned char *data = static_cast<unsigned char *>(BeginRawAccess());
    if (data == nullptr)
    {
        return false;
    }

    unsigned char *rowPtr = data;

    for (int y = 0; y < height; y++)
    {
        unsigned char *dstPtr = rowPtr;

        for (int x = 0; x < width; x++)
        {
            *dstPtr++ = image.GetRed(x, y);
            *dstPtr++ = image.GetGreen(x, y);
            *dstPtr++ = image.GetBlue(x, y);
            *dstPtr++ = hasAlpha ? image.GetAlpha(x, y) : 0xff;
        }
        rowPtr += bytesPerRow;
    }

    EndRawAccess();

    if (image.HasMask())
    {
        wxColour maskColor(image.GetMaskRed(),
                           image.GetMaskGreen(),
                           image.GetMaskBlue());
        SetMask(new wxMask(*this, maskColor));
    }

    return true;
}

wxImage wxBitmap::ConvertToImage() const
{
    wxCHECK_MSG(IsOk(), wxNullImage, wxT("invalid bitmap"));

    const int width = GetWidth();
    const int height = GetHeight();
    const int depth = GetDepth();
    const bool hasAlpha = HasAlpha();

    wxCHECK_MSG(depth == 32 || depth == 24, wxNullImage, wxT("unsupported depth"));

    int bytesPerRow = GetBytesPerRow();
    if (bytesPerRow < 0)
    {
        return wxNullImage;
    }

    unsigned char *data = static_cast<unsigned char *>(BeginRawAccess());
    if (data == nullptr)
    {
        return wxNullImage;
    }

    unsigned char *rowPtr = data;

    wxImage image(width, height, false);

    if (hasAlpha)
    {
        image.InitAlpha();
    }

    for (int y = 0; y < height; y++)
    {
        unsigned char *srcPtr = rowPtr;

        for (int x = 0; x < width; x++)
        {
            const unsigned char r = *srcPtr++;
            const unsigned char g = *srcPtr++;
            const unsigned char b = *srcPtr++;
            const unsigned char a = *srcPtr++;

            image.SetRGB(x, y, r, g, b);

            if (hasAlpha)
            {
                image.SetAlpha(x, y, a);
            }
        }
        rowPtr += bytesPerRow;
    }

    EndRawAccess();

    return image;
}

#endif // wxUSE_IMAGE

int wxBitmap::GetHeight() const
{
    wxCHECK_MSG(IsOk(), -1, wxT("invalid bitmap"));
    return M_BITMAPDATA->m_height;
}

int wxBitmap::GetWidth() const
{
    wxCHECK_MSG(IsOk(), -1, wxT("invalid bitmap"));
    return M_BITMAPDATA->m_width;
}

int wxBitmap::GetDepth() const
{
    wxCHECK_MSG(IsOk(), -1, wxT("invalid bitmap"));
    return M_BITMAPDATA->m_depth;
}

double wxBitmap::GetScaleFactor() const
{
    wxCHECK_MSG(IsOk(), -1, wxT("invalid bitmap"));
    return M_BITMAPDATA->GetScaleFactor();
}

double wxBitmap::GetScaledWidth() const
{
    return M_BITMAPDATA->GetScaledWidth();
}

double wxBitmap::GetScaledHeight() const
{
    return M_BITMAPDATA->GetScaledHeight();
}

wxMask *wxBitmap::GetMask() const
{
    wxCHECK_MSG(IsOk(), nullptr, wxT("invalid bitmap"));
    return M_BITMAPDATA->m_mask;
}

void wxBitmap::SetMask(wxMask *mask)
{
    AllocExclusive();

    delete M_BITMAPDATA->m_mask;
    M_BITMAPDATA->m_mask = nullptr;

    if (mask->GetDataSize() != GetScaledWidth() * GetScaledHeight())
    {
        wxFAIL_MSG("bitmap and mask dimensions must match");
        delete mask;
        return;
    }

    M_BITMAPDATA->m_mask = mask;

    M_BITMAPDATA->SyncToCpp();
}

wxBitmap wxBitmap::GetSubBitmap(const wxRect& rect) const
{
    wxBitmap subBitmap;

    wxCHECK_MSG(IsOk(), subBitmap, wxT("invalid bitmap"));

    const wxBitmapRefData* bitmapData = M_BITMAPDATA;

    wxCHECK_MSG(rect.x >= 0 && rect.y >= 0 &&
                rect.x + rect.width <= bitmapData->m_width &&
                rect.y + rect.height <= bitmapData->m_height,
                subBitmap, wxT("invalid bitmap region"));

    M_BITMAPDATA->SyncToCpp();

    wxBitmapRefData * const newRef =
        new wxBitmapRefData(rect.width,
                            rect.height,
                            bitmapData->m_depth,
                            bitmapData->m_scaleFactor);

    subBitmap.m_refData = newRef;

    double sf = GetScaleFactor();

    const int scaledWidth = rect.width * sf;
    const int scaledHeight = rect.height * sf;
    const int srcX = rect.x * sf;
    const int srcY = rect.y * sf;
    const int srcBytesPerRow = GetBytesPerRow();
    const int dstBytesPerRow = subBitmap.GetBytesPerRow();

    const int bytesPerPixel = GetBytesPerPixel();
    const int rowSize = bytesPerPixel * scaledWidth;
    const unsigned char* srcPtr = M_BITMAPDATA->m_bitmap + srcY * srcBytesPerRow + bytesPerPixel * srcX;
    unsigned char *dstPtr = static_cast<unsigned char*>(subBitmap.BeginRawAccess());

    for (int y = 0; y < scaledHeight; y++)
    {
        memcpy(dstPtr, srcPtr, rowSize);
        srcPtr += srcBytesPerRow;
        dstPtr += dstBytesPerRow;
    }

    subBitmap.EndRawAccess();

    // TODO: copy mask

    return subBitmap;
}

bool wxBitmap::SaveFile(const wxString &WXUNUSED(name),
                        wxBitmapType WXUNUSED(type),
                        const wxPalette *WXUNUSED(palette)) const
{
    // TODO: implement
    wxFAIL_MSG(wxT("SaveFile is not implemented"));
    return true;
}

bool wxBitmap::LoadFile(const wxString &WXUNUSED(filename), wxBitmapType WXUNUSED(type))
{
    // TODO: implement
    wxFAIL_MSG(wxT("LoadFile is not implemented"));
    return false;
}

void *wxBitmap::GetRawData(wxPixelDataBase& data, int bpp)
{
    wxCHECK_MSG(IsOk(), nullptr, wxT("invalid bitmap"));
    wxCHECK_MSG(bpp == GetDepth(), nullptr, wxT("wrong depth"));

    data.m_width = GetScaledWidth();
    data.m_height = GetScaledHeight();
    data.m_stride = GetBytesPerRow();

    return BeginRawAccess();
}

void wxBitmap::UngetRawData(wxPixelDataBase& WXUNUSED(data))
{
    EndRawAccess();
}

void *wxBitmap::BeginRawAccess() const
{
    wxCHECK_MSG(IsOk(), nullptr, wxT("invalid bitmap"));

    M_BITMAPDATA->SyncToCpp();

    return M_BITMAPDATA->m_bitmap;
}

void wxBitmap::EndRawAccess() const
{
    wxCHECK_RET(IsOk(), wxT("invalid bitmap"));
}

int wxBitmap::GetBytesPerPixel() const
{
    wxCHECK_MSG(IsOk(), -1, wxT("invalid bitmap"));

    return M_BITMAPDATA->GetBytesPerPixel();
}

int wxBitmap::GetBytesPerRow() const
{
    wxCHECK_MSG(IsOk(), -1, wxT("invalid bitmap"));

    return M_BITMAPDATA->GetBytesPerRow();
}

#if wxUSE_PALETTE
wxPalette *wxBitmap::GetPalette() const
{
    // TODO: implement
    wxFAIL_MSG(wxT("GetPalette is not implemented"));
    return nullptr;
}

void wxBitmap::SetPalette(const wxPalette& WXUNUSED(palette))
{
    // TODO: implement
    wxFAIL_MSG(wxT("SetPalette is not implemented"));
}
#endif // wxUSE_PALETTE

bool wxBitmap::CopyFromIcon(const wxIcon& icon)
{
    *this = icon;
    return IsOk();
}

void wxBitmap::SetHeight(int height)
{
    AllocExclusive();
    M_BITMAPDATA->m_height = height;
}

void wxBitmap::SetWidth(int width)
{
    AllocExclusive();
    M_BITMAPDATA->m_width = width;
}

void wxBitmap::SetDepth(int depth)
{
    wxCHECK_RET(depth == 24 || depth == 32,
                wxString::Format("unsupported bitmap depth: %d", depth));
    AllocExclusive();
    M_BITMAPDATA->m_depth = depth;
}

/* static */
void wxBitmap::InitStandardHandlers()
{
}

wxGDIRefData* wxBitmap::CreateGDIRefData() const
{
    return new wxBitmapRefData(0, 0, 0, 1.0);
}

wxGDIRefData* wxBitmap::CloneGDIRefData(const wxGDIRefData* data) const
{
    const wxBitmapRefData* oldRef = static_cast<const wxBitmapRefData*>(data);
    wxBitmapRefData *const newRef = new wxBitmapRefData(oldRef->m_width,
            oldRef->m_height,
            oldRef->m_depth,
            oldRef->m_scaleFactor);

    if (oldRef->GetDataSource() == BITMAP_DATA_SOURCE_JS)
    {
        newRef->m_jsId = oldRef->m_jsId;
        newRef->m_dataSource = BITMAP_DATA_SOURCE_JS;
        newRef->SyncToCpp();
        newRef->m_jsId = -1;
    }
    else if (oldRef->m_bitmap != nullptr)
    {
        int size = oldRef->GetDataSize();
        newRef->m_bitmap = new unsigned char[size];
        memcpy(newRef->m_bitmap, oldRef->m_bitmap, size);
    }

    newRef->m_dataSource = BITMAP_DATA_SOURCE_CPP;

    if (oldRef->m_mask != nullptr)
    {
        newRef->m_mask = new wxMask(*oldRef->m_mask);
    }

    return newRef;
}

void wxBitmap::SyncToCpp() const
{
    M_BITMAPDATA->SyncToCpp();
}

void wxBitmap::SyncToJs() const
{
    M_BITMAPDATA->SyncToJs();
}

int wxBitmap::GetJavascriptId() const
{
    return M_BITMAPDATA->GetJavascriptId();
}
