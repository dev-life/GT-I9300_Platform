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
#include "RenderThemeAndroid.h"

#include "Color.h"
#include "Element.h"
#include "GraphicsContext.h"
#include "HTMLNames.h"
#include "HTMLOptionElement.h"
#include "HTMLSelectElement.h"
#include "Node.h"
#include "PlatformGraphicsContext.h"
#if ENABLE(VIDEO)
#include "RenderMediaControls.h"
#endif
#include "RenderSkinAndroid.h"
#include "RenderSkinButton.h"
#include "RenderSkinCombo.h"
#include "RenderSkinMediaButton.h"
#include "RenderSkinRadio.h"
#include "SkCanvas.h"
#include "UserAgentStyleSheets.h"
#include "WebCoreFrameBridge.h"
#if ENABLE(PROGRESS_TAG)
#include "RenderProgress.h"
#include "SkShader.h"
#include "SkGradientShader.h"
#endif
#include "RenderSlider.h"
#include "Settings.h"  // SAMSUNG CHANGE : ADVANCED COPY & PASTE

namespace WebCore {

// Add padding to the fontSize of ListBoxes to get their maximum sizes.
// Listboxes often have a specified size.  Since we change them into
// dropdowns, we want a much smaller height, which encompasses the text.
const int listboxPadding = 5;

// This is the color of selection in a textfield.  It was computed from
// frameworks/base/core/res/res/values/colors.xml, which uses #9983CC39
// (decimal a = 153, r = 131, g = 204, b = 57)
// for all four highlighted text values. Blending this with white yields:
// R = (131 * 153 + 255 * (255 - 153)) / 255  -> 180.6
// G = (204 * 153 + 255 * (255 - 153)) / 255  -> 224.4
// B = ( 57 * 153 + 255 * (255 - 153)) / 255  -> 136.2

const RGBA32 selectionColor = makeRGB(66, 142, 186); //makeRGB(181, 224, 136); // SAMSUNG CHANGE

static SkCanvas* getCanvasFromInfo(const PaintInfo& info)
{
    return info.context->platformContext()->mCanvas;
}

static android::WebFrame* getWebFrame(const Node* node)
{
    if (!node)
        return 0;
    return android::WebFrame::getWebFrame(node->document()->frame());
}

//SAMSUNG INPUT TYPE RANGE CHANGES <<
static void drawVertLine(SkCanvas* canvas, int x, int y1, int y2, const SkPaint& paint)
{
    SkIRect skrect;
    skrect.set(x, y1, x + 1, y2 + 1);
    canvas->drawIRect(skrect, paint);
}

static void drawHorizLine(SkCanvas* canvas, int x1, int x2, int y, const SkPaint& paint)
{
    SkIRect skrect;
    skrect.set(x1, y, x2 + 1, y + 1);
    canvas->drawIRect(skrect, paint);
}

static void drawBox(SkCanvas* canvas, const IntRect& rect, const SkPaint& paint)
{
    const int right = rect.x() + rect.width() - 1;
    const int bottom = rect.y() + rect.height() - 1;
    drawHorizLine(canvas, rect.x(), right, rect.y(), paint);
    drawVertLine(canvas, right, rect.y(), bottom, paint);
    drawHorizLine(canvas, rect.x(), right, bottom, paint);
    drawVertLine(canvas, rect.x(), rect.y(), bottom, paint);
}
//SAMSUNG INPUT TYPE RANGE CHANGES >>

RenderTheme* theme()
{
    DEFINE_STATIC_LOCAL(RenderThemeAndroid, androidTheme, ());
    return &androidTheme;
}

PassRefPtr<RenderTheme> RenderTheme::themeForPage(Page* page)
{
    static RenderTheme* rt = RenderThemeAndroid::create().releaseRef();
    return rt;
}

PassRefPtr<RenderTheme> RenderThemeAndroid::create()
{
    return adoptRef(new RenderThemeAndroid());
}

RenderThemeAndroid::RenderThemeAndroid()
{
}

RenderThemeAndroid::~RenderThemeAndroid()
{
}

void RenderThemeAndroid::close()
{
}

bool RenderThemeAndroid::stateChanged(RenderObject* obj, ControlState state) const
{
    if (CheckedState == state) {
        obj->repaint();
        return true;
    }
    return false;
}

Color RenderThemeAndroid::platformActiveSelectionBackgroundColor() const
{
    return Color(selectionColor);
}

// SAMSUNG CHANGE : ADVANCED COPY & PASTE >>
Color RenderThemeAndroid::platformActiveSelectionBackgroundColor(Settings* s) const
{
    Color color(selectionColor);
    if (s && s->advancedSelectionBgColor().isValid()) {
        color = s->advancedSelectionBgColor();
        float r, g, b, a;
        color.getRGBA(r, g, b, a);
    }
    return color;
}
// SAMSUNG CHANGE : ADVANCED COPY & PASTE <<

Color RenderThemeAndroid::platformInactiveSelectionBackgroundColor() const
{
    return Color(Color::transparent);
}

// SAMSUNG CHANGE : ADVANCED COPY & PASTE >>
Color RenderThemeAndroid::platformInactiveSelectionBackgroundColor(Settings* s) const
{
    Color color(selectionColor);
    if (s && s->advancedSelectionBgColor().isValid()) {
        color = s->advancedSelectionBgColor();
        float r, g, b, a;
        color.getRGBA(r, g, b, a);
    }
    return color;
}
// SAMSUNG CHANGE : ADVANCED COPY & PASTE <<

Color RenderThemeAndroid::platformActiveSelectionForegroundColor() const
{
    return Color::black;
}

Color RenderThemeAndroid::platformInactiveSelectionForegroundColor() const
{
    return Color::black;
}

Color RenderThemeAndroid::platformTextSearchHighlightColor() const
{
    return Color(Color::transparent);
}

Color RenderThemeAndroid::platformActiveListBoxSelectionBackgroundColor() const
{
    return Color(Color::transparent);
}

Color RenderThemeAndroid::platformInactiveListBoxSelectionBackgroundColor() const
{
    return Color(Color::transparent);
}

Color RenderThemeAndroid::platformActiveListBoxSelectionForegroundColor() const
{
    return Color(Color::transparent);
}

Color RenderThemeAndroid::platformInactiveListBoxSelectionForegroundColor() const
{
    return Color(Color::transparent);
}

int RenderThemeAndroid::baselinePosition(const RenderObject* obj) const
{
    // From the description of this function in RenderTheme.h:
    // A method to obtain the baseline position for a "leaf" control.  This will only be used if a baseline
    // position cannot be determined by examining child content. Checkboxes and radio buttons are examples of
    // controls that need to do this.
    //
    // Our checkboxes and radio buttons need to be offset to line up properly.
    return RenderTheme::baselinePosition(obj) - 2;
}

void RenderThemeAndroid::addIntrinsicMargins(RenderStyle* style) const
{
    // Cut out the intrinsic margins completely if we end up using a small font size
    if (style->fontSize() < 11)
        return;
    
    // Intrinsic margin value.
    const int m = 2;
    
    // FIXME: Using width/height alone and not also dealing with min-width/max-width is flawed.
    if (style->width().isIntrinsicOrAuto()) {
        if (style->marginLeft().quirk())
            style->setMarginLeft(Length(m, Fixed));
        if (style->marginRight().quirk())
            style->setMarginRight(Length(m, Fixed));
    }

    if (style->height().isAuto()) {
        if (style->marginTop().quirk())
            style->setMarginTop(Length(m, Fixed));
        if (style->marginBottom().quirk())
            style->setMarginBottom(Length(m, Fixed));
    }
}

bool RenderThemeAndroid::supportsFocus(ControlPart appearance)
{
    switch (appearance) {
    case PushButtonPart:
    case ButtonPart:
    case TextFieldPart:
        return true;
    default:
        return false;
    }

    return false;
}

void RenderThemeAndroid::adjustButtonStyle(CSSStyleSelector*, RenderStyle* style, WebCore::Element*) const
{
    // Code is taken from RenderThemeSafari.cpp
    // It makes sure we have enough space for the button text.
    const int paddingHoriz = 12;
    //SISO CHANGES START>> MPSG100003464 Improper search button display fetching http://m.rediff.com
    //WAS: const int paddingVert = 8;
    //SISO CHANGES END<<
    style->setPaddingLeft(Length(paddingHoriz, Fixed));
    style->setPaddingRight(Length(paddingHoriz, Fixed));
   //SISO CHANGES START>> MPSG100003464 Improper search button display fetching http://m.rediff.com
   //If we provide fixed vertical padding when button style has "height" property, causes wrong alignment of text in button.
   //WAS: style->setPaddingTop(Length(paddingVert, Fixed));
   //WAS: style->setPaddingBottom(Length(paddingVert, Fixed));
   //SISO CHANGES END<<
    // Set a min-height so that we can't get smaller than the mini button.
   // Fix GA0100449299 . Change button min height. 
    style->setMinHeight(Length(30, Fixed));
}

bool RenderThemeAndroid::paintCheckbox(RenderObject* obj, const PaintInfo& info, const IntRect& rect)
{
    RenderSkinRadio::Draw(getCanvasFromInfo(info), obj->node(), rect, true);
    return false;
}

bool RenderThemeAndroid::paintButton(RenderObject* obj, const PaintInfo& info, const IntRect& rect)
{
    // If it is a disabled button, simply paint it to the master picture.
    Node* node = obj->node();
    Element* formControlElement = static_cast<Element*>(node);
    if (formControlElement) {
        android::WebFrame* webFrame = getWebFrame(node);
        if (webFrame) {
            RenderSkinAndroid* skins = webFrame->renderSkins();
            if (skins) {
                RenderSkinAndroid::State state = RenderSkinAndroid::kNormal;
                if (!formControlElement->isEnabledFormControl())
                    state = RenderSkinAndroid::kDisabled;
                skins->renderSkinButton()->draw(getCanvasFromInfo(info), rect, state);
            }
        }
    }

    // We always return false so we do not request to be redrawn.
    return false;
}

#if ENABLE(VIDEO)

String RenderThemeAndroid::extraMediaControlsStyleSheet()
{
      return String(mediaControlsAndroidUserAgentStyleSheet, sizeof(mediaControlsAndroidUserAgentStyleSheet));
}

bool RenderThemeAndroid::shouldRenderMediaControlPart(ControlPart part, Element* e)
{
      HTMLMediaElement* mediaElement = static_cast<HTMLMediaElement*>(e);
      switch (part) {
      case MediaMuteButtonPart:
          return false;
      case MediaSeekBackButtonPart:
      case MediaSeekForwardButtonPart:
          return false;
      case MediaRewindButtonPart:
          return mediaElement->movieLoadType() != MediaPlayer::LiveStream;
      case MediaReturnToRealtimeButtonPart:
          return mediaElement->movieLoadType() == MediaPlayer::LiveStream;
      case MediaFullscreenButtonPart:
          return mediaElement->supportsFullscreen();
      case MediaToggleClosedCaptionsButtonPart:
          return mediaElement->hasClosedCaptions();
      default:
          return true;
      }
}

bool RenderThemeAndroid::paintMediaFullscreenButton(RenderObject* o, const PaintInfo& paintInfo, const IntRect& rect)
{
      bool translucent = false;
      if (o && toParentMediaElement(o) && toParentMediaElement(o)->hasTagName(HTMLNames::videoTag))
          translucent = true;
      RenderSkinMediaButton::Draw(getCanvasFromInfo(paintInfo), rect, RenderSkinMediaButton::FULLSCREEN, translucent);
      return false;
}

bool RenderThemeAndroid::paintMediaMuteButton(RenderObject* o, const PaintInfo& paintInfo, const IntRect& rect)
{
      bool translucent = false;
      if (o && toParentMediaElement(o) && toParentMediaElement(o)->hasTagName(HTMLNames::videoTag))
          translucent = true;
      RenderSkinMediaButton::Draw(getCanvasFromInfo(paintInfo), rect, RenderSkinMediaButton::MUTE, translucent);
      return false;
}

bool RenderThemeAndroid::paintMediaPlayButton(RenderObject* o, const PaintInfo& paintInfo, const IntRect& rect)
{
      bool translucent = false;
      if (o && toParentMediaElement(o) && toParentMediaElement(o)->hasTagName(HTMLNames::videoTag))
          translucent = true;
      if (MediaControlPlayButtonElement* btn = static_cast<MediaControlPlayButtonElement*>(o->node())) {
          if (btn->displayType() == MediaPlayButton)
              RenderSkinMediaButton::Draw(getCanvasFromInfo(paintInfo), rect, RenderSkinMediaButton::PLAY, translucent);
          else
              RenderSkinMediaButton::Draw(getCanvasFromInfo(paintInfo), rect, RenderSkinMediaButton::PAUSE, translucent);
          return false;
      }
      return true;
}

bool RenderThemeAndroid::paintMediaSeekBackButton(RenderObject* o, const PaintInfo& paintInfo, const IntRect& rect)
{
      bool translucent = false;
      if (o && toParentMediaElement(o) && toParentMediaElement(o)->hasTagName(HTMLNames::videoTag))
          translucent = true;
      RenderSkinMediaButton::Draw(getCanvasFromInfo(paintInfo), rect, RenderSkinMediaButton::REWIND, translucent);
      return false;
}

bool RenderThemeAndroid::paintMediaSeekForwardButton(RenderObject* o, const PaintInfo& paintInfo, const IntRect& rect)
{
      bool translucent = false;
      if (o && toParentMediaElement(o) && toParentMediaElement(o)->hasTagName(HTMLNames::videoTag))
          translucent = true;
      RenderSkinMediaButton::Draw(getCanvasFromInfo(paintInfo), rect, RenderSkinMediaButton::FORWARD, translucent);
      return false;
}

bool RenderThemeAndroid::paintMediaControlsBackground(RenderObject* o, const PaintInfo& paintInfo, const IntRect& rect)
{
      bool translucent = false;
      if (o && toParentMediaElement(o) && toParentMediaElement(o)->hasTagName(HTMLNames::videoTag))
          translucent = true;
      RenderSkinMediaButton::Draw(getCanvasFromInfo(paintInfo), rect, RenderSkinMediaButton::BACKGROUND_SLIDER, translucent);
      return false;
}

bool RenderThemeAndroid::paintMediaSliderTrack(RenderObject* o, const PaintInfo& paintInfo, const IntRect& rect)
{
      bool translucent = false;
      if (o && toParentMediaElement(o) && toParentMediaElement(o)->hasTagName(HTMLNames::videoTag))
          translucent = true;
      RenderSkinMediaButton::Draw(getCanvasFromInfo(paintInfo), rect,
                                  RenderSkinMediaButton::SLIDER_TRACK, translucent, o);
      return false;
}

bool RenderThemeAndroid::paintMediaSliderThumb(RenderObject* o, const PaintInfo& paintInfo, const IntRect& rect)
{
      bool translucent = false;
      if (o && toParentMediaElement(o) && toParentMediaElement(o)->hasTagName(HTMLNames::videoTag))
          translucent = true;
      RenderSkinMediaButton::Draw(getCanvasFromInfo(paintInfo), rect, RenderSkinMediaButton::SLIDER_THUMB, translucent);
      return false;
}

void RenderThemeAndroid::adjustSliderThumbSize(RenderObject* o) const
{
    static const int sliderThumbWidth = RenderSkinMediaButton::sliderThumbWidth();
    static const int sliderThumbHeight = RenderSkinMediaButton::sliderThumbHeight();

//SAMSUNG INPUT TYPE RANGE CHANGES <<
    if(o->style()->appearance() == SliderThumbHorizontalPart){
	o->style()->setWidth(Length(sliderThumbWidth, Fixed));
        o->style()->setHeight(Length(sliderThumbHeight, Fixed));
    }	
    else if(o->style()->appearance() == SliderThumbVerticalPart){
	o->style()->setWidth(Length(sliderThumbHeight, Fixed));
        o->style()->setHeight(Length(sliderThumbWidth, Fixed));
    }
//SAMSUNG INPUT TYPE RANGE CHANGES >>
    else if(o->style()->appearance() == MediaSliderThumbPart) {
        o->style()->setWidth(Length(sliderThumbWidth, Fixed));
        o->style()->setHeight(Length(sliderThumbHeight, Fixed));
    }
}

#endif

bool RenderThemeAndroid::paintRadio(RenderObject* obj, const PaintInfo& info, const IntRect& rect)
{
    RenderSkinRadio::Draw(getCanvasFromInfo(info), obj->node(), rect, false);
    return false;
}

void RenderThemeAndroid::setCheckboxSize(RenderStyle* style) const
{
    //SAMSUNG_CHANGE_BEGIN
    style->setWidth(Length(15, Fixed));
    style->setHeight(Length(15, Fixed));
    //SAMSUNG_CHANGE_END

    /*
    style->setWidth(Length(19, Fixed));
    style->setHeight(Length(19, Fixed));
    */
}

void RenderThemeAndroid::setRadioSize(RenderStyle* style) const
{
    // This is the same as checkboxes.
    setCheckboxSize(style);
}

void RenderThemeAndroid::adjustTextFieldStyle(CSSStyleSelector*, RenderStyle* style, WebCore::Element*) const
{
    addIntrinsicMargins(style);
}

bool RenderThemeAndroid::paintTextField(RenderObject*, const PaintInfo&, const IntRect&)
{
    return true;    
}

void RenderThemeAndroid::adjustTextAreaStyle(CSSStyleSelector*, RenderStyle* style, WebCore::Element*) const
{
    addIntrinsicMargins(style);
}

bool RenderThemeAndroid::paintTextArea(RenderObject* obj, const PaintInfo& info, const IntRect& rect)
{
    if (obj->isMenuList())
        paintCombo(obj, info, rect);
    return true;
}

void RenderThemeAndroid::adjustSearchFieldStyle(CSSStyleSelector*, RenderStyle* style, Element*) const
{
    addIntrinsicMargins(style);
}

bool RenderThemeAndroid::paintSearchField(RenderObject*, const PaintInfo&, const IntRect&)
{
    return true;    
}

static void adjustMenuListStyleCommon(RenderStyle* style)
{
    // Added to make room for our arrow and make the touch target less cramped.
    style->setPaddingLeft(Length(RenderSkinCombo::padding(), Fixed));
    style->setPaddingTop(Length(RenderSkinCombo::padding(), Fixed));
    style->setPaddingBottom(Length(RenderSkinCombo::padding(), Fixed));
    style->setPaddingRight(Length(RenderSkinCombo::extraWidth(), Fixed));
    style->setMinHeight(Length(RenderSkinCombo::minHeight(), Fixed));
}

void RenderThemeAndroid::adjustListboxStyle(CSSStyleSelector*, RenderStyle* style, Element*) const
{
    adjustMenuListButtonStyle(0, style, 0);
}

void RenderThemeAndroid::adjustMenuListStyle(CSSStyleSelector*, RenderStyle* style, Element* e) const
{
    adjustMenuListStyleCommon(style);
    addIntrinsicMargins(style);
}

bool RenderThemeAndroid::paintCombo(RenderObject* obj, const PaintInfo& info,  const IntRect& rect)
{
  if (obj->style() && !obj->style()->visitedDependentColor(CSSPropertyBackgroundColor).alpha())
        return true;
    return RenderSkinCombo::Draw(getCanvasFromInfo(info), obj->node(), rect.x(), rect.y(), rect.width(), rect.height());
}

bool RenderThemeAndroid::paintMenuList(RenderObject* obj, const PaintInfo& info, const IntRect& rect) 
{ 
    return paintCombo(obj, info, rect);
}

void RenderThemeAndroid::adjustMenuListButtonStyle(CSSStyleSelector*,
        RenderStyle* style, Element*) const
{
    // Copied from RenderThemeSafari.
    const float baseFontSize = 11.0f;
    const int baseBorderRadius = 5;
    float fontScale = style->fontSize() / baseFontSize;
    
    style->resetPadding();
    style->setBorderRadius(IntSize(int(baseBorderRadius + fontScale - 1), int(baseBorderRadius + fontScale - 1))); // FIXME: Round up?

    const int minHeight = 15;
    style->setMinHeight(Length(minHeight, Fixed));
    
    style->setLineHeight(RenderStyle::initialLineHeight());
    // Found these padding numbers by trial and error.
    const int padding = 4;
    style->setPaddingTop(Length(padding, Fixed));
    style->setPaddingLeft(Length(padding, Fixed));
    adjustMenuListStyleCommon(style);
}

bool RenderThemeAndroid::paintMenuListButton(RenderObject* obj, const PaintInfo& info, const IntRect& rect) 
{
    return paintCombo(obj, info, rect);
}

bool RenderThemeAndroid::supportsFocusRing(const RenderStyle* style) const
{
    return style->opacity() > 0
        && style->hasAppearance() 
        && style->appearance() != TextFieldPart 
        && style->appearance() != SearchFieldPart 
        && style->appearance() != TextAreaPart 
        && style->appearance() != CheckboxPart
        && style->appearance() != RadioPart
        && style->appearance() != PushButtonPart
        && style->appearance() != SquareButtonPart
        && style->appearance() != ButtonPart
        && style->appearance() != ButtonBevelPart
        && style->appearance() != MenulistPart
        && style->appearance() != MenulistButtonPart;
}

//SAMSUNG PROGRESS TAG CHANGES <<
#if ENABLE(PROGRESS_TAG)
// MSDN says that update intervals for the bar is 30ms.
// http://msdn.microsoft.com/en-us/library/bb760842(v=VS.85).aspx
static const double progressAnimationFrameRate = 0.03;

double RenderThemeAndroid::animationRepeatIntervalForProgressBar(RenderProgress*) const
{
    return progressAnimationFrameRate;
}

double RenderThemeAndroid::animationDurationForProgressBar(RenderProgress* renderProgress) const
{
    // On Chromium Windows port, animationProgress() and associated values aren't used.
    // So here we can return arbitrary positive value.
    return progressAnimationFrameRate;
}

void RenderThemeAndroid::adjustProgressBarStyle(CSSStyleSelector*, RenderStyle*, Element*) const
{
}

bool RenderThemeAndroid::paintProgressBar(RenderObject* o, const PaintInfo& i, const IntRect& rect)
{
    if (!o->isProgress())
        return true;

    RenderProgress* renderProgress = toRenderProgress(o);
    IntRect valueRect = progressValueRectFor(renderProgress, rect);//progressRect;

    SkPaint paint;
    SkRect  dstRect;
    SkShader* m_shader = NULL;
    SkPoint pts[2];
    SkColor colors[3];
    SkScalar pos[] = { 0.0f, 0.5f, 1.0f};
    SkCanvas* const canvas = i.context->platformContext()->mCanvas;

    pts[0].fX = valueRect.x();
    pts[0].fY = valueRect.y();
    pts[1].fX = valueRect.x();
    pts[1].fY = valueRect.y() + valueRect.height();

    colors[0] = 0xFFC4EEA4;
    colors[1] = 0xFF3DC032;
    colors[2] = 0xFFC4EEA4;

    m_shader = SkGradientShader::CreateLinear(pts, colors, pos, 3, SkShader::kClamp_TileMode);

    // Paint the bar, the bar is valueRect
    dstRect.set(valueRect.x(), valueRect.y(), (valueRect.x() + valueRect.width()), (valueRect.y() + valueRect.height()));

    paint.setAntiAlias(true);
    paint.setFilterBitmap(true);
    paint.setShader(m_shader);
    canvas->drawRect(dstRect, paint);

    // Paint the border for progress bar, set the rect to complete progress bar rect
    dstRect.set(rect.x(), rect.y(), (rect.x() + rect.width()), (rect.y() + rect.height()));
    paint.setShader(NULL);
    paint.setColor(0x80000000);
    paint.setStyle(SkPaint::kStroke_Style);
    canvas->drawRect(dstRect, paint);

    if (m_shader)
    {
        m_shader->unref();
    }

    return false;
}

IntRect RenderThemeAndroid::determinateProgressValueRectFor(RenderProgress* renderProgress, const IntRect& rect) const
{
    int dx = rect.width() * renderProgress->position();
    if (renderProgress->style()->direction() == RTL)
        return IntRect(rect.x() + rect.width() - dx, rect.y(), dx, rect.height());
    return IntRect(rect.x(), rect.y(), dx, rect.height());
}

IntRect RenderThemeAndroid::indeterminateProgressValueRectFor(RenderProgress* renderProgress, const IntRect& rect) const
{

    int valueWidth = 45;//rect.width() / progressActivityBlocks;
    int movableWidth = rect.width() - valueWidth;
    if (movableWidth <= 0)
        return IntRect();

    double progress = renderProgress->animationProgress();

    //IntRect r = IntRect(rect.x() + progress * 2 * movableWidth, rect.y(), valueWidth, rect.height());
    ////if (r.x() > 2 * rect.width())
    ////    r.setX(rect.x());
    //if (r.x() > rect.x() + rect.width() + valueWidth)
    //    r.setX(rect.x() - valueWidth);

    int progressValue = renderProgress->progressValue();

    IntRect r = IntRect(rect.x() + progressValue, rect.y(), valueWidth, rect.height());
    
    // increment the progress value
    progressValue += 5;
    if (progressValue > rect.width())
        progressValue = 0;
    renderProgress->setProgressValue(progressValue);
    
    return r;
    //if (progress < 0.5)
    //    return IntRect(rect.x() + progress * 2 * movableWidth, rect.y(), valueWidth, rect.height());
    //return IntRect(rect.x() + (1.0 - progress) * movableWidth, rect.y(), valueWidth, rect.height());
}

IntRect RenderThemeAndroid::progressValueRectFor(RenderProgress* renderProgress, const IntRect& rect) const
{
    return renderProgress->isDeterminate() ? determinateProgressValueRectFor(renderProgress, rect) : indeterminateProgressValueRectFor(renderProgress, rect);
}
#endif
//SAMSUNG PROGRESS TAG CHANGES >>

//SAMSUNG INPUT TYPE RANGE CHANGES <<
bool RenderThemeAndroid::paintSliderTrack(RenderObject*, const PaintInfo& i, const IntRect& rect)
{
    // Just paint a grey box for now (matches the color of a scrollbar background.
    SkCanvas* const canvas = i.context->platformContext()->mCanvas;
    int verticalCenter = rect.y() + rect.height() / 2;
    int top = std::max(rect.y(), verticalCenter - 2);
    int bottom = std::min(rect.y() + rect.height(), verticalCenter + 2);

    SkPaint paint;
    const SkColor grey = SkColorSetARGB(0xff, 0xe3, 0xdd, 0xd8);
    paint.setColor(grey);

    SkRect skrect;
    skrect.set(rect.x(), top, rect.x() + rect.width(), bottom);
    canvas->drawRect(skrect, paint);

    return false;
}

bool RenderThemeAndroid::paintSliderThumb(RenderObject* o, const PaintInfo& i, const IntRect& rect)
{
    // Make a thumb similar to the scrollbar thumb.
    const bool hovered = isHovered(o) || toRenderSlider(o->parent())->inDragMode();
    const int midx = rect.x() + rect.width() / 2;
    const int midy = rect.y() + rect.height() / 2;
    const bool vertical = (o->style()->appearance() == SliderThumbVerticalPart);
    SkCanvas* const canvas = i.context->platformContext()->mCanvas;

    const SkColor thumbLightGrey = SkColorSetARGB(0xff, 0xf4, 0xf2, 0xef);
    const SkColor thumbDarkGrey = SkColorSetARGB(0xff, 0xea, 0xe5, 0xe0);
    SkPaint paint;
    paint.setColor(hovered ? SK_ColorWHITE : thumbLightGrey);

    SkIRect skrect;
    if (vertical)
        skrect.set(rect.x(), rect.y(), midx + 1, rect.maxY());
    else
        skrect.set(rect.x(), rect.y(), rect.maxX(), midy + 1);

    canvas->drawIRect(skrect, paint);

    paint.setColor(hovered ? thumbLightGrey : thumbDarkGrey);

    if (vertical)
        skrect.set(midx + 1, rect.y(), rect.maxX(), rect.maxY());
    else
        skrect.set(rect.x(), midy + 1, rect.maxX(), rect.maxY());

    canvas->drawIRect(skrect, paint);

    const SkColor borderDarkGrey = SkColorSetARGB(0xff, 0x9d, 0x96, 0x8e);
    paint.setColor(borderDarkGrey);
    drawBox(canvas, rect, paint);

    if (rect.height() > 10 && rect.width() > 10) {
        drawHorizLine(canvas, midx - 2, midx + 2, midy, paint);
        drawHorizLine(canvas, midx - 2, midx + 2, midy - 3, paint);
        drawHorizLine(canvas, midx - 2, midx + 2, midy + 3, paint);
    }

    return false;
}
//SAMSUNG INPUT TYPE RANGE CHANGES >>

} // namespace WebCore
