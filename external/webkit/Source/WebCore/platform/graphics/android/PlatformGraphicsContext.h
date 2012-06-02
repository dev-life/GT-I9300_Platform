/*
 * Copyright 2006, The Android Open Source Project
 * Copyright (c) 2011, Code Aurora Forum. All rights reserved.
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

#ifndef platform_graphics_context_h
#define platform_graphics_context_h

#include "IntRect.h"
#include "RenderSkinAndroid.h"
#include "RenderSkinButton.h"
#include "SkCanvas.h"
#include "SkPicture.h"
#include "SkTDArray.h"

class SkCanvas;

namespace WebCore {

    class GraphicsContext;
    
class PlatformGraphicsContext {
public:
    PlatformGraphicsContext();
    // Pass in a recording canvas, and an array of button information to be 
    // updated.
    PlatformGraphicsContext(SkCanvas* canvas);
    // Create a recording canvas
    PlatformGraphicsContext(int width, int height);
    ~PlatformGraphicsContext();
    
    SkCanvas*                   mCanvas;
    
    bool deleteUs() const { return m_deleteCanvas; }
    void convertToNonRecording();
    void clearRecording();
    SkPicture* getRecordingPicture() const { return m_picture; }

    // When we detect animation we switch to the recording canvas for better
    // performance. If JS tries to access the pixels of the canvas, the
    // recording canvas becomes tainted and must be converted back to a bitmap
    // backed canvas.
    // CanvasState represents a directed acyclic graph:
    // DEFAULT ----> ANIMATION_DETECTED ----> RECORDING ----> DIRTY
    enum CanvasState {
        DEFAULT, // SkBitmap backed
        ANIMATION_DETECTED, // JavaScript clearRect of canvas is occuring at a high enough rate SkBitmap backed
        RECORDING, // SkPicture backed
        DIRTY // A pixel readback occured; convert to SkBitmap backed.
    };

    bool isDefault() const { return m_canvasState == DEFAULT; }
    bool isAnimating() const { return m_canvasState == ANIMATION_DETECTED; }
    bool isRecording() const { return m_canvasState == RECORDING; }
    bool isDirty() const { return m_canvasState == DIRTY; }

    void setIsAnimating();
private:
    bool                     m_deleteCanvas;
    enum CanvasState m_canvasState;

    SkPicture* m_picture;
};

}
#endif
