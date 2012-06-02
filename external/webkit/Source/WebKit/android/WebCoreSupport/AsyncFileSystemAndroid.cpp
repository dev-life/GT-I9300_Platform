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
#include "AsyncFileSystemAndroid.h"

#define LOG_NDEBUG 0
#define LOGTAG "FileSystem"

#if ENABLE(FILE_SYSTEM)

#include "AsyncFileSystemCallbacks.h"
#include "AsyncFileWriterAndroid.h"
#include "WebFileInfo.h"
#include "WebFileWriter.h"
#include "WebKit.h"
#include "FileSystem.h"
#include "Logging.h"
#include "FileError.h"
#include "FileMetadata.h"
#include <wtf/text/CString.h>
#undef LOG
#include <utils/Log.h>

namespace WebCore {

AsyncFileSystemAndroid::AsyncFileSystemAndroid(AsyncFileSystem::Type type, const String& rootPath)
    : AsyncFileSystem(type, rootPath)
{
    isDirectory = false;
}

AsyncFileSystemAndroid::~AsyncFileSystemAndroid()
{
}

void AsyncFileSystemAndroid::openFileSystem(const String& basePath, const String& storageIdentifier, Type type, bool, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    String typeString = (type == Persistent) ? "Persistent" : "Temporary";

    String name = storageIdentifier;
    name += ":";
    name += typeString;

    String rootPath = basePath;
    LOGE("AsyncFileSystemAndroid::openFileSystem basePath value is %s",basePath.utf8().data());
    rootPath.append(PlatformFilePathSeparator);
    rootPath += storageIdentifier;
    rootPath.append(PlatformFilePathSeparator);
    rootPath += typeString;
    if(makeAllDirectories(rootPath)){
       rootPath.append(PlatformFilePathSeparator);
       LOGE("AsyncFileSystemAndroid::openFileSystem path value is %s",rootPath.utf8().data());
       callbacks->didOpenFileSystem(name, AsyncFileSystemAndroid::create(type, rootPath));
    }
}

void AsyncFileSystemAndroid::move(const String& sourcePath, const String& destinationPath, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    
}

void AsyncFileSystemAndroid::copy(const String& sourcePath, const String& destinationPath, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    
}

void AsyncFileSystemAndroid::remove(const String& path, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    LOGE("Remove path value is %s",path.utf8().data());
    int res = deleteFile(path);    
    LOGE("res is %d",res);
    if(!res)
        callbacks->didSucceed();
    else
        callbacks->didFail(res);
           
}

void AsyncFileSystemAndroid::removeRecursively(const String& path, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    LOGE("Removerecurse path value is %s",path.utf8().data());
    int res = deleteDirectory(path);
    LOGE("res recurse is %d",res);
    if(!res)
        callbacks->didSucceed();
    else
        callbacks->didFail(res);
}

void AsyncFileSystemAndroid::readMetadata(const String& path, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    FileMetadata metadata;
    long long size;
    time_t modtime;
    if(getFileSize(path,size)){
       if(getFileModificationTime(path,modtime)){
          metadata.modificationTime = modtime;
          metadata.length = size;
          callbacks->didReadMetadata(metadata);
       }
    }
}

void AsyncFileSystemAndroid::createFile(const String& path, bool exclusive, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    LOGE("AsyncFileSystemAndroid:: createFile Path value is %s",path.utf8().data());
    PlatformFileHandle file;
    file=openFile(path,OpenForWrite);
    if(isHandleValid(file)){
        closeFile(file);
        callbacks->didSucceed();
    }
    else
    {
       callbacks->didFail(FileError::NOT_FOUND_ERR);
    }
    
}

void AsyncFileSystemAndroid::createDirectory(const String& path, bool exclusive, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
     LOGE("AsyncFileSystemAndroid:: createDirectory Path value is %s",path.utf8().data());
     if(makeAllDirectories(path)){
       callbacks->didSucceed();
      }
     else
     {
       callbacks->didFail(FileError::NOT_FOUND_ERR);
     }
}

void AsyncFileSystemAndroid::fileExists(const String& path, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    if(WebCore::fileExists(path))
        callbacks->didSucceed();
}

void AsyncFileSystemAndroid::directoryExists(const String& path, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    
}

void AsyncFileSystemAndroid::readDirectory(const String& path, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    LOGE("directory path is %s",path.utf8().data());
    Vector<String> list = listDirectory(path,"*");
    unsigned size = list.size();
    for(unsigned i=0;i<size;i++){
     String dirName = list[i];
     LOGE("directory Name is %s",dirName.utf8().data());
     int index = dirName.reverseFind("///");
     String dirExt = dirName.substring(index + 1);
     LOGE("directory Ext is %s",dirExt.utf8().data());
     int index1 = dirExt.reverseFind('.');
     if(index1 == -1)
        isDirectory = true;
     else
        isDirectory = false;
     callbacks->didReadDirectoryEntry(dirName,isDirectory);
    }
    callbacks->didReadDirectoryEntries(false);
      
}

void AsyncFileSystemAndroid::createWriter(AsyncFileWriterClient* client, const String& path, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    long long int len ;
    getFileSize(path,len);
    LOGE("AsyncFileSystemAndroid::createWriter len is %lld",len);
    PassOwnPtr<AsyncFileWriterAndroid> asyncFileWriterAndroid = adoptPtr(new AsyncFileWriterAndroid(client,path));
    callbacks->didCreateFileWriter(asyncFileWriterAndroid,len);
    
}

} // namespace WebCore

#endif
