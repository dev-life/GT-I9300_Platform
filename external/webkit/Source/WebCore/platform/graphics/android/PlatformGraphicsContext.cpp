/*
 * Copyright 2006, The Android Open Source Project
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

#define LOG_TAG "PlatformGraphicsContext"
#include "config.h"
#include "Node.h"
#include "PlatformGraphicsContext.h"
#include "SkCanvas.h"
#include <utils/Log.h>

namespace WebCore {

PlatformGraphicsContext::PlatformGraphicsContext(SkCanvas* canvas)
        : mCanvas(canvas), m_deleteCanvas(false)
        , m_canvasState(DEFAULT)
        , m_picture(0)
{
}

PlatformGraphicsContext::PlatformGraphicsContext()
    : mCanvas(new SkCanvas), m_deleteCanvas(true)
//    , m_buttons(0)
    , m_canvasState(DEFAULT)
    , m_picture(0)
{
}

PlatformGraphicsContext::PlatformGraphicsContext(int width, int height)
    : m_deleteCanvas(false)
    , m_canvasState(RECORDING)
    , m_picture(new SkPicture)
{
    mCanvas = m_picture->beginRecording(width, height, 0);
}

PlatformGraphicsContext::~PlatformGraphicsContext()
{
    if (m_picture) {
        delete m_picture; // The SkPicture will free mCanvas in its destructor.
        m_picture = 0;
        return;
    }

    if (m_deleteCanvas) {
        delete mCanvas;
        mCanvas = 0;
    }
}

void PlatformGraphicsContext::clearRecording()
{
    if (!isRecording() || !m_picture)
        return;

    int width = m_picture->width();
    int height = m_picture->height();

    delete m_picture;
    m_picture = new SkPicture;

    mCanvas = m_picture->beginRecording(width, height, 0);
}

void PlatformGraphicsContext::convertToNonRecording()
{
    if (!isRecording() || !m_picture)
        return;

    SkBitmap bitmap;

    bitmap.setConfig(SkBitmap::kARGB_8888_Config, m_picture->width(), m_picture->height());
    bitmap.allocPixels();
    bitmap.eraseColor(0);

    SkCanvas* canvas = new SkCanvas;
    canvas->setBitmapDevice(bitmap);

    m_picture->draw(canvas);

    mCanvas = canvas;
    m_deleteCanvas = true;

    delete m_picture;
    m_picture = 0;

    SLOGD("[%s] The HTML5 recording canvas has been tainted. Converting to bitmap buffer.", __FUNCTION__);
    m_canvasState = DIRTY;
}

void PlatformGraphicsContext::setIsAnimating()
{
    if (!m_canvasState == DEFAULT)
        return;

    m_canvasState = ANIMATION_DETECTED;
}

}   // WebCore
