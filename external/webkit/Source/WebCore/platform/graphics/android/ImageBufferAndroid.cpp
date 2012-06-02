/*
 * Copyright 2007, The Android Open Source Project
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
#include "ImageBuffer.h"

#include "Base64.h"
#include "BitmapImage.h"
#include "ColorSpace.h"
#include "GraphicsContext.h"
#include "NotImplemented.h"
#include "PlatformBridge.h"
#include "PlatformGraphicsContext.h"
#include "SkBitmapRef.h"
#include "SkCanvas.h"
#include "SkColorPriv.h"
#include "SkDevice.h"
#include "SkImageEncoder.h"
#include "SkPicture.h"
#include "SkStream.h"
#include "SkUnPreMultiply.h"
#include "android_graphics.h"
#include "CanvasLayerAndroid.h"


using namespace std;

namespace WebCore {

ImageBufferData::ImageBufferData(const IntSize&)
{
}

ImageBuffer::ImageBuffer(const IntSize& size, ColorSpace, RenderingMode, bool& success)
    : m_data(size)
    , m_size(size)
{
    // GraphicsContext creates a 32bpp SkBitmap, so 4 bytes per pixel.
    if (!PlatformBridge::canSatisfyMemoryAllocation(size.width() * size.height() * 4))
        success = false;
    else {
        m_context.set(GraphicsContext::createOffscreenContext(size.width(), size.height()));
        success = true;
    }
}

ImageBuffer::~ImageBuffer()
{
}

GraphicsContext* ImageBuffer::context() const
{
    return m_context.get();
}

void ImageBuffer::convertToRecording()
{
    GraphicsContext* context = GraphicsContext::createOffscreenRecordingContext(m_size.width(), m_size.height());
    context->copyState(m_context.get());
    m_context.set(context);
}

bool ImageBuffer::drawsUsingRecording() const
{
    if (!context() || !context()->platformContext())
        return false;

    return context()->platformContext()->isRecording();
}

bool ImageBuffer::isAnimating() const
{
    if (!context() || !context()->platformContext())
        return false;

    return context()->platformContext()->isAnimating();
}

void ImageBuffer::setIsAnimating() const
{
    if (!context() || !context()->platformContext())
        return;

    return context()->platformContext()->setIsAnimating();
}

void ImageBuffer::clearRecording() const
{
    if (!context() || !context()->platformContext())
        return;

    context()->platformContext()->clearRecording();
}

bool ImageBuffer::canUseGpuRendering()
{
#ifdef CANVAS2D_BMP_OPT
    SkPicture* canvasRecording = context()->platformContext()->getRecordingPicture();
    if(canvasRecording != NULL)
 	  {
	//return true;
        return canvasRecording->canUseGpuRendering();
    }
    else
    {
        return false;
    }
#else
	return false ;
#endif 
}

void ImageBuffer::copyRecordingToLayer( GraphicsContext* paintContext, const IntRect& r,
                                        CanvasLayerAndroid* canvasLayer) const
{
    SkPicture* canvasRecording = context()->platformContext()->getRecordingPicture();
    SkPicture dstPicture(*canvasRecording);
    canvasLayer->setPicture(dstPicture);
    //canvasLayer->setRect(r);
    clearRecording();
}

void ImageBuffer::resetRecordingToLayer( GraphicsContext* paintContext, const IntRect& r,
                                         CanvasLayerAndroid* canvasLayer) const
{
    SkPicture* canvasRecording = new SkPicture();
    SkPicture dstPicture(*canvasRecording);
    canvasLayer->setPicture(dstPicture);
    //canvasLayer->setRect(r);
    clearRecording();
}
void ImageBuffer::copyRecordingToCanvas(GraphicsContext* paintContext, const IntRect& r) const
{
    SkCanvas* paintCanvas = paintContext->platformContext()->mCanvas;
    SkPicture* canvasRecording = context()->platformContext()->getRecordingPicture();

    SkPaint paint;
    paint.setXfermodeMode(SkXfermode::kSrcOver_Mode);
	const SkIRect& clipBounds = paintCanvas->getTotalClip().getBounds();
	SkRect rect;
	int saveCount = 0; 
	if( (clipBounds.width() > r.width() * 2 ) || (clipBounds.height() > r.height() * 2))
	{
		rect.set(r.location().x(), r.location().y(),r.width(), r.height()); 
		saveCount = paintCanvas->saveLayer(&rect, &paint);
	}
	else
		saveCount = paintCanvas->saveLayer(0, &paint);
    paintCanvas->translate(r.location().x(), r.location().y());
    // Resize to the now known viewport.
    paintCanvas->scale(r.width() / width(), r.height() / height());
    // Draw the canvas recording into the layer recording canvas.
    canvasRecording->draw(paintCanvas);
    context()->platformContext()->clearRecording();
    paintCanvas->restoreToCount(saveCount);

}
bool ImageBuffer::drawsUsingCopy() const
{
    return true;
}

PassRefPtr<Image> ImageBuffer::copyImage() const
{
    ASSERT(context() && context()->platformContext());
#if ENABLE(ACCELERATED_2D_CANVAS)
    context()->syncSoftwareCanvas();
#endif
    // Request for canvas bitmap; conversion required.
    if (context()->platformContext()->isRecording())
        context()->platformContext()->convertToNonRecording();

    SkCanvas* canvas = context()->platformContext()->mCanvas;
    SkDevice* device = canvas->getDevice();
    const SkBitmap& orig = device->accessBitmap(false);

    SkBitmap copy;
    if (PlatformBridge::canSatisfyMemoryAllocation(orig.getSize()))
        orig.copyTo(&copy, orig.config());

    SkBitmapRef* ref = new SkBitmapRef(copy);
    RefPtr<Image> image = BitmapImage::create(ref, 0);
    ref->unref();
    return image;
}

void ImageBuffer::clip(GraphicsContext* context, const FloatRect& rect) const
{
    SkDebugf("xxxxxxxxxxxxxxxxxx clip not implemented\n");
}

void ImageBuffer::draw(GraphicsContext* context, ColorSpace styleColorSpace, const FloatRect& destRect, const FloatRect& srcRect, CompositeOperator op, bool useLowQualityScale)
{
#if ENABLE(ACCELERATED_2D_CANVAS)
    context->syncSoftwareCanvas();
#endif
    RefPtr<Image> imageCopy = copyImage();
    context->drawImage(imageCopy.get(), styleColorSpace, destRect, srcRect, op, useLowQualityScale);
}

void ImageBuffer::drawPattern(GraphicsContext* context, const FloatRect& srcRect, const AffineTransform& patternTransform, const FloatPoint& phase, ColorSpace styleColorSpace, CompositeOperator op, const FloatRect& destRect)
{
    RefPtr<Image> imageCopy = copyImage();
    imageCopy->drawPattern(context, srcRect, patternTransform, phase, styleColorSpace, op, destRect);
}

PassRefPtr<ByteArray> ImageBuffer::getUnmultipliedImageData(const IntRect& rect) const
{
    ASSERT(context() && context()->platformContext());

    // Request for canvas bitmap; conversion required.
    if (context()->platformContext()->isRecording())
        context()->platformContext()->convertToNonRecording();
    GraphicsContext* gc = this->context();
    if (!gc) {
        return 0;
    }

    const SkBitmap& src = android_gc2canvas(gc)->getDevice()->accessBitmap(false);
    SkAutoLockPixels alp(src);
    if (!src.getPixels()) {
        return 0;
    }

#if ENABLE(ACCELERATED_2D_CANVAS)
    context()->syncSoftwareCanvas();
#endif
    RefPtr<ByteArray> result = ByteArray::create(rect.width() * rect.height() * 4);
    unsigned char* data = result->data();

    if (rect.x() < 0 || rect.y() < 0 || rect.maxX() > m_size.width() || rect.maxY() > m_size.height())
        memset(data, 0, result->length());

    int originx = rect.x();
    int destx = 0;
    if (originx < 0) {
        destx = -originx;
        originx = 0;
    }
    int endx = rect.x() + rect.width();
    if (endx > m_size.width())
        endx = m_size.width();
    int numColumns = endx - originx;

    int originy = rect.y();
    int desty = 0;
    if (originy < 0) {
        desty = -originy;
        originy = 0;
    }
    int endy = rect.y() + rect.height();
    if (endy > m_size.height())
        endy = m_size.height();
    int numRows = endy - originy;

    unsigned srcPixelsPerRow = src.rowBytesAsPixels();
    unsigned destBytesPerRow = 4 * rect.width();

    const SkPMColor* srcRows = src.getAddr32(originx, originy);
    unsigned char* destRows = data + desty * destBytesPerRow + destx * 4;
    for (int y = 0; y < numRows; ++y) {
        for (int x = 0; x < numColumns; x++) {
            // ugh, it appears they want unpremultiplied pixels
            SkColor c = SkUnPreMultiply::PMColorToColor(srcRows[x]);
            int basex = x * 4;
            destRows[basex + 0] = SkColorGetR(c);
            destRows[basex + 1] = SkColorGetG(c);
            destRows[basex + 2] = SkColorGetB(c);
            destRows[basex + 3] = SkColorGetA(c);
        }
        srcRows += srcPixelsPerRow;
        destRows += destBytesPerRow;
    }
    return result.release();
}

void ImageBuffer::putUnmultipliedImageData(ByteArray* source, const IntSize& sourceSize, const IntRect& sourceRect, const IntPoint& destPoint)
{
    ASSERT(context() && context()->platformContext());

    // Request for canvas bitmap; conversion required.
    if (context()->platformContext()->isRecording())
        context()->platformContext()->convertToNonRecording();
    GraphicsContext* gc = this->context();
    if (!gc) {
        return;
    }

    const SkBitmap& dst = android_gc2canvas(gc)->getDevice()->accessBitmap(true);
    SkAutoLockPixels alp(dst);
    if (!dst.getPixels()) {
        return;
    }

#if ENABLE(ACCELERATED_2D_CANVAS)
    context()->syncSoftwareCanvas();
#endif
    ASSERT(sourceRect.width() > 0);
    ASSERT(sourceRect.height() > 0);

    int originx = sourceRect.x();
    int destx = destPoint.x() + sourceRect.x();
    ASSERT(destx >= 0);
    ASSERT(destx < m_size.width());
    ASSERT(originx >= 0);
    ASSERT(originx <= sourceRect.right());

    int endx = destPoint.x() + sourceRect.maxX();
    ASSERT(endx <= m_size.width());

    int numColumns = endx - destx;

    int originy = sourceRect.y();
    int desty = destPoint.y() + sourceRect.y();
    ASSERT(desty >= 0);
    ASSERT(desty < m_size.height());
    ASSERT(originy >= 0);
    ASSERT(originy <= sourceRect.bottom());

    int endy = destPoint.y() + sourceRect.maxY();
    ASSERT(endy <= m_size.height());
    int numRows = endy - desty;

    unsigned srcBytesPerRow = 4 * sourceSize.width();
    unsigned dstPixelsPerRow = dst.rowBytesAsPixels();

    unsigned char* srcRows = source->data() + originy * srcBytesPerRow + originx * 4;
    SkPMColor* dstRows = dst.getAddr32(destx, desty);
    for (int y = 0; y < numRows; ++y) {
        for (int x = 0; x < numColumns; x++) {
            int basex = x * 4;
            dstRows[x] = SkPackARGB32(srcRows[basex + 3],
                                      srcRows[basex + 0],
                                      srcRows[basex + 1],
                                      srcRows[basex + 2]);
        }
        dstRows += dstPixelsPerRow;
        srcRows += srcBytesPerRow;
    }
}


String ImageBuffer::toDataURL(const String&, const double*) const
{
    ASSERT(context() && context()->platformContext());

    // Request for canvas bitmap; conversion required.
    if (context()->platformContext()->isRecording())
        context()->platformContext()->convertToNonRecording();
    // Encode the image into a vector.
    SkDynamicMemoryWStream pngStream;
    const SkBitmap& dst = android_gc2canvas(context())->getDevice()->accessBitmap(true);
    SkImageEncoder::EncodeStream(&pngStream, dst, SkImageEncoder::kPNG_Type, 100);

    // Convert it into base64.
    Vector<char> pngEncodedData;
    pngEncodedData.append(pngStream.getStream(), pngStream.getOffset());
    Vector<char> base64EncodedData;
    base64Encode(pngEncodedData, base64EncodedData);
    // Append with a \0 so that it's a valid string.
    base64EncodedData.append('\0');

    // And the resulting string.
    return String::format("data:image/png;base64,%s", base64EncodedData.data());
}

void ImageBuffer::platformTransformColorSpace(const Vector<int>& lookupTable)
{
    notImplemented();
}

size_t ImageBuffer::dataSize() const
{
    return m_size.width() * m_size.height() * 4;
}

}
