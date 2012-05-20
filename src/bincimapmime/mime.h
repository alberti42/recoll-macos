/* -*- mode:c++;c-basic-offset:2 -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    src/parsers/mime/mime.h
 *  
 *  Description:
 *    Declaration of main mime parser components
 *  --------------------------------------------------------------------
 *  Copyright 2002-2005 Andreas Aardal Hanssen
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *  --------------------------------------------------------------------
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef mime_h_included
#define mime_h_included
#include <string>
#include <vector>
#include <map>
#include <stdio.h>

namespace Binc {
  //---------------------------------------------------------------------- 
  class HeaderItem {
  private:
    mutable std::string key;
    mutable std::string value;

  public:
    inline const std::string &getKey(void) const { return key; }
    inline const std::string &getValue(void) const { return value; }

    //--
    HeaderItem(void);
    HeaderItem(const std::string &key, const std::string &value);
  };

  //---------------------------------------------------------------------- 
  class Header {
  private:
    mutable std::vector<HeaderItem> content;

  public:
    bool getFirstHeader(const std::string &key, HeaderItem &dest) const;
    bool getAllHeaders(const std::string &key, std::vector<HeaderItem> &dest) const;
    void add(const std::string &name, const std::string &content);
    void clear(void) const;

    //--
    Header(void);
    ~Header(void);
  };

  //----------------------------------------------------------------------
  class IODevice;
  class MimeDocument;
  class MimePart {
  protected:
  public:
    mutable bool multipart;
    mutable bool messagerfc822;
    mutable std::string subtype;
    mutable std::string boundary;

    mutable unsigned int headerstartoffsetcrlf;
    mutable unsigned int headerlength;

    mutable unsigned int bodystartoffsetcrlf;
    mutable unsigned int bodylength;
    mutable unsigned int nlines;
    mutable unsigned int nbodylines;
    mutable unsigned int size;

  public:
    enum FetchType {
      FetchBody,
      FetchHeader,
      FetchMime
    };

    mutable Header h;

    mutable std::vector<MimePart> members;

    inline const std::string &getSubType(void) const { return subtype; }
    inline bool isMultipart(void) const { return multipart; }
    inline bool isMessageRFC822(void) const { return messagerfc822; }
    inline unsigned int getSize(void) const { return bodylength; }
    inline unsigned int getNofLines(void) const { return nlines; }
    inline unsigned int getNofBodyLines(void) const { return nbodylines; }
    inline unsigned int getBodyLength(void) const { return bodylength; }
    inline unsigned int getBodyStartOffset(void) const { return bodystartoffsetcrlf; }

    void printBody(int fd, Binc::IODevice &output, unsigned int startoffset, unsigned int length) const;
      void getBody(int fd, std::string& s, unsigned int startoffset, unsigned int length) const;
      void getBody(std::string& s, unsigned int startoffset, unsigned int length) const;
    void printHeader(int fd, Binc::IODevice &output, std::vector<std::string> headers, bool includeheaders, unsigned int startoffset, unsigned int length, std::string &storage) const;
    void printDoc(int fd, Binc::IODevice &output, unsigned int startoffset, unsigned int length) const;
    virtual void clear(void) const;

    const MimePart *getPart(const std::string &findpart, std::string genpart, FetchType fetchType = FetchBody) const;
    virtual int doParseOnlyHeader(const std::string &toboundary) const;
    virtual int doParseFull(const std::string &toboundary, int &boundarysize) const;

    MimePart(void);
    virtual ~MimePart(void);
  };

  //----------------------------------------------------------------------
  class MimeDocument : public MimePart {
  private:
    mutable bool headerIsParsed;
    mutable bool allIsParsed;

  public:
    void parseOnlyHeader(int fd) const;
    void parseFull(int fd) const;
    void parseOnlyHeader(std::istream& s) const;
    void parseFull(std::istream& s) const;
    void clear(void) const;
    
    inline bool isHeaderParsed(void) { return headerIsParsed; }
    inline bool isAllParsed(void) { return allIsParsed; }

    //--
    MimeDocument(void);
    ~MimeDocument(void);
  };

};

#endif
