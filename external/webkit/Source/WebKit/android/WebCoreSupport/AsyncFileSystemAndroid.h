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

#ifndef  AsyncFileSystemAndroid_h
#define  AsyncFileSystemAndroid_h

#if ENABLE(FILE_SYSTEM)

#include "AsyncFileSystem.h"
#include <wtf/PassOwnPtr.h>

namespace WebKit {
class WebFileSystem;
}

namespace WebCore {

class AsyncFileSystemCallbacks;

class AsyncFileSystemAndroid : public AsyncFileSystem {
public:
    static PassOwnPtr<AsyncFileSystem> create(AsyncFileSystem::Type type, const String& rootPath)
    {
        return adoptPtr(new AsyncFileSystemAndroid(type, rootPath));
    }

    virtual ~AsyncFileSystemAndroid();

    virtual void move(const String& sourcePath, const String& destinationPath, PassOwnPtr<AsyncFileSystemCallbacks>);
    virtual void copy(const String& sourcePath, const String& destinationPath, PassOwnPtr<AsyncFileSystemCallbacks>);
    virtual void remove(const String& path, PassOwnPtr<AsyncFileSystemCallbacks>);
    virtual void removeRecursively(const String& path, PassOwnPtr<AsyncFileSystemCallbacks>);
    virtual void readMetadata(const String& path, PassOwnPtr<AsyncFileSystemCallbacks>);
    virtual void createFile(const String& path, bool exclusive, PassOwnPtr<AsyncFileSystemCallbacks>);
    virtual void createDirectory(const String& path, bool exclusive, PassOwnPtr<AsyncFileSystemCallbacks>);
    virtual void fileExists(const String& path, PassOwnPtr<AsyncFileSystemCallbacks>);
    virtual void directoryExists(const String& path, PassOwnPtr<AsyncFileSystemCallbacks>);
    virtual void readDirectory(const String& path, PassOwnPtr<AsyncFileSystemCallbacks>);
    virtual void createWriter(AsyncFileWriterClient* client, const String& path, PassOwnPtr<AsyncFileSystemCallbacks>);
    static void openFileSystem(const String& basePath, const String& storageIdentifier, Type, bool create, PassOwnPtr<AsyncFileSystemCallbacks>);
    //void didReadMetadata(const WebKit::WebFileInfo& info);

private:
    AsyncFileSystemAndroid(AsyncFileSystem::Type, const String& rootPath);
    bool isDirectory;
   };

} // namespace WebCore

#endif

#endif //  AsyncFileSystemAndroid_h
