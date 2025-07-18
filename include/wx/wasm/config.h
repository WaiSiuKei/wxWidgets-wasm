/////////////////////////////////////////////////////////////////////////////
// Name:        wx/wasm/config.h
// Purpose:     wxLocalStorageConfig class
// Author:      Adam Hilss
// Copyright:   (c) 2019 Adam Hilss
// Licence:     LGPL v2
/////////////////////////////////////////////////////////////////////////////


#ifndef _WX_WASM_CONFIG_H_
#define _WX_WASM_CONFIG_H_

#include "wx/defs.h"

#if wxUSE_CONFIG

#include "wx/string.h"
#include "wx/confbase.h"

// ----------------------------------------------------------------------------
// wxLocalStorageConfig
// ----------------------------------------------------------------------------

/*
*/

class WXDLLIMPEXP_BASE wxLocalStorageConfig : public wxConfigBase
{
public:
  // ctor & dtor
  wxLocalStorageConfig(const wxString& appName = wxEmptyString,
                       const wxString& vendorName = wxEmptyString,
                       const wxString& localFilename = wxEmptyString,
                       const wxString& globalFilename = wxEmptyString,
                       long style = 0);

  virtual ~wxLocalStorageConfig();

  // implement inherited pure virtual functions
  // ------------------------------------------

  // path management
  virtual void SetPath(const wxString& strPath) override;
  virtual const wxString& GetPath() const override { return m_strPath; }

  // entry/subgroup info
  // enumerate all of them
  virtual bool GetFirstGroup(wxString& str, long& lIndex) const override;
  virtual bool GetNextGroup (wxString& str, long& lIndex) const override;
  virtual bool GetFirstEntry(wxString& str, long& lIndex) const override;
  virtual bool GetNextEntry (wxString& str, long& lIndex) const override;

  // tests for existence
  virtual bool HasGroup(const wxString& strName) const override;
  virtual bool HasEntry(const wxString& strName) const override;

  // get number of entries/subgroups in the current group, with or without
  // it's subgroups
  virtual size_t GetNumberOfEntries(bool bRecursive = false) const override;
  virtual size_t GetNumberOfGroups(bool bRecursive = false) const override;

  virtual bool Flush(bool WXUNUSED(bCurrentOnly) = false) override { return true; }

  // rename
  virtual bool RenameEntry(const wxString& oldName, const wxString& newName) override;
  virtual bool RenameGroup(const wxString& oldName, const wxString& newName) override;

  // delete
  virtual bool DeleteEntry(const wxString& key, bool bGroupIfEmptyAlso = true) override;
  virtual bool DeleteGroup(const wxString& key) override;
  virtual bool DeleteAll() override;

protected:
  virtual bool DoReadString(const wxString& key, wxString *pstr) const override;
  virtual bool DoReadLong(const wxString& key, long *pl) const override;
  virtual bool DoReadBool(const wxString& key, bool *pb) const override;
#if wxUSE_BASE64
  virtual bool DoReadBinary(const wxString& key, wxMemoryBuffer* buf) const override;
#endif // wxUSE_BASE64

  virtual bool DoWriteString(const wxString& key, const wxString& str) override;
  virtual bool DoWriteLong(const wxString& key, long l) override;
  virtual bool DoWriteBool(const wxString& key, bool b) override;
#if wxUSE_BASE64
  virtual bool DoWriteBinary(const wxString& key, const wxMemoryBuffer& buf) override;
#endif // wxUSE_BASE64

  wxString MakeEntryKey(const wxString& key) const;
  wxString MakeGroupKey(const wxString& key) const;

private:
  // member variables
  // ----------------
  wxString  m_strPath;

  wxDECLARE_NO_COPY_CLASS(wxLocalStorageConfig);
  wxDECLARE_ABSTRACT_CLASS(wxLocalStorageConfig);
};

#endif // wxUSE_CONFIG

#endif // _WX_WASM_CONFIG_H_
