/*
 * Copyright 2009, The Android Open Source Project
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "config.h"
#include "AsyncFileWriterAndroid.h"

#define LOG_NDEBUG 0
#define LOGTAG "FileSystem"

#if ENABLE(FILE_SYSTEM)

#include "AsyncFileWriterClient.h"
#include "Blob.h"
#include "WebFileWriter.h"
#include "FileSystem.h"
#include <wtf/text/CString.h>
#undef LOG
#include <utils/Log.h>
#include "BlobRegistry.h"
#include "BlobRegistryImpl.h"
#include "BlobStorageData.h"

namespace WebCore {

AsyncFileWriterAndroid::AsyncFileWriterAndroid(AsyncFileWriterClient* client, String path)
    : m_client(client),m_path(path)
{
}

AsyncFileWriterAndroid::~AsyncFileWriterAndroid()
{
}

void AsyncFileWriterAndroid::write(long long position, Blob* data)
{
    ASSERT(m_writer);
    String type = data->type();
    
    BlobRegistryImpl &blobRegistryimpl = static_cast<BlobRegistryImpl&>(blobRegistry());
    RefPtr<BlobStorageData> blobstoragedata = blobRegistryimpl.getBlobDataFromURL(data->url()).get();
    BlobDataItemList blobdataitemlist = blobstoragedata->items();
    BlobDataItem blobdataitem = blobdataitemlist[blobdataitemlist.size()-1];
    const char *str = blobdataitem.data->data();
    int length = blobdataitem.data->mutableData()->size();
    PlatformFileHandle file=openFile(m_path,OpenForWriteOnly);
    if(isHandleValid(file)){
      position = WebCore::seekFile(file,position,SeekFromBeginning);    
      WebCore::writeToFile(file,str,length);
    }

    closeFile(file);

}

void AsyncFileWriterAndroid::truncate(long long length)
{
    //ASSERT(m_writer);
    //m_writer->truncate(length);
}

void AsyncFileWriterAndroid::abort()
{
    //ASSERT(m_writer);
    //m_writer->cancel();
}




} // namespace

#endif // ENABLE(FILE_SYSTEM)
