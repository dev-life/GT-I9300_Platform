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

#define LOG_TAG "webcoreglue"

#include "config.h"
#include "WebViewCore.h"

#include "AccessibilityObject.h"
#include "Attribute.h"
#include "BaseLayerAndroid.h"
#include "CachedNode.h"
#include "CachedRoot.h"
#include "Chrome.h"
#include "ChromeClientAndroid.h"
#include "ChromiumIncludes.h"
#include "ClientRect.h"
#include "ClientRectList.h"
#include "Color.h"
#include "CSSPropertyNames.h"
#include "CSSValueKeywords.h"
#include "DatabaseTracker.h"
#include "Document.h"
#include "DOMWindow.h"
#include "DOMSelection.h"
#include "Element.h"
#include "Editor.h"
#include "EditorClientAndroid.h"
#include "EventHandler.h"
#include "EventNames.h"
#include "ExceptionCode.h"
#include "FocusController.h"
#include "Font.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClientAndroid.h"
#include "FrameTree.h"
#include "FrameView.h"
#include "Geolocation.h"
#include "GraphicsContext.h"
#include "GraphicsJNI.h"
#include "HTMLAnchorElement.h"
#include "HTMLAreaElement.h"
#include "HTMLElement.h"
#include "HTMLFormControlElement.h"
#include "HTMLImageElement.h"
#include "HTMLInputElement.h"
#include "HTMLLabelElement.h"
#include "HTMLLinkElement.h"	//SAMSUNG CHANGE +
//SAMSUNG CHANGES- EMAIL APP CUSTOMIZATION >>
#include "HTMLCollection.h" 
//SAMSUNG CHANGES <<
#include "HTMLMapElement.h"
#include "HTMLNames.h"
#include "HTMLOptGroupElement.h"
#include "HTMLOptionElement.h"
#include "HTMLSelectElement.h"
#include "HTMLTextAreaElement.h"
#include "HistoryItem.h"
#include "HitTestRequest.h"
#include "HitTestResult.h"
#if ENABLE(WML)
#include "WMLOptGroupElement.h"
#include "WMLOptionElement.h"
#include "WMLSelectElement.h"
#include "WMLInputElement.h"
#include "WMLNames.h"
#endif
#include "InlineTextBox.h"
#include "MemoryUsage.h"
#include "NamedNodeMap.h"
#include "Navigator.h"
#include "Node.h"
#include "NodeList.h"
// Samsung Change - HTML5 Web Notification	>>
#if ENABLE(NOTIFICATIONS)
#include "Notification.h"
#include "NotificationPresenter.h"
#endif
// Samsung Change - HTML5 Web Notification	<<
#include "Page.h"
#include "PageGroup.h"
#include "PlatformKeyboardEvent.h"
#include "PlatformString.h"
#include "PluginWidgetAndroid.h"
#include "PluginView.h"
#include "Position.h"
#include "ProgressTracker.h"
#include "Range.h"
#include "RenderBox.h"
#include "RenderInline.h"
#include "RenderLayer.h"
#include "RenderPart.h"
#include "RenderText.h"
#include "RenderTextControl.h"
#include "RenderThemeAndroid.h"
#include "RenderView.h"
#include "ResourceRequest.h"
#include "RuntimeEnabledFeatures.h"
//SAMSUNG CHANGE >> LIGHT TOUCH
#include "RenderImage.h"
//SAMSUNG CHANGE << LIGHT TOUCH
#include "SchemeRegistry.h"
#include "SelectionController.h"
#include "Settings.h"
//SAMSUNG CHANGES- EMAIL APP CUSTOMIZATION >>
#include "SharedBuffer.h"
//SAMSUNG CHANGES <<
#include "SkANP.h"
#include "SkTemplates.h"
#include "SkTDArray.h"
#include "SkTypes.h"
#include "SkCanvas.h"
#include "SkPicture.h"
#include "SkUtils.h"
#include "Text.h"
#include "TypingCommand.h"
#include "WebCache.h"
#include "WebCoreFrameBridge.h"
#include "WebFrameView.h"
#include "WindowsKeyboardCodes.h"
#include "android_graphics.h"
#include "autofill/WebAutofill.h"
#include "htmlediting.h"
#include "markup.h"
//+HTML_COMPOSER
#include "RemoveNodeCommand.h"
//-HTML_COMPOSER

#include <JNIHelp.h>
#include <JNIUtility.h>
#include <ui/KeycodeLabels.h>
#include <wtf/CurrentTime.h>
#include <wtf/text/AtomicString.h>
#include <wtf/text/StringImpl.h>


#if USE(V8)
#include "ScriptController.h"
#include "V8Counters.h"
//SAMSUNG_WEB_WORKER_CHANGES >>
#include "V8Binding.h"
//SAMSUNG_WEB_WORKER_CHANGES <<
#include <wtf/text/CString.h>
#endif

#if DEBUG_NAV_UI
#include "SkTime.h"
#endif

#if ENABLE(TOUCH_EVENTS) // Android
#include "PlatformTouchEvent.h"
#endif

#ifdef ANDROID_DOM_LOGGING
#include "AndroidLog.h"
#include "RenderTreeAsText.h"
#include <wtf/text/CString.h>

FILE* gDomTreeFile = 0;
FILE* gRenderTreeFile = 0;
#endif

#ifdef ANDROID_INSTRUMENT
#include "TimeCounter.h"
#endif

#if USE(ACCELERATED_COMPOSITING)
#include "GraphicsLayerAndroid.h"
#include "RenderLayerCompositor.h"
#endif

#if USE(V8)
#include <v8.h>
#endif
// SAMSUNG CHANGE : ADVANCED_TEXT_SELECTION
#include <visible_units.h>

// In some cases, too many invalidations passed to the UI will slow us down.
// Limit ourselves to 32 rectangles, past this just send the area bounds to the UI.
// see WebViewCore::recordPictureSet().
#define MAX_INVALIDATIONS 32

/*  We pass this flag when recording the actual content, so that we don't spend
    time actually regionizing complex path clips, when all we really want to do
    is record them.
 */
#define PICT_RECORD_FLAGS   SkPicture::kUsePathBoundsForClip_RecordingFlag

////////////////////////////////////////////////////////////////////////////////////////////////

//SISO_HTMLCOMPOSER begin
namespace WebCore {
    extern WTF::String createLocalResource(WebCore::Frame* frame , WTF::String url);
    extern bool saveCachedImageToFile(WebCore::Frame* frame, WTF::String imageUrl, WTF::String filePath);;
    extern void copyImagePathToClipboard(const WTF::String& imagePath);
    extern android::WebHTMLMarkupData* createFullMarkup(const Node* node,const WTF::String& basePath = WTF::String());
}
static const UChar NonBreakingSpaceCharacter = 0xA0;
static const UChar SpaceCharacter = ' ';
//SISO_HTMLCOMPOSER end
namespace android {

static SkTDArray<WebViewCore*> gInstanceList;

void WebViewCore::addInstance(WebViewCore* inst) {
    *gInstanceList.append() = inst;
}

void WebViewCore::removeInstance(WebViewCore* inst) {
    int index = gInstanceList.find(inst);
    LOG_ASSERT(index >= 0, "RemoveInstance inst not found");
    if (index >= 0) {
        gInstanceList.removeShuffle(index);
    }
}

bool WebViewCore::isInstance(WebViewCore* inst) {
    return gInstanceList.find(inst) >= 0;
}

jobject WebViewCore::getApplicationContext() {

    // check to see if there is a valid webviewcore object
    if (gInstanceList.isEmpty())
        return 0;

    // get the context from the webview
    jobject context = gInstanceList[0]->getContext();

    if (!context)
        return 0;

    // get the application context using JNI
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    jclass contextClass = env->GetObjectClass(context);
    jmethodID appContextMethod = env->GetMethodID(contextClass, "getApplicationContext", "()Landroid/content/Context;");
    env->DeleteLocalRef(contextClass);
    jobject result = env->CallObjectMethod(context, appContextMethod);
    checkException(env);
    return result;
}


struct WebViewCoreStaticMethods {
    jmethodID    m_isSupportedMediaMimeType;
} gWebViewCoreStaticMethods;

// Check whether a media mimeType is supported in Android media framework.
bool WebViewCore::isSupportedMediaMimeType(const WTF::String& mimeType) {
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    jstring jMimeType = wtfStringToJstring(env, mimeType);
    jclass webViewCore = env->FindClass("android/webkit/WebViewCore");
    bool val = env->CallStaticBooleanMethod(webViewCore,
          gWebViewCoreStaticMethods.m_isSupportedMediaMimeType, jMimeType);
    checkException(env);
    env->DeleteLocalRef(webViewCore);
    env->DeleteLocalRef(jMimeType);

    return val;
}

// ----------------------------------------------------------------------------

#define GET_NATIVE_VIEW(env, obj) ((WebViewCore*)env->GetIntField(obj, gWebViewCoreFields.m_nativeClass))

// Field ids for WebViewCore
struct WebViewCoreFields {
    jfieldID    m_nativeClass;
    jfieldID    m_viewportWidth;
    jfieldID    m_viewportHeight;
    jfieldID    m_viewportInitialScale;
    jfieldID    m_viewportMinimumScale;
    jfieldID    m_viewportMaximumScale;
    jfieldID    m_viewportUserScalable;
    jfieldID    m_viewportDensityDpi;
    jfieldID    m_webView;
    jfieldID    m_drawIsPaused;
    jfieldID    m_lowMemoryUsageMb;
    jfieldID    m_highMemoryUsageMb;
    jfieldID    m_highUsageDeltaMb;
} gWebViewCoreFields;

// ----------------------------------------------------------------------------

struct WebViewCore::JavaGlue {
    jweak       m_obj;
    jmethodID   m_scrollTo;
    jmethodID   m_contentDraw;
    jmethodID   m_layersDraw;
    jmethodID   m_requestListBox;
    jmethodID   m_openFileChooser;
    jmethodID   m_requestSingleListBox;
    jmethodID   m_jsAlert;
    jmethodID   m_jsConfirm;
    jmethodID   m_jsPrompt;
    jmethodID   m_jsUnload;
    jmethodID   m_jsInterrupt;
    jmethodID   m_didFirstLayout;
    jmethodID   m_updateViewport;
    jmethodID   m_sendNotifyProgressFinished;
    jmethodID   m_sendViewInvalidate;
    jmethodID   m_updateTextfield;
    jmethodID   m_updateTextSelection;
    jmethodID   m_clearTextEntry;
    jmethodID   m_restoreScale;
    jmethodID   m_needTouchEvents;
    jmethodID   m_requestKeyboard;
    jmethodID   m_requestKeyboardWithSelection;
    jmethodID   m_exceededDatabaseQuota;
    jmethodID   m_reachedMaxAppCacheSize;
    jmethodID   m_populateVisitedLinks;
    jmethodID   m_geolocationPermissionsShowPrompt;
    jmethodID   m_geolocationPermissionsHidePrompt;
    jmethodID   m_getDeviceMotionService;
    jmethodID   m_getDeviceOrientationService;
    jmethodID   m_addMessageToConsole;
    jmethodID   m_formDidBlur;
    jmethodID   m_getPluginClass;
    jmethodID   m_showFullScreenPlugin;
    jmethodID   m_hideFullScreenPlugin;
    jmethodID   m_createSurface;
    jmethodID   m_addSurface;
    jmethodID   m_updateSurface;
    jmethodID   m_destroySurface;
    jmethodID   m_getContext;
    jmethodID   m_keepScreenOn;
    jmethodID   m_sendFindAgain;
    jmethodID   m_showRect;
    jmethodID   m_centerFitRect;
    jmethodID   m_setScrollbarModes;
    jmethodID   m_setInstallableWebApp;
    jmethodID   m_enterFullscreenForVideoLayer;
    jmethodID   m_setWebTextViewAutoFillable;
    jmethodID   m_selectAt;
//SISO_HTMLCOMPOSER begin
    jmethodID   m_isEditableSupport;
//SISO_HTMLCOMPOSER end
//SAMSUNG HTML5 INPUT TYPE DATE/TIME CHANGES <<
	jmethodID   m_requestDateTimePickers;
//SAMSUNG HTML5 INPUT TYPE DATE/TIME CHANGES >>
//SAMSUNG CHANGE >> LIGHT TOUCH
    jmethodID   m_setNavType ;
//SAMSUNG CHANGE << LIGHT TOUCH
// Samsung Change - HTML5 Web Notification	>>
    #if ENABLE(NOTIFICATIONS)
    jmethodID   m_notificationPermissionsShowPrompt;
    jmethodID   m_notificationManagershow;
    jmethodID   m_notificationManagerCancel;
    jmethodID m_notificationPermissionsHidePrompt;
    #endif
// Samsung Change - HTML5 Web Notification	<<
//SAMSUNG CHANGE HTML5 COLOR <<
    jmethodID   m_openColorChooser;  
//SAMSUNG CHANGE HTML5 COLOR >>

    AutoJObject object(JNIEnv* env) {
        // We hold a weak reference to the Java WebViewCore to avoid memeory
        // leaks due to circular references when WebView.destroy() is not
        // called manually. The WebView and hence the WebViewCore could become
        // weakly reachable at any time, after which the GC could null our weak
        // reference, so we have to check the return value of this method at
        // every use. Note that our weak reference will be nulled before the
        // WebViewCore is finalized.
        return getRealObject(env, m_obj);
    }
};

/*
 * WebViewCore Implementation
 */

static jmethodID GetJMethod(JNIEnv* env, jclass clazz, const char name[], const char signature[])
{
    jmethodID m = env->GetMethodID(clazz, name, signature);
    LOG_ASSERT(m, "Could not find method %s", name);
    return m;
}

Mutex WebViewCore::gFrameCacheMutex;
Mutex WebViewCore::gCursorBoundsMutex;

WebViewCore::WebViewCore(JNIEnv* env, jobject javaWebViewCore, WebCore::Frame* mainframe)
    : m_frameCacheKit(0)
    , m_navPictureKit(0)
    , m_moveGeneration(0)
    , m_touchGeneration(0)
    , m_lastGeneration(0)
    , m_updatedFrameCache(true)
    , m_findIsUp(false)
    , m_hasCursorBounds(false)
    , m_cursorBounds(WebCore::IntRect(0, 0, 0, 0))
    , m_cursorHitBounds(WebCore::IntRect(0, 0, 0, 0))
    , m_cursorFrame(0)
    , m_cursorLocation(WebCore::IntPoint(0, 0))
    , m_cursorNode(0)
    , m_javaGlue(new JavaGlue)
    , m_mainFrame(mainframe)
    , m_popupReply(0)
    , m_lastFocused(0)
    , m_lastFocusedBounds(WebCore::IntRect(0,0,0,0))
    , m_blurringNodePointer(0)
    , m_lastFocusedSelStart(0)
    , m_lastFocusedSelEnd(0)
    , m_blockTextfieldUpdates(false)
    , m_focusBoundsChanged(false)
    , m_skipContentDraw(false)
    , m_textGeneration(0)
    , m_temp(0)
    , m_tempPict(0)
    , m_maxXScroll(320/4)
    , m_maxYScroll(240/4)
    , m_scrollOffsetX(0)
    , m_scrollOffsetY(0)
    , m_mousePos(WebCore::IntPoint(0,0))
    , m_frameCacheOutOfDate(true)
    , m_progressDone(false)
    , m_screenWidth(320)
    , m_screenHeight(240)
    , m_textWrapWidth(320)
    , m_scale(1.0f)
    , m_domtree_version(0)
    , m_check_domtree_version(true)
    , m_groupForVisitedLinks(0)
    , m_isPaused(false)
    , m_cacheMode(0)
    , m_shouldPaintCaret(true)
    , m_pluginInvalTimer(this, &WebViewCore::pluginInvalTimerFired)
    , m_screenOnCounter(0)
    , m_currentNodeDomNavigationAxis(0)
    , m_deviceMotionAndOrientationManager(this)
#if ENABLE(TOUCH_EVENTS)
    , m_forwardingTouchEvents(false)
#endif
#if USE(CHROME_NETWORK_STACK)
    , m_webRequestContext(0)
#endif
// SAMSUNG CHANGE +
   , animatedImage(0)
// SAMSUNG CHANGE -
//SAMSUNG CHANGE HTML5 COLOR <<
   , m_colorChooser(0)
//SAMSUNG CHANGE HTML5 COLOR >>
{
    LOG_ASSERT(m_mainFrame, "Uh oh, somehow a frameview was made without an initial frame!");

    jclass clazz = env->GetObjectClass(javaWebViewCore);
    m_javaGlue->m_obj = env->NewWeakGlobalRef(javaWebViewCore);
    m_javaGlue->m_scrollTo = GetJMethod(env, clazz, "contentScrollTo", "(IIZZ)V");
    m_javaGlue->m_contentDraw = GetJMethod(env, clazz, "contentDraw", "()V");
    m_javaGlue->m_layersDraw = GetJMethod(env, clazz, "layersDraw", "()V");
    //SAMSUNG CHANGE - Form Navigation
    m_javaGlue->m_requestListBox = GetJMethod(env, clazz, "requestListBox", "([Ljava/lang/String;Ljava/lang/String;[I[II)V");
    m_javaGlue->m_openFileChooser = GetJMethod(env, clazz, "openFileChooser", "(Ljava/lang/String;)Ljava/lang/String;");
    //SAMSUNG CHANGE - Form Navigation
//SISO_HTMLCOMPOSER begin
    m_javaGlue->m_isEditableSupport = GetJMethod(env, clazz, "isEditableSupport", "()Z");
//SISO_HTMLCOMPOSER end
    m_javaGlue->m_requestSingleListBox = GetJMethod(env, clazz, "requestListBox", "([Ljava/lang/String;Ljava/lang/String;[III)V");
    m_javaGlue->m_jsAlert = GetJMethod(env, clazz, "jsAlert", "(Ljava/lang/String;Ljava/lang/String;)V");
    m_javaGlue->m_jsConfirm = GetJMethod(env, clazz, "jsConfirm", "(Ljava/lang/String;Ljava/lang/String;)Z");
    m_javaGlue->m_jsPrompt = GetJMethod(env, clazz, "jsPrompt", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
    m_javaGlue->m_jsUnload = GetJMethod(env, clazz, "jsUnload", "(Ljava/lang/String;Ljava/lang/String;)Z");
    m_javaGlue->m_jsInterrupt = GetJMethod(env, clazz, "jsInterrupt", "()Z");
    m_javaGlue->m_didFirstLayout = GetJMethod(env, clazz, "didFirstLayout", "(Z)V");
    m_javaGlue->m_updateViewport = GetJMethod(env, clazz, "updateViewport", "()V");
    m_javaGlue->m_sendNotifyProgressFinished = GetJMethod(env, clazz, "sendNotifyProgressFinished", "()V");
    m_javaGlue->m_sendViewInvalidate = GetJMethod(env, clazz, "sendViewInvalidate", "(IIII)V");
    m_javaGlue->m_updateTextfield = GetJMethod(env, clazz, "updateTextfield", "(IZLjava/lang/String;I)V");
    m_javaGlue->m_updateTextSelection = GetJMethod(env, clazz, "updateTextSelection", "(IIII)V");
    m_javaGlue->m_clearTextEntry = GetJMethod(env, clazz, "clearTextEntry", "()V");
    m_javaGlue->m_restoreScale = GetJMethod(env, clazz, "restoreScale", "(FF)V");
    m_javaGlue->m_needTouchEvents = GetJMethod(env, clazz, "needTouchEvents", "(Z)V");
    m_javaGlue->m_requestKeyboard = GetJMethod(env, clazz, "requestKeyboard", "(Z)V");
    m_javaGlue->m_requestKeyboardWithSelection = GetJMethod(env, clazz, "requestKeyboardWithSelection", "(IIII)V");
    m_javaGlue->m_exceededDatabaseQuota = GetJMethod(env, clazz, "exceededDatabaseQuota", "(Ljava/lang/String;Ljava/lang/String;JJ)V");
    m_javaGlue->m_reachedMaxAppCacheSize = GetJMethod(env, clazz, "reachedMaxAppCacheSize", "(J)V");
    m_javaGlue->m_populateVisitedLinks = GetJMethod(env, clazz, "populateVisitedLinks", "()V");
    m_javaGlue->m_geolocationPermissionsShowPrompt = GetJMethod(env, clazz, "geolocationPermissionsShowPrompt", "(Ljava/lang/String;)V");
    m_javaGlue->m_geolocationPermissionsHidePrompt = GetJMethod(env, clazz, "geolocationPermissionsHidePrompt", "()V");
    m_javaGlue->m_getDeviceMotionService = GetJMethod(env, clazz, "getDeviceMotionService", "()Landroid/webkit/DeviceMotionService;");
    m_javaGlue->m_getDeviceOrientationService = GetJMethod(env, clazz, "getDeviceOrientationService", "()Landroid/webkit/DeviceOrientationService;");
    m_javaGlue->m_addMessageToConsole = GetJMethod(env, clazz, "addMessageToConsole", "(Ljava/lang/String;ILjava/lang/String;I)V");
    m_javaGlue->m_formDidBlur = GetJMethod(env, clazz, "formDidBlur", "(I)V");
    m_javaGlue->m_getPluginClass = GetJMethod(env, clazz, "getPluginClass", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/Class;");
    m_javaGlue->m_showFullScreenPlugin = GetJMethod(env, clazz, "showFullScreenPlugin", "(Landroid/webkit/ViewManager$ChildView;II)V");
    m_javaGlue->m_hideFullScreenPlugin = GetJMethod(env, clazz, "hideFullScreenPlugin", "()V");
    m_javaGlue->m_createSurface = GetJMethod(env, clazz, "createSurface", "(Landroid/view/View;)Landroid/webkit/ViewManager$ChildView;");
    m_javaGlue->m_addSurface = GetJMethod(env, clazz, "addSurface", "(Landroid/view/View;IIII)Landroid/webkit/ViewManager$ChildView;");
    m_javaGlue->m_updateSurface = GetJMethod(env, clazz, "updateSurface", "(Landroid/webkit/ViewManager$ChildView;IIII)V");
    m_javaGlue->m_destroySurface = GetJMethod(env, clazz, "destroySurface", "(Landroid/webkit/ViewManager$ChildView;)V");
    m_javaGlue->m_getContext = GetJMethod(env, clazz, "getContext", "()Landroid/content/Context;");
    m_javaGlue->m_keepScreenOn = GetJMethod(env, clazz, "keepScreenOn", "(Z)V");
    m_javaGlue->m_sendFindAgain = GetJMethod(env, clazz, "sendFindAgain", "()V");
    m_javaGlue->m_showRect = GetJMethod(env, clazz, "showRect", "(IIIIIIFFFF)V");
    m_javaGlue->m_centerFitRect = GetJMethod(env, clazz, "centerFitRect", "(IIII)V");
    m_javaGlue->m_setScrollbarModes = GetJMethod(env, clazz, "setScrollbarModes", "(II)V");
    m_javaGlue->m_setInstallableWebApp = GetJMethod(env, clazz, "setInstallableWebApp", "()V");
    // Samsung Change - HTML5 Web Notification	>>
    #if ENABLE(NOTIFICATIONS)
    m_javaGlue->m_notificationPermissionsShowPrompt = GetJMethod(env, clazz, "notificationPermissionsShowPrompt", "(Ljava/lang/String;)V");
    m_javaGlue->m_notificationManagershow = GetJMethod(env, clazz, "notificationManagershow", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)V");
    m_javaGlue->m_notificationManagerCancel = GetJMethod(env, clazz, "notificationManagerCancel", "(I)V");
     m_javaGlue->m_notificationPermissionsHidePrompt = GetJMethod(env, clazz, "notificationPermissionsHidePrompt", "()V");
    #endif
    // Samsung Change - HTML5 Web Notification	<<
#if ENABLE(VIDEO)
    m_javaGlue->m_enterFullscreenForVideoLayer = GetJMethod(env, clazz, "enterFullscreenForVideoLayer", "(ILjava/lang/String;)V");
#endif
    m_javaGlue->m_setWebTextViewAutoFillable = GetJMethod(env, clazz, "setWebTextViewAutoFillable", "(ILjava/lang/String;)V");
    m_javaGlue->m_selectAt = GetJMethod(env, clazz, "selectAt", "(II)V");

//SAMSUNG HTML5 INPUT TYPE DATE/TIME CHANGES <<
    m_javaGlue->m_requestDateTimePickers = GetJMethod(env, clazz, "requestDateTimePickers", "(Ljava/lang/String;Ljava/lang/String;)V");	
//SAMSUNG HTML5 INPUT TYPE DATE/TIME CHANGES >>	
//SAMSUNG CHANGE >> LIGHT TOUCH
    m_javaGlue->m_setNavType = GetJMethod(env, clazz, "setNavType", "(I)V");
//SAMSUNG CHANGE << LIGHT TOUCH
//SAMSUNG CHANGE HTML5 COLOR <<
    m_javaGlue->m_openColorChooser = GetJMethod(env, clazz, "openColorChooser", "()V");
//SAMSUNG CHANGE HTML5 COLOR >>

    env->DeleteLocalRef(clazz);

    env->SetIntField(javaWebViewCore, gWebViewCoreFields.m_nativeClass, (jint)this);

    PageGroup::setShouldTrackVisitedLinks(true);

    clearContent();

    MemoryUsage::setLowMemoryUsageMb(env->GetIntField(javaWebViewCore, gWebViewCoreFields.m_lowMemoryUsageMb));
    MemoryUsage::setHighMemoryUsageMb(env->GetIntField(javaWebViewCore, gWebViewCoreFields.m_highMemoryUsageMb));
    MemoryUsage::setHighUsageDeltaMb(env->GetIntField(javaWebViewCore, gWebViewCoreFields.m_highUsageDeltaMb));

    WebViewCore::addInstance(this);

//SISO_HTMLCOMPOSER begin
    m_composingVisibleSelection = VisibleSelection();
    m_underLineVisibleSelection = VisibleSelection();
    m_imStr = 0;
    m_imEnd = 0;
//SISO_HTMLCOMPOSER end
#if USE(CHROME_NETWORK_STACK)
    AndroidNetworkLibraryImpl::InitWithApplicationContext(env, 0);
#endif

#if USE(V8)
    // Static initialisation of certain important V8 static data gets performed at system startup when
    // libwebcore gets loaded. We now need to associate the WebCore thread with V8 to complete
    // initialisation.
    v8::V8::Initialize();
//SAMSUNG_WEB_WORKER_CHANGES >>
    WebCore::V8BindingPerIsolateData::ensureInitialized(v8::Isolate::GetCurrent());
//SAMSUNG_WEB_WORKER_CHANGES <<
#endif

    // Configure any RuntimeEnabled features that we need to change from their default now.
    // See WebCore/bindings/generic/RuntimeEnabledFeatures.h

    // HTML5 History API
    RuntimeEnabledFeatures::setPushStateEnabled(true);
}

WebViewCore::~WebViewCore()
{
    WebViewCore::removeInstance(this);

    // Release the focused view
    Release(m_popupReply);

    if (m_javaGlue->m_obj) {
        JNIEnv* env = JSC::Bindings::getJNIEnv();
        env->DeleteWeakGlobalRef(m_javaGlue->m_obj);
        m_javaGlue->m_obj = 0;
    }
    delete m_javaGlue;
    delete m_frameCacheKit;
    delete m_navPictureKit;
}

WebViewCore* WebViewCore::getWebViewCore(const WebCore::FrameView* view)
{
    return getWebViewCore(static_cast<const WebCore::ScrollView*>(view));
}

WebViewCore* WebViewCore::getWebViewCore(const WebCore::ScrollView* view)
{
    if (!view)
        return 0;

    WebFrameView* webFrameView = static_cast<WebFrameView*>(view->platformWidget());
    if (!webFrameView)
        return 0;
    return webFrameView->webViewCore();
}

static bool layoutIfNeededRecursive(WebCore::Frame* f)
{
    if (!f)
        return true;

    WebCore::FrameView* v = f->view();
    if (!v)
        return true;

    if (v->needsLayout())
        v->layout(f->tree()->parent());

    WebCore::Frame* child = f->tree()->firstChild();
    bool success = true;
    while (child) {
        success &= layoutIfNeededRecursive(child);
        child = child->tree()->nextSibling();
    }

    return success && !v->needsLayout();
}

CacheBuilder& WebViewCore::cacheBuilder()
{
    return FrameLoaderClientAndroid::get(m_mainFrame)->getCacheBuilder();
}

WebCore::Node* WebViewCore::currentFocus()
{
//SAMSUNG CHANGE - gmail keypad issue fix >>
    Frame* focusFrame = NULL;
    Node* focusNode = NULL;
    if (m_mainFrame->page() && m_mainFrame->page()->focusController()) {
        focusFrame = m_mainFrame->page()->focusController()->focusedOrMainFrame();
    }

    if (NULL == focusFrame) {
        DBG_NAV_LOG("focusFrame is NULL");
        focusFrame = m_mainFrame;
    }

    Document* doc = focusFrame->document();
    if (doc != NULL) {
        focusNode = doc->focusedNode();
    }

    if (NULL != focusNode) {
        return focusNode;
    }
    DBG_NAV_LOG("focusNode is NULL");
//SAMSUNG CHANGE - gmail keypad issue fix <<
    return cacheBuilder().currentFocus();
}

void WebViewCore::recordPicture(SkPicture* picture)
{
    // if there is no document yet, just return
    if (!m_mainFrame->document()) {
        DBG_NAV_LOG("no document");
        return;
    }
    // Call layout to ensure that the contentWidth and contentHeight are correct
    if (!layoutIfNeededRecursive(m_mainFrame)) {
        DBG_NAV_LOG("layout failed");
        return;
    }
    // draw into the picture's recording canvas
    WebCore::FrameView* view = m_mainFrame->view();
    DBG_NAV_LOGD("view=(w=%d,h=%d)", view->contentsWidth(),
        view->contentsHeight());
    SkAutoPictureRecord arp(picture, view->contentsWidth(),
                            view->contentsHeight(), PICT_RECORD_FLAGS);
    SkAutoMemoryUsageProbe mup(__FUNCTION__);

    WebCore::PlatformGraphicsContext pgc(arp.getRecordingCanvas());
    WebCore::GraphicsContext gc(&pgc);
    view->platformWidget()->draw(&gc, WebCore::IntRect(0, 0,
        view->contentsWidth(), view->contentsHeight()));
}

void WebViewCore::recordPictureSet(PictureSet* content)
{
    // if there is no document yet, just return
    if (!m_mainFrame->document()) {
        DBG_SET_LOG("!m_mainFrame->document()");
        return;
    }
    if (m_addInval.isEmpty()) {
        DBG_SET_LOG("m_addInval.isEmpty()");
        return;
    }
    // Call layout to ensure that the contentWidth and contentHeight are correct
    // it's fine for layout to gather invalidates, but defeat sending a message
    // back to java to call webkitDraw, since we're already in the middle of
    // doing that
    m_skipContentDraw = true;
    bool success = layoutIfNeededRecursive(m_mainFrame);
    m_skipContentDraw = false;

    // We may be mid-layout and thus cannot draw.
    if (!success)
        return;

    {   // collect WebViewCoreRecordTimeCounter after layoutIfNeededRecursive
#ifdef ANDROID_INSTRUMENT
    TimeCounterAuto counter(TimeCounter::WebViewCoreRecordTimeCounter);
#endif

    // if the webkit page dimensions changed, discard the pictureset and redraw.
    WebCore::FrameView* view = m_mainFrame->view();
    int width = view->contentsWidth();
    int height = view->contentsHeight();

    // Use the contents width and height as a starting point.
    SkIRect contentRect;
    contentRect.set(0, 0, width, height);
    SkIRect total(contentRect);

    // Traverse all the frames and add their sizes if they are in the visible
    // rectangle.
    for (WebCore::Frame* frame = m_mainFrame->tree()->traverseNext(); frame;
            frame = frame->tree()->traverseNext()) {
        // If the frame doesn't have an owner then it is the top frame and the
        // view size is the frame size.
        WebCore::RenderPart* owner = frame->ownerRenderer();
        if (owner && owner->style()->visibility() == VISIBLE) {
            int x = owner->x();
            int y = owner->y();

            // Traverse the tree up to the parent to find the absolute position
            // of this frame.
            WebCore::Frame* parent = frame->tree()->parent();
            while (parent) {
                WebCore::RenderPart* parentOwner = parent->ownerRenderer();
                if (parentOwner) {
                    x += parentOwner->x();
                    y += parentOwner->y();
                }
                parent = parent->tree()->parent();
            }
            // Use the owner dimensions so that padding and border are
            // included.
            int right = x + owner->width();
            int bottom = y + owner->height();
            SkIRect frameRect = {x, y, right, bottom};
            // Ignore a width or height that is smaller than 1. Some iframes
            // have small dimensions in order to be hidden. The iframe
            // expansion code does not expand in that case so we should ignore
            // them here.
            if (frameRect.width() > 1 && frameRect.height() > 1
                    && SkIRect::Intersects(total, frameRect))
                total.join(x, y, right, bottom);
        }
    }

    // If the new total is larger than the content, resize the view to include
    // all the content.
    if (!contentRect.contains(total)) {
        // Resize the view to change the overflow clip.
        view->resize(total.fRight, total.fBottom);

        // We have to force a layout in order for the clip to change.
        m_mainFrame->contentRenderer()->setNeedsLayoutAndPrefWidthsRecalc();
        view->forceLayout();

        // Relayout similar to above
        m_skipContentDraw = true;
        bool success = layoutIfNeededRecursive(m_mainFrame);
        m_skipContentDraw = false;
        if (!success)
            return;

        // Set the computed content width
        width = view->contentsWidth();
        height = view->contentsHeight();
    }

    if (cacheBuilder().pictureSetDisabled())
        content->clear();

#if USE(ACCELERATED_COMPOSITING)
    // The invals are not always correct when the content size has changed. For
    // now, let's just reset the inval so that it invalidates the entire content
    // -- the pictureset will be fully repainted, tiles will be marked dirty and
    // will have to be repainted.

    // FIXME: the webkit invals ought to have been enough...
    if (content->width() != width || content->height() != height) {
        SkIRect r;
        r.fLeft = 0;
        r.fTop = 0;
        r.fRight = width;
        r.fBottom = height;
        m_addInval.setRect(r);
    }
#endif

    content->setDimensions(width, height, &m_addInval);

    // Add the current inval rects to the PictureSet, and rebuild it.
    content->add(m_addInval, 0, 0, false);

    // If we have too many invalidations, just get the area bounds
    SkRegion::Iterator iterator(m_addInval);
    int nbInvals = 0;
    while (!iterator.done()) {
        iterator.next();
        nbInvals++;
        if (nbInvals > MAX_INVALIDATIONS)
            break;
    }
    if (nbInvals > MAX_INVALIDATIONS) {
        SkIRect r = m_addInval.getBounds();
        m_addInval.setRect(r);
    }

    // Rebuild the pictureset (webkit repaint)
    rebuildPictureSet(content);
    } // WebViewCoreRecordTimeCounter

    WebCore::Node* oldFocusNode = currentFocus();
    m_frameCacheOutOfDate = true;
    WebCore::IntRect oldBounds;
    int oldSelStart = 0;
    int oldSelEnd = 0;
    if (oldFocusNode) {
        oldBounds = oldFocusNode->getRect();
        RenderObject* renderer = oldFocusNode->renderer();
        if (renderer && (renderer->isTextArea() || renderer->isTextField())) {
            WebCore::RenderTextControl* rtc =
                static_cast<WebCore::RenderTextControl*>(renderer);
            oldSelStart = rtc->selectionStart();
            oldSelEnd = rtc->selectionEnd();
        }
    } else
        oldBounds = WebCore::IntRect(0,0,0,0);
    unsigned latestVersion = 0;
    if (m_check_domtree_version) {
        // as domTreeVersion only increment, we can just check the sum to see
        // whether we need to update the frame cache
        for (Frame* frame = m_mainFrame; frame; frame = frame->tree()->traverseNext()) {
            const Document* doc = frame->document();
            latestVersion += doc->domTreeVersion() + doc->styleVersion();
        }
    }
    DBG_NAV_LOGD("m_lastFocused=%p oldFocusNode=%p"
        " m_lastFocusedBounds={%d,%d,%d,%d} oldBounds={%d,%d,%d,%d}"
        " m_lastFocusedSelection={%d,%d} oldSelection={%d,%d}"
        " m_check_domtree_version=%s latestVersion=%d m_domtree_version=%d",
        m_lastFocused, oldFocusNode,
        m_lastFocusedBounds.x(), m_lastFocusedBounds.y(),
        m_lastFocusedBounds.width(), m_lastFocusedBounds.height(),
        oldBounds.x(), oldBounds.y(), oldBounds.width(), oldBounds.height(),
        m_lastFocusedSelStart, m_lastFocusedSelEnd, oldSelStart, oldSelEnd,
        m_check_domtree_version ? "true" : "false",
        latestVersion, m_domtree_version);
    if (m_lastFocused == oldFocusNode && m_lastFocusedBounds == oldBounds
            && m_lastFocusedSelStart == oldSelStart
            && m_lastFocusedSelEnd == oldSelEnd
            && !m_findIsUp
            && (!m_check_domtree_version || latestVersion == m_domtree_version))
    {
        return;
    }
    m_focusBoundsChanged |= m_lastFocused == oldFocusNode
        && m_lastFocusedBounds != oldBounds;
    m_lastFocused = oldFocusNode;
    m_lastFocusedBounds = oldBounds;
    m_lastFocusedSelStart = oldSelStart;
    m_lastFocusedSelEnd = oldSelEnd;
    m_domtree_version = latestVersion;
    DBG_NAV_LOG("call updateFrameCache");
    updateFrameCache();
    if (m_findIsUp) {
        LOG_ASSERT(m_javaGlue->m_obj, "A Java widget was not associated with this view bridge!");
        JNIEnv* env = JSC::Bindings::getJNIEnv();
        AutoJObject javaObject = m_javaGlue->object(env);
        if (javaObject.get()) {
            env->CallVoidMethod(javaObject.get(), m_javaGlue->m_sendFindAgain);
            checkException(env);
        }
    }
}

// note: updateCursorBounds is called directly by the WebView thread
// This needs to be called each time we call CachedRoot::setCursor() with
// non-null CachedNode/CachedFrame, since otherwise the WebViewCore's data
// about the cursor is incorrect.  When we call setCursor(0,0), we need
// to set hasCursorBounds to false.
void WebViewCore::updateCursorBounds(const CachedRoot* root,
        const CachedFrame* cachedFrame, const CachedNode* cachedNode)
{
    LOG_ASSERT(root, "updateCursorBounds: root cannot be null");
    LOG_ASSERT(cachedNode, "updateCursorBounds: cachedNode cannot be null");
    LOG_ASSERT(cachedFrame, "updateCursorBounds: cachedFrame cannot be null");
    gCursorBoundsMutex.lock();
    m_hasCursorBounds = !cachedNode->isHidden();
    // If m_hasCursorBounds is false, we never look at the other
    // values, so do not bother setting them.
    if (m_hasCursorBounds) {
        WebCore::IntRect bounds = cachedNode->bounds(cachedFrame);
        if (m_cursorBounds != bounds)
            DBG_NAV_LOGD("new cursor bounds=(%d,%d,w=%d,h=%d)",
                bounds.x(), bounds.y(), bounds.width(), bounds.height());
        m_cursorBounds = bounds;
        m_cursorHitBounds = cachedNode->hitBounds(cachedFrame);
        m_cursorFrame = cachedFrame->framePointer();
        root->getSimulatedMousePosition(&m_cursorLocation);
        m_cursorNode = cachedNode->nodePointer();
    }
    gCursorBoundsMutex.unlock();
}

void WebViewCore::clearContent()
{
    DBG_SET_LOG("");
    m_content.clear();
    m_addInval.setEmpty();
    m_rebuildInval.setEmpty();
}

bool WebViewCore::focusBoundsChanged()
{
    bool result = m_focusBoundsChanged;
    m_focusBoundsChanged = false;
    return result;
}

SkPicture* WebViewCore::rebuildPicture(const SkIRect& inval)
{
    WebCore::FrameView* view = m_mainFrame->view();
    int width = view->contentsWidth();
    int height = view->contentsHeight();
    SkPicture* picture = new SkPicture();
    SkAutoPictureRecord arp(picture, width, height, PICT_RECORD_FLAGS);
    SkAutoMemoryUsageProbe mup(__FUNCTION__);
    SkCanvas* recordingCanvas = arp.getRecordingCanvas();

    WebCore::PlatformGraphicsContext pgc(recordingCanvas);
    WebCore::GraphicsContext gc(&pgc);
    IntPoint origin = view->minimumScrollPosition();
    WebCore::IntRect drawArea(inval.fLeft + origin.x(), inval.fTop + origin.y(),
            inval.width(), inval.height());
    recordingCanvas->translate(-drawArea.x(), -drawArea.y());
    recordingCanvas->save();
    view->platformWidget()->draw(&gc, drawArea);
    m_rebuildInval.op(inval, SkRegion::kUnion_Op);
    DBG_SET_LOGD("m_rebuildInval={%d,%d,r=%d,b=%d}",
        m_rebuildInval.getBounds().fLeft, m_rebuildInval.getBounds().fTop,
        m_rebuildInval.getBounds().fRight, m_rebuildInval.getBounds().fBottom);

    return picture;
}

void WebViewCore::rebuildPictureSet(PictureSet* pictureSet)
{
    WebCore::FrameView* view = m_mainFrame->view();

#ifdef FAST_PICTURESET
    WTF::Vector<Bucket*>* buckets = pictureSet->bucketsToUpdate();

    for (unsigned int i = 0; i < buckets->size(); i++) {
        Bucket* bucket = (*buckets)[i];
        for (unsigned int j = 0; j < bucket->size(); j++) {
            BucketPicture& bucketPicture = (*bucket)[j];
            const SkIRect& inval = bucketPicture.mRealArea;
            SkPicture* picture = rebuildPicture(inval);
            SkSafeUnref(bucketPicture.mPicture);
            bucketPicture.mPicture = picture;
        }
    }
    buckets->clear();
#else
    size_t size = pictureSet->size();
    for (size_t index = 0; index < size; index++) {
        if (pictureSet->upToDate(index))
            continue;
        const SkIRect& inval = pictureSet->bounds(index);
        DBG_SET_LOGD("pictSet=%p [%d] {%d,%d,w=%d,h=%d}", pictureSet, index,
            inval.fLeft, inval.fTop, inval.width(), inval.height());
        pictureSet->setPicture(index, rebuildPicture(inval));
    }

    pictureSet->validate(__FUNCTION__);
#endif
}

bool WebViewCore::updateLayers(LayerAndroid* layers)
{
    // We update the layers
    ChromeClientAndroid* chromeC = static_cast<ChromeClientAndroid*>(m_mainFrame->page()->chrome()->client());
    GraphicsLayerAndroid* root = static_cast<GraphicsLayerAndroid*>(chromeC->layersSync());
    if (root) {
        LayerAndroid* updatedLayer = root->contentLayer();
        return layers->updateWithTree(updatedLayer);
    }
    return true;
}

void WebViewCore::notifyAnimationStarted()
{
    // We notify webkit that the animations have begun
    // TODO: handle case where not all have begun
    ChromeClientAndroid* chromeC = static_cast<ChromeClientAndroid*>(m_mainFrame->page()->chrome()->client());
    GraphicsLayerAndroid* root = static_cast<GraphicsLayerAndroid*>(chromeC->layersSync());
    if (root)
        root->notifyClientAnimationStarted();

}

BaseLayerAndroid* WebViewCore::createBaseLayer(SkRegion* region)
{
    BaseLayerAndroid* base = new BaseLayerAndroid();
    base->setContent(m_content);

        m_skipContentDraw = true;
        bool layoutSucceeded = layoutIfNeededRecursive(m_mainFrame);
        m_skipContentDraw = false;
        // Layout only fails if called during a layout.
        LOG_ASSERT(layoutSucceeded, "Can never be called recursively");

#if USE(ACCELERATED_COMPOSITING)
    // We set the background color
    if (m_mainFrame && m_mainFrame->document()
        && m_mainFrame->document()->body()) {
        Document* document = m_mainFrame->document();
        RefPtr<RenderStyle> style = document->styleForElementIgnoringPendingStylesheets(document->body());
        if (style->hasBackground()) {
            Color color = style->visitedDependentColor(CSSPropertyBackgroundColor);
            if (color.isValid() && color.alpha() > 0)
// SAMSUNG CHANGE >> White flickering issue.
// WAS:base->setBackgroundColor(color);
			{
				SkColor c = SkColorSetARGB(color.alpha(), color.red(), color.green(), color.blue());
                base->setBackgroundColor(c);
			}
        }
		else
		{
			WebCore::FrameView* view = m_mainFrame->view();			
			if( view )
			{	
				Color color = view->baseBackgroundColor();				
				if (color.isValid() && color.alpha() > 0)
				{
					SkColor c = SkColorSetARGB(color.alpha(), color.red(), color.green(), color.blue());
					base->setBackgroundColor(c);
				}
			}			
// SAMSUNG CHANGE <<
        }
    }

    // We update the layers
    ChromeClientAndroid* chromeC = static_cast<ChromeClientAndroid*>(m_mainFrame->page()->chrome()->client());
    GraphicsLayerAndroid* root = static_cast<GraphicsLayerAndroid*>(chromeC->layersSync());
    if (root) {
        LayerAndroid* copyLayer = new LayerAndroid(*root->contentLayer());
        base->addChild(copyLayer);
        copyLayer->unref();
        root->contentLayer()->clearDirtyRegion();
    }
#endif

    return base;
}

BaseLayerAndroid* WebViewCore::recordContent(SkRegion* region, SkIPoint* point)
{
    DBG_SET_LOG("start");
    // If there is a pending style recalculation, just return.
    if (m_mainFrame->document()->isPendingStyleRecalc()) {
        DBG_SET_LOGD("recordContent: pending style recalc, ignoring.");
        return 0;
    }
    float progress = (float) m_mainFrame->page()->progress()->estimatedProgress();
    m_progressDone = progress <= 0.0f || progress >= 1.0f;
    recordPictureSet(&m_content);
    //Fix for [MPSG100004555]
    if (m_mainFrame->view()->didFirstLayout())
    	m_mainFrame->view()->forceLayout();
    //Fix for [MPSG100004555]
    if (!m_progressDone && m_content.isEmpty()) {
        DBG_SET_LOGD("empty (progress=%g)", progress);
        return 0;
    }
    region->set(m_addInval);
    m_addInval.setEmpty();
#if USE(ACCELERATED_COMPOSITING)
#else
    region->op(m_rebuildInval, SkRegion::kUnion_Op);
#endif
    m_rebuildInval.setEmpty();
    point->fX = m_content.width();
    point->fY = m_content.height();
    DBG_SET_LOGD("region={%d,%d,r=%d,b=%d}", region->getBounds().fLeft,
        region->getBounds().fTop, region->getBounds().fRight,
        region->getBounds().fBottom);
    DBG_SET_LOG("end");

    //SAMSUNG CHNAGES >>
    //Commenting this patch for vellamo benchmark issue.
    //TODO: Find the alertanative patch.
    //if ( m_content.width() == 0 || m_content.height() == 0){
    //	DBG_SET_LOGD("Picture set is empty. It has to be of screen size atleast. Recording again");
    //    return 0;
    //}
    //SAMSUNG CHNAGES <<

    return createBaseLayer(region);
}

void WebViewCore::splitContent(PictureSet* content)
{
#ifdef FAST_PICTURESET
#else
    bool layoutSucceeded = layoutIfNeededRecursive(m_mainFrame);
    LOG_ASSERT(layoutSucceeded, "Can never be called recursively");
    content->split(&m_content);
    rebuildPictureSet(&m_content);
    content->set(m_content);
#endif // FAST_PICTURESET
}

void WebViewCore::scrollTo(int x, int y, bool animate)
{
    LOG_ASSERT(m_javaGlue->m_obj, "A Java widget was not associated with this view bridge!");

//    LOGD("WebViewCore::scrollTo(%d %d)\n", x, y);

    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return;
    env->CallVoidMethod(javaObject.get(), m_javaGlue->m_scrollTo,
            x, y, animate, false);
    checkException(env);
}

void WebViewCore::sendNotifyProgressFinished()
{
    LOG_ASSERT(m_javaGlue->m_obj, "A Java widget was not associated with this view bridge!");
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return;
    env->CallVoidMethod(javaObject.get(), m_javaGlue->m_sendNotifyProgressFinished);
    checkException(env);
}

void WebViewCore::viewInvalidate(const WebCore::IntRect& rect)
{
    LOG_ASSERT(m_javaGlue->m_obj, "A Java widget was not associated with this view bridge!");
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return;
    env->CallVoidMethod(javaObject.get(),
                        m_javaGlue->m_sendViewInvalidate,
                        rect.x(), rect.y(), rect.maxX(), rect.maxY());
    checkException(env);
}

void WebViewCore::contentDraw()
{
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return;
    env->CallVoidMethod(javaObject.get(), m_javaGlue->m_contentDraw);
    checkException(env);
}

void WebViewCore::layersDraw()
{
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return;
    env->CallVoidMethod(javaObject.get(), m_javaGlue->m_layersDraw);
    checkException(env);
}

void WebViewCore::contentInvalidate(const WebCore::IntRect &r)
{
    DBG_SET_LOGD("rect={%d,%d,w=%d,h=%d}", r.x(), r.y(), r.width(), r.height());
    SkIRect rect(r);
    if (!rect.intersect(0, 0, INT_MAX, INT_MAX))
        return;
    m_addInval.op(rect, SkRegion::kUnion_Op);
    DBG_SET_LOGD("m_addInval={%d,%d,r=%d,b=%d}",
        m_addInval.getBounds().fLeft, m_addInval.getBounds().fTop,
        m_addInval.getBounds().fRight, m_addInval.getBounds().fBottom);
    if (!m_skipContentDraw)
        contentDraw();
}

//SAMSUNG CHANGE: <MPSG100003899> xhtml page zoom scale change issue on orientation change and back key press >>
void WebViewCore::recalcWidthAndForceLayout()
{
    if(!m_mainFrame->document()){
        return;
    }
    if(!m_mainFrame->document()->isXHTMLDocument()) {
        return;
    }
    WebCore::FrameView* view = m_mainFrame->view();
    m_mainFrame->contentRenderer()->setNeedsLayoutAndPrefWidthsRecalc();
    view->forceLayout();
}
//SAMSUNG CHANGE: <MPSG100003899> xhtml page zoom scale change issue on orientation change and back key press <<

void WebViewCore::contentInvalidateAll()
{
    WebCore::FrameView* view = m_mainFrame->view();
    contentInvalidate(WebCore::IntRect(0, 0,
        view->contentsWidth(), view->contentsHeight()));
}

void WebViewCore::offInvalidate(const WebCore::IntRect &r)
{
    // FIXME: these invalidates are offscreen, and can be throttled or
    // deferred until the area is visible. For now, treat them as
    // regular invals so that drawing happens (inefficiently) for now.
    contentInvalidate(r);
}

static int pin_pos(int x, int width, int targetWidth)
{
    if (x + width > targetWidth)
        x = targetWidth - width;
    if (x < 0)
        x = 0;
    return x;
}

void WebViewCore::didFirstLayout()
{
    DEBUG_NAV_UI_LOGD("%s", __FUNCTION__);
    LOG_ASSERT(m_javaGlue->m_obj, "A Java widget was not associated with this view bridge!");

    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return;

    const WebCore::KURL& url = m_mainFrame->document()->url();
    if (url.isEmpty())
        return;
    LOGV("::WebCore:: didFirstLayout %s", url.string().ascii().data());
    DBG_NAV_LOG("didFirstLayout ; image set to NULL");
    // SAMSUNG CHANGE ++	
    if (animatedImage)	{
	animatedImage->deref();
    	animatedImage = NULL; 
    }
    // SAMSUNG CHANGE --	
    WebCore::FrameLoadType loadType = m_mainFrame->loader()->loadType();

    env->CallVoidMethod(javaObject.get(), m_javaGlue->m_didFirstLayout,
            loadType == WebCore::FrameLoadTypeStandard
            // When redirect with locked history, we would like to reset the
            // scale factor. This is important for www.yahoo.com as it is
            // redirected to www.yahoo.com/?rs=1 on load.
            || loadType == WebCore::FrameLoadTypeRedirectWithLockedBackForwardList
            // When "request desktop page" is used, we want to treat it as
            // a newly-loaded page.
            || loadType == WebCore::FrameLoadTypeSame);
    checkException(env);

    DBG_NAV_LOG("call updateFrameCache");
    m_check_domtree_version = false;
    updateFrameCache();
    m_history.setDidFirstLayout(true);
}

void WebViewCore::updateViewport()
{
    DEBUG_NAV_UI_LOGD("%s", __FUNCTION__);
    LOG_ASSERT(m_javaGlue->m_obj, "A Java widget was not associated with this view bridge!");

    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return;
    env->CallVoidMethod(javaObject.get(), m_javaGlue->m_updateViewport);
    checkException(env);
}

void WebViewCore::restoreScale(float scale, float textWrapScale)
{
    DEBUG_NAV_UI_LOGD("%s", __FUNCTION__);
    LOG_ASSERT(m_javaGlue->m_obj, "A Java widget was not associated with this view bridge!");

    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return;
    env->CallVoidMethod(javaObject.get(), m_javaGlue->m_restoreScale, scale, textWrapScale);
    checkException(env);
}

void WebViewCore::needTouchEvents(bool need)
{
    DEBUG_NAV_UI_LOGD("%s", __FUNCTION__);
    LOG_ASSERT(m_javaGlue->m_obj, "A Java widget was not associated with this view bridge!");

#if ENABLE(TOUCH_EVENTS)
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return;

    if (m_forwardingTouchEvents == need)
        return;

    env->CallVoidMethod(javaObject.get(), m_javaGlue->m_needTouchEvents, need);
    checkException(env);

    m_forwardingTouchEvents = need;
#endif
}

void WebViewCore::requestKeyboardWithSelection(const WebCore::Node* node,
        int selStart, int selEnd)
{
    DEBUG_NAV_UI_LOGD("%s", __FUNCTION__);
    LOG_ASSERT(m_javaGlue->m_obj, "A Java widget was not associated with this view bridge!");

    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return;
    env->CallVoidMethod(javaObject.get(),
            m_javaGlue->m_requestKeyboardWithSelection,
            reinterpret_cast<int>(node), selStart, selEnd, m_textGeneration);
    checkException(env);
}

void WebViewCore::requestKeyboard(bool showKeyboard)
{
    DEBUG_NAV_UI_LOGD("%s", __FUNCTION__);
    LOG_ASSERT(m_javaGlue->m_obj, "A Java widget was not associated with this view bridge!");

    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return;
    env->CallVoidMethod(javaObject.get(), m_javaGlue->m_requestKeyboard, showKeyboard);
    checkException(env);
}

//SAMSUNG HTML5 INPUT TYPE DATE/TIME CHANGES <<
void WebViewCore::requestDateTimePickers(const WTF::String& type , const WTF::String& value)
{
    DEBUG_NAV_UI_LOGD("%s", __FUNCTION__);
    LOG_ASSERT(m_javaGlue->m_obj, "A Java widget was not associated with this view bridge!");

    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return;
    jstring jinputStr = wtfStringToJstring(env, type);	
    jstring jvalueStr = NULL;
    if(value != NULL)
        jvalueStr = wtfStringToJstring(env, value);	
    env->CallVoidMethod(javaObject.get(), m_javaGlue->m_requestDateTimePickers,jinputStr, jvalueStr);
    checkException(env);
}
//SAMSUNG HTML5 INPUT TYPE DATE/TIME CHANGES >>

void WebViewCore::notifyProgressFinished()
{
    m_check_domtree_version = true;
    sendNotifyProgressFinished();
}

void WebViewCore::doMaxScroll(CacheBuilder::Direction dir)
{
    int dx = 0, dy = 0;

    switch (dir) {
    case CacheBuilder::LEFT:
        dx = -m_maxXScroll;
        break;
    case CacheBuilder::UP:
        dy = -m_maxYScroll;
        break;
    case CacheBuilder::RIGHT:
        dx = m_maxXScroll;
        break;
    case CacheBuilder::DOWN:
        dy = m_maxYScroll;
        break;
    case CacheBuilder::UNINITIALIZED:
    default:
        LOG_ASSERT(0, "unexpected focus selector");
    }
    WebCore::FrameView* view = m_mainFrame->view();
    this->scrollTo(view->scrollX() + dx, view->scrollY() + dy, true);
}

void WebViewCore::setScrollOffset(int moveGeneration, bool sendScrollEvent, int dx, int dy)
{
    DBG_NAV_LOGD("{%d,%d} m_scrollOffset=(%d,%d), sendScrollEvent=%d", dx, dy,
        m_scrollOffsetX, m_scrollOffsetY, sendScrollEvent);
    if (m_scrollOffsetX != dx || m_scrollOffsetY != dy) {
        m_scrollOffsetX = dx;
        m_scrollOffsetY = dy;
        // The visible rect is located within our coordinate space so it
        // contains the actual scroll position. Setting the location makes hit
        // testing work correctly.
        m_mainFrame->view()->platformWidget()->setLocation(m_scrollOffsetX,
                m_scrollOffsetY);
        if (sendScrollEvent) {
            m_mainFrame->eventHandler()->sendScrollEvent();

            // Only update history position if it's user scrolled.
            // Update history item to reflect the new scroll position.
            // This also helps save the history information when the browser goes to
            // background, so scroll position will be restored if browser gets
            // killed while in background.
            WebCore::HistoryController* history = m_mainFrame->loader()->history();
            // Because the history item saving could be heavy for large sites and
            // scrolling can generate lots of small scroll offset, the following code
            // reduces the saving frequency.
            static const int MIN_SCROLL_DIFF = 32;
            if (history->currentItem()) {
                WebCore::IntPoint currentPoint = history->currentItem()->scrollPoint();
                if (std::abs(currentPoint.x() - dx) >= MIN_SCROLL_DIFF ||
                    std::abs(currentPoint.y() - dy) >= MIN_SCROLL_DIFF) {
                    history->saveScrollPositionAndViewStateToItem(history->currentItem());
                }
            }
        }

        // update the currently visible screen
        sendPluginVisibleScreen();
    }
    gCursorBoundsMutex.lock();
    bool hasCursorBounds = m_hasCursorBounds;
    Frame* frame = (Frame*) m_cursorFrame;
    IntPoint location = m_cursorLocation;
    gCursorBoundsMutex.unlock();
    // SAMSUNG CHANGE +
    if (animatedImage) {
	DBG_NAV_LOG("Animated Image Preset");
	if (animatedImage->checkForVisibleImageAnimation()) {
	    animatedImage->deref();	
	animatedImage = NULL;
	    DBG_NAV_LOG("Animation Started");
	}
    }
    // SAMSUNG CHANGE -
    if (!hasCursorBounds)
        return;
    moveMouseIfLatest(moveGeneration, frame, location.x(), location.y());
}

void WebViewCore::setGlobalBounds(int x, int y, int h, int v)
{
    DBG_NAV_LOGD("{%d,%d}", x, y);
    m_mainFrame->view()->platformWidget()->setWindowBounds(x, y, h, v);
}

void WebViewCore::setSizeScreenWidthAndScale(int width, int height,
    int textWrapWidth, float scale, int screenWidth, int screenHeight,
    int anchorX, int anchorY, bool ignoreHeight)
{
//SAMSUNG CHANGES : FACEBOOK PERFORMANCE IMPROVEMENT : Praveen Munukutla(sataya.m@samsung.com)>>> When Textarea is on focus and user rotates the screen repainting should happen
	 m_mainFrame->document()->setCheckNode(0);
//SAMSUNG CHANGES : FACEBOOK PERFORMANCE IMPROVEMENT : Praveen Munukutla(sataya.m@samsung.com)<<<
 
    // Ignore the initial empty document.
    const WebCore::KURL& url = m_mainFrame->document()->url();
    if (url.isEmpty())
        return;

    WebCoreViewBridge* window = m_mainFrame->view()->platformWidget();
    int ow = window->width();
    int oh = window->height();
    int osw = m_screenWidth;
    int osh = m_screenHeight;
    int otw = m_textWrapWidth;
    float oldScale = m_scale;
    DBG_NAV_LOGD("old:(w=%d,h=%d,sw=%d,scale=%g) new:(w=%d,h=%d,sw=%d,scale=%g)",
        ow, oh, osw, m_scale, width, height, screenWidth, scale);
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;
    m_textWrapWidth = textWrapWidth;
    if (scale >= 0) // negative means keep the current scale
        m_scale = scale;
    m_maxXScroll = screenWidth >> 2;
    m_maxYScroll = m_maxXScroll * height / width;
    // Don't reflow if the diff is small.
    const bool reflow = otw && textWrapWidth &&
        ((float) abs(otw - textWrapWidth) / textWrapWidth) >= 0.01f;

    // When the screen size change, fixed positioned element should be updated.
    // This is supposed to be light weighted operation without a full layout.
    if (osh != screenHeight || osw != screenWidth)
        m_mainFrame->view()->updatePositionedObjects();

    if (ow != width || (!ignoreHeight && oh != height) || reflow) {
        WebCore::RenderObject *r = m_mainFrame->contentRenderer();
        DBG_NAV_LOGD("renderer=%p view=(w=%d,h=%d)", r,
                screenWidth, screenHeight);
        if (r) {
            WebCore::IntPoint anchorPoint = WebCore::IntPoint(anchorX, anchorY);
            DBG_NAV_LOGD("anchorX=%d anchorY=%d", anchorX, anchorY);
            RefPtr<WebCore::Node> node;
            WebCore::IntRect bounds;
            WebCore::IntPoint offset;
            // If the text wrap changed, it is probably zoom change or
            // orientation change. Try to keep the anchor at the same place.
            if (otw && textWrapWidth && otw != textWrapWidth &&
                (anchorX != 0 || anchorY != 0)) {
                WebCore::HitTestResult hitTestResult =
                        m_mainFrame->eventHandler()->hitTestResultAtPoint(
                                anchorPoint, false);
                node = hitTestResult.innerNode();
				// SAMSUNG CHANGES >>
				// if node is fullscreen size node, it returns 0,0. so viewpoint goes to 0,0.
				// eg. naver pc page, double tap blank space of top-right side and rotate > viewpoint goes to 0,0
				if(node) {
					bounds = node->getRect();
					if ( bounds.width() == ow ) node = NULL ;
				}
				// SAMSUNG CHANGES <<
            }
            if (node) {
                bounds = node->getRect();
                DBG_NAV_LOGD("ob:(x=%d,y=%d,w=%d,h=%d)",
                    bounds.x(), bounds.y(), bounds.width(), bounds.height());
                // sites like nytimes.com insert a non-standard tag <nyt_text>
                // in the html. If it is the HitTestResult, it may have zero
                // width and height. In this case, use its parent node.
                if (bounds.width() == 0) {
                    node = node->parentOrHostNode();
                    if (node) {
                        bounds = node->getRect();
                        DBG_NAV_LOGD("found a zero width node and use its parent, whose ob:(x=%d,y=%d,w=%d,h=%d)",
                                bounds.x(), bounds.y(), bounds.width(), bounds.height());
                    }
                }
            }

            // Set the size after finding the old anchor point as
            // hitTestResultAtPoint causes a layout.
            window->setSize(width, height);
            window->setVisibleSize(screenWidth, screenHeight);
            if (width != screenWidth) {
                m_mainFrame->view()->setUseFixedLayout(true);
                m_mainFrame->view()->setFixedLayoutSize(IntSize(width, height));
            } else
                m_mainFrame->view()->setUseFixedLayout(false);
            r->setNeedsLayoutAndPrefWidthsRecalc();
//SAMSUNG CHANGE -[MPSG100003482] - Display issue fix in amazon.com
            //if (m_mainFrame->view()->didFirstLayout())
//SAMSUNG CHANGE -[MPSG100003482] - Display issue fix in amazon.com
            m_mainFrame->view()->forceLayout();

            // scroll to restore current screen center
            if (node) {
                const WebCore::IntRect& newBounds = node->getRect();
                DBG_NAV_LOGD("nb:(x=%d,y=%d,w=%d,"
                    "h=%d)", newBounds.x(), newBounds.y(),
                    newBounds.width(), newBounds.height());
                if ((osw && osh && bounds.width() && bounds.height())
                    && (bounds != newBounds)) {
                    WebCore::FrameView* view = m_mainFrame->view();
                    // force left align if width is not changed while height changed.
                    // the anchorPoint is probably at some white space in the node
                    // which is affected by text wrap around the screen width.
                    const bool leftAlign = (otw != textWrapWidth)
                        && (bounds.width() == newBounds.width())
                        && (bounds.height() != newBounds.height());
                    const float xPercentInDoc =
                        leftAlign ? 0.0 : (float) (anchorX - bounds.x()) / bounds.width();
                    const float xPercentInView =
                        leftAlign ? 0.0 : (float) (anchorX - m_scrollOffsetX) / osw;
                    const float yPercentInDoc = (float) (anchorY - bounds.y()) / bounds.height();
                    const float yPercentInView = (float) (anchorY - m_scrollOffsetY) / osh;
                    showRect(newBounds.x(), newBounds.y(), newBounds.width(),
                             newBounds.height(), view->contentsWidth(),
                             view->contentsHeight(),
                             xPercentInDoc, xPercentInView,
                             yPercentInDoc, yPercentInView);
                }
            }
        }
    } else {
        window->setSize(width, height);
        window->setVisibleSize(screenWidth, screenHeight);
        m_mainFrame->view()->resize(width, height);
        if (width != screenWidth) {
            m_mainFrame->view()->setUseFixedLayout(true);
            m_mainFrame->view()->setFixedLayoutSize(IntSize(width, height));
        } else
            m_mainFrame->view()->setUseFixedLayout(false);
        }

    // update the currently visible screen as perceived by the plugin
    sendPluginVisibleScreen();
}

void WebViewCore::dumpDomTree(bool useFile)
{
#ifdef ANDROID_DOM_LOGGING
    if (useFile)
        gDomTreeFile = fopen(DOM_TREE_LOG_FILE, "w");
    m_mainFrame->document()->showTreeForThis();
    if (gDomTreeFile) {
        fclose(gDomTreeFile);
        gDomTreeFile = 0;
    }
#endif
}

void WebViewCore::dumpRenderTree(bool useFile)
{
#ifdef ANDROID_DOM_LOGGING
    WTF::CString renderDump = WebCore::externalRepresentation(m_mainFrame).utf8();
    const char* data = renderDump.data();
    if (useFile) {
        gRenderTreeFile = fopen(RENDER_TREE_LOG_FILE, "w");
        DUMP_RENDER_LOGD("%s", data);
        fclose(gRenderTreeFile);
        gRenderTreeFile = 0;
    } else {
        // adb log can only output 1024 characters, so write out line by line.
        // exclude '\n' as adb log adds it for each output.
        int length = renderDump.length();
        for (int i = 0, last = 0; i < length; i++) {
            if (data[i] == '\n') {
                if (i != last)
                    DUMP_RENDER_LOGD("%.*s", (i - last), &(data[last]));
                last = i + 1;
            }
        }
    }
#endif
}

void WebViewCore::dumpNavTree()
{
#if DUMP_NAV_CACHE
    cacheBuilder().mDebug.print();
#endif
}

HTMLElement* WebViewCore::retrieveElement(int x, int y,
    const QualifiedName& tagName)
{
    HitTestResult hitTestResult = m_mainFrame->eventHandler()
        ->hitTestResultAtPoint(IntPoint(x, y), false, false,
        DontHitTestScrollbars, HitTestRequest::Active | HitTestRequest::ReadOnly,
        IntSize(1, 1));
    if (!hitTestResult.innerNode() || !hitTestResult.innerNode()->inDocument()) {
        LOGE("Should not happen: no in document Node found");
        return 0;
    }
    const ListHashSet<RefPtr<Node> >& list = hitTestResult.rectBasedTestResult();
    if (list.isEmpty()) {
        LOGE("Should not happen: no rect-based-test nodes found");
        return 0;
    }
    Node* node = hitTestResult.innerNode();
    Node* element = node;
    while (element && (!element->isElementNode()
        || !element->hasTagName(tagName))) {
        element = element->parentNode();
    }
    DBG_NAV_LOGD("node=%p element=%p x=%d y=%d nodeName=%s tagName=%s", node,
        element, x, y, node->nodeName().utf8().data(),
        element ? ((Element*) element)->tagName().utf8().data() : "<none>");
    return static_cast<WebCore::HTMLElement*>(element);
}

HTMLAnchorElement* WebViewCore::retrieveAnchorElement(int x, int y)
{
    return static_cast<HTMLAnchorElement*>
        (retrieveElement(x, y, HTMLNames::aTag));
}

HTMLImageElement* WebViewCore::retrieveImageElement(int x, int y)
{
    return static_cast<HTMLImageElement*>
        (retrieveElement(x, y, HTMLNames::imgTag));
}

WTF::String WebViewCore::retrieveHref(int x, int y)
{
    WebCore::HTMLAnchorElement* anchor = retrieveAnchorElement(x, y);
    return anchor ? anchor->href() : WTF::String();
}

WTF::String WebViewCore::retrieveAnchorText(int x, int y)
{
    WebCore::HTMLAnchorElement* anchor = retrieveAnchorElement(x, y);
    return anchor ? anchor->text() : WTF::String();
}

WTF::String WebViewCore::retrieveImageSource(int x, int y)
{
    HTMLImageElement* image = retrieveImageElement(x, y);
    return image ? image->src().string() : WTF::String();
}

WTF::String WebViewCore::requestLabel(WebCore::Frame* frame,
        WebCore::Node* node)
{
    if (node && CacheBuilder::validNode(m_mainFrame, frame, node)) {
        RefPtr<WebCore::NodeList> list = node->document()->getElementsByTagName("label");
        unsigned length = list->length();
        for (unsigned i = 0; i < length; i++) {
            WebCore::HTMLLabelElement* label = static_cast<WebCore::HTMLLabelElement*>(
                    list->item(i));
            if (label->control() == node) {
                Node* node = label;
                String result;
                while ((node = node->traverseNextNode(label))) {
                    if (node->isTextNode() && node->renderer()) { //SISO_CHANGE, [MPSG100003942] [GA0100484981]
                        Text* textNode = static_cast<Text*>(node);
                        result += textNode->dataImpl();
                    }
                }
                return result;
            }
        }
    }
    return WTF::String();
}

static bool isContentEditable(const WebCore::Node* node)
{
    if (!node) return false;
    return node->document()->frame()->selection()->isContentEditable();
}

// Returns true if the node is a textfield, textarea, or contentEditable
static bool isTextInput(const WebCore::Node* node)
{
    if (isContentEditable(node))
        return true;
    if (!node)
        return false;
    WebCore::RenderObject* renderer = node->renderer();
    return renderer && (renderer->isTextField() || renderer->isTextArea());
}

void WebViewCore::revealSelection()
{
    WebCore::Node* focus = currentFocus();
    if (!focus)
        return;
    if (!isTextInput(focus))
        return;
    WebCore::Frame* focusedFrame = focus->document()->frame();
    if (!focusedFrame->page()->focusController()->isActive())
        return;
//SAMSUNG CHANGE >>
// Original google code alligned to edge, but we have modified it to align to center
//AJAY - Align to center is causing flickering effect while entering in multiline input box
//Changing to align to edge
    focusedFrame->selection()->revealSelection(ScrollAlignment::alignToEdgeIfNeeded);
//SAMSUNG CHANGE <<
}

void WebViewCore::updateCacheOnNodeChange()
{
    gCursorBoundsMutex.lock();
    bool hasCursorBounds = m_hasCursorBounds;
    Frame* frame = (Frame*) m_cursorFrame;
    Node* node = (Node*) m_cursorNode;
    IntRect bounds = m_cursorHitBounds;
    gCursorBoundsMutex.unlock();
    if (!hasCursorBounds || !node)
        return;
    if (CacheBuilder::validNode(m_mainFrame, frame, node)) {
        RenderObject* renderer = node->renderer();
        if (renderer && renderer->style()->visibility() != HIDDEN) {
            IntRect absBox = renderer->absoluteBoundingBoxRect();
            int globalX, globalY;
            CacheBuilder::GetGlobalOffset(frame, &globalX, &globalY);
            absBox.move(globalX, globalY);
            if (absBox == bounds)
                return;
            DBG_NAV_LOGD("absBox=(%d,%d,%d,%d) bounds=(%d,%d,%d,%d)",
                absBox.x(), absBox.y(), absBox.width(), absBox.height(),
                bounds.x(), bounds.y(), bounds.width(), bounds.height());
        }
    }
    DBG_NAV_LOGD("updateFrameCache node=%p", node);
    updateFrameCache();
}

void WebViewCore::updateFrameCache()
{
    if (!m_frameCacheOutOfDate) {
        DBG_NAV_LOG("!m_frameCacheOutOfDate");
        return;
    }

    // If there is a pending style recalculation, do not update the frame cache.
    // Until the recalculation is complete, there may be internal objects that
    // are in an inconsistent state (such as font pointers).
    // In any event, there's not much point to updating the cache while a style
    // recalculation is pending, since it will simply have to be updated again
    // once the recalculation is complete.
    // TODO: Do we need to reschedule an update for after the style is recalculated?
    if (m_mainFrame && m_mainFrame->document() && m_mainFrame->document()->isPendingStyleRecalc()) {
        LOGW("updateFrameCache: pending style recalc, ignoring.");
        return;
    }
#ifdef ANDROID_INSTRUMENT
    TimeCounterAuto counter(TimeCounter::WebViewCoreBuildNavTimeCounter);
#endif
    m_frameCacheOutOfDate = false;
    m_temp = new CachedRoot();
    m_temp->init(m_mainFrame, &m_history);
#if USE(ACCELERATED_COMPOSITING)
    GraphicsLayerAndroid* graphicsLayer = graphicsRootLayer();
    if (graphicsLayer)
        m_temp->setRootLayer(graphicsLayer->contentLayer());
#endif
    CacheBuilder& builder = cacheBuilder();
    WebCore::Settings* settings = m_mainFrame->page()->settings();
    builder.allowAllTextDetection();
#ifdef ANDROID_META_SUPPORT
    if (settings) {
        if (!settings->formatDetectionAddress())
            builder.disallowAddressDetection();
        if (!settings->formatDetectionEmail())
            builder.disallowEmailDetection();
        if (!settings->formatDetectionTelephone())
            builder.disallowPhoneDetection();
    }
#endif
    builder.buildCache(m_temp);
    m_tempPict = new SkPicture();
    recordPicture(m_tempPict);
    m_temp->setPicture(m_tempPict);
    m_temp->setTextGeneration(m_textGeneration);
    WebCoreViewBridge* window = m_mainFrame->view()->platformWidget();
    m_temp->setVisibleRect(WebCore::IntRect(m_scrollOffsetX,
        m_scrollOffsetY, window->width(), window->height()));
    gFrameCacheMutex.lock();
    delete m_frameCacheKit;
    delete m_navPictureKit;
    m_frameCacheKit = m_temp;
    m_navPictureKit = m_tempPict;
    m_updatedFrameCache = true;
#if DEBUG_NAV_UI
    const CachedNode* cachedFocusNode = m_frameCacheKit->currentFocus();
    DBG_NAV_LOGD("cachedFocusNode=%d (nodePointer=%p)",
        cachedFocusNode ? cachedFocusNode->index() : 0,
        cachedFocusNode ? cachedFocusNode->nodePointer() : 0);
#endif
    gFrameCacheMutex.unlock();
}

void WebViewCore::updateFrameCacheIfLoading()
{
    if (!m_check_domtree_version)
        updateFrameCache();
}

struct TouchNodeData {
    Node* mNode;
    IntRect mBounds;
};

//SAMSUNG CHANGE >> LIGHT TOUCH
/* HTMLArea needs speical handling for getting the bounds */
static IntRect getAreaRect(const HTMLAreaElement* area)
{
    Node* node = area->document();
    while ((node = node->traverseNextNode()) != NULL) {
        RenderObject* renderer = node->renderer();
        if (renderer && renderer->isRenderImage()) {
            RenderImage* image = static_cast<RenderImage*>(renderer);
            HTMLMapElement* map = image->imageMap();
            if (map) {
                Node* n;
                for (n = map->firstChild(); n;
                        n = n->traverseNextNode(map)) {
                    if (n == area) {
                        if (area->isDefault())
                            return image->absoluteBoundingBoxRect();
                        return area->computeRect(image);
                    }
                }
            }
        }
    }
    return IntRect();
}

// get the bounding box of the Node
static IntRect getAbsoluteBoundingBox(Node* node) {
    if(!node)
        return IntRect(0,0,0,0) ;
    if(node->hasTagName(HTMLNames::areaTag)){
        HTMLAreaElement* area = static_cast<HTMLAreaElement*>(node);
        return getAreaRect(area) ;
    }
    RenderObject* render = node->renderer();
    if(!render){
        return IntRect(0,0,0,0) ;
    }

    IntRect rect;
    if (render->isRenderInline()){
        rect = toRenderInline(render)->linesVisualOverflowBoundingBox();
    }else if (render->isBox()){
        rect = toRenderBox(render)->visualOverflowRect();
    }
    else if (render->isText()){
        rect = toRenderText(render)->linesBoundingBox();
    }
    else{
        LOGE("getAbsoluteBoundingBox failed for node %p, name %s", node, render->renderName());
    }
     FloatPoint  absPos = render->localToAbsolute(FloatPoint(),false,true);   //Fix for MPSG100004672
    rect.move(absPos.x(), absPos.y());
    RenderBlock *rBlock = render->containingBlock() ;
    if(rBlock){
             IntRect clipRect ;
             Node *tNode = ((RenderObject*)rBlock)->node() ;
             clipRect =  rBlock->absoluteBoundingBoxRect(true);   //Fix for MPSG100004672
             DBG_NAV_LOGD("clip rect(%d %d %d %d) Node name %s", \
                clipRect.x(), clipRect.y(), clipRect.width(), clipRect.height(),\
                tNode?tNode->nodeName().ascii().data():"NULL") ;
             if (render->style()) {
             DBG_NAV_LOGD("positioned element is %s", \
                           render->style()->position() == FixedPosition ? "FixedPosition" :"AbsolutePosition");
             }
             // Do not intersect in case of absolute and fixed positioned elements.
             if (render->style() && (!(render->style()->position() == FixedPosition || render->style()->position() == AbsolutePosition))) {
             rect.intersect(clipRect) ;
             }
// SAMSUNG CHANGE MPSG 3590 >>> If the container block has the same rect after intersecting with the child rect
// (in case of input element with background-position with negative values) focus ring is drawing
// with the container node rect, made it reset to the child node bound rect
	     IntRect absoluteBoundingRect = render->absoluteBoundingBoxRect(true);   //Fix for MPSG100004672
             if(rect.contains(absoluteBoundingRect)) {
                rect = absoluteBoundingRect;
             }
// SAMSUNG CHANGE MPSG 3590 <<<
    }
    return rect;
}
static void  getFocusRingRects(WebCore::Node *node, Vector<IntRect> &ringRect, int offsetX, int offsetY, const WebCore::IntPoint &pt, bool &modClickPoint){
    RenderObject *renderer = NULL ;
    DBG_NAV_LOGD("Orig Pt(%d %d)", pt.x(), pt.y()) ;
    if(node && (renderer = node->renderer())){
        Vector<FloatQuad> quads ;
        RenderBlock *rBlock = renderer->containingBlock() ;
        IntRect clipRect ;
        if(rBlock){
             Node *tNode = ((RenderObject*)rBlock)->node() ;
             clipRect =  rBlock->absoluteBoundingBoxRect();
             DBG_NAV_LOGD("clip rect(%d %d %d %d) Node name %s", \
                clipRect.x(), clipRect.y(), clipRect.width(), clipRect.height(),\
                tNode?tNode->nodeName().ascii().data():"NULL") ;
         }
        node->renderer()->absoluteFocusRingQuads(quads) ;
        for(unsigned int i = 0; i < quads.size() ; ++i ){
            FloatQuad rect = quads[i];
            IntRect tRect = rect.enclosingBoundingBox() ;
            if(rBlock && !clipRect.isEmpty()){
// SAMSUNG CHANGE >>> MPSG 3731, Avoiding the clipping when "nowrap" is applied for the WML <p> element, which is the parent of the anchor.
                if (renderer->style() && renderer->style()->whiteSpace() != NOWRAP){
                    tRect.intersect(clipRect) ;
                }
// SAMSUNG CHANGE <<< MPSG 3731, Avoiding the clipping when "nowrap" is applied for the WML <p> element, which is the parent of the anchor.
                if(tRect.isEmpty()){
                    DBG_NAV_LOG("Clipped out") ;
                    continue ;
                }
            }
            tRect.move(offsetX, offsetY);
            DBG_NAV_LOGD("focus ring rects (%d %d %d %d)", tRect.x(), tRect.y(), tRect.width(), tRect.height()) ;
            if(tRect.contains(pt) && (modClickPoint == true)){
              DBG_NAV_LOG("Not modifying the click point") ;   
		modClickPoint = false ;
	    } 
            ringRect.append(tRect) ;
        }
    }
}
static bool hasTrigger(WebCore::Node *node){

         bool t1 = node->hasEventListeners(eventNames().clickEvent) ;
         bool t2 = node->hasEventListeners(eventNames().mousedownEvent) ;
         bool t3 = node->hasEventListeners(eventNames().mouseupEvent) ;
         bool t4 = node->hasEventListeners(eventNames().keydownEvent) ;
         bool t5 = node->hasEventListeners(eventNames().keyupEvent) ;
         bool t6 = false ; //node->hasEventListeners(eventNames().mouseoverEvent) ;
         bool t7 = false ; //node->hasEventListeners(eventNames().mouseoutEvent) ;

         DBG_NAV_LOGD("t1(%d)t2(%d)t3(%d)t4(%d)t5(%d)t6(%d)t7(%d)", t1, t2, t3, t4, t5, t6, t7) ;
         return t1||t2||t3||t4||t5||t6||t7 ;
}

static bool getBestRect(Vector<IntRect> &vectRect, IntRect &rect){
        if(vectRect.size() == 0){
          return false ;
        }
        rect = vectRect[0] ;
        for(unsigned int i= 1; i < vectRect.size(); ++i){
           if(rect.contains(vectRect[i])){
              rect = vectRect[i] ;
           }
        }
        return true ; 
}
/*
    1. Update any of the Layers in the engine, before doing the hit test. If we are lucky with the direct hit test, use the
    result. Else, go for the Rect based Hit test result.
    2. For all the nodes other than the anchor, use the absolute bounds. For Anchor tags, get the bunch of rects that form the
    focus ring rects
    3. Update the Mouse Position for Mouse Events later on
    */

// get the highlight rectangles for the touch point (x, y) with the slop
Vector<IntRect> WebViewCore::getTouchHighlightRects(int x, int y, int slop)
{
    Vector<IntRect> rects;
    m_mousePosLT = IntPoint(x - m_scrollOffsetX, y - m_scrollOffsetY);
    DBG_NAV_LOGD("The original X Y (%d %d) offset(%d %d)", x - m_scrollOffsetX, y - m_scrollOffsetY, m_scrollOffsetX, m_scrollOffsetY) ;

    HitTestResult hitTestResult = m_mainFrame->eventHandler()->hitTestResultAtPoint(IntPoint(x, y),
            false, false, DontHitTestScrollbars, HitTestRequest::ReadOnly | HitTestRequest::Active, IntSize(slop, slop));
    Node *directHit  = hitTestResult.innerNode() ;
    if(!directHit || !directHit->inDocument()){
        DBG_NAV_LOG("Should not happen: no in document Node found");
        return rects;
    }

    Frame* frame = m_mainFrame ;
    if(directHit->document() && directHit->document()->frame()){
        frame = directHit->document()->frame() ;
    }
    bool found = false;
    TouchNodeData final ;
    m_mousePosFrame = frame->view()->windowToContents(m_mousePosLT);
    IntRect testRect(m_mousePosFrame.x() - slop, m_mousePosFrame.y() - slop, 2 * slop + 1, 2 * slop + 1);

    if(hitTestResult.URLElement() && hitTestResult.URLElement()->isLink()){
        Node* node = directHit ;
        while(node && !node->isLink()){
            node = node->parentNode() ;
        }
        if(node){
            directHit = node ;
        }
    }
    if(!found){
        const ListHashSet<RefPtr<Node> >& list = hitTestResult.rectBasedTestResult();
        if (list.isEmpty()) {
            DBG_NAV_LOG("Should not happen: no rect-based-test nodes found");
            return rects;
        }
        frame = hitTestResult.innerNode()->document()->frame();
        Vector<TouchNodeData> nodeDataList;
        ListHashSet<RefPtr<Node> >::const_iterator last = list.end();
        for (ListHashSet<RefPtr<Node> >::const_iterator it = list.begin(); it != last; ++it) {
            // TODO: it seems reasonable to not search across the frame. Isn't it?
            // if the node is not in the same frame as the innerNode, skip it
            if (it->get()->document()->frame() != frame)
                continue;
            // traverse up the tree to find the first node that needs highlight
            Node* eventNode = it->get();
            while (eventNode) {
                DBG_NAV_LOGD("Node %s", eventNode->nodeName().latin1().data()) ;
                RenderObject* render = eventNode->renderer();
                if(eventNode->disabled())
                    break ;
                if (render && (render->isBody() || render->isRenderView()))
                    break;
                if (eventNode->isFocusable() || hasTrigger(eventNode)) {
                    DBG_NAV_LOGD("Node %s has event Listners", eventNode->nodeName().latin1().data()) ;
                    found = true;
                    break;
                }else{
                    DBG_NAV_LOG("has no Event Handlers") ;
                }
                // the nodes in the rectBasedTestResult() are ordered based on z-index during hit testing.
                // so do not search for the eventNode across explicit z-index border.
                // TODO: this is a hard one to call. z-index is quite complicated as its value only
                // matters when you compare two RenderLayer in the same hierarchy level. e.g. in
                // the following example, "b" is on the top as its z level is the highest. even "c"
                // has 100 as z-index, it is still below "d" as its parent has the same z-index as
                // "d" and logically before "d". Of course "a" is the lowest in the z level.
                //
                // z-index:auto "a"
                //   z-index:2 "b"
                //   z-index:1
                //     z-index:100 "c"
                //   z-index:1 "d"
                //
                // If the fat point touches everyone, the order in the list should be "b", "d", "c"
                // and "a". When we search for the event node for "b", we really don't want "a" as
                // in the z-order it is behind everything else.
                if (!render->style()->hasAutoZIndex())
                    break;
                eventNode = eventNode->parentNode();
            }
            // didn't find any eventNode, skip it
            if (!found){
                DBG_NAV_LOG("Did Not find anything in the current Tree. Moving to the next node in the HitTest Result") ;
                continue;
            }
            // first quick check whether it is a duplicated node before computing bounding box
            Vector<TouchNodeData>::const_iterator nlast = nodeDataList.end();
            DBG_NAV_LOGD("Node Data List %d", nodeDataList.size()) ;
            for (Vector<TouchNodeData>::const_iterator n = nodeDataList.begin(); n != nlast; ++n) {
                // found the same node, skip it
                if (eventNode == n->mNode) {
                    found = false;
                    break;
                }
            }
            if (!found)
                continue;
            // next check whether the node is fully covered by or fully covering another node.
            found = false;
            IntRect rect = getAbsoluteBoundingBox(eventNode);
            if (rect.isEmpty()) {
                // if the node's bounds is empty and it is not a ContainerNode, skip it.
                if (!eventNode->isContainerNode())
                    continue;
                // if the node's children are all positioned objects, its bounds can be empty.
                // Walk through the children to find the bounding box.
                DBG_NAV_LOG("Node's children are all positioned") ;
                Node* child = static_cast<const ContainerNode*>(eventNode)->firstChild();
                while (child) {
                    IntRect childrect;
                    if (child->renderer())
                        childrect = getAbsoluteBoundingBox(child);
                    if (!childrect.isEmpty()) {
                        rect.unite(childrect);
                        child = child->traverseNextSibling(eventNode);
                    } else
                        child = child->traverseNextNode(eventNode);
                }
                DBG_NAV_LOGD("rect (%d %d %d %d)", rect.x(), rect.y(), rect.width(), rect.height()) ;
            }
            for (int i = nodeDataList.size() - 1; i >= 0; i--) {
                TouchNodeData n = nodeDataList.at(i);
                // the new node is enclosing an existing node, skip it
                if (rect.contains(n.mBounds)) {
                    found = true;
                    break;
                }
                // the new node is fully inside an existing node, remove the existing node
                if (n.mBounds.contains(rect))
                    nodeDataList.remove(i);
            }
            if (!found) {
                DBG_NAV_LOGD("Found a Node(%s) that  has to be added to the stack bounds (%d %d %d %d)",\
                    eventNode->nodeName().latin1().data(), rect.x(), rect.y(), rect.width(), rect.height()) ;
                TouchNodeData newNode;
                newNode.mNode = eventNode;
                newNode.mBounds = rect;
                nodeDataList.append(newNode);
            }
        }
        if (!nodeDataList.size()){
            DBG_NAV_LOG("Hit Test Returned Empty") ;
            if(!directHit || !(directHit->isFocusable() || hasTrigger(directHit))){
                DBG_NAV_LOG("directHit has no trigger as well") ;
            return rects;
            }else{
                DBG_NAV_LOG("directHit added to Empty Node List") ;
                TouchNodeData data ;
                data.mNode = directHit ;
                data.mBounds = getAbsoluteBoundingBox(directHit) ;
                nodeDataList.append(data) ;
        }
        }
        // finally select the node with the largest overlap with the fat point
        final.mNode = 0;
        DBG_NAV_LOGD("Test Rect (%d %d %d %d)", testRect.x(), testRect.y(), testRect.width(), testRect.height()) ;
        int area = 0;
        Vector<TouchNodeData>::const_iterator nlast = nodeDataList.end();
        for (Vector<TouchNodeData>::const_iterator n = nodeDataList.begin(); n != nlast; ++n) {
            IntRect rect = n->mBounds;
            rect.intersect(testRect);
            int a = rect.width() * rect.height();
            if (a > area) {
                final = *n;
                area = a;
            }else{
                // Give Preference to the child
                if(n->mNode->isDescendantOf(final.mNode)){
                    final = *n ;
                    area = a ;
                }
            }
        }
    }

	
    // now get the node's highlight rectangles in the page coordinate system
    if (final.mNode) {

        if(directHit && directHit != final.mNode){
            if(directHit->isDescendantOf(final.mNode)){
                if(directHit->isFocusable() || hasTrigger(directHit)){
                    final.mNode = directHit ;
                    final.mBounds = getAbsoluteBoundingBox(directHit) ;
                    DBG_NAV_LOG("Direct Hit replacing indirect Hit") ;
                }else{
                    DBG_NAV_LOG("Direct Hit on a Descendant without any triggers") ;
                    //return rects ;
                }
            }
        }

        IntPoint frameAdjust = IntPoint(0,0);
        if (frame != m_mainFrame) {
            frameAdjust = frame->view()->contentsToWindow(IntPoint());
            frameAdjust.move(m_scrollOffsetX, m_scrollOffsetY);
        }
        DBG_NAV_LOGD("Final Node name %s bounds(%d %d %d %d) ptr(%p) ", \
            final.mNode->nodeName().latin1().data(),\
            final.mBounds.x(), final.mBounds.y(), final.mBounds.width(), final.mBounds.height(),\
            final.mNode) ;
        DBG_NAV_LOGD("Final Node is a link: %d",final.mNode->isLink());
        if (final.mNode->isLink()) {
//SAMSUNG CHANGE >> LIGHT TOUCH
            setNavType(1) ;
//SAMSUNG CHANGE << LIGHT TOUCH
            const HTMLAnchorElement* anchorNode =
                (const HTMLAnchorElement*) (final.mNode);
            KURL href = anchorNode->href();
            String exported ;
            if (!href.isEmpty() && !WebCore::protocolIsJavaScript(href.string()))
                // Set the exported string for all non-javascript anchors.
                exported = href.string().threadsafeCopy();
            DBG_NAV_LOGD("Link %s", exported.latin1().data()) ;
            // most of the links are inline instead of box style. So the bounding box is not
            // a good representation for the highlights. Get the list of rectangles instead.
            bool changeClickPt = true ;
            getFocusRingRects(final.mNode, rects, frameAdjust.x(), \
		frameAdjust.y(), WebCore::IntPoint(x,y), changeClickPt) ;

            if (!rects.isEmpty()) {
                    // if neither x nor y has overlap, just pick the top/left of the first rectangle
                    int newx = x ;
                    int newy = y ;
                    DBG_NAV_LOGD("newx, new y -->1  (%d %d)", newx, newy) ;
                    if(changeClickPt){
                       IntRect tRect(0,0,0,0) ;
                       if(getBestRect(rects, tRect)){
                          if(!tRect.contains(IntPoint(newx, newy))){
                            newx = tRect.x() ;
                            newy = tRect.y() ;
                          }
                       }   
                    } 
                    DBG_NAV_LOGD("newx, new y -->2  (%d %d)", newx, newy) ;          
                    m_mousePosLT.setX(newx - m_scrollOffsetX);
                    m_mousePosLT.setY(newy - m_scrollOffsetY);
                    DBG_NAV_LOGD("Move x/y from (%d, %d) to (%d, %d) scrollOffset is (%d, %d) MousePos(%d %d)",
                            x, y, m_mousePosLT.x() + m_scrollOffsetX, m_mousePosLT.y() + m_scrollOffsetY,
                            m_scrollOffsetX, m_scrollOffsetY, m_mousePosFrame.x(), m_mousePosFrame.y());
                    DBG_NAV_LOGD("1 rects size %d", rects.size()) ;
                    for(unsigned int i = 0; i  < rects.size(); ++i){
                        DBG_NAV_LOGD("rects(%d %d %d %d)", rects[i].x(), rects[i].y(), rects[i].width(),rects[i].height()) ;
                    }
                    DBG_NAV_LOGD("The New X Y (%d %d) offset(%d %d)", newx - m_scrollOffsetX, newy - m_scrollOffsetY, m_scrollOffsetX, m_scrollOffsetY) ;
                    m_mousePosFrame = frame->view()->windowToContents(m_mousePosLT);
                    return rects;
            }
        }else{
             if (final.mNode->hasTagName(HTMLNames::inputTag)) {
                 DBG_NAV_LOG("is a inputTag ");
                 HTMLInputElement *inputElement = static_cast<HTMLInputElement *>(final.mNode);
                 if (!inputElement->isText() && !inputElement->isEmailField() && !inputElement->isFileUpload() &&
                     !inputElement->isInputTypeHidden() && !inputElement->isPasswordField()) {
                     setNavType(1) ;
                 }
             }else if (!(final.mNode->hasTagName(HTMLNames::objectTag)
                         || final.mNode->hasTagName(HTMLNames::selectTag) || final.mNode->hasTagName(HTMLNames::embedTag)  
#if ENABLE(WML)
                         || final.mNode->hasTagName(WMLNames::selectTag)
#endif
																		 )) {
                       DBG_NAV_LOG("not a selectTag ");
		              setNavType(1) ;
             }
            RenderObject *renderer ;
            if(final.mNode && (renderer = final.mNode->renderer())){
                RenderBlock *rBlock = renderer->containingBlock() ;
                if(rBlock){
                   if (renderer->style()) {
                       DBG_NAV_LOGD("positioned element is %s", \
                                  renderer->style()->position() == FixedPosition ? "FixedPosition" :"AbsolutePosition");
                   }
                   if (renderer->style() && (!(renderer->style()->position() == FixedPosition || renderer->style()->position() == AbsolutePosition))) {
                    IntRect clipRect  =  rBlock->absoluteBoundingBoxRect();
                    if(!clipRect.isEmpty()){
                        final.mBounds.intersect(clipRect) ;
                       }
                    }
                }
            }
        }
        IntRect rect = final.mBounds;
        rect.move(frameAdjust.x(), frameAdjust.y());
        rects.append(rect);
        // adjust m_mousePos if it is not inside the returned highlight rectangle
        testRect.move(frameAdjust.x(), frameAdjust.y());
        testRect.intersect(rect);
        if (!testRect.contains(x, y)) {
            m_mousePosLT = WebCore::IntPoint(testRect.x(), testRect.y()) ;
            m_mousePosLT.move(-m_scrollOffsetX, -m_scrollOffsetY);
            DBG_NAV_LOGD("Move x/y from (%d, %d) to (%d, %d) scrollOffset is (%d, %d) mousePos(%d %d)",
                    x, y, m_mousePosLT.x() + m_scrollOffsetX, m_mousePosLT.y() + m_scrollOffsetY,
                    m_scrollOffsetX, m_scrollOffsetY, m_mousePosFrame.x(), m_mousePosFrame.y());
            m_mousePosFrame = frame->view()->windowToContents(m_mousePosLT);
        }
    }
    DBG_NAV_LOGD("the New X Y (%d %d) offset(%d %d)",m_mousePosLT.x(),m_mousePosLT.y(),\
        m_scrollOffsetX, m_scrollOffsetY) ;
    DBG_NAV_LOGD("rects size %d", rects.size()) ;
    if(final.mNode && !final.mNode->isFocusable()) {
	// Do not draw the focus ring for non-focusable nodes
	DBG_NAV_LOGD("node name %s is not focusable; clearing the rects",final.mNode->nodeName().latin1().data()) ;
	Vector<IntRect> empty_rects;
	return empty_rects;
    }
    return rects;
}

static WebCore::IntRect toContainingView(const WebCore::RenderObject* renderer, const WebCore::IntRect& rendererRect)
{
    WebCore::IntRect result = rendererRect;
    WebCore::RenderView *view = renderer->view() ;
    LOGD("toContainingView: rendererRect(%d, %d, %d, %d)", result.x(), result.y(), result.width(), result.height());

    if (view && view->frameView() ) {
        WebCore::FrameView * frameView = view->frameView() ;
        if (const WebCore::ScrollView* parentScrollView = frameView->parent()) {
            if (parentScrollView->isFrameView()) {

                const FrameView* parentView = static_cast<const WebCore::FrameView*>(parentScrollView);

                // Get our renderer in the parent view
                WebCore::RenderPart* renderer = frameView->frame()->ownerRenderer();
                if (renderer) {
                    WebCore::IntPoint point(rendererRect.location());

                    // Add borders and padding
                    point.move(renderer->borderLeft() + renderer->paddingLeft(),
                        renderer->borderTop() + renderer->paddingTop());
                    WebCore::IntPoint pt = WebCore::roundedIntPoint(renderer->localToAbsolute(point, false, true /* use transforms */));
                    result.setLocation(pt);
                }

                //Let us verify the calculated location
                WebCore::IntPoint test(rendererRect.location());
                LOGD("toContainingView: test(%d, %d)", test.x(), test.y());
                ScrollView* view = frameView;
                //while (view) {
                    LOGD("toContainingView: frame position(%d, %d)", view->x(), view->y());
                    //test.move(view->x(), view->y());
                    //test = _convertToContainingWindow(frameView, test) ;
                    test = view->convertToContainingWindow(test);
                    //view = view->parent();
                //}
                IntPoint scroll ;
                while (view) {
                    scroll.move(view->scrollX(), view->scrollY());
                    view = view->parent();
                }
                test.move(scroll.x(), scroll.y()) ;

                if (test.x() > result.x() || test.y() > result.y()) {
                    LOGD("toContainingView: Inconsistant result(%d, %d, %d, %d), recalculating using frame positions...", result.x(), result.y(), result.width(), result.height());
                    result.setLocation(test);
                }
            }
            else {
                result = frameView->Widget::convertToContainingView(result);
            }
        }
    }

    LOGD("toContainingView: result(%d, %d, %d, %d)", result.x(), result.y(), result.width(), result.height());
    return result ;
}

WebCore::IntRect WebViewCore::getBlockBounds(WebCore::Node* node)
{
    WebCore::IntRect result;
    if (!node) {
        DBG_NAV_LOG("getRenderBlockBounds : HitTest Result Node is NULL!");
        return result;
    }
    WTF::String nodeName = node->nodeName() ;
    WTF::CString nodeNameLatin1 = nodeName.latin1() ;
    DBG_NAV_LOGD("getRenderBlockBounds: node name = %s", nodeNameLatin1.data());


    WebCore::RenderObject *renderer = NULL ;
    WebCore::RenderObject* nodeRenderer = node->renderer();
    if (nodeRenderer != NULL) {
        DBG_NAV_LOGD("getRenderBlockBounds: nodeRenderer = %s", nodeRenderer->renderName());

        if (nodeRenderer->isRenderPart()){
            renderer = nodeRenderer ;
        }
        else if (!nodeRenderer->isRenderBlock() && !nodeRenderer->isRenderImage()) {
            WebCore::RenderBlock *block = nodeRenderer->containingBlock() ;
            if (block) {
                renderer = block ;
            }
        }
        else {
            renderer = nodeRenderer ;
        }
    }
    else if (node->hasTagName(HTMLNames::areaTag) ){
        HTMLAreaElement *area = static_cast<HTMLAreaElement*>(node) ;

        if (area->shape() == HTMLAreaElement::Rect
            && node->parentNode()
            && node->parentNode()->hasTagName(HTMLNames::mapTag)) {

            Node *map = node->parentNode() ;
            if ( map->parentNode()) {
                WebCore::RenderObject *r = map->parentNode()->renderer() ;
                if (r->isRenderBlock()) {
                    IntRect parentRect = r->absoluteBoundingBoxRect() ;
                    result = area->rect() ;
                    result.move(parentRect.x(), parentRect.y()) ;

                    if (r->view() && r->view()->frameView())
                        result = toContainingView(r, result) ;
                }
            }
        }
    }

    if (renderer) {
        result = renderer->absoluteBoundingBoxRect() ;
        result = toContainingView(renderer, result) ;
    }

    if ( renderer == NULL)
        DBG_NAV_LOG("getRenderBlockBounds: No render block found!");
    else
        DBG_NAV_LOGD("getRenderBlockBounds: node=%p result(%d, %d, %d, %d)", node, result.x(), result.y(), result.width(), result.height());

    return result;

}

WebCore::IntRect WebViewCore::getBlockBounds(const WebCore::IntPoint &pt)
{
    WebCore::IntRect result;
    LOGD("getRenderBlockBounds: point=(%d, %d)", pt.x(), pt.y() );

    WebCore::HitTestResult hitTestResult = m_mainFrame->eventHandler()->hitTestResultAtPoint(pt, false, true);

    WebCore::Node* node = hitTestResult.innerNode();
    if (!node) {
        DBG_NAV_LOG("getRenderBlockBounds : HitTest Result Node is NULL!");
        return result;
    }else{
        return getBlockBounds(node);
    }
}
///////////////////////////////////////////////////////////////////////////////

void WebViewCore::addPlugin(PluginWidgetAndroid* w)
{
//    SkDebugf("----------- addPlugin %p", w);
    /* The plugin must be appended to the end of the array. This ensures that if
       the plugin is added while iterating through the array (e.g. sendEvent(...))
       that the iteration process is not corrupted.
     */
    *m_plugins.append() = w;
}

void WebViewCore::removePlugin(PluginWidgetAndroid* w)
{
//    SkDebugf("----------- removePlugin %p", w);
    int index = m_plugins.find(w);
    if (index < 0) {
        SkDebugf("--------------- pluginwindow not found! %p\n", w);
    } else {
        m_plugins.removeShuffle(index);
    }
}

bool WebViewCore::isPlugin(PluginWidgetAndroid* w) const
{
    return m_plugins.find(w) >= 0;
}

void WebViewCore::invalPlugin(PluginWidgetAndroid* w)
{
    const double PLUGIN_INVAL_DELAY = 1.0 / 60;

    if (!m_pluginInvalTimer.isActive()) {
        m_pluginInvalTimer.startOneShot(PLUGIN_INVAL_DELAY);
    }
}

void WebViewCore::drawPlugins()
{
    SkRegion inval; // accumualte what needs to be redrawn
    PluginWidgetAndroid** iter = m_plugins.begin();
    PluginWidgetAndroid** stop = m_plugins.end();

    for (; iter < stop; ++iter) {
        PluginWidgetAndroid* w = *iter;
        SkIRect dirty;
        if (w->isDirty(&dirty)) {
            w->draw();
            inval.op(dirty, SkRegion::kUnion_Op);
        }
    }

    if (!inval.isEmpty()) {
        // inval.getBounds() is our rectangle
        const SkIRect& bounds = inval.getBounds();
        WebCore::IntRect r(bounds.fLeft, bounds.fTop,
                           bounds.width(), bounds.height());
        this->viewInvalidate(r);
    }
}

void WebViewCore::notifyPluginsOnFrameLoad(const Frame* frame) {
    // if frame is the parent then notify all plugins
    if (!frame->tree()->parent()) {
        // trigger an event notifying the plugins that the page has loaded
        ANPEvent event;
        SkANP::InitEvent(&event, kLifecycle_ANPEventType);
        event.data.lifecycle.action = kOnLoad_ANPLifecycleAction;
        sendPluginEvent(event);
        // trigger the on/off screen notification if the page was reloaded
        sendPluginVisibleScreen();
    }
    // else if frame's parent has completed
    else if (!frame->tree()->parent()->loader()->isLoading()) {
        // send to all plugins who have this frame in their heirarchy
        PluginWidgetAndroid** iter = m_plugins.begin();
        PluginWidgetAndroid** stop = m_plugins.end();
        for (; iter < stop; ++iter) {
            Frame* currentFrame = (*iter)->pluginView()->parentFrame();
            while (currentFrame) {
                if (frame == currentFrame) {
                    ANPEvent event;
                    SkANP::InitEvent(&event, kLifecycle_ANPEventType);
                    event.data.lifecycle.action = kOnLoad_ANPLifecycleAction;
                    (*iter)->sendEvent(event);

                    // trigger the on/off screen notification if the page was reloaded
                    ANPRectI visibleRect;
                    getVisibleScreen(visibleRect);
                    (*iter)->setVisibleScreen(visibleRect, m_scale);

                    break;
                }
                currentFrame = currentFrame->tree()->parent();
            }
        }
    }
}

void WebViewCore::getVisibleScreen(ANPRectI& visibleRect)
{
    visibleRect.left = m_scrollOffsetX;
    visibleRect.top = m_scrollOffsetY;
    visibleRect.right = m_scrollOffsetX + m_screenWidth;
    visibleRect.bottom = m_scrollOffsetY + m_screenHeight;
}

void WebViewCore::sendPluginVisibleScreen()
{
    /* We may want to cache the previous values and only send the notification
       to the plugin in the event that one of the values has changed.
     */

    ANPRectI visibleRect;
    getVisibleScreen(visibleRect);

    PluginWidgetAndroid** iter = m_plugins.begin();
    PluginWidgetAndroid** stop = m_plugins.end();
    for (; iter < stop; ++iter) {
        (*iter)->setVisibleScreen(visibleRect, m_scale);
    }
}

void WebViewCore::sendPluginSurfaceReady()
{
    PluginWidgetAndroid** iter = m_plugins.begin();
    PluginWidgetAndroid** stop = m_plugins.end();
    for (; iter < stop; ++iter) {
        (*iter)->checkSurfaceReady();
    }
}

void WebViewCore::sendPluginEvent(const ANPEvent& evt)
{
    /* The list of plugins may be manipulated as we iterate through the list.
       This implementation allows for the addition of new plugins during an
       iteration, but may fail if a plugin is removed. Currently, there are not
       any use cases where a plugin is deleted while processing this loop, but
       if it does occur we will have to use an alternate data structure and/or
       iteration mechanism.
     */
    for (int x = 0; x < m_plugins.count(); x++) {
        m_plugins[x]->sendEvent(evt);
    }
}

PluginWidgetAndroid* WebViewCore::getPluginWidget(NPP npp)
{
    PluginWidgetAndroid** iter = m_plugins.begin();
    PluginWidgetAndroid** stop = m_plugins.end();
    for (; iter < stop; ++iter) {
        if ((*iter)->pluginView()->instance() == npp) {
            return (*iter);
        }
    }
    return 0;
}

static PluginView* nodeIsPlugin(Node* node) {
    RenderObject* renderer = node->renderer();
    if (renderer && renderer->isWidget()) {
        Widget* widget = static_cast<RenderWidget*>(renderer)->widget();
        if (widget && widget->isPluginView())
            return static_cast<PluginView*>(widget);
    }
    return 0;
}

Node* WebViewCore::cursorNodeIsPlugin() {
    gCursorBoundsMutex.lock();
    bool hasCursorBounds = m_hasCursorBounds;
    Frame* frame = (Frame*) m_cursorFrame;
    Node* node = (Node*) m_cursorNode;
    gCursorBoundsMutex.unlock();
    if (hasCursorBounds && CacheBuilder::validNode(m_mainFrame, frame, node)
            && nodeIsPlugin(node)) {
        return node;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
void WebViewCore::moveMouseIfLatest(int moveGeneration,
    WebCore::Frame* frame, int x, int y)
{
    DBG_NAV_LOGD("m_moveGeneration=%d moveGeneration=%d"
        " frame=%p x=%d y=%d",
        m_moveGeneration, moveGeneration, frame, x, y);
    if (m_moveGeneration > moveGeneration) {
        DBG_NAV_LOGD("m_moveGeneration=%d > moveGeneration=%d",
            m_moveGeneration, moveGeneration);
        return; // short-circuit if a newer move has already been generated
    }
    m_lastGeneration = moveGeneration;
    moveMouse(frame, x, y);
}

void WebViewCore::moveFocus(WebCore::Frame* frame, WebCore::Node* node)
{
    DBG_NAV_LOGD("frame=%p node=%p", frame, node);
    if (!node || !CacheBuilder::validNode(m_mainFrame, frame, node)
            || !node->isElementNode())
        return;
    // Code borrowed from FocusController::advanceFocus
    WebCore::FocusController* focusController
            = m_mainFrame->page()->focusController();
    WebCore::Document* oldDoc
            = focusController->focusedOrMainFrame()->document();
    if (oldDoc->focusedNode() == node)
        return;
    if (node->document() != oldDoc)
        oldDoc->setFocusedNode(0);
    focusController->setFocusedFrame(frame);
    static_cast<WebCore::Element*>(node)->focus(false);
}

// Update mouse position
void WebViewCore::moveMouse(WebCore::Frame* frame, int x, int y)
{
    DBG_NAV_LOGD("frame=%p x=%d y=%d scrollOffset=(%d,%d)", frame,
        x, y, m_scrollOffsetX, m_scrollOffsetY);
    if (!frame || !CacheBuilder::validNode(m_mainFrame, frame, 0))
        frame = m_mainFrame;
    // mouse event expects the position in the window coordinate
    m_mousePos = WebCore::IntPoint(x - m_scrollOffsetX, y - m_scrollOffsetY);
    // validNode will still return true if the node is null, as long as we have
    // a valid frame.  Do not want to make a call on frame unless it is valid.
    WebCore::PlatformMouseEvent mouseEvent(m_mousePos, m_mousePos,
        WebCore::NoButton, WebCore::MouseEventMoved, 1, false, false, false,
        false, WTF::currentTime());
    frame->eventHandler()->handleMouseMoveEvent(mouseEvent);
    updateCacheOnNodeChange();
}

// SAMSUNG CHANGE : ADVANCED_TEXT_SELECTION >>
WTF::String WebViewCore::getSelectedText()
{
    DEBUG_NAV_UI_LOGD("%s", __FUNCTION__);
    WebCore::Frame* frame = m_mainFrame;
    WTF::String str = frame->editor()->selectedText();
    DEBUG_NAV_UI_LOGD("%s: End", __FUNCTION__);
    return str;
}
// SAMSUNG CHANGE : ADVANCED_TEXT_SELECTION <<
//SISO_HTMLCOMPOSER begin
void WebViewCore::insertContent(WTF::String content,int newcursorpostion, bool composing, Vector<CompositionUnderline> undVec,
                                int& startOffset,int& endOffset)
{
    LOGD("insertContent enter %d",composing);

    SelectionController* selectionContrler = m_mainFrame->selection();
    if(selectionContrler != NULL && !(selectionContrler->isNone()))
    {
        Position m_endSelPos;
        if(composing == true)
        {
            m_mainFrame->editor()->setUnderLineUpdateFlag(true);
            LOGD(" undVec %d len " ,  undVec.size());
            undelineVec = undVec;
            LOGD(" undelineVec %d len " ,  undelineVec.size());
            m_mainFrame->editor()->setComposition(content, undelineVec , content.length(), content.length());
            startOffset =0;
            endOffset = content.length();
        }
        else
        {
            m_mainFrame->editor()->confirmComposition(content);
            startOffset = -1;
            endOffset = -1;
            m_mainFrame->editor()->setUnderLineUpdateFlag(false);
        }

        m_endSelPos = selectionContrler->selection().visibleEnd().deepEquivalent();

        if(newcursorpostion > 0)
        {
            int forWrdMove = newcursorpostion - 1;
            if(forWrdMove > 0)
            {
                Position tempPosition = getNextPosition(m_endSelPos , forWrdMove) ;
                selectionContrler->setSelection(VisibleSelection(tempPosition));
            }
        }
        else
        {
            int backMove = newcursorpostion < 0 ? -newcursorpostion : 0;
            backMove = content.length() - backMove;
            Position tempPosition = getPreviousPosition(m_endSelPos , backMove) ;
            selectionContrler->setSelection(VisibleSelection(tempPosition));

        }

    }
}

Position WebViewCore::getPreviousPosition(Position& pos , int prevNum)
{
    Node* positionNode = pos.anchorNode(); 
    RenderObject* renderer = 0;

    if(positionNode)
        renderer= positionNode->renderer();
    else
        LOGD("getPreviousPosition position node is null");
    
    if (renderer && renderer->isImage()) {
        LOGD("getPreviousPosition the node before cursor is an image so returning");
        return pos;
    }
    else{
        LOGD("getPreviousPosition the node before cursor is not an image");
    
    }

    if(pos.atStartOfTree())
        return pos;

    if(pos.anchorNode()->renderer() && pos.anchorNode()->renderer()->isBR())
     LOGD(" &&&&&&&&&&&&&&&&&&&&&& node()->renderer()->isBR() pos %s %d " , pos.anchorNode()->nodeName().utf8().data() , pos.deprecatedEditingOffset());

    if(!pos.anchorNode()->offsetInCharacters() )
    {
        LOGD("!pos.anchorNode()->offsetInCharacters() getPreviousPosition");
        pos = pos.previous();
        return getPreviousPosition(pos , prevNum);
    }
    Position retVal = pos;
    int cnt = 0;
    while(cnt < prevNum)
    {
        retVal = retVal.previous();
        if(retVal.anchorNode()->offsetInCharacters() )
        {
            cnt++;
            if(cnt != prevNum && retVal.deprecatedEditingOffset() == 0)
            {
                cnt--;
            }
        }
        if(retVal.atStartOfTree())
            break;
    }
    return retVal;
}

Position WebViewCore::getNextPosition(Position& pos ,int nextNum)
{
    Node* positionNode = pos.anchorNode(); 
    RenderObject* renderer = 0;
    if(positionNode)
        renderer= positionNode->renderer();
    else
        LOGD("getPreviousPosition position node is null");

    if (renderer && renderer->isImage()) {
        LOGD("getPreviousPosition the node before cursor is an image so returning");
        return pos;
    }
    else{
        LOGD("getPreviousPosition the node before cursor is not an image");
    }

    if(pos.atEndOfTree())
        return pos;

    if(!pos.anchorNode()->offsetInCharacters()  )
    {
        LOGD("!pos.anchorNode()->offsetInCharacters() getNextPosition");
        pos = pos.next();
        return getNextPosition(pos , nextNum);
    }

    Position retVal = pos;
    int cnt = 0;
    while(cnt < nextNum)
    {
        retVal = retVal.next();
        if(retVal.anchorNode()->offsetInCharacters()  )
        {
            cnt++;
            if(cnt != nextNum && retVal.deprecatedEditingOffset() == 0)
            {
                cnt--;
            }
        }

        if(retVal.atEndOfTree())
            break;
    }
    return retVal;
}

void WebViewCore::simulateDelKeyForCount(int count)
{
    LOGD("simulateDelKeyForCount enter");

    if( m_mainFrame->editor()->hasComposition() ) {
        LOGD("simulateDelKeyForCount hasComposition == true");

        m_mainFrame->editor()->deleteBackwardByDecomposingPreviousCharacter(count);

        return;
    }
	
    PlatformKeyboardEvent down(AKEYCODE_DEL, 0, 0, true, false, false, false);
    PlatformKeyboardEvent up(AKEYCODE_DEL, 0, 0, false, false, false, false);
    for(int cnt = 0 ; cnt < count ; cnt++)
    {
        key(down);
        key(up);
    }
    LOGD("simulateDelKeyForCount exit");
}

WTF::String WebViewCore::getTextAroundCursor(int count , bool isBefore)
{
    LOGD("getTextAroundCursor enter");
    if(count == -1)
    {
        LOGD("getTextAroundCursor -1 ");
        SelectionController* selectionContrler = m_mainFrame->selection();
        if(selectionContrler != NULL && !(selectionContrler->isNone()))
        {
            Position pos = selectionContrler->selection().visibleEnd().deepEquivalent();
            IntPoint pt = IntPoint(0, 0);
            Position startPos = m_mainFrame->visiblePositionForPoint(pt).deepEquivalent();

            VisibleSelection newSelection;
            SelectionController newSelectionControler;
            //if (comparePositions(pos , startPos) <= 0)
                    //  newSelection = VisibleSelection(pos, startPos);
                //else
                        newSelection = VisibleSelection(startPos, pos);

            newSelectionControler.setSelection(newSelection);
            PassRefPtr<Range> rangePtr = newSelectionControler.toNormalizedRange();
            WebCore::Range* range = rangePtr.get();
            if(range != NULL)
            {
                WTF::String plainText = range->text();
                plainText.replace(NonBreakingSpaceCharacter, SpaceCharacter);
                LOGD("getTextAroundCursor -1 ret");
                return plainText;
            }

        }
    }
    else
    {
        SelectionController* frameSelectionContrler = m_mainFrame->selection();
        if(frameSelectionContrler == NULL)
            return "";
        LOGD("getTextAroundCursor setSelection ent");
        SelectionController newSelection;

        Position m_endSelPos;///////////
        Position m_startSelPos;///////////////////////
        bool isBr = false;
        if(isBefore)
        {
            LOGD("getTextAroundCursor setSelection Inside isBefore ");
            m_endSelPos = frameSelectionContrler->selection().visibleStart().deepEquivalent();//////////////
            if(m_endSelPos.anchorNode() && m_endSelPos.anchorNode()->renderer() && m_endSelPos.anchorNode()->renderer()->isBR() && m_endSelPos.deprecatedEditingOffset() == 0)
            {
                LOGD(" &&&&&&&&&&&&&&&&&&&&&& node()->renderer()->isBR()  ");

                //m_endSelPos = Position(m_endSelPos.anchorNode() , Position::PositionIsAfterAnchor);
                isBr = true;
            }
            m_startSelPos = getPreviousPosition(m_endSelPos , count);
        }
        else
        {
            LOGD("getTextAroundCursor setSelection Inside NOT isBefore ");
            m_startSelPos = frameSelectionContrler->selection().visibleEnd().deepEquivalent();
            if(m_startSelPos.anchorNode() && m_startSelPos.anchorNode()->renderer() && m_startSelPos.anchorNode()->renderer()->isBR() /*&& m_startSelPos.deprecatedEditingOffset() == 0*/)
            {
                LOGD(" &&&&&&&&&&&&&&&&&&&&&& node()->renderer()->isBR()  ");

                //m_endSelPos = Position(m_endSelPos.anchorNode() , Position::PositionIsAfterAnchor);
                isBr = true;
            }
            m_endSelPos = getNextPosition(m_startSelPos,count);
        }
        RefPtr<Range> rangePtr;
        if(isBr)
        {
            SelectionController newSelection;
            newSelection.setSelection(frameSelectionContrler->selection());
            LOGD("getTextAroundCursor setSelection exit");
            for(int cnt = 0 ; cnt < count ; cnt++)
            {
                if(isBefore)
                {
                    if(frameSelectionContrler->isRange())
                  newSelection.modify(SelectionController::AlterationMove, DirectionBackward, CharacterGranularity);           
                    newSelection.modify(SelectionController::AlterationExtend, DirectionBackward, CharacterGranularity);
                }           
                else
                {
                    if(frameSelectionContrler->isRange())
                  newSelection.modify(SelectionController::AlterationMove, DirectionForward, CharacterGranularity);
                    newSelection.modify(SelectionController::AlterationExtend, DirectionForward, CharacterGranularity);
                }
            }

            rangePtr = newSelection.toNormalizedRange();
        }
        else
        {
            rangePtr = Range::create(m_mainFrame->document(), m_startSelPos , m_endSelPos);
        }
        LOGD("getTextAroundCursor getting rangePtr from toNormalizedRange   ");

        LOGD("getTextAroundCursor getting range from rangePtr  ");
        if(rangePtr == NULL )
        {
            LOGD("getTextAroundCursor rangePtr  is NULL  ");
            return "";
        }
        WebCore::Range* range = rangePtr.get();
        LOGD("getTextAroundCursor range->text ent");
        if(range != NULL)
        {
            WTF::String plainText = range->text();
            LOGD("HTML getTextAroundCursor range->text exit with following plainText %p:  plainText.length()  =  %d  count = %d " ,  plainText.utf8().data() , plainText.length(), count );//+ plainText);
            LOGD("getTextAroundCursor exit");
            if(plainText.length() > count)
            {
                if(isBefore)
                {
                    plainText = plainText.substring(plainText.length()-count,count);
                }
                else
                {
                    plainText = plainText.substring(0,count);
                }
            }
            LOGD("HTML getTextAroundCursor rreturns following plainText :  " );//+ plainText);
            return plainText ;
        }
    }
    LOGD("HTML getTextAroundCursor exit");
    return "";
}

void WebViewCore::updateIMSelection(int curStr , int curEnd){
    m_imStr = curStr;
    m_imEnd = curEnd;
}

int WebViewCore::checkSelectionAtBoundry()
{
        int result = 0;

    SelectionController* frameSelectionContrler = m_mainFrame->selection();
    if(frameSelectionContrler == NULL)
        return result;


    IntPoint pt = IntPoint(0, 0);
    VisiblePosition startPos = m_mainFrame->visiblePositionForPoint(pt);

    VisibleSelection newSelection;
    newSelection = VisibleSelection(startPos);
    SelectionController newSelectionControler;
    newSelectionControler.setSelection(newSelection);


    if( (newSelectionControler.selection().visibleStart() == frameSelectionContrler->selection().visibleStart()) &&
            (newSelectionControler.selection().visibleEnd() == frameSelectionContrler->selection().visibleEnd()) )
        {
        result |= 0x1;
        }

    newSelectionControler.modify(SelectionController::AlterationMove, DirectionForward, DocumentBoundary);
    if( (newSelectionControler.selection().visibleStart() == frameSelectionContrler->selection().visibleStart()) &&
            (newSelectionControler.selection().visibleEnd() == frameSelectionContrler->selection().visibleEnd()) )
        {
        result |= 0x2;
        }

    return result;
}


void WebViewCore::saveSelectionController()
{
    LOGD("VIN saveSelectionController called here");

    m_VisibleSelection = m_mainFrame->selection()->selection();
}

void WebViewCore::restorePreviousSelectionController()
{
    LOGD("VIN restorePreviousSelectionController called here");

    SelectionController* selectionContrler = m_mainFrame->selection();
    Position endSelPos = selectionContrler->selection().visibleEnd().deepEquivalent();

    if(selectionContrler != NULL)
    {
        LOGD("VIN selectionContrler->setSelection called here");
    selectionContrler->setSelection(VisibleSelection(endSelPos));
       
     WebCore::Node* focus = currentFocus();
     if (!focus) {
      DBG_NAV_LOG("!focus");
      return;
     }
     setFocusControllerActive(true);
        
    }
}


WTF::String WebViewCore::getBodyText()
{
    return m_mainFrame->document()->body()->innerText();
}


int WebViewCore::contentSelectionType()
{
    SelectionController* selectionContrler = m_mainFrame->selection();
    if(selectionContrler != NULL && !(selectionContrler->isNone()))
    {
        if (m_mainFrame->selection()->isRange())
            return 1;
        else
            return 0;
    }
    return -1;
}


bool WebViewCore::getBodyEmpty()
{
    bool retVal = false;
    Node* bodyFirstChild = m_mainFrame->document()->body()->firstChild();
    if(bodyFirstChild)
    {
        if( !bodyFirstChild->firstChild() && !bodyFirstChild->nextSibling() )
        {
            if(bodyFirstChild->isElementNode())
            {
                String tagname = bodyFirstChild->nodeName().lower();
                if(tagname == "br")
                {
                    retVal = true;
                }
            }
        }

    }
    else
    {
        retVal = true;
    }
    return retVal;
    /*bool retVal = false;
    unsigned childCount = m_mainFrame->document()->body()->childNodeCount();
    if(childCount > 1)
        return retVal;
    else if(childCount == 1)
    {
        Node* bodyFirstChild = m_mainFrame->document()->body()->firstChild();
        if(bodyFirstChild->isElementNode())
        {
            String tagname = bodyFirstChild->nodeName().lower();
            if(tagname == "br")
            {
                retVal = true;
            }
        }
    }
    else
        retVal = true;
    return retVal;*/
}

WTF::String WebViewCore::getBodyHTML()
{
    return m_mainFrame->document()->body()->outerHTML();
}

WebCore::IntRect WebViewCore::getCursorRect(bool giveContentRect)
{
    if(giveContentRect)
        return m_mainFrame->selection()->absoluteCaretBounds();
    else
    {
        WebCore::IntRect caretRect;
        caretRect = m_mainFrame->selection()->absoluteCaretBounds();//localCaretRect();
//        LOGD("getCursorRect %d %d %d %d " , caretRect.x() , caretRect.y() , caretRect.right() , caretRect.bottom());
        WebCore::IntPoint locInWindow = m_mainFrame->view()->contentsToWindow(caretRect.location());
        //caretRect = m_mainFrame->view()->convertToContainingView(caretRect);
        caretRect.setLocation(locInWindow);
 //       LOGD("getCursorRect %d %d %d %d " , caretRect.x() , caretRect.y() , caretRect.right() , caretRect.bottom());
        LOGD("getCursorRect exit");
        return caretRect;
    }

}

void WebViewCore::setSelectionNone()
{
    m_mainFrame->selection()->setSelection(VisibleSelection());
}

bool WebViewCore::getSelectionNone()
{
      bool result = false;
      result = m_mainFrame->selection()->isCaret();
      return result;
}


void WebViewCore::setComposingSelectionNone()
{
      LOGD("setComposingSelectionNone enter");
      m_mainFrame->editor()->confirmCompositionWithoutDisturbingSelection();
      m_mainFrame->editor()->setUnderLineUpdateFlag(false);
      //undelineVec.clear();
}

void WebViewCore::deleteSurroundingText(int left , int right)
{
    LOGD("deleteSurroundingText enter");
    int cnt;
    if(left > 0)
    {
        for(cnt = 0 ; cnt < left ; cnt++)
        {
            m_mainFrame->selection()->modify(SelectionController::AlterationExtend, DirectionBackward, CharacterGranularity);
        }
        simulateDelKeyForCount(1);
    }

    if(right > 0)
    {
        for(cnt = 0 ; cnt < right ; cnt++)
        {
            m_mainFrame->selection()->modify(SelectionController::AlterationExtend, DirectionForward, CharacterGranularity);
        }
        simulateDelKeyForCount(1);
    }
    LOGD("deleteSurroundingText exit");

}

void  WebViewCore::getSelectionOffsetImage(int x1 , int y1,int x2 , int y2)
{
    SelectionController* selectionContrler = m_mainFrame->selection();
    if(selectionContrler != NULL && !(selectionContrler->isNone()))
    {
        // Test code - selection of node using range 
        ExceptionCode ec = 0;

        //DOMSelection* selection = m_mainFrame->domWindow()->getSelection();
          
        PassRefPtr<Range> rangeRef = m_mainFrame->document()->createRange();
        rangeRef->selectNode(m_SelectedImageNode, ec);
        EAffinity affinity = selectionContrler->affinity();
        selectionContrler->setSelectedRange(rangeRef.get(), affinity, false);
    }
}


#define DEFAULT_OFFSET 0
void WebViewCore::getSelectionOffset(int& startOffset , int& endOffset)
{
    SelectionController* selectionContrler = m_mainFrame->selection();
    if(selectionContrler != NULL && !(selectionContrler->isNone()))
    {
            Position pos;
                  pos = selectionContrler->selection().visibleStart().deepEquivalent();

            IntPoint pt = IntPoint(0, 0);
            Position startPos = m_mainFrame->visiblePositionForPoint(pt).deepEquivalent();

            VisibleSelection newSelection;
            SelectionController newSelectionControler;
            //if (comparePositions(pos , startPos) <= 0)
                  //    newSelection = VisibleSelection(pos, startPos);
            //else
                        newSelection = VisibleSelection(startPos, pos);

            newSelectionControler.setSelection(newSelection);
            PassRefPtr<Range> rangePtr = newSelectionControler.toNormalizedRange();
            WebCore::Range* range = rangePtr.get();
            if(range != NULL)
            {
                  WTF::String plainText = range->text();
                  //plainText.replace(NonBreakingSpaceCharacter, SpaceCharacter);

                  DEBUG_NAV_UI_LOGD("getSelectionOffset %s len %d" , plainText.utf8().data() , plainText.length());

                  //return plainText.length();
                  startOffset = plainText.length();
            if(selectionContrler->isRange())
            {
                endOffset = startOffset + m_mainFrame->editor()->selectedText().length();
            }
            else
            {
                endOffset = startOffset;
            }
            LOGD("getSelectionOffset str %d end %d" , startOffset ,endOffset);
    }
      }


}


bool WebViewCore::execCommand(WTF::String& commandName ,  WTF::String& value)
{
    //ANDROID_LOG_PRINT(ANDROID_LOG_DEBUG, "HTML_EDIT", " WebViewCore.cpp ::execCommand command: dfgdsf sdgfgs:%s", commandName.utf8().data());
    //LOGD("execCommand entered %s value %s " , commandName.utf8().data() , value.utf8().data());
    bool retval = false;
    if(commandName == "MoveToBeginningOfDocument" || commandName == "MoveToEndOfDocument")
    {
        SelectionController* selectionContrler = m_mainFrame->selection();
        if(selectionContrler->isNone())
        {
            LOGD("execCommand selection none");
            //VisiblePosition vPos = VisiblePosition(m_mainFrame->document()->body() , 0 );
            Position startPos = Position(m_mainFrame->document()->body() , 0 );//vPos.deepEquivalent();

            VisibleSelection newSelection;
            newSelection = VisibleSelection(startPos);
            selectionContrler->setSelection(newSelection);
        }
        else
            LOGD("execCommand selection is Not none");
    }
    retval = m_mainFrame->editor()->command(commandName).execute(value);
    return retval;
}

bool WebViewCore::canUndo()
{
    return m_mainFrame->editor()->canUndo();
}

bool WebViewCore::canRedo()
{
    return m_mainFrame->editor()->canRedo();
}
void WebViewCore::undoRedoStateReset()
{
    return m_mainFrame->editor()->client()->clearUndoRedoOperations();
}


bool WebViewCore::copyAndSaveImage(WTF::String& imageUrl)
{
    WTF::String filePath;
    filePath = WebCore::createLocalResource(m_mainFrame , imageUrl);
    if(filePath.isEmpty())
        return false;

    WebCore::copyImagePathToClipboard(filePath);
    return true;
    //return false;
}

bool WebViewCore::saveCachedImageToFile(WTF::String& imageUrl, WTF::String& filePath)
{
    return WebCore::saveCachedImageToFile( m_mainFrame, imageUrl, filePath );
}

WebHTMLMarkupData* WebViewCore::getFullMarkupData(){
    //ANDROID_LOG_PRINT(ANDROID_LOG_DEBUG, "HTML_EDIT", " WebViewCore::getFullMarkupData() Called");

    setEditable(false);
    WebHTMLMarkupData* webMarkupData = WebCore::createFullMarkup(m_mainFrame->document() , "");
    setEditable(true);

    return webMarkupData;
}
void WebViewCore::setSelectionEditable(int start, int end)
{
    VisibleSelection retSel = setSelectionInVisibleSelection(start, end);
    if(!retSel.isNone())
    {
        m_mainFrame->selection()->setSelection(retSel);
    }
}

void WebViewCore::setComposingRegion(int start, int end)
{
    setComposingSelectionNone();
    VisibleSelection retSel = setSelectionInVisibleSelection(start, end);
    if(!retSel.isNone())
    {
            //m_composingVisibleSelection = retSel;
            VisibleSelection oldSelec = m_mainFrame->selection()->selection();
            m_mainFrame->selection()->setSelection(retSel);
            WTF::String selectedText = getSelectedText();
            int startOffset =-1;
            int endOffset = -1;
            Vector<CompositionUnderline> undVec;
            CompositionUnderline compositionDeco;
            compositionDeco.startOffset = 0;
           compositionDeco.endOffset = selectedText.length();
            compositionDeco.isHighlightColor = false;
            undVec.append(compositionDeco);
            insertContent(selectedText , 1 , true , undVec, startOffset , endOffset);
            if(oldSelec.start().isCandidate())
            {
                  m_mainFrame->selection()->setSelection(oldSelec);
    }
      }
}



VisibleSelection WebViewCore::setSelectionInVisibleSelection(int start, int end)
{

    int tempStart = start - m_imStr;
    int tempEnd = end - m_imEnd;

    LOGD("setSelectionInVisibleSelection Enter - start = %d , end= %d  , m_imStr =%d , m_imEnd=%d " , start, end,m_imStr,m_imEnd);

    SelectionController* selectionContrler = m_mainFrame->selection();
    if(selectionContrler != NULL)
    {
        LOGD("setSelectionInVisibleSelection Enter 1");

        if(selectionContrler->isNone())
            {
                LOGD("setSelectionInVisibleSelection Enter 2");

            Position startPos = Position(m_mainFrame->document()->body() , 0 );//vPos.deepEquivalent();
            VisibleSelection newSelection;
                    newSelection = VisibleSelection(startPos);
                    selectionContrler->setSelection(newSelection);
            }
        SelectionController newSelectionControler;
        newSelectionControler.setSelection(selectionContrler->selection());

        if(newSelectionControler.isRange())
        {
            LOGD("newSelectionControler.isRange() moving backward once");
                  newSelectionControler.modify(SelectionController::AlterationMove, DirectionBackward, CharacterGranularity);
        }

        int uTempStart  = tempStart >= 0 ? tempStart  : -tempStart;
        int cnt;
        LOGD("setSelectionInVisibleSelection tempStart =%d ",tempStart);

        if(tempStart >= 0)
        {

            for(cnt = 0 ; cnt < uTempStart ; cnt++)
            {
                LOGD("setSelectionInVisibleSelection (Move-forward) cnt =%d ",cnt);
                newSelectionControler.modify(SelectionController::AlterationMove, DirectionForward, CharacterGranularity);
            }
        }
        else
        {
            for(cnt = 0 ; cnt < uTempStart ; cnt++)
            {
                LOGD("setSelectionInVisibleSelection (Move-backward) cnt =%d ",cnt);
                newSelectionControler.modify(SelectionController::AlterationMove, DirectionBackward, CharacterGranularity);
            }
        }
        for(cnt = 0 ; cnt < (end - start) ; cnt++)
        {
            LOGD("setSelectionInVisibleSelection (extend forward) cnt =%d ",cnt);
            newSelectionControler.modify(SelectionController::AlterationExtend, DirectionForward, CharacterGranularity);
        }
        //selectionContrler->setSelection(newSelectionControler.selection());
        return newSelectionControler.selection();
    }
    LOGD("setSelectionInVisibleSelection return VisibleSelection ");
    return VisibleSelection();
}


void WebViewCore::setEditable(bool enableEditing)
{
	ExceptionCode ec;
    if(enableEditing)
        m_mainFrame->document()->body()->setContentEditable("true", ec);
    else
        m_mainFrame->document()->body()->setContentEditable("false", ec);
}

// There is the case that only "\n" is selected or only " " is selected. 
// In the case of "\n", the shadow for Selection Area doesn't appear.
// So, it can be misunderstood as cursor is not displayed.
// In the case of " ", it is not the same concept as EditText.
// EditText doesn't support to select " " when selecting word in ICS.
// This function will check the status and display the cursor instead of Selection Area.
// And a selected " " will be ignored by it.
// jaesung.yi@samsung.com
void WebViewCore::checkSelectedClosestWord()
{
    int selectedType = -2;
    WTF::String selectedText = getSelectedText();

    selectedType = contentSelectionType();
	
    if( 1 == selectedType /*range*/ && NULL != selectedText 
        && ( selectedText == WTF::String("\n") || ( selectedText.length() == 1 && selectedText.contains(NonBreakingSpaceCharacter) ) ) )
    {
        SelectionController* selectionContrler = m_mainFrame->selection();

        if(selectionContrler != NULL)
        {
            Position startSelPos = selectionContrler->selection().visibleStart().deepEquivalent();

            selectionContrler->setSelection(VisibleSelection(startSelPos));

            selectionContrler->setCaretVisible(true);
        }
    }
}

bool WebViewCore::isEditableSupport()
{
    JNIEnv* env = JSC::Bindings::getJNIEnv();

    bool result = env->CallBooleanMethod(m_javaGlue->object(env).get(),
        m_javaGlue->m_isEditableSupport);

    checkException(env);
    return result;
}


void WebViewCore::moveSingleCursorHandler(int x,int y){

    LOGD("moveSingleCursorHandler Enter - x = %d , y= %d" , x, y);

    SelectionController* frameSelectionContrler = m_mainFrame->selection();
    if(frameSelectionContrler == NULL)
        return ;
//    y=y-20;
    IntPoint pt = IntPoint(x, y);
    VisiblePosition startPos = m_mainFrame->visiblePositionForPoint(pt);
    VisibleSelection newSelection;
    newSelection = VisibleSelection(startPos);
    frameSelectionContrler->setSelection(newSelection/*(newSelectionControler.selection()*/);

    imageVisiblePosition = startPos;

    if(newSelection.start().isCandidate()) {
        imageCanMove= true;
    }
    else {
        imageCanMove= false;
    }

    LOGD("moveSingleCursorHandler Leave ");

    return;
}



//+ by jaesung.yi 110802
int WebViewCore::getStateInRichlyEditableText()
{
    LOGD("getStateInRichlyEditableText()"); 

    int totalResult = 0x0;
    int result = 0;

    result = m_mainFrame->editor()->command("Bold").state();

    if( TrueTriState == result ) {
        totalResult |= 0x0001;
    } else if( MixedTriState == result ) {
        totalResult |= 0x0002;
    }

    result = m_mainFrame->editor()->command("Italic").state();;

    if( TrueTriState == result ) {
        totalResult |= 0x0004;
    } else if( MixedTriState == result ) {
        totalResult |= 0x0008;
    }

    result = m_mainFrame->editor()->command("Underline").state();

    if( TrueTriState == result ) {
        totalResult |= 0x0010;
    } else if( MixedTriState == result ) {
        totalResult |= 0x0020;
    }

    result = m_mainFrame->editor()->command("InsertOrderedList").state();

    if( TrueTriState == result ) {
        totalResult |= 0x0040;
    } else if( MixedTriState == result ) {
        totalResult |= 0x0080;
    }

    result = m_mainFrame->editor()->command("InsertUnorderedList").state();

    if( TrueTriState == result ) {
        totalResult |= 0x0100;
    } else if( MixedTriState == result ) {
        totalResult |= 0x0200;
    }

    return totalResult;
}
//- by jaesung.yi 110802

bool WebViewCore::checkEndofWordAtPosition(int x, int y)
{
    //SAMSUNG CHANGE, yeonju.ann : Remove security logs
    //LOGD("isTouchedPositionAtEndOfWord Enter - x = %d , y= %d" , x, y);

    SelectionController* frameSelectionContrler = m_mainFrame->selection();
    if(frameSelectionContrler == NULL)
        return false;

    IntPoint pt = IntPoint(x, y);
    VisiblePosition startPos = m_mainFrame->visiblePositionForPoint(pt);
    VisibleSelection newSelection;
    newSelection = VisibleSelection(startPos);

    SelectionController newSelectionControler;
    newSelectionControler.setSelection(newSelection);

    newSelectionControler.modify(SelectionController::AlterationExtend, DirectionForward, CharacterGranularity);

    PassRefPtr<Range> rangePtr = newSelectionControler.toNormalizedRange();
    WebCore::Range* range = rangePtr.get();
    if(range != NULL)
    {
        WTF::String plainText = range->text();
        if( 1 < plainText.length() ) {
            LOGD("isTouchedPositionAtEndOfWord The selected character to check the end of word is more than one character. ");
            return false;
        } else {
            if( 0 == plainText.length() ) {
                LOGD("isTouchedPositionAtEndOfWord A touched position is the end of a document.");
                newSelectionControler.modify(SelectionController::AlterationMove, DirectionForward, DocumentBoundary);
                frameSelectionContrler->setSelection(newSelectionControler.selection());
                return true;
            }
            else if( 1 == plainText.length() && ( plainText.contains(NonBreakingSpaceCharacter) || plainText == WTF::String("\n") ) ) {
                LOGD("isTouchedPositionAtEndOfWord A touched position is the end of a word.");
                frameSelectionContrler->setSelection(newSelection);
                return true;
            }
        }
    }

    return false;
}
//SISO_HTMLCOMPOSER end

//SAMSUNG_THAI_EDITOR_FIX ++
void WebViewCore::setSelectionWithoutValidation(int start, int end)
{
    WebCore::Node* focus = currentFocus();
    if (!focus)
        return;
    WebCore::RenderObject* renderer = focus->renderer();
    if (!renderer || (!renderer->isTextField() && !renderer->isTextArea()))
        return;
    if (start > end) {
        int temp = start;
        start = end;
        end = temp;
    }
    // Tell our EditorClient that this change was generated from the UI, so it
    // does not need to echo it to the UI.
    EditorClientAndroid* client = static_cast<EditorClientAndroid*>(
            m_mainFrame->editor()->client());
    client->setUiGeneratedSelectionChange(true);
    setSelectionRange(focus, start, end);
    if (start != end) {
        // Fire a select event. No event is sent when the selection reduces to
        // an insertion point
        RenderTextControl* control = toRenderTextControl(renderer);
        control->selectionChanged(true);
    }
    client->setUiGeneratedSelectionChange(false);
    WebCore::Frame* focusedFrame = focus->document()->frame();
    VisibleSelection selection = focusedFrame->selection()->selection();
    if (start != end 
        && selection.end().deprecatedEditingOffset() == selection.start().deprecatedEditingOffset()
        && selection.end().anchorNode() == selection.start().anchorNode()) {
        int e = selection.end().deprecatedEditingOffset() ;
        int s = e - (end - start) ;
        Position base(selection.end().anchorNode(), s) ;
        Position extent(selection.end().anchorNode(), e) ;
        if (!base.isNull() && !extent.isNull() && base != extent) {
            selection.setWithoutValidation(base, extent);
            focusedFrame->selection()->setSelection(selection);        
        }
    }
    bool isPasswordField = false;
    if (focus->isElementNode()) {
        WebCore::Element* element = static_cast<WebCore::Element*>(focus);
        if (WebCore::InputElement* inputElement = element->toInputElement())
    //SAMSUNG WML CHANGES >>
    {
    #if ENABLE(WML)
        if(focus->isWMLElement())
                    isPasswordField = static_cast<WebCore::WMLInputElement*>(inputElement)->isPasswordField();
        else
            isPasswordField = static_cast<WebCore::HTMLInputElement*>(inputElement)->isPasswordField();
    #endif
    }
    //SAMSUNG WML CHANGES <<
    }
    // For password fields, this is done in the UI side via
    // bringPointIntoView, since the UI does the drawing.
    if (renderer->isTextArea() || !isPasswordField)
        revealSelection();
}
//SAMSUNG_THAI_EDITOR_FIX --

void WebViewCore::setSelection(int start, int end)
{
    WebCore::Node* focus = currentFocus();
    if (!focus)
        return;
    WebCore::RenderObject* renderer = focus->renderer();
    if (!renderer || (!renderer->isTextField() && !renderer->isTextArea()))
        return;
    if (start > end) {
        int temp = start;
        start = end;
        end = temp;
    }
    // Tell our EditorClient that this change was generated from the UI, so it
    // does not need to echo it to the UI.
    EditorClientAndroid* client = static_cast<EditorClientAndroid*>(
            m_mainFrame->editor()->client());
    client->setUiGeneratedSelectionChange(true);
    setSelectionRange(focus, start, end);
    if (start != end) {
        // Fire a select event. No event is sent when the selection reduces to
        // an insertion point
        RenderTextControl* control = toRenderTextControl(renderer);
        control->selectionChanged(true);
    }
    client->setUiGeneratedSelectionChange(false);
    WebCore::Frame* focusedFrame = focus->document()->frame();
    bool isPasswordField = false;
    if (focus->isElementNode()) {
        WebCore::Element* element = static_cast<WebCore::Element*>(focus);
        if (WebCore::InputElement* inputElement = element->toInputElement())
    //SAMSUNG WML CHANGES >>
    {
    #if ENABLE(WML)
        if(focus->isWMLElement())
                    isPasswordField = static_cast<WebCore::WMLInputElement*>(inputElement)->isPasswordField();
        else
            isPasswordField = static_cast<WebCore::HTMLInputElement*>(inputElement)->isPasswordField();
    #endif
    }
    //SAMSUNG WML CHANGES <<
    }
    // For password fields, this is done in the UI side via
    // bringPointIntoView, since the UI does the drawing.
    if (renderer->isTextArea() || !isPasswordField)
        revealSelection();
}

String WebViewCore::modifySelection(const int direction, const int axis)
{
    DOMSelection* selection = m_mainFrame->domWindow()->getSelection();
    ASSERT(selection);
    // We've seen crashes where selection is null, but we don't know why
    // See http://b/5244036
    if (!selection)
        return String();
    if (selection->rangeCount() > 1)
        selection->removeAllRanges();
    switch (axis) {
        case AXIS_CHARACTER:
        case AXIS_WORD:
        case AXIS_SENTENCE:
            return modifySelectionTextNavigationAxis(selection, direction, axis);
        case AXIS_HEADING:
        case AXIS_SIBLING:
        case AXIS_PARENT_FIRST_CHILD:
        case AXIS_DOCUMENT:
            return modifySelectionDomNavigationAxis(selection, direction, axis);
        default:
            LOGE("Invalid navigation axis: %d", axis);
            return String();
    }
}

void WebViewCore::scrollNodeIntoView(Frame* frame, Node* node)
{
    if (!frame || !node)
        return;

    Element* elementNode = 0;

    // If not an Element, find a visible predecessor
    // Element to scroll into view.
    if (!node->isElementNode()) {
        HTMLElement* body = frame->document()->body();
        do {
            if (node == body)
                return;
            node = node->parentNode();
        } while (node && !node->isElementNode() && !isVisible(node));
    }

    // Couldn't find a visible predecessor.
    if (!node)
        return;

    elementNode = static_cast<Element*>(node);
    elementNode->scrollIntoViewIfNeeded(true);
}

String WebViewCore::modifySelectionTextNavigationAxis(DOMSelection* selection, int direction, int axis)
{
    Node* body = m_mainFrame->document()->body();

    ExceptionCode ec = 0;
    String markup;

    // initialize the selection if necessary
    if (selection->rangeCount() == 0) {
        if (m_currentNodeDomNavigationAxis
                && CacheBuilder::validNode(m_mainFrame,
                m_mainFrame, m_currentNodeDomNavigationAxis)) {
            PassRefPtr<Range> rangeRef =
                selection->frame()->document()->createRange();
            rangeRef->selectNode(m_currentNodeDomNavigationAxis, ec);
            m_currentNodeDomNavigationAxis = 0;
            if (ec)
                return String();
            selection->addRange(rangeRef.get());
        } else if (currentFocus()) {
            selection->setPosition(currentFocus(), 0, ec);
        } else if (m_cursorNode
                && CacheBuilder::validNode(m_mainFrame,
                m_mainFrame, m_cursorNode)) {
            PassRefPtr<Range> rangeRef =
                selection->frame()->document()->createRange();
            rangeRef->selectNode(reinterpret_cast<Node*>(m_cursorNode), ec);
            if (ec)
                return String();
            selection->addRange(rangeRef.get());
        } else {
            selection->setPosition(body, 0, ec);
        }
        if (ec)
            return String();
    }

    // collapse the selection
    if (direction == DIRECTION_FORWARD)
        selection->collapseToEnd(ec);
    else
        selection->collapseToStart(ec);
    if (ec)
        return String();

    // Make sure the anchor node is a text node since we are generating
    // the markup of the selection which includes the anchor, the focus,
    // and any crossed nodes. Forcing the condition that the selection
    // starts and ends on text nodes guarantees symmetric selection markup.
    // Also this way the text content, rather its container, is highlighted.
    Node* anchorNode = selection->anchorNode();
    if (anchorNode->isElementNode()) {
        // Collapsed selection while moving forward points to the
        // next unvisited node and while moving backward to the
        // last visited node.
        if (direction == DIRECTION_FORWARD)
            advanceAnchorNode(selection, direction, markup, false, ec);
        else
            advanceAnchorNode(selection, direction, markup, true, ec);
        if (ec)
            return String();
        if (!markup.isEmpty())
            return markup;
    }

    // If the selection is at the end of a non white space text move
    // it to the next visible text node with non white space content.
    // This is a workaround for the selection getting stuck.
    anchorNode = selection->anchorNode();
    if (anchorNode->isTextNode()) {
        if (direction == DIRECTION_FORWARD) {
            String suffix = anchorNode->textContent().substring(
                    selection->anchorOffset(), caretMaxOffset(anchorNode));
            // If at the end of non white space text we advance the
            // anchor node to either an input element or non empty text.
            if (suffix.stripWhiteSpace().isEmpty()) {
                advanceAnchorNode(selection, direction, markup, true, ec);
            }
        } else {
            String prefix = anchorNode->textContent().substring(0,
                    selection->anchorOffset());
            // If at the end of non white space text we advance the
            // anchor node to either an input element or non empty text.
            if (prefix.stripWhiteSpace().isEmpty()) {
                advanceAnchorNode(selection, direction, markup, true, ec);
            }
        }
        if (ec)
            return String();
        if (!markup.isEmpty())
            return markup;
    }

    // extend the selection
    String directionStr;
    if (direction == DIRECTION_FORWARD)
        directionStr = "forward";
    else
        directionStr = "backward";

    String axisStr;
    if (axis == AXIS_CHARACTER)
        axisStr = "character";
    else if (axis == AXIS_WORD)
        axisStr = "word";
    else
        axisStr = "sentence";

    selection->modify("extend", directionStr, axisStr);

    // Make sure the focus node is a text node in order to have the
    // selection generate symmetric markup because the latter
    // includes all nodes crossed by the selection.  Also this way
    // the text content, rather its container, is highlighted.
    Node* focusNode = selection->focusNode();
    if (focusNode->isElementNode()) {
        focusNode = getImplicitBoundaryNode(selection->focusNode(),
                selection->focusOffset(), direction);
        if (!focusNode)
            return String();
        if (direction == DIRECTION_FORWARD) {
            focusNode = focusNode->traversePreviousSiblingPostOrder(body);
            if (focusNode && !isContentTextNode(focusNode)) {
                Node* textNode = traverseNextContentTextNode(focusNode,
                        anchorNode, DIRECTION_BACKWARD);
                if (textNode)
                    anchorNode = textNode;
            }
            if (focusNode && isContentTextNode(focusNode)) {
                selection->extend(focusNode, caretMaxOffset(focusNode), ec);
                if (ec)
                    return String();
            }
        } else {
            focusNode = focusNode->traverseNextSibling();
            if (focusNode && !isContentTextNode(focusNode)) {
                Node* textNode = traverseNextContentTextNode(focusNode,
                        anchorNode, DIRECTION_FORWARD);
                if (textNode)
                    anchorNode = textNode;
            }
            if (anchorNode && isContentTextNode(anchorNode)) {
                selection->extend(focusNode, 0, ec);
                if (ec)
                    return String();
            }
        }
    }

    // Enforce that the selection does not cross anchor boundaries. This is
    // a workaround for the asymmetric behavior of WebKit while crossing
    // anchors.
    anchorNode = getImplicitBoundaryNode(selection->anchorNode(),
            selection->anchorOffset(), direction);
    focusNode = getImplicitBoundaryNode(selection->focusNode(),
            selection->focusOffset(), direction);
    if (anchorNode && focusNode && anchorNode != focusNode) {
        Node* inputControl = getIntermediaryInputElement(anchorNode, focusNode,
                direction);
        if (inputControl) {
            if (direction == DIRECTION_FORWARD) {
                if (isDescendantOf(inputControl, anchorNode)) {
                    focusNode = inputControl;
                } else {
                    focusNode = inputControl->traversePreviousSiblingPostOrder(
                            body);
                    if (!focusNode)
                        focusNode = inputControl;
                }
                // We prefer a text node contained in the input element.
                if (!isContentTextNode(focusNode)) {
                    Node* textNode = traverseNextContentTextNode(focusNode,
                        anchorNode, DIRECTION_BACKWARD);
                    if (textNode)
                        focusNode = textNode;
                }
                // If we found text in the input select it.
                // Otherwise, select the input element itself.
                if (isContentTextNode(focusNode)) {
                    selection->extend(focusNode, caretMaxOffset(focusNode), ec);
                } else if (anchorNode != focusNode) {
                    // Note that the focusNode always has parent and that
                    // the offset can be one more that the index of the last
                    // element - this is how WebKit selects such elements.
                    selection->extend(focusNode->parentNode(),
                            focusNode->nodeIndex() + 1, ec);
                }
                if (ec)
                    return String();
            } else {
                if (isDescendantOf(inputControl, anchorNode)) {
                    focusNode = inputControl;
                } else {
                    focusNode = inputControl->traverseNextSibling();
                    if (!focusNode)
                        focusNode = inputControl;
                }
                // We prefer a text node contained in the input element.
                if (!isContentTextNode(focusNode)) {
                    Node* textNode = traverseNextContentTextNode(focusNode,
                            anchorNode, DIRECTION_FORWARD);
                    if (textNode)
                        focusNode = textNode;
                }
                // If we found text in the input select it.
                // Otherwise, select the input element itself.
                if (isContentTextNode(focusNode)) {
                    selection->extend(focusNode, caretMinOffset(focusNode), ec);
                } else if (anchorNode != focusNode) {
                    // Note that the focusNode always has parent and that
                    // the offset can be one more that the index of the last
                    // element - this is how WebKit selects such elements.
                    selection->extend(focusNode->parentNode(),
                            focusNode->nodeIndex() + 1, ec);
                }
                if (ec)
                   return String();
            }
        }
    }

    // make sure the selection is visible
    if (direction == DIRECTION_FORWARD)
        scrollNodeIntoView(m_mainFrame, selection->focusNode());
    else
        scrollNodeIntoView(m_mainFrame, selection->anchorNode());

    // format markup for the visible content
    PassRefPtr<Range> range = selection->getRangeAt(0, ec);
    if (ec)
        return String();
    IntRect bounds = range->boundingBox();
    selectAt(bounds.center().x(), bounds.center().y());
    markup = formatMarkup(selection);
    LOGV("Selection markup: %s", markup.utf8().data());

    return markup;
}

Node* WebViewCore::getImplicitBoundaryNode(Node* node, unsigned offset, int direction)
{
    if (node->offsetInCharacters())
        return node;
    if (!node->hasChildNodes())
        return node;
    if (offset < node->childNodeCount())
        return node->childNode(offset);
    else
        if (direction == DIRECTION_FORWARD)
            return node->traverseNextSibling();
        else
            return node->traversePreviousNodePostOrder(
                    node->document()->body());
}

Node* WebViewCore::getNextAnchorNode(Node* anchorNode, bool ignoreFirstNode, int direction)
{
    Node* body = 0;
    Node* currentNode = 0;
    if (direction == DIRECTION_FORWARD) {
        if (ignoreFirstNode)
            currentNode = anchorNode->traverseNextNode(body);
        else
            currentNode = anchorNode;
    } else {
        body = anchorNode->document()->body();
        if (ignoreFirstNode)
            currentNode = anchorNode->traversePreviousSiblingPostOrder(body);
        else
            currentNode = anchorNode;
    }
    while (currentNode) {
        if (isContentTextNode(currentNode)
                || isContentInputElement(currentNode))
            return currentNode;
        if (direction == DIRECTION_FORWARD)
            currentNode = currentNode->traverseNextNode();
        else
            currentNode = currentNode->traversePreviousNodePostOrder(body);
    }
    return 0;
}

void WebViewCore::advanceAnchorNode(DOMSelection* selection, int direction,
        String& markup, bool ignoreFirstNode, ExceptionCode& ec)
{
    Node* anchorNode = getImplicitBoundaryNode(selection->anchorNode(),
            selection->anchorOffset(), direction);
    if (!anchorNode) {
        ec = NOT_FOUND_ERR;
        return;
    }
    // If the anchor offset is invalid i.e. the anchor node has no
    // child with that index getImplicitAnchorNode returns the next
    // logical node in the current direction. In such a case our
    // position in the DOM tree was has already been advanced,
    // therefore we there is no need to do that again.
    if (selection->anchorNode()->isElementNode()) {
        unsigned anchorOffset = selection->anchorOffset();
        unsigned childNodeCount = selection->anchorNode()->childNodeCount();
        if (anchorOffset >= childNodeCount)
            ignoreFirstNode = false;
    }
    // Find the next anchor node given our position in the DOM and
    // whether we want the current node to be considered as well.
    Node* nextAnchorNode = getNextAnchorNode(anchorNode, ignoreFirstNode,
            direction);
    if (!nextAnchorNode) {
        ec = NOT_FOUND_ERR;
        return;
    }
    if (nextAnchorNode->isElementNode()) {
        // If this is an input element tell the WebView thread
        // to set the cursor to that control.
        if (isContentInputElement(nextAnchorNode)) {
            IntRect bounds = nextAnchorNode->getRect();
            selectAt(bounds.center().x(), bounds.center().y());
        }
        Node* textNode = 0;
        // Treat the text content of links as any other text but
        // for the rest input elements select the control itself.
        if (nextAnchorNode->hasTagName(WebCore::HTMLNames::aTag))
            textNode = traverseNextContentTextNode(nextAnchorNode,
                    nextAnchorNode, direction);
        // We prefer to select the text content of the link if such,
        // otherwise just select the element itself.
        if (textNode) {
            nextAnchorNode = textNode;
        } else {
            if (direction == DIRECTION_FORWARD) {
                selection->setBaseAndExtent(nextAnchorNode,
                        caretMinOffset(nextAnchorNode), nextAnchorNode,
                        caretMaxOffset(nextAnchorNode), ec);
            } else {
                selection->setBaseAndExtent(nextAnchorNode,
                        caretMaxOffset(nextAnchorNode), nextAnchorNode,
                        caretMinOffset(nextAnchorNode), ec);
            }
            if (!ec)
                markup = formatMarkup(selection);
            // make sure the selection is visible
            scrollNodeIntoView(selection->frame(), nextAnchorNode);
            return;
        }
    }
    if (direction == DIRECTION_FORWARD)
        selection->setPosition(nextAnchorNode,
                caretMinOffset(nextAnchorNode), ec);
    else
        selection->setPosition(nextAnchorNode,
                caretMaxOffset(nextAnchorNode), ec);
}

bool WebViewCore::isContentInputElement(Node* node)
{
  return (isVisible(node)
          && (node->hasTagName(WebCore::HTMLNames::selectTag)
          || node->hasTagName(WebCore::HTMLNames::aTag)
          || node->hasTagName(WebCore::HTMLNames::inputTag)
          || node->hasTagName(WebCore::HTMLNames::buttonTag)));
}

bool WebViewCore::isContentTextNode(Node* node)
{
   if (!node || !node->isTextNode())
       return false;
   Text* textNode = static_cast<Text*>(node);
   return (isVisible(textNode) && textNode->length() > 0
       && !textNode->containsOnlyWhitespace());
}

Text* WebViewCore::traverseNextContentTextNode(Node* fromNode, Node* toNode, int direction)
{
    Node* currentNode = fromNode;
    do {
        if (direction == DIRECTION_FORWARD)
            currentNode = currentNode->traverseNextNode(toNode);
        else
            currentNode = currentNode->traversePreviousNodePostOrder(toNode);
    } while (currentNode && !isContentTextNode(currentNode));
    return static_cast<Text*>(currentNode);
}

Node* WebViewCore::getIntermediaryInputElement(Node* fromNode, Node* toNode, int direction)
{
    if (fromNode == toNode)
        return 0;
    if (direction == DIRECTION_FORWARD) {
        Node* currentNode = fromNode;
        while (currentNode && currentNode != toNode) {
            if (isContentInputElement(currentNode))
                return currentNode;
            currentNode = currentNode->traverseNextNodePostOrder();
        }
        currentNode = fromNode;
        while (currentNode && currentNode != toNode) {
            if (isContentInputElement(currentNode))
                return currentNode;
            currentNode = currentNode->traverseNextNode();
        }
    } else {
        Node* currentNode = fromNode->traversePreviousNode();
        while (currentNode && currentNode != toNode) {
            if (isContentInputElement(currentNode))
                return currentNode;
            currentNode = currentNode->traversePreviousNode();
        }
        currentNode = fromNode->traversePreviousNodePostOrder();
        while (currentNode && currentNode != toNode) {
            if (isContentInputElement(currentNode))
                return currentNode;
            currentNode = currentNode->traversePreviousNodePostOrder();
        }
    }
    return 0;
}

bool WebViewCore::isDescendantOf(Node* parent, Node* node)
{
    Node* currentNode = node;
    while (currentNode) {
        if (currentNode == parent) {
            return true;
        }
        currentNode = currentNode->parentNode();
    }
    return false;
}

String WebViewCore::modifySelectionDomNavigationAxis(DOMSelection* selection, int direction, int axis)
{
    HTMLElement* body = m_mainFrame->document()->body();
    if (!m_currentNodeDomNavigationAxis && selection->focusNode()) {
        m_currentNodeDomNavigationAxis = selection->focusNode();
        selection->empty();
        if (m_currentNodeDomNavigationAxis->isTextNode())
            m_currentNodeDomNavigationAxis =
                m_currentNodeDomNavigationAxis->parentNode();
    }
    if (!m_currentNodeDomNavigationAxis)
        m_currentNodeDomNavigationAxis = currentFocus();
    if (!m_currentNodeDomNavigationAxis
            || !CacheBuilder::validNode(m_mainFrame, m_mainFrame,
                                        m_currentNodeDomNavigationAxis))
        m_currentNodeDomNavigationAxis = body;
    Node* currentNode = m_currentNodeDomNavigationAxis;
    if (axis == AXIS_HEADING) {
        if (currentNode == body && direction == DIRECTION_BACKWARD)
            currentNode = currentNode->lastDescendant();
        do {
            if (direction == DIRECTION_FORWARD)
                currentNode = currentNode->traverseNextNode(body);
            else
                currentNode = currentNode->traversePreviousNode(body);
        } while (currentNode && (currentNode->isTextNode()
            || !isVisible(currentNode) || !isHeading(currentNode)));
    } else if (axis == AXIS_PARENT_FIRST_CHILD) {
        if (direction == DIRECTION_FORWARD) {
            currentNode = currentNode->firstChild();
            while (currentNode && (currentNode->isTextNode()
                    || !isVisible(currentNode)))
                currentNode = currentNode->nextSibling();
        } else {
            do {
                if (currentNode == body)
                    return String();
                currentNode = currentNode->parentNode();
            } while (currentNode && (currentNode->isTextNode()
                    || !isVisible(currentNode)));
        }
    } else if (axis == AXIS_SIBLING) {
        do {
            if (direction == DIRECTION_FORWARD)
                currentNode = currentNode->nextSibling();
            else {
                if (currentNode == body)
                    return String();
                currentNode = currentNode->previousSibling();
            }
        } while (currentNode && (currentNode->isTextNode()
                || !isVisible(currentNode)));
    } else if (axis == AXIS_DOCUMENT) {
        currentNode = body;
        if (direction == DIRECTION_FORWARD)
            currentNode = currentNode->lastDescendant();
    } else {
        LOGE("Invalid axis: %d", axis);
        return String();
    }
    if (currentNode) {
        m_currentNodeDomNavigationAxis = currentNode;
        scrollNodeIntoView(m_mainFrame, currentNode);
        String selectionString = createMarkup(currentNode);
        LOGV("Selection markup: %s", selectionString.utf8().data());
        return selectionString;
    }
    return String();
}

bool WebViewCore::isHeading(Node* node)
{
    if (node->hasTagName(WebCore::HTMLNames::h1Tag)
            || node->hasTagName(WebCore::HTMLNames::h2Tag)
            || node->hasTagName(WebCore::HTMLNames::h3Tag)
            || node->hasTagName(WebCore::HTMLNames::h4Tag)
            || node->hasTagName(WebCore::HTMLNames::h5Tag)
            || node->hasTagName(WebCore::HTMLNames::h6Tag)) {
        return true;
    }

    if (node->isElementNode()) {
        Element* element = static_cast<Element*>(node);
        String roleAttribute =
            element->getAttribute(WebCore::HTMLNames::roleAttr).string();
        if (equalIgnoringCase(roleAttribute, "heading"))
            return true;
    }

    return false;
}

bool WebViewCore::isVisible(Node* node)
{
    // start off an element
    Element* element = 0;
    if (node->isElementNode())
        element = static_cast<Element*>(node);
    else
        element = node->parentElement();
    // check renderer
    if (!element->renderer()) {
        return false;
    }
    // check size
    if (element->offsetHeight() == 0 || element->offsetWidth() == 0) {
        return false;
    }
    // check style
    Node* body = m_mainFrame->document()->body();
    Node* currentNode = element;
    while (currentNode && currentNode != body) {
        RenderStyle* style = currentNode->computedStyle();
        if (style &&
                (style->display() == NONE || style->visibility() == HIDDEN)) {
            return false;
        }
        currentNode = currentNode->parentNode();
    }
    return true;
}

String WebViewCore::formatMarkup(DOMSelection* selection)
{
    ExceptionCode ec = 0;
    String markup = String();
    PassRefPtr<Range> wholeRange = selection->getRangeAt(0, ec);
    if (ec)
        return String();
    if (!wholeRange->startContainer() || !wholeRange->startContainer())
        return String();
    // Since formatted markup contains invisible nodes it
    // is created from the concatenation of the visible fragments.
    Node* firstNode = wholeRange->firstNode();
    Node* pastLastNode = wholeRange->pastLastNode();
    Node* currentNode = firstNode;
    PassRefPtr<Range> currentRange;

    while (currentNode != pastLastNode) {
        Node* nextNode = currentNode->traverseNextNode();
        if (!isVisible(currentNode)) {
            if (currentRange) {
                markup = markup + currentRange->toHTML().utf8().data();
                currentRange = 0;
            }
        } else {
            if (!currentRange) {
                currentRange = selection->frame()->document()->createRange();
                if (ec)
                    break;
                if (currentNode == firstNode) {
                    currentRange->setStart(wholeRange->startContainer(),
                        wholeRange->startOffset(), ec);
                    if (ec)
                        break;
                } else {
                    currentRange->setStart(currentNode->parentNode(),
                        currentNode->nodeIndex(), ec);
                    if (ec)
                       break;
                }
            }
            if (nextNode == pastLastNode) {
                currentRange->setEnd(wholeRange->endContainer(),
                    wholeRange->endOffset(), ec);
                if (ec)
                    break;
                markup = markup + currentRange->toHTML().utf8().data();
            } else {
                if (currentNode->offsetInCharacters())
                    currentRange->setEnd(currentNode,
                        currentNode->maxCharacterOffset(), ec);
                else
                    currentRange->setEnd(currentNode->parentNode(),
                            currentNode->nodeIndex() + 1, ec);
                if (ec)
                    break;
            }
        }
        currentNode = nextNode;
    }
    return markup.stripWhiteSpace();
}

void WebViewCore::selectAt(int x, int y)
{
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return;
    env->CallVoidMethod(javaObject.get(), m_javaGlue->m_selectAt, x, y);
    checkException(env);
}

void WebViewCore::deleteSelection(int start, int end, int textGeneration)
{
    setSelection(start, end);
    if (start == end)
        return;
    WebCore::Node* focus = currentFocus();
    if (!focus)
        return;
    // Prevent our editor client from passing a message to change the
    // selection.
    EditorClientAndroid* client = static_cast<EditorClientAndroid*>(
            m_mainFrame->editor()->client());
    client->setUiGeneratedSelectionChange(true);
    PlatformKeyboardEvent down(AKEYCODE_DEL, 0, 0, true, false, false, false);
    PlatformKeyboardEvent up(AKEYCODE_DEL, 0, 0, false, false, false, false);
    key(down);
    key(up);
    client->setUiGeneratedSelectionChange(false);
    m_textGeneration = textGeneration;
    m_shouldPaintCaret = true;
}

void WebViewCore::replaceTextfieldText(int oldStart,
        int oldEnd, const WTF::String& replace, int start, int end,
        int textGeneration)
{
    WebCore::Node* focus = currentFocus();
    if (!focus)
        return;
    //SAMSUNG_THAI_EDITOR_FIX ++
    //setSelection(oldStart, oldEnd);
    setSelectionWithoutValidation(oldStart, oldEnd);
    //SAMSUNG_THAI_EDITOR_FIX --
    // Prevent our editor client from passing a message to change the
    // selection.
    EditorClientAndroid* client = static_cast<EditorClientAndroid*>(
            m_mainFrame->editor()->client());
    client->setUiGeneratedSelectionChange(true);
    WebCore::TypingCommand::insertText(focus->document(), replace,
        false);
    client->setUiGeneratedSelectionChange(false);
    // setSelection calls revealSelection, so there is no need to do it here.
    setSelection(start, end);
    m_textGeneration = textGeneration;
    m_shouldPaintCaret = true;
}

void WebViewCore::passToJs(int generation, const WTF::String& current,
    const PlatformKeyboardEvent& event)
{
    WebCore::Node* focus = currentFocus();
    if (!focus) {
        DBG_NAV_LOG("!focus");
        clearTextEntry();
        return;
    }
    WebCore::RenderObject* renderer = focus->renderer();
    if (!renderer || (!renderer->isTextField() && !renderer->isTextArea())) {
        DBG_NAV_LOGD("renderer==%p || not text", renderer);
        clearTextEntry();
        return;
    }
    // Block text field updates during a key press.
    m_blockTextfieldUpdates = true;
    // Also prevent our editor client from passing a message to change the
    // selection.
    EditorClientAndroid* client = static_cast<EditorClientAndroid*>(
            m_mainFrame->editor()->client());
    client->setUiGeneratedSelectionChange(true);
    key(event);
    client->setUiGeneratedSelectionChange(false);
    m_blockTextfieldUpdates = false;
    m_textGeneration = generation;
    WebCore::RenderTextControl* renderText =
        static_cast<WebCore::RenderTextControl*>(renderer);
    WTF::String test = renderText->text();
    if (test != current) {
        // If the text changed during the key event, update the UI text field.
        updateTextfield(focus, false, test);
    } else {
        DBG_NAV_LOG("test == current");
    }
    // Now that the selection has settled down, send it.
    updateTextSelection();
    m_shouldPaintCaret = true;
}

//SAMSUNG CHANGES: MPSG100004155 - input background color is not displayed >>
int WebViewCore::getInputTextBackgroundColor(WebCore::Node* inputNodePtr)
{
    if (inputNodePtr) {
        WebCore::RenderObject* renderer = inputNodePtr->renderer();
        if (renderer && renderer->style()) {
            Color color = renderer->style()->backgroundStyleColor();
            if (color.isValid()) {
                return SkColorSetARGB(color.alpha(), color.red(),color.green(), color.blue());				
            }
        }
    }
    return SK_ColorWHITE;	
}
//SAMSUNG CHANGES: MPSG100004155 - input background color is not displayed <<
/*
//SISO change for hotmail cursor issue >>
bool WebViewCore::currentFocusNodeIsContentEditable()
{
    WebCore::Node* focusNode = currentFocus();
	return isContentEditable(focusNode);
}
//SISO change for hotmail cursor issue <<
*/
void WebViewCore::scrollFocusedTextInput(float xPercent, int y)
{
    WebCore::Node* focus = currentFocus();
    if (!focus) {
        DBG_NAV_LOG("!focus");
        clearTextEntry();
        return;
    }
    WebCore::RenderObject* renderer = focus->renderer();
    if (!renderer || (!renderer->isTextField() && !renderer->isTextArea())) {
        DBG_NAV_LOGD("renderer==%p || not text", renderer);
        clearTextEntry();
        return;
    }
    WebCore::RenderTextControl* renderText =
        static_cast<WebCore::RenderTextControl*>(renderer);
    int x = (int) (xPercent * (renderText->scrollWidth() -
        renderText->clientWidth()));
    DBG_NAV_LOGD("x=%d y=%d xPercent=%g scrollW=%d clientW=%d", x, y,
        xPercent, renderText->scrollWidth(), renderText->clientWidth());
    renderText->setScrollLeft(x);
    renderText->setScrollTop(y);
}

void WebViewCore::setFocusControllerActive(bool active)
{
    m_mainFrame->page()->focusController()->setActive(active);
}

void WebViewCore::saveDocumentState(WebCore::Frame* frame)
{
    if (!CacheBuilder::validNode(m_mainFrame, frame, 0))
        frame = m_mainFrame;
    WebCore::HistoryItem *item = frame->loader()->history()->currentItem();

    // item can be null when there is no offical URL for the current page. This happens
    // when the content is loaded using with WebCoreFrameBridge::LoadData() and there
    // is no failing URL (common case is when content is loaded using data: scheme)
    if (item) {
        item->setDocumentState(frame->document()->formElementsState());
    }
}

// Create an array of java Strings.
static jobjectArray makeLabelArray(JNIEnv* env, const uint16_t** labels, size_t count)
{
    jclass stringClass = env->FindClass("java/lang/String");
    LOG_ASSERT(stringClass, "Could not find java/lang/String");
    jobjectArray array = env->NewObjectArray(count, stringClass, 0);
    LOG_ASSERT(array, "Could not create new string array");

    for (size_t i = 0; i < count; i++) {
        jobject newString = env->NewString(&labels[i][1], labels[i][0]);
        env->SetObjectArrayElement(array, i, newString);
        env->DeleteLocalRef(newString);
        checkException(env);
    }
    env->DeleteLocalRef(stringClass);
    return array;
}

void WebViewCore::openFileChooser(PassRefPtr<WebCore::FileChooser> chooser)
{
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return;

    if (!chooser)
        return;

    WTF::String acceptType = chooser->acceptTypes();
    jstring jAcceptType = wtfStringToJstring(env, acceptType, true);
    jstring jName = (jstring) env->CallObjectMethod(
            javaObject.get(), m_javaGlue->m_openFileChooser, jAcceptType);
    checkException(env);
    env->DeleteLocalRef(jAcceptType);

    WTF::String wtfString = jstringToWtfString(env, jName);
    env->DeleteLocalRef(jName);

    if (!wtfString.isEmpty()) {
	// SAMSUNG CHANGE + need to escape special characters as they are in encoded form, otherwise file open will fail.
        std::string filename = wtfString.utf8().data();
        if (filename.find("file://") == 0) {// starts with file://
            wtfString = decodeURLEscapeSequences(wtfString);
            DBG_NAV_LOG("file: escaped");
    	}
	// SAMSUNG CHANGE -
        chooser->chooseFile(wtfString);
    }
}

// SAMSUNG CHANGE - FORM NAVIGATION >>
Node* WebViewCore::getCurrentCursor()
{
	gCursorBoundsMutex.lock();
	Node * node= (Node*) m_cursorNode;
	gCursorBoundsMutex.unlock();
	return node;
}

// Convert a WTF::String into an array of characters where the first
// character represents the length, for easy conversion to java.
static uint16_t* stringConverter(const WTF::String& text)
{
    size_t length = text.length();
    uint16_t* itemName = new uint16_t[length+1];
    itemName[0] = (uint16_t)length;
    uint16_t* firstChar = &(itemName[1]);
    memcpy((void*)firstChar, text.characters(), sizeof(UChar)*length);
    return itemName;
}

// Response to dropdown created for a listbox.
class ListBoxReply : public WebCoreReply {
public:
#if ENABLE(WML)
    ListBoxReply(WebCore::Element* select, WebCore::Frame* frame, WebViewCore* view)
#else
    ListBoxReply(WebCore::HTMLSelectElement* select, WebCore::Frame* frame, WebViewCore* view)
#endif
        : m_select(select)
        , m_frame(frame)
        , m_viewImpl(view)
    {}

    // Response used if the listbox only allows single selection.
    // index is listIndex of the selected item, or -1 if nothing is selected.
    virtual void replyInt(int index)
    {
        if (-2 == index) {
            // Special value for cancel. Do nothing.
            return;
        }
        // If the select element no longer exists, due to a page change, etc,
        // silently return.
        if (!m_select || !CacheBuilder::validNode(m_viewImpl->m_mainFrame,
                m_frame, m_select))
            return;
        // Use a pointer to HTMLSelectElement's superclass, where
        // listToOptionIndex is public.
#if ENABLE(WML)
        WebCore::SelectElement* selectElement = WebCore::toSelectElement(m_select);
        int optionIndex = selectElement->listToOptionIndex(index);
        selectElement->setSelectedIndex(optionIndex, true);
#else
        SelectElement* selectElement = m_select;
        int optionIndex = selectElement->listToOptionIndex(index);
        m_select->setSelectedIndex(optionIndex, true);
#endif
        m_select->dispatchFormControlChangeEvent();
        m_viewImpl->contentInvalidate(m_select->getRect());
    }

    // Response if the listbox allows multiple selection.  array stores the listIndices
    // of selected positions.
    virtual void replyIntArray(const int* array, int count)
    {
        // If the select element no longer exists, due to a page change, etc,
        // silently return.
        if (!m_select || !CacheBuilder::validNode(m_viewImpl->m_mainFrame,
                m_frame, m_select))
            return;

        // If count is 1 or 0, use replyInt.
        SkASSERT(count > 1);
#if ENABLE(WML)
        const WTF::Vector<Element*>& items = WebCore::toSelectElement(m_select)->listItems();
#else
        const WTF::Vector<Element*>& items = m_select->listItems();
#endif
        int totalItems = static_cast<int>(items.size());
        // Keep track of the position of the value we are comparing against.
        int arrayIndex = 0;
        // The value we are comparing against.
        int selection = array[arrayIndex];
#if ENABLE(WML)
        // SAMSUNG_WML_FIXES+
        // http://spe.mobilephone.net/wit/wmlv2/formsubpost.wml
        // http://spe.mobilephone.net/wit/wmlv2/formselect.wml
        WebCore::OptionElement* option   = NULL;
        // SAMSUNG_WML_FIXES-
#else
        WebCore::HTMLOptionElement* option;
#endif
        for (int listIndex = 0; listIndex < totalItems; listIndex++) {
            if (items[listIndex]->hasLocalName(WebCore::HTMLNames::optionTag)) {
#if ENABLE(WML)
                // SAMSUNG_WML_FIXES+
                option = WebCore::toOptionElement(items[listIndex]);
                // SAMSUNG_WML_FIXES-
#else
                option = static_cast<WebCore::HTMLOptionElement*>(
                        items[listIndex]);
#endif
                if (listIndex == selection) {
                    option->setSelectedState(true);
                    arrayIndex++;
                    if (arrayIndex == count)
                        selection = -1;
                    else
                        selection = array[arrayIndex];
                } else
                    option->setSelectedState(false);
            }
        }
        m_select->dispatchFormControlChangeEvent();
        m_viewImpl->contentInvalidate(m_select->getRect());
    }
private:
    // The select element associated with this listbox.
#if ENABLE(WML)
    WebCore::Element* m_select;
#else
    WebCore::HTMLSelectElement* m_select;
#endif
    // The frame of this select element, to verify that it is valid.
    WebCore::Frame* m_frame;
    // For calling invalidate and checking the select element's validity
    WebViewCore* m_viewImpl;
};
// SAMSUNG CHANGE - FORM NAVIGATION <<

//SAMSUNG CHANGE HTML5 COLOR <<
void WebViewCore::openColorChooser(WebCore::ColorChooserClientAndroid* chooserclient)
{
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return;

    if (m_colorChooser != 0)
        return;

    env->CallVoidMethod(javaObject.get(), m_javaGlue->m_openColorChooser);
    checkException(env);

    m_colorChooser = chooserclient;

}
//SAMSUNG CHANGE HTML5 COLOR <<
void WebViewCore::listBoxRequest(WebCoreReply* reply, const uint16_t** labels, size_t count, const int enabled[], size_t enabledCount,
        bool multiple, const int selected[], size_t selectedCountOrSelection, const WebCore::Element *select/*SAMSUNG CHANGE - Form Navigation*/)
{
    LOG_ASSERT(m_javaGlue->m_obj, "No java widget associated with this view!");

    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return;

    // If m_popupReply is not null, then we already have a list showing.
    if (m_popupReply != 0)
        return;

    // Create an array of java Strings for the drop down.
    jobjectArray labelArray = makeLabelArray(env, labels, count);

    // Create an array determining whether each item is enabled.
    jintArray enabledArray = env->NewIntArray(enabledCount);
    checkException(env);
    jint* ptrArray = env->GetIntArrayElements(enabledArray, 0);
    checkException(env);
    for (size_t i = 0; i < enabledCount; i++) {
        ptrArray[i] = enabled[i];
    }
    env->ReleaseIntArrayElements(enabledArray, ptrArray, 0);
    checkException(env);

	//SAMSUNG CHANGE Form Navigation>>
    String nodeName = select->formControlName() ;
    jstring jName = env->NewString((jchar*) nodeName.characters(), nodeName.length());
	//SAMSUNG CHANGE Form Navigation<<

    if (multiple) {
        // Pass up an array representing which items are selected.
        jintArray selectedArray = env->NewIntArray(selectedCountOrSelection);
        checkException(env);
        jint* selArray = env->GetIntArrayElements(selectedArray, 0);
        checkException(env);
        for (size_t i = 0; i < selectedCountOrSelection; i++) {
            selArray[i] = selected[i];
        }
        env->ReleaseIntArrayElements(selectedArray, selArray, 0);

        env->CallVoidMethod(javaObject.get(),
                        m_javaGlue->m_requestListBox, labelArray, jName, enabledArray,
                        selectedArray, (jint) select /*SAMSUNG CHANGE Form Navigation*/);
        env->DeleteLocalRef(selectedArray);
    } else {
        // Pass up the single selection.
        env->CallVoidMethod(javaObject.get(),
                m_javaGlue->m_requestSingleListBox, labelArray, jName, enabledArray,
                selectedCountOrSelection, (jint) select/*SAMSUNG CHANGE Form Navigation*/);
    }

    env->DeleteLocalRef(labelArray);
    env->DeleteLocalRef(enabledArray);
    checkException(env);

    Retain(reply);
    m_popupReply = reply;
}

bool WebViewCore::key(const PlatformKeyboardEvent& event)
{
    WebCore::EventHandler* eventHandler;
    WebCore::Node* focusNode = currentFocus();
    DBG_NAV_LOGD("keyCode=%s unichar=%d focusNode=%p",
        event.keyIdentifier().utf8().data(), event.unichar(), focusNode);
    if (focusNode) {
        WebCore::Frame* frame = focusNode->document()->frame();
        WebFrame* webFrame = WebFrame::getWebFrame(frame);
        eventHandler = frame->eventHandler();
        VisibleSelection old = frame->selection()->selection();
        bool handled = eventHandler->keyEvent(event);
        if (isContentEditable(focusNode)) {
            // keyEvent will return true even if the contentEditable did not
            // change its selection.  In the case that it does not, we want to
            // return false so that the key will be sent back to our navigation
            // system.
            handled |= frame->selection()->selection() != old;
        }
        return handled;
    } else {
        eventHandler = m_mainFrame->eventHandler();
    }
    return eventHandler->keyEvent(event);
}

// For when the user clicks the trackball, presses dpad center, or types into an
// unfocused textfield.  In the latter case, 'fake' will be true
void WebViewCore::click(WebCore::Frame* frame, WebCore::Node* node, bool fake) {
    if (!node) {
        WebCore::IntPoint pt = m_mousePos;
        pt.move(m_scrollOffsetX, m_scrollOffsetY);
        WebCore::HitTestResult hitTestResult = m_mainFrame->eventHandler()->
                hitTestResultAtPoint(pt, false);
        node = hitTestResult.innerNode();
        frame = node->document()->frame();
        DBG_NAV_LOGD("m_mousePos=(%d,%d) m_scrollOffset=(%d,%d) pt=(%d,%d)"
            " node=%p", m_mousePos.x(), m_mousePos.y(),
            m_scrollOffsetX, m_scrollOffsetY, pt.x(), pt.y(), node);
    }
    if (node) {
        EditorClientAndroid* client
                = static_cast<EditorClientAndroid*>(
                m_mainFrame->editor()->client());
        client->setShouldChangeSelectedRange(false);
        handleMouseClick(frame, node, fake);
        client->setShouldChangeSelectedRange(true);
    }
}

#if USE(ACCELERATED_COMPOSITING)
GraphicsLayerAndroid* WebViewCore::graphicsRootLayer() const
{
    RenderView* contentRenderer = m_mainFrame->contentRenderer();
    if (!contentRenderer)
        return 0;
    return static_cast<GraphicsLayerAndroid*>(
          contentRenderer->compositor()->rootPlatformLayer());
}
#endif

bool WebViewCore::handleTouchEvent(int action, Vector<int>& ids, Vector<IntPoint>& points, int actionIndex, int metaState)
{
    bool preventDefault = false;

#if USE(ACCELERATED_COMPOSITING)
    GraphicsLayerAndroid* rootLayer = graphicsRootLayer();
    if (rootLayer)
      rootLayer->pauseDisplay(true);
#endif

#if ENABLE(TOUCH_EVENTS) // Android
    #define MOTION_EVENT_ACTION_POINTER_DOWN 5
    #define MOTION_EVENT_ACTION_POINTER_UP 6

    WebCore::TouchEventType type = WebCore::TouchStart;
    WebCore::PlatformTouchPoint::State defaultTouchState;
    Vector<WebCore::PlatformTouchPoint::State> touchStates(points.size());

    switch (action) {
    case 0: // MotionEvent.ACTION_DOWN
        type = WebCore::TouchStart;
        defaultTouchState = WebCore::PlatformTouchPoint::TouchPressed;
        break;
    case 1: // MotionEvent.ACTION_UP
        type = WebCore::TouchEnd;
        defaultTouchState = WebCore::PlatformTouchPoint::TouchReleased;
        break;
    case 2: // MotionEvent.ACTION_MOVE
        type = WebCore::TouchMove;
        defaultTouchState = WebCore::PlatformTouchPoint::TouchMoved;
        break;
    case 3: // MotionEvent.ACTION_CANCEL
        type = WebCore::TouchCancel;
        defaultTouchState = WebCore::PlatformTouchPoint::TouchCancelled;
        break;
    case 5: // MotionEvent.ACTION_POINTER_DOWN
        type = WebCore::TouchStart;
        defaultTouchState = WebCore::PlatformTouchPoint::TouchStationary;
        break;
    case 6: // MotionEvent.ACTION_POINTER_UP
        type = WebCore::TouchEnd;
        defaultTouchState = WebCore::PlatformTouchPoint::TouchStationary;
        break;
    case 0x100: // WebViewCore.ACTION_LONGPRESS
        type = WebCore::TouchLongPress;
        defaultTouchState = WebCore::PlatformTouchPoint::TouchPressed;
        break;
    case 0x200: // WebViewCore.ACTION_DOUBLETAP
        type = WebCore::TouchDoubleTap;
        defaultTouchState = WebCore::PlatformTouchPoint::TouchPressed;
        break;
    default:
        // We do not support other kinds of touch event inside WebCore
        // at the moment.
        LOGW("Java passed a touch event type that we do not support in WebCore: %d", action);
        return 0;
    }

    for (int c = 0; c < static_cast<int>(points.size()); c++) {
        points[c].setX(points[c].x() - m_scrollOffsetX);
        points[c].setY(points[c].y() - m_scrollOffsetY);

        // Setting the touch state for each point.
        // Note: actionIndex will be 0 for all actions that are not ACTION_POINTER_DOWN/UP.
        if (action == MOTION_EVENT_ACTION_POINTER_DOWN && c == actionIndex) {
            touchStates[c] = WebCore::PlatformTouchPoint::TouchPressed;
        } else if (action == MOTION_EVENT_ACTION_POINTER_UP && c == actionIndex) {
            touchStates[c] = WebCore::PlatformTouchPoint::TouchReleased;
        } else {
            touchStates[c] = defaultTouchState;
        };
    }

    WebCore::PlatformTouchEvent te(ids, points, type, touchStates, metaState);
    preventDefault = m_mainFrame->eventHandler()->handleTouchEvent(te);
#endif

#if USE(ACCELERATED_COMPOSITING)
    if (rootLayer)
      rootLayer->pauseDisplay(false);
#endif
    return preventDefault;
}

void WebViewCore::touchUp(int touchGeneration,
    WebCore::Frame* frame, WebCore::Node* node, int x, int y
//LIGHT TOUCH
    , bool useLT)
//LIGHT TOUCH    
{
// LIGHT TOUCH
    if (useLT) {
        // m_mousePos should be set in getTouchHighlightRects()
    	IntPoint xy(m_mousePosLT.x() + m_scrollOffsetX, m_mousePosLT.y() + m_scrollOffsetY);
        WebCore::HitTestResult hitTestResult = m_mainFrame->eventHandler()->hitTestResultAtPoint(xy, false); //changes for MPSG100004590
        node = hitTestResult.innerNode();
        if (node)
            frame = node->document()->frame();
        else
            frame = 0;
//LIGHT TOUCH
        // This moves m_mousePos to the correct place, and handleMouseClick uses
        // m_mousePos to determine where the click happens.
        moveMouse(frame, m_mousePosLT.x() + m_scrollOffsetX, m_mousePosLT.y() + m_scrollOffsetY);
//LIGHT TOUCH
        DBG_NAV_LOGD("touch up on (%d, %d), scrollOffset is (%d, %d), node:%p, frame:%p", m_mousePos.x() + m_scrollOffsetX, m_mousePos.y() + m_scrollOffsetY, m_scrollOffsetX, m_scrollOffsetY, node, frame);

    } else {
        if (m_touchGeneration > touchGeneration) {
            DBG_NAV_LOGD("m_touchGeneration=%d > touchGeneration=%d"
                " x=%d y=%d", m_touchGeneration, touchGeneration, x, y);
            return; // short circuit if a newer touch has been generated
        }
        // This moves m_mousePos to the correct place, and handleMouseClick uses
        // m_mousePos to determine where the click happens.
        moveMouse(frame, x, y);
    }
//LIGHT TOUCH
    /* Light touch did not update this variable causing some Nav Cache updates to be skipped. Though Nav Cahce would
    be updated through New Picture message later, the delayed update could cause a few issues. So now we keep this 
    updated for an updated Nav Cache! */
    m_lastGeneration = touchGeneration;
//LIGHT TOUCH

    if (frame && CacheBuilder::validNode(m_mainFrame, frame, 0)) {
        frame->loader()->resetMultipleFormSubmissionProtection();
    }
    DBG_NAV_LOGD("touchGeneration=%d handleMouseClick frame=%p node=%p"
        " x=%d y=%d", touchGeneration, frame, node, x, y);
    handleMouseClick(frame, node, false);
}

// Check for the "x-webkit-soft-keyboard" attribute.  If it is there and
// set to hidden, do not show the soft keyboard.  Node passed as a parameter
// must not be null.
static bool shouldSuppressKeyboard(const WebCore::Node* node) {
    LOG_ASSERT(node, "node passed to shouldSuppressKeyboard cannot be null");
    const NamedNodeMap* attributes = node->attributes();
    if (!attributes) return false;
    size_t length = attributes->length();
    for (size_t i = 0; i < length; i++) {
        const Attribute* a = attributes->attributeItem(i);
        if (a->localName() == "x-webkit-soft-keyboard" && a->value() == "hidden")
            return true;
    }
    return false;
}

// Common code for both clicking with the trackball and touchUp
// Also used when typing into a non-focused textfield to give the textfield focus,
// in which case, 'fake' is set to true
bool WebViewCore::handleMouseClick(WebCore::Frame* framePtr, WebCore::Node* nodePtr, bool fake)
{
    bool valid = !framePtr || CacheBuilder::validNode(m_mainFrame, framePtr, nodePtr);
    WebFrame* webFrame = WebFrame::getWebFrame(m_mainFrame);
    if (valid && nodePtr) {
    // Need to special case area tags because an image map could have an area element in the middle
    // so when attempting to get the default, the point chosen would be follow the wrong link.
        if (nodePtr->hasTagName(WebCore::HTMLNames::areaTag)) {
            webFrame->setUserInitiatedAction(true);
            nodePtr->dispatchSimulatedClick(0, true, true);
            webFrame->setUserInitiatedAction(false);
            DBG_NAV_LOG("area");
            return true;
        }

        WebCore::RenderObject* renderer = nodePtr->renderer();
        if (renderer && (renderer->isMenuList() || renderer->isListBox())) {
#if ENABLE(WML)
            WebCore::Element *elementPtr = static_cast<WebCore::Element*>(nodePtr) ;
            WebCore::SelectElement* select = WebCore::toSelectElement(elementPtr);
#else
            WebCore::HTMLSelectElement* select = static_cast<WebCore::HTMLSelectElement*>(nodePtr);
#endif
            const WTF::Vector<WebCore::Element*>& listItems = select->listItems();
            SkTDArray<const uint16_t*> names;
            // Possible values for enabledArray.  Keep in Sync with values in
            // InvokeListBox.Container in WebView.java
            enum OptionStatus {
                OPTGROUP = -1,
                OPTION_DISABLED = 0,
                OPTION_ENABLED = 1,
            };
            SkTDArray<int> enabledArray;
            SkTDArray<int> selectedArray;
            int size = listItems.size();
            bool multiple = select->multiple();
            for (int i = 0; i < size; i++) {
#if ENABLE(WML)
                if(WebCore::isOptionElement(listItems[i])){
                    WebCore::OptionElement *op = WebCore::toOptionElement(listItems[i]);
                    if(listItems[i]->isWMLElement()) {
                        WebCore::WMLOptionElement* option = static_cast<WebCore::WMLOptionElement*>(listItems[i]);
                        *enabledArray.append() = option->disabled() ? OPTION_DISABLED : OPTION_ENABLED;
                    } else {
                        WebCore::HTMLOptionElement* option = static_cast<WebCore::HTMLOptionElement*>(listItems[i]);
                        *enabledArray.append() = option->disabled() ? OPTION_DISABLED : OPTION_ENABLED;
                    }
                    String label = op->textIndentedToRespectGroupLabel();
                    *names.append() = stringConverter(label);
                    if (multiple && op->selected())
                        *selectedArray.append() = i;
                } else if (WebCore::isOptionGroupElement(listItems[i])) {
                    WebCore::OptionGroupElement* optGroup = WebCore::toOptionGroupElement(listItems[i]);
                    *names.append() = stringConverter(optGroup->groupLabelText());
                    *enabledArray.append() = OPTGROUP;
                }
#else
                if (listItems[i]->hasTagName(WebCore::HTMLNames::optionTag)) {
                    WebCore::HTMLOptionElement* option = static_cast<WebCore::HTMLOptionElement*>(listItems[i]);
                    *names.append() = stringConverter(option->textIndentedToRespectGroupLabel());
                    *enabledArray.append() = option->disabled() ? OPTION_DISABLED : OPTION_ENABLED;
                    if (multiple && option->selected())
                        *selectedArray.append() = i;
                } else if (listItems[i]->hasTagName(WebCore::HTMLNames::optgroupTag)) {
                    WebCore::HTMLOptGroupElement* optGroup = static_cast<WebCore::HTMLOptGroupElement*>(listItems[i]);
                    *names.append() = stringConverter(optGroup->groupLabelText());
                    *enabledArray.append() = OPTGROUP;
                }
#endif
            }
#if ENABLE(WML)
            WebCoreReply* reply = new ListBoxReply(elementPtr, elementPtr->document()->frame(), this);
#else
            WebCoreReply* reply = new ListBoxReply(select, select->document()->frame(), this);
#endif
            // Use a pointer to HTMLSelectElement's superclass, where
            // optionToListIndex is public.
            SelectElement* selectElement = select;
            listBoxRequest(reply, names.begin(), size, enabledArray.begin(), enabledArray.count(),
                    multiple, selectedArray.begin(), multiple ? selectedArray.count() :
                    selectElement->optionToListIndex(select->selectedIndex()), static_cast<WebCore::Element*>(nodePtr)/*SAMSUNG CHANGE*/);
            DBG_NAV_LOG("menu list");
            return true;
        }
    }
    if (!valid || !framePtr)
        framePtr = m_mainFrame;
    webFrame->setUserInitiatedAction(true);
    WebCore::PlatformMouseEvent mouseDown(m_mousePos, m_mousePos, WebCore::LeftButton,
            WebCore::MouseEventPressed, 1, false, false, false, false,
            WTF::currentTime());
    // ignore the return from as it will return true if the hit point can trigger selection change
    framePtr->eventHandler()->handleMousePressEvent(mouseDown);
    WebCore::PlatformMouseEvent mouseUp(m_mousePos, m_mousePos, WebCore::LeftButton,
            WebCore::MouseEventReleased, 1, false, false, false, false,
            WTF::currentTime());
    bool handled = framePtr->eventHandler()->handleMouseReleaseEvent(mouseUp);
    webFrame->setUserInitiatedAction(false);

    // If the user clicked on a textfield, make the focusController active
    // so we show the blinking cursor.
    WebCore::Node* focusNode = currentFocus();
    DBG_NAV_LOGD("m_mousePos={%d,%d} focusNode=%p handled=%s", m_mousePos.x(),
        m_mousePos.y(), focusNode, handled ? "true" : "false");
    if (focusNode) {
        WebCore::RenderObject* renderer = focusNode->renderer();
        if (renderer && (renderer->isTextField() || renderer->isTextArea())) {
            bool ime = !shouldSuppressKeyboard(focusNode)
                    && !(static_cast<WebCore::HTMLInputElement*>(focusNode))->readOnly();
				//SAMSUNG FIX >>
#if ENABLE(WML)
				if (renderer->isTextArea()) {
					ime = !(static_cast<WebCore::HTMLTextAreaElement*>(focusNode))->readOnly();
				} else {
					WebCore::Element* element = static_cast<WebCore::Element*>(focusNode);
					WebCore::InputElement *ie = element->toInputElement();
//SAMSUNG WML CHANGE
//					ime = ie ? !ie->readOnly() : true ;
//SAMSUNG WML CHANGE
				}
#else
	if (renderer->isTextField()) {
		ime = !(static_cast<WebCore::HTMLInputElement*>(focusNode))->readOnly();
		//            bool ime = !(static_cast<WebCore::HTMLInputElement*>(focusNode))
	} else {
		ime = !(static_cast<WebCore::HTMLTextAreaElement*>(focusNode))->readOnly();
	}
#endif
//SISO Changes Begin <Fix for issue: Text input box not invoking softkeypad in naver.com (Code merged from Victory)>
	m_frameCacheOutOfDate = true;
	updateFrameCache();
	//SISO Changes End
	//SAMSUNG FIX >>
	//                    ->readOnly();
            if (ime) {
#if ENABLE(WEB_AUTOFILL)
		//SAMSUNG WML CHANGES >>
		if(!focusNode->isWMLElement()) {
			//SAMSUNG WML CHANGES <<
                if (renderer->isTextField()) {
                    EditorClientAndroid* editorC = static_cast<EditorClientAndroid*>(framePtr->page()->editorClient());
                    WebAutofill* autoFill = editorC->getAutofill();
                    autoFill->formFieldFocused(static_cast<HTMLFormControlElement*>(focusNode));
                }
			//SAMSUNG WML CHANGES >>
		}
		//SAMSUNG WML CHANGES <<
#endif
                if (!fake) {
                    RenderTextControl* rtc
                            = static_cast<RenderTextControl*> (renderer);
                    // Force an update of the navcache as this will fire off a
                    // message to WebView that *must* have an updated focus.
                    m_frameCacheOutOfDate = true;
                    updateFrameCache();
				//SAMSUNG HTML5 INPUT TYPE DATE/TIME CHANGES >>
		 		    WebCore::Element* ele = static_cast<Element*>(focusNode);
				    const AtomicString &typestr = ele->getAttribute(HTMLNames::typeAttr);		    
				    const WTF::String& typestring = typestr.string();
				    if((typestr == "date") || (typestr == "datetime") || (typestr == "datetime-local") || (typestr == "time")){
						const AtomicString &valuestr = ele->getAttribute(HTMLNames::valueAttr);		        	
						const WTF::String& text = rtc->text();
						if(!text){	
		        	    	if((valuestr.isNull()))			    		                   
			        		requestDateTimePickers(typestring,"");
				    		else{
				        		const WTF::String& valuestring = valuestr.string();
				        		requestDateTimePickers(typestring,valuestring);	
				    		}
						}
			       		else{
                           requestDateTimePickers(typestring,text);	
			       		}						
			    	}				
				    else
					//SAMSUNG HTML5 INPUT TYPE DATE/TIME CHANGES <<
                        requestKeyboardWithSelection(focusNode, rtc->selectionStart(),
                            rtc->selectionEnd());
                }
            } else if (!fake) {
                requestKeyboard(false);
            }
        } else if (!fake){
            // If the selection is contentEditable, show the keyboard so the
            // user can type.  Otherwise hide the keyboard because no text
            // input is needed.
            if (isContentEditable(focusNode)) {
                requestKeyboard(true);
            } else if (!nodeIsPlugin(focusNode)) {
                clearTextEntry();
            }
        }
    } else if (!fake) {
        // There is no focusNode, so the keyboard is not needed.
        clearTextEntry();
    }
    return handled;
}

void WebViewCore::popupReply(int index)
{
    if (m_popupReply) {
        m_popupReply->replyInt(index);
        //SAMSUNG CHANGE Form Navigation >>
        if (index == -2) {
        	Release(m_popupReply);
        	m_popupReply = 0;
        }
        //SAMSUNG CHANGE Form Navigation <<
    }
}

void WebViewCore::popupReply(const int* array, int count)
{
    if (m_popupReply) {
        m_popupReply->replyIntArray(array, count);
        Release(m_popupReply);
        m_popupReply = 0;
    }
}

//SAMSUNG CHANGE HTML5 COLOR <<
//Set the color value back to engine
void WebViewCore::ColorChooserReply(int color)
{
    if (m_colorChooser && color!=0) {
        m_colorChooser->didChooseColor(color); 
	IntRect r = m_colorChooser->getRect();
        contentInvalidate(r); 
	delete(m_colorChooser); 	
        m_colorChooser = 0;	
    }
    else{
        if(m_colorChooser){
	    delete(m_colorChooser); 	
	    m_colorChooser = 0;    
	}
    }	  	
}
//SAMSUNG CHANGE HTML5 COLOR <<

void WebViewCore::formDidBlur(const WebCore::Node* node)
{
    // If the blur is on a text input, keep track of the node so we can
    // hide the soft keyboard when the new focus is set, if it is not a
    // text input.
    if (isTextInput(node))
        m_blurringNodePointer = reinterpret_cast<int>(node);
}

void WebViewCore::focusNodeChanged(const WebCore::Node* newFocus)
{
    if (isTextInput(newFocus))
        m_shouldPaintCaret = true;
    else if (m_blurringNodePointer) {
        JNIEnv* env = JSC::Bindings::getJNIEnv();
        AutoJObject javaObject = m_javaGlue->object(env);
        if (!javaObject.get())
            return;
        env->CallVoidMethod(javaObject.get(), m_javaGlue->m_formDidBlur, m_blurringNodePointer);
        checkException(env);
        m_blurringNodePointer = 0;
    }
}

void WebViewCore::addMessageToConsole(const WTF::String& message, unsigned int lineNumber, const WTF::String& sourceID, int msgLevel) {
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return;
    jstring jMessageStr = wtfStringToJstring(env, message);
    jstring jSourceIDStr = wtfStringToJstring(env, sourceID);
    env->CallVoidMethod(javaObject.get(),
            m_javaGlue->m_addMessageToConsole, jMessageStr, lineNumber,
            jSourceIDStr, msgLevel);
    env->DeleteLocalRef(jMessageStr);
    env->DeleteLocalRef(jSourceIDStr);
    checkException(env);
}

void WebViewCore::jsAlert(const WTF::String& url, const WTF::String& text)
{
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return;
    jstring jInputStr = wtfStringToJstring(env, text);
    jstring jUrlStr = wtfStringToJstring(env, url);
    env->CallVoidMethod(javaObject.get(), m_javaGlue->m_jsAlert, jUrlStr, jInputStr);
    env->DeleteLocalRef(jInputStr);
    env->DeleteLocalRef(jUrlStr);
    checkException(env);
}

bool WebViewCore::exceededDatabaseQuota(const WTF::String& url, const WTF::String& databaseIdentifier, const unsigned long long currentQuota, unsigned long long estimatedSize)
{
#if ENABLE(DATABASE)
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return false;
    jstring jDatabaseIdentifierStr = wtfStringToJstring(env, databaseIdentifier);
    jstring jUrlStr = wtfStringToJstring(env, url);
    env->CallVoidMethod(javaObject.get(),
            m_javaGlue->m_exceededDatabaseQuota, jUrlStr,
            jDatabaseIdentifierStr, currentQuota, estimatedSize);
    env->DeleteLocalRef(jDatabaseIdentifierStr);
    env->DeleteLocalRef(jUrlStr);
    checkException(env);
    return true;
#endif
}

bool WebViewCore::reachedMaxAppCacheSize(const unsigned long long spaceNeeded)
{
#if ENABLE(OFFLINE_WEB_APPLICATIONS)
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return false;
    env->CallVoidMethod(javaObject.get(), m_javaGlue->m_reachedMaxAppCacheSize, spaceNeeded);
    checkException(env);
    return true;
#endif
}

void WebViewCore::populateVisitedLinks(WebCore::PageGroup* group)
{
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return;
    m_groupForVisitedLinks = group;
    env->CallVoidMethod(javaObject.get(), m_javaGlue->m_populateVisitedLinks);
    checkException(env);
}

void WebViewCore::geolocationPermissionsShowPrompt(const WTF::String& origin)
{
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return;
    jstring originString = wtfStringToJstring(env, origin);
    env->CallVoidMethod(javaObject.get(),
                        m_javaGlue->m_geolocationPermissionsShowPrompt,
                        originString);
    env->DeleteLocalRef(originString);
    checkException(env);
}

// Samsung Change - HTML5 Web Notification	>>
void WebViewCore::notificationPermissionsShowPrompt(const WTF::String& url)
{
    #if ENABLE(NOTIFICATIONS)
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return;
    LOGV("WebViewCore::notificationPermissionsShowPrompt URL is %s",url.utf8().data());
    jstring jUrlStr = wtfStringToJstring(env, url);    
    env->CallVoidMethod(javaObject.get(),
                        m_javaGlue->m_notificationPermissionsShowPrompt,
                        jUrlStr);
    env->DeleteLocalRef(jUrlStr);
    checkException(env);
    #endif
}

void WebViewCore::notificationManagershow(const WTF::String& iconUrl, const WTF::String& titleStr, const WTF::String& bodyStr
	,int counter)
{
    #if ENABLE(NOTIFICATIONS)
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env); 
    LOGV("Inside WebViewCore::notificationManagershow ");
    if (!javaObject.get())
        return;
    jstring jIconUrlStr = wtfStringToJstring(env, iconUrl);
    jstring jtitleStr = wtfStringToJstring(env, titleStr);
    jstring jbodyStr = wtfStringToJstring(env, bodyStr);
    env->CallVoidMethod(javaObject.get(),m_javaGlue->m_notificationManagershow,jIconUrlStr,jtitleStr,jbodyStr,
		counter);
    env->DeleteLocalRef(jIconUrlStr);
    env->DeleteLocalRef(jtitleStr);
    env->DeleteLocalRef(jbodyStr);
    checkException(env);
    #endif
}

void WebViewCore::notificationManagerCancel(int notificationID)
{
   #if ENABLE(NOTIFICATIONS)
   JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env); 
    LOGV("Inside WebViewCore::notificationManagerCancel %d", notificationID);
    if (!javaObject.get())
        return;
   env->CallVoidMethod(javaObject.get(),m_javaGlue->m_notificationManagerCancel, notificationID);
   checkException(env);
   #endif
}

void WebViewCore::notificationPermissionsHidePrompt()
{
   #if ENABLE(NOTIFICATIONS)
   JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env); 
    LOGV("Inside WebViewCore::notificationPermissionsHidePrompt");
    if (!javaObject.get())
        return;
   env->CallVoidMethod(javaObject.get(),m_javaGlue->m_notificationPermissionsHidePrompt);
   checkException(env);
   #endif
}
// Samsung Change - HTML5 Web Notification	<<

void WebViewCore::geolocationPermissionsHidePrompt()
{
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return;
    env->CallVoidMethod(javaObject.get(), m_javaGlue->m_geolocationPermissionsHidePrompt);
    checkException(env);
}

jobject WebViewCore::getDeviceMotionService()
{
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return 0;
    jobject object = env->CallObjectMethod(javaObject.get(), m_javaGlue->m_getDeviceMotionService);
    checkException(env);
    return object;
}

jobject WebViewCore::getDeviceOrientationService()
{
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return 0;
    jobject object = env->CallObjectMethod(javaObject.get(), m_javaGlue->m_getDeviceOrientationService);
    checkException(env);
    return object;
}

bool WebViewCore::jsConfirm(const WTF::String& url, const WTF::String& text)
{
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return false;
    jstring jInputStr = wtfStringToJstring(env, text);
    jstring jUrlStr = wtfStringToJstring(env, url);
    jboolean result = env->CallBooleanMethod(javaObject.get(), m_javaGlue->m_jsConfirm, jUrlStr, jInputStr);
    env->DeleteLocalRef(jInputStr);
    env->DeleteLocalRef(jUrlStr);
    checkException(env);
    return result;
}

bool WebViewCore::jsPrompt(const WTF::String& url, const WTF::String& text, const WTF::String& defaultValue, WTF::String& result)
{
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return false;
    jstring jUrlStr = wtfStringToJstring(env, url);
    jstring jInputStr = wtfStringToJstring(env, text);
    jstring jDefaultStr = wtfStringToJstring(env, defaultValue);
    jstring returnVal = static_cast<jstring>(env->CallObjectMethod(javaObject.get(), m_javaGlue->m_jsPrompt, jUrlStr, jInputStr, jDefaultStr));
    env->DeleteLocalRef(jUrlStr);
    env->DeleteLocalRef(jInputStr);
    env->DeleteLocalRef(jDefaultStr);
    checkException(env);

    // If returnVal is null, it means that the user cancelled the dialog.
    if (!returnVal)
        return false;

    result = jstringToWtfString(env, returnVal);
    env->DeleteLocalRef(returnVal);
    return true;
}

bool WebViewCore::jsUnload(const WTF::String& url, const WTF::String& message)
{
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return false;
    jstring jInputStr = wtfStringToJstring(env, message);
    jstring jUrlStr = wtfStringToJstring(env, url);
    jboolean result = env->CallBooleanMethod(javaObject.get(), m_javaGlue->m_jsUnload, jUrlStr, jInputStr);
    env->DeleteLocalRef(jInputStr);
    env->DeleteLocalRef(jUrlStr);
    checkException(env);
    return result;
}

bool WebViewCore::jsInterrupt()
{
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return false;
    jboolean result = env->CallBooleanMethod(javaObject.get(), m_javaGlue->m_jsInterrupt);
    checkException(env);
    return result;
}

AutoJObject
WebViewCore::getJavaObject()
{
    return m_javaGlue->object(JSC::Bindings::getJNIEnv());
}

jobject
WebViewCore::getWebViewJavaObject()
{
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return 0;
    return env->GetObjectField(javaObject.get(), gWebViewCoreFields.m_webView);
}

void WebViewCore::updateTextSelection()
{
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return;
    WebCore::Node* focusNode = currentFocus();
    if (!focusNode)
        return;
    RenderObject* renderer = focusNode->renderer();
    if (!renderer || (!renderer->isTextArea() && !renderer->isTextField()))
        return;
    RenderTextControl* rtc = static_cast<RenderTextControl*>(renderer);
    env->CallVoidMethod(javaObject.get(),
            m_javaGlue->m_updateTextSelection, reinterpret_cast<int>(focusNode),
            rtc->selectionStart(), rtc->selectionEnd(), m_textGeneration);
    checkException(env);
}

void WebViewCore::updateTextfield(WebCore::Node* ptr, bool changeToPassword,
        const WTF::String& text)
{
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return;
    if (m_blockTextfieldUpdates)
        return;
    if (changeToPassword) {
        env->CallVoidMethod(javaObject.get(), m_javaGlue->m_updateTextfield,
                (int) ptr, true, 0, m_textGeneration);
        checkException(env);
        return;
    }
    jstring string = wtfStringToJstring(env, text);
    env->CallVoidMethod(javaObject.get(), m_javaGlue->m_updateTextfield,
            (int) ptr, false, string, m_textGeneration);
    env->DeleteLocalRef(string);
    checkException(env);
}

void WebViewCore::clearTextEntry()
{
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return;
    env->CallVoidMethod(javaObject.get(), m_javaGlue->m_clearTextEntry);
}

void WebViewCore::setBackgroundColor(SkColor c)
{
    WebCore::FrameView* view = m_mainFrame->view();
    if (!view)
        return;

    // need (int) cast to find the right constructor
    WebCore::Color bcolor((int)SkColorGetR(c), (int)SkColorGetG(c),
                          (int)SkColorGetB(c), (int)SkColorGetA(c));
    view->setBaseBackgroundColor(bcolor);

    // Background color of 0 indicates we want a transparent background
    if (c == 0)
        view->setTransparent(true);
}

jclass WebViewCore::getPluginClass(const WTF::String& libName, const char* className)
{
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return 0;

    jstring libString = wtfStringToJstring(env, libName);
    jstring classString = env->NewStringUTF(className);
    jobject pluginClass = env->CallObjectMethod(javaObject.get(),
                                           m_javaGlue->m_getPluginClass,
                                           libString, classString);
    checkException(env);

    // cleanup unneeded local JNI references
    env->DeleteLocalRef(libString);
    env->DeleteLocalRef(classString);

    if (pluginClass != 0) {
        return static_cast<jclass>(pluginClass);
    } else {
        return 0;
    }
}

void WebViewCore::showFullScreenPlugin(jobject childView, int32_t orientation, NPP npp)
{
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return;

    env->CallVoidMethod(javaObject.get(),
                        m_javaGlue->m_showFullScreenPlugin,
                        childView, orientation, reinterpret_cast<int>(npp));
    checkException(env);
}

void WebViewCore::hideFullScreenPlugin()
{
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return;
    env->CallVoidMethod(javaObject.get(), m_javaGlue->m_hideFullScreenPlugin);
    checkException(env);
}

jobject WebViewCore::createSurface(jobject view)
{
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return 0;
    jobject result = env->CallObjectMethod(javaObject.get(), m_javaGlue->m_createSurface, view);
    checkException(env);
    return result;
}

jobject WebViewCore::addSurface(jobject view, int x, int y, int width, int height)
{
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return 0;
    jobject result = env->CallObjectMethod(javaObject.get(),
                                           m_javaGlue->m_addSurface,
                                           view, x, y, width, height);
    checkException(env);
    return result;
}

void WebViewCore::updateSurface(jobject childView, int x, int y, int width, int height)
{
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return;
    env->CallVoidMethod(javaObject.get(),
                        m_javaGlue->m_updateSurface, childView,
                        x, y, width, height);
    checkException(env);
}

void WebViewCore::destroySurface(jobject childView)
{
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return;
    env->CallVoidMethod(javaObject.get(), m_javaGlue->m_destroySurface, childView);
    checkException(env);
}

jobject WebViewCore::getContext()
{
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return 0;

    jobject result = env->CallObjectMethod(javaObject.get(), m_javaGlue->m_getContext);
    checkException(env);
    return result;
}

void WebViewCore::keepScreenOn(bool screenOn) {
    if ((screenOn && m_screenOnCounter == 0) || (!screenOn && m_screenOnCounter == 1)) {
        JNIEnv* env = JSC::Bindings::getJNIEnv();
        AutoJObject javaObject = m_javaGlue->object(env);
        if (!javaObject.get())
            return;
        env->CallVoidMethod(javaObject.get(), m_javaGlue->m_keepScreenOn, screenOn);
        checkException(env);
    }

    // update the counter
    if (screenOn)
        m_screenOnCounter++;
    else if (m_screenOnCounter > 0)
        m_screenOnCounter--;
}

bool WebViewCore::validNodeAndBounds(Frame* frame, Node* node,
    const IntRect& originalAbsoluteBounds)
{
    bool valid = CacheBuilder::validNode(m_mainFrame, frame, node);
    if (!valid)
        return false;
    RenderObject* renderer = node->renderer();
    if (!renderer)
        return false;
    IntRect absBounds = node->hasTagName(HTMLNames::areaTag)
        ? CacheBuilder::getAreaRect(static_cast<HTMLAreaElement*>(node))
        : renderer->absoluteBoundingBoxRect();
    return absBounds == originalAbsoluteBounds;
}

void WebViewCore::showRect(int left, int top, int width, int height,
        int contentWidth, int contentHeight, float xPercentInDoc,
        float xPercentInView, float yPercentInDoc, float yPercentInView)
{
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return;
    env->CallVoidMethod(javaObject.get(), m_javaGlue->m_showRect,
            left, top, width, height, contentWidth, contentHeight,
            xPercentInDoc, xPercentInView, yPercentInDoc, yPercentInView);
    checkException(env);
}

void WebViewCore::centerFitRect(int x, int y, int width, int height)
{
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return;
    env->CallVoidMethod(javaObject.get(), m_javaGlue->m_centerFitRect, x, y, width, height);
    checkException(env);
}

void WebViewCore::setScrollbarModes(ScrollbarMode horizontalMode, ScrollbarMode verticalMode)
{
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return;
    env->CallVoidMethod(javaObject.get(), m_javaGlue->m_setScrollbarModes, horizontalMode, verticalMode);
    checkException(env);
}

void WebViewCore::notifyWebAppCanBeInstalled()
{
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return;
    env->CallVoidMethod(javaObject.get(), m_javaGlue->m_setInstallableWebApp);
    checkException(env);
}

#if ENABLE(VIDEO)
void WebViewCore::enterFullscreenForVideoLayer(int layerId, const WTF::String& url)
{
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return;
    jstring jUrlStr = wtfStringToJstring(env, url);
    env->CallVoidMethod(javaObject.get(), m_javaGlue->m_enterFullscreenForVideoLayer, layerId, jUrlStr);
    checkException(env);
}
#endif

//SAMSUNG CHANGE >> LIGHT TOUCH
void WebViewCore::setNavType(int type)
{
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return;
    env->CallVoidMethod(javaObject.get(), m_javaGlue->m_setNavType, type);
    checkException(env);
}
//SAMSUNG CHANGE << LIGHT TOUCH
void WebViewCore::setWebTextViewAutoFillable(int queryId, const string16& previewSummary)
{
#if ENABLE(WEB_AUTOFILL)
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    AutoJObject javaObject = m_javaGlue->object(env);
    if (!javaObject.get())
        return;
    jstring preview = env->NewString(previewSummary.data(), previewSummary.length());
    env->CallVoidMethod(javaObject.get(), m_javaGlue->m_setWebTextViewAutoFillable, queryId, preview);
    env->DeleteLocalRef(preview);
#endif
}

bool WebViewCore::drawIsPaused() const
{
    // returning true says scrollview should be offscreen, which pauses
    // gifs. because this is not again queried when we stop scrolling, we don't
    // use the stopping currently.
    return false;
}

#if USE(CHROME_NETWORK_STACK)
void WebViewCore::setWebRequestContextUserAgent()
{
    // We cannot create a WebRequestContext, because we might not know it this is a private tab or not yet
    if (m_webRequestContext)
        m_webRequestContext->setUserAgent(WebFrame::getWebFrame(m_mainFrame)->userAgentForURL(0)); // URL not used
}

void WebViewCore::setWebRequestContextCacheMode(int cacheMode)
{
    m_cacheMode = cacheMode;
    // We cannot create a WebRequestContext, because we might not know it this is a private tab or not yet
    if (!m_webRequestContext)
        return;

    m_webRequestContext->setCacheMode(cacheMode);
}

WebRequestContext* WebViewCore::webRequestContext()
{
    if (!m_webRequestContext) {
        Settings* settings = mainFrame()->settings();
        m_webRequestContext = new WebRequestContext(settings && settings->privateBrowsingEnabled());
        setWebRequestContextUserAgent();
        setWebRequestContextCacheMode(m_cacheMode);
    }
    return m_webRequestContext.get();
}
#endif

// SAMSUNG CHANGE : ADVANCED_TEXT_SELECTION >>
void WebViewCore::webTextSelectionAll(int x1, int y1, int parma1, int param2)
{
	DEBUG_NAV_UI_LOGD("webTextSelectionAll : called  %d, %d",  x1, y1);

	WebCore::Frame* frame = m_mainFrame;
	if(frame->selection()){
		//param1 Indicate  Falg for first selection need to do before select all.
		//param2 is not used .
		if(parma1 == 1){
			// Select text First
			selectClosestWord(x1,y1,1.0f,true);
			DEBUG_NAV_UI_LOGD("%s: first Word Select  ", __FUNCTION__);
		}
		frame->selection()->selectAll();
		//frame->selection()->setGranularity(WebCore::ParagraphGranularity);
	}else{
		DEBUG_NAV_UI_LOGD("%s: Exception:  Frame Selection is null ", __FUNCTION__);
	}
}

int mSelectionDirection = DirectionForward;

void WebViewCore::copyMoveSelection(int x, int y,  int controller, bool smartGranularity, bool selectionMove, float zoomLevel , int granularity)
{
	DEBUG_NAV_UI_LOGD("%s: x,y position: %d, %d", __FUNCTION__, x, y);
	WebCore::Frame* frame = m_mainFrame;
	WebCore::IntPoint contentsPoint = WebCore::IntPoint(x, y);
	WebCore::IntPoint wndPoint = frame->view()->contentsToWindow(contentsPoint);

	DEBUG_NAV_UI_LOGD("%s: second time click", __FUNCTION__);

	//Set Direction
	VisibleSelection visSel = frame->selection()->selection();
	if (selectionMove == false)
	{
		copySetSelectionDirection(controller);
		if(frame->selection()->granularity() == WebCore::WordGranularity )
		{
			frame->selection()->setGranularity(WebCore::CharacterGranularity);
			visSel.expandUsingGranularity(WebCore::CharacterGranularity);
			DEBUG_NAV_UI_LOGD("Changed from Word to character Granularity");
		}else   if(frame->selection()->granularity() == WebCore::CharacterGranularity )
		{
			visSel.expandUsingGranularity(WebCore::CharacterGranularity);
			LOGD("Set the character Granularity");
		}
		return;
	}

	if (smartGranularity == true) {
		if (!inSameParagraph(WebCore::VisiblePosition(frame->selection()->base()),
				WebCore::VisiblePosition(frame->selection()->extent()))) {
			if (frame->selection()->granularity() != WebCore::ParagraphGranularity) {
				frame->selection()->setGranularity(WebCore::ParagraphGranularity);
				visSel.expandUsingGranularity(WebCore::ParagraphGranularity);
				DEBUG_NAV_UI_LOGD("Set Paragraph Granularity");
			}
		} else {
			if (frame->selection()->granularity() == WebCore::ParagraphGranularity) {
				frame->selection()->setGranularity(WebCore::CharacterGranularity);
				visSel.expandUsingGranularity(WebCore::CharacterGranularity);
				DEBUG_NAV_UI_LOGD("Change from Paragraph to Character Granularity");
			}
		}
		//Set In Paragraph mode when zoom level is less than 0.8
		if (zoomLevel < 0.8 && frame->selection()->granularity() != WebCore::ParagraphGranularity) {
			frame->selection()->setGranularity(WebCore::ParagraphGranularity);
			visSel.expandUsingGranularity(WebCore::ParagraphGranularity);
			DEBUG_NAV_UI_LOGD("Set Paragraph Granularity for Less Zoom Level");
		}
	}

	WebCore::TextGranularity CurrGranulaity = frame->selection()->granularity() ;

	//User Granularity Apply if Set
	if(granularity != -1 && CurrGranulaity == WebCore::CharacterGranularity){
		frame->selection()->setGranularity((WebCore::TextGranularity) granularity );
		LOGD("Set  Granularity by client  %d",  granularity);
		webkitCopyMoveSelection(wndPoint, contentsPoint, controller);
		frame->selection()->setGranularity((WebCore::TextGranularity) CurrGranulaity );
	}  else{
		webkitCopyMoveSelection(wndPoint, contentsPoint, controller);
	}
    // One more check to make sure that granularity matches with the current points
    if (smartGranularity == true) {
	    if(!inSameParagraph(WebCore::VisiblePosition(frame->selection()->base()),
	        WebCore::VisiblePosition(frame->selection()->extent())))
	    {
	        if (frame->selection()->granularity() != WebCore::ParagraphGranularity) {
	            frame->selection()->setGranularity(WebCore::ParagraphGranularity);
	            visSel.expandUsingGranularity(WebCore::ParagraphGranularity);
	            DEBUG_NAV_UI_LOGD("Correct granularity to Paragraph Granularity");
	        }
	    }
	    else
	    {
	        if(frame->selection()->granularity() == WebCore::ParagraphGranularity) {
	            frame->selection()->setGranularity(WebCore::CharacterGranularity);
	            visSel.expandUsingGranularity(WebCore::CharacterGranularity);
	            DEBUG_NAV_UI_LOGD("Correct granularity to Character Granularity");
	        }
	    }
    }
    // End

	DEBUG_NAV_UI_LOGD("%s: End", __FUNCTION__);
}

void WebViewCore::clearTextSelection(int contentX, int contentY)
{
	DEBUG_NAV_UI_LOGD("%s: x,y position: %d, %d", __FUNCTION__, contentX, contentY);
	WebCore::Frame* frame = m_mainFrame;
	if (frame->selection()){
		frame->selection()->clear();
	} else {
		DEBUG_NAV_UI_LOGD("%s: Exception:  Frame Selection is null ", __FUNCTION__);
	}
}

void WebViewCore::copySetSelectionDirection(int controller)
{
	DEBUG_NAV_UI_LOGD("%s: Set the Selection Direction: %d", __FUNCTION__, controller);

	WebCore::Frame* frame = m_mainFrame;
	frame->eventHandler()->setMousePressed(true);
	frame->selection()->setIsDirectional(false); // Need to set to make selection work in all directions
	switch(controller)
	{
	case 2:
	case 5:
		mSelectionDirection = DirectionForward;
		frame->selection()->willBeModified(SelectionController::AlterationExtend, DirectionForward);
		break;

	case 3:
		mSelectionDirection = DirectionLeft;
		frame->selection()->willBeModified(SelectionController::AlterationExtend, DirectionLeft);
		break;
	case 4:
		mSelectionDirection = DirectionRight;
		frame->selection()->willBeModified(SelectionController::AlterationExtend, DirectionRight);
		break;

	case 1:
	case 6:
		mSelectionDirection = DirectionBackward;
		frame->selection()->willBeModified(SelectionController::AlterationExtend, DirectionBackward);
		break;
	default:
		DEBUG_NAV_UI_LOGD("%s: Invalid Direction: %d", __FUNCTION__, controller);
		frame->eventHandler()->setMousePressed(false);
		break;
	}
}

void WebViewCore::webkitCopyMoveSelection(WebCore::IntPoint wndPoint, WebCore::IntPoint contentPoint, int controller)
{
	DEBUG_NAV_UI_LOGD("%s", __FUNCTION__);
	WebCore::Frame* frame = m_mainFrame;
	DEBUG_NAV_UI_LOGD("%s: Frame=%s", __FUNCTION__, frame);
	WebCore::FrameView *frameview = frame->view();

	if(frame->selection()->granularity() == WebCore::ParagraphGranularity)
	{
		DEBUG_NAV_UI_LOGD("%s: Moving in Paragraph Granularity", __FUNCTION__);
		WebCore::IntRect box = WebCore::IntRect(0,0,0,0);
		int left = 0, top = 0, right = 0, bottom = 0;

		if (RefPtr<Range> range = frame->selection()->toNormalizedRange())
		{
			ExceptionCode ec = 0;
			RefPtr<Range> tempRange = range->cloneRange(ec);
			box = tempRange->boundingBox();
			left = box.x();
			top = box.y();
			right = left + box.width();
			bottom = top + box.height();
			DEBUG_NAV_UI_LOGD("%s: BoundingRect:[%d, %d, %d, %d]", __FUNCTION__, box.x(), box.y(), box.width(), box.height());
		}
		else
		{
			DEBUG_NAV_UI_LOGD("%s: Exception in getting Selection Region", __FUNCTION__);
			return;
		}
		switch(mSelectionDirection)
		{
		case WebCore::DirectionForward:
			contentPoint=contentPoint.shrunkTo(WebCore::IntPoint(right, contentPoint.y()));
			contentPoint=contentPoint.expandedTo(WebCore::IntPoint(left, top));
			break;

		case DirectionBackward:
			contentPoint=contentPoint.expandedTo(WebCore::IntPoint(left, contentPoint.y()));
			contentPoint=contentPoint.shrunkTo(WebCore::IntPoint(right, bottom));
			break;

		case DirectionLeft:
			contentPoint=contentPoint.expandedTo(WebCore::IntPoint(contentPoint.x(), top));
			contentPoint=contentPoint.shrunkTo(WebCore::IntPoint(right, bottom));
			break;

		case DirectionRight:
			contentPoint=contentPoint.shrunkTo(WebCore::IntPoint(contentPoint.x(), bottom));
			contentPoint=contentPoint.expandedTo(WebCore::IntPoint(left, top));
			break;

		default:
			break;
		}
	}
	else
	{
		DEBUG_NAV_UI_LOGD("%s: Character Granularity", __FUNCTION__);
	}

	DEBUG_NAV_UI_LOGD("%s: Point after expansion: %d, %d", __FUNCTION__, contentPoint.x(), contentPoint.y());
	DEBUG_NAV_UI_LOGD("%s: WindowPoint: %d, %d", __FUNCTION__, wndPoint.x(), wndPoint.y());
	WebCore::PlatformMouseEvent pme(wndPoint, contentPoint, LeftButton,
			WebCore::MouseEventMoved, 0, false, true, false, false, 0);

	frameview->frame()->eventHandler()->mouseMoved(pme);
	frameview->frame()->eventHandler()->stopAutoscrollTimer();

	DEBUG_NAV_UI_LOGD("%s: End", __FUNCTION__);
	return;
}

//arvind.maan RTL selection fix
bool WebViewCore::recordSelectionCopiedData(SkRegion* prev_region,SkRegion* region, SkIRect* startRect,
		SkIRect* endRect, int granularity ){

	DBG_SET_LOG("start");
	WebCore::Frame* frame =  m_mainFrame;
	WebCore::IntRect box, start, end;
    WTF::Vector<IntRect> boxVector;
	int boxX, boxY, boxWidth, boxHeight, endX, endY, temp;
	bool result = false;

	RefPtr<Range> range;
	//m_contentMutex.lock();
	if ( /*SISO_HTMLCOMPOSER*/ frame->selection()->isRange() && /*SISO_HTMLCOMPOSER*/ (range = frame->selection()->toNormalizedRange()) )
	{
		ExceptionCode ec = 0;
		RefPtr<Range> tempRange = range->cloneRange(ec);

		// consider image selection also while getting the bounds.
        boxVector = tempRange->boundingBoxEx();
        if (!boxVector.isEmpty())
        {
		region->setRect(boxVector[0]);
        for (size_t i = 1; i < boxVector.size(); i++) {
            region->op(boxVector[i], SkRegion::kUnion_Op);
        }
	box=boxVector[0];
	for (size_t i = 1; i < boxVector.size(); ++i){
        	box.unite(boxVector[i]);
	}
	DEBUG_NAV_UI_LOGD("%s: BoundingRect:[%d, %d, %d, %d]", __FUNCTION__, box.x(), box.y(), box.width(), box.height());
           prev_region->setRect(box.x(), box.y(), box.x() + box.width(), box.y() + box.height());
//arvind.maan RTL selection fix
			start = frame->editor()->firstRectForRange(tempRange.get());
			DEBUG_NAV_UI_LOGD("%s: StartRect:[%d, %d, %d, %d]", __FUNCTION__, start.x(), start.y(), start.width(), start.height());
			startRect->set(start.x(), start.y(), start.x() + start.width(),  start.y() + start.height());

			end = frame->editor()->lastRectForRange(tempRange.get());
			DEBUG_NAV_UI_LOGD("%s: EndRect:[%d, %d, %d, %d]", __FUNCTION__, end.x(), end.y(), end.width(), end.height());
			endRect->set(end.x(), end.y(), end.x() + end.width(),  end.y() + end.height());

			// Validation of BOUND RECT X and Y
			// Validate START and END RECTs assuming that BOUND RECT is correct
			boxX = box.x();
			boxY = box.y();
			boxWidth = box.width();
			boxHeight = box.height();
			if (boxX < 0)
			{
				boxX = 0;
			}
			if (boxY < 0)
			{
				boxY = 0;
			}
			if (box.x() < 0 || box.y() < 0)
			{
				region->setRect(boxX, boxY, boxX + boxWidth, boxY + boxHeight);
			}
			// SAMSUNG CHANGE : Fix for Email selection handle error >>
			// Auto fit long text without wrap cases validate lastrect range selection.
			if (frame->selection()->granularity() == WebCore::CharacterGranularity ||
					frame->selection()->granularity() == WebCore::WordGranularity) {
				int boxRight = boxX + boxWidth;
				if ((box.y() == end.y()) && ((end.x() + end.width()) < boxRight)) {
					endRect->set(boxRight - 1, end.y(), boxRight,  end.y() + end.height());
					DEBUG_NAV_UI_LOGD("%s:Validated EndRect:[%d, %d, %d, %d]", __FUNCTION__, end.x(), end.y(), end.width(), end.height());
				}
			}
			// SAMSUNG CHANGE <<
			// Remove the validation : have side effect in selection bound rect
			/*
         // If START RECT is not within BOUND REC,T push the START RECT to LEFT TOP corner of BOUND RECT
         // Also START RECT width and height should not be more than BOUND RECT width and height
         if (!(region->contains(*startRect)))
             {
                 temp = start.height();
                  if (temp > boxHeight)
              {
                temp = boxHeight;
              }
          endX = start.width();
             if (endX > boxWidth)
             {
                endX = boxWidth;
             }
              startRect->set(boxX, boxY, boxX + boxWidth, boxY + temp);
             }

         // If END RECT is not within BOUND RECT, push the END RECT to RIGHT BOTTOM corner of BOUND RECT
         // Also END RECT width and height should not be more than BOUND RECT width and height
         if (!(region->contains(*endRect)))
             {
                   endX = boxX + boxWidth;
           endY = boxY + boxHeight;
           temp = end.height();
           if (temp > boxHeight)
           {
            temp = boxHeight;
           }
           if (end.width() < boxWidth)
           {
            endX = endX - end.width();
           }
           endY = endY - temp;
                  endRect->set(endX, endY, boxX + boxWidth, boxY + boxHeight);
             }
			 */
			//Validation : Text selection is not happend,though engine have selection region bound.
			WTF::String str = getSelectedText();
			if(NULL == str || str.isEmpty() || str == "\n"){
				DEBUG_NAV_UI_LOGD("%s: text Selection is not happend", __FUNCTION__);
			}else{
				result = true;
			}
		}
		else
		{
			DEBUG_NAV_UI_LOGD("%s: Selection Bound Rect is Empty", __FUNCTION__);
			startRect->set(0, 0, 0, 0);
			endRect->set(0, 0, 0, 0);
		}
	}
	else{
		DEBUG_NAV_UI_LOGD("%s: recordSelectionCopiedData  is false", __FUNCTION__);
	}

	granularity = frame->selection()->granularity();
	DEBUG_NAV_UI_LOGD("%s: Granularity: %d", __FUNCTION__, granularity);

	//m_contentMutex.unlock();

	DBG_SET_LOG("end");
   boxVector.clear();//arvind.maan RTL selection fix
	return result;

}

int WebViewCore::getSelectionGranularity()
{
	WebCore::Frame* frame =  m_mainFrame;
	return frame->selection()->granularity();
}

// Adding for Multicolumn text selection - Begin
bool WebViewCore::getSelectionMultiColInfo()
{
	WebCore::Frame* frame =  m_mainFrame;
	bool isMultiColumn = false;
	RefPtr<Range> range;

	if (  frame->selection()->isRange() && (range = frame->selection()->toNormalizedRange()) )
	{
		ExceptionCode ec = 0;
		RefPtr<Range> tempRange = range->cloneRange(ec);

		isMultiColumn = frame->editor()->getMultiColinfoOfSelection(tempRange.get());

		DEBUG_NAV_UI_LOGD("%s: MultiColumn info: %d", __FUNCTION__, isMultiColumn);
	}

	return isMultiColumn;
}
// Adding for Multicolumn text selection - End

//ADVANCED TEXT SELECTION - SAMSUNG + 

bool  WebViewCore::getClosestWord(IntPoint m_globalpos, IntPoint& m_mousePos)
{
    int slop =16;
    Frame* frame = m_mainFrame ;

    m_mousePos = IntPoint(m_globalpos.x() - m_scrollOffsetX,m_globalpos.y() - m_scrollOffsetY);

    HitTestResult hitTestResult = m_mainFrame->eventHandler()->hitTestResultAtPoint(m_globalpos,
            false, false, DontHitTestScrollbars, HitTestRequest::ReadOnly | HitTestRequest::Active, IntSize(slop, slop));

    bool found = false;
    TouchNodeData final ;
    m_mousePosFrame = frame->view()->windowToContents(m_mousePos);


    IntRect testRect(m_mousePosFrame.x() - slop, m_mousePosFrame.y() - slop, 2 * slop + 1, 2 * slop + 1);

     const ListHashSet<RefPtr<Node> >& list = hitTestResult.rectBasedTestResult();

        if (list.isEmpty()) 
	{
            DBG_NAV_LOG("Should not happen: no rect-based-test nodes found");
            return false;
        }

        frame = hitTestResult.innerNode()->document()->frame();
        Vector<TouchNodeData> nodeDataList;
        ListHashSet<RefPtr<Node> >::const_iterator last = list.end();

        for (ListHashSet<RefPtr<Node> >::const_iterator it = list.begin(); it != last; ++it) 
	{
		Node* it_Node = it->get();

		while (it_Node) 
		{
			if (it_Node->nodeType() == Node::TEXT_NODE)
			{
				found = true;
				break;
			}
			else
				it_Node = it_Node->parentNode();
			}

		if (found)
		{
			TouchNodeData newNode;
			newNode.mNode = it_Node;
			IntRect rect = getAbsoluteBoundingBox(it_Node);
			newNode.mBounds = rect;
			nodeDataList.append(newNode);  
		}
		else
			continue;
        }

        
     	//get best intersecting rect
       final.mNode = 0;
       DBG_NAV_LOGD("Test Rect (%d %d %d %d)", testRect.x(), testRect.y(), testRect.width(), testRect.height()) ;


       int area = 0;
       Vector<TouchNodeData>::const_iterator nlast = nodeDataList.end();
       for (Vector<TouchNodeData>::const_iterator n = nodeDataList.begin(); n != nlast; ++n) 
       {
           IntRect rect = n->mBounds;
           rect.intersect(testRect);
           int a = rect.width() * rect.height();
           if (a > area)
           {
               final = *n;
               area = a;
            }
       }


     //Adjust mouse position

      IntPoint frameAdjust = IntPoint(0,0);
      if (frame != m_mainFrame) {
          frameAdjust = frame->view()->contentsToWindow(IntPoint());
          frameAdjust.move(m_scrollOffsetX, m_scrollOffsetY);
      }



      IntRect rect = final.mBounds;


       rect.move(frameAdjust.x(), frameAdjust.y());


     // adjust m_mousePos if it is not inside the returned highlight rectangle

      testRect.move(frameAdjust.x(), frameAdjust.y());


     IntPoint RectSample = IntPoint(testRect.x(), testRect.y());

     testRect.intersect(rect);

// bounding rect of node is area which cover the surrounding area of the text.

	if ((testRect.width()!=0) && (testRect.height()!=0))
	{
		m_mousePos = WebCore::IntPoint(testRect.x(), testRect.y()) ;

		return true;
	}
	else
	{
		return false;
	}

}

//ADVANCED TEXT SELECTION - SAMSUNG - 
bool WebViewCore::selectClosestWord(int x , int y , float zoomLevel, bool flagGranularity){
	DEBUG_NAV_UI_LOGD("%s: x,y position: %d, %d", __FUNCTION__, x, y);
	WebCore::Frame* frame = m_mainFrame;
	WebCore::IntPoint contentsPoint = WebCore::IntPoint(x, y);
	WebCore::IntPoint wndPoint = frame->view()->contentsToWindow(contentsPoint);

	if (!frame->eventHandler())
	{
		DEBUG_NAV_UI_LOGD("%s: Eventhandler is NULL", __FUNCTION__);
		return false;
	}
	DEBUG_NAV_UI_LOGD("First time selection: Zoom Level: %f", zoomLevel);

	//if (zoomLevel >= 0.8)
	{
		WebCore::MouseEventType met1 = WebCore::MouseEventMoved;
		WebCore::PlatformMouseEvent pme1(wndPoint, contentsPoint, NoButton, met1,
				false, false, false, false, false, 0);
// ADVANCED TEXT SELECTION - SAMSUNG +
		bool bReturn;
		bReturn = frame->eventHandler()->sendContextMenuEventForWordSelection(pme1, flagGranularity);

		 SelectionController* selectionContrler = frame->selection();

		if ((true  == bReturn) &&  !(selectionContrler->selection().isRange()))
		{
			IntPoint m_MousePos; 

			if ( true == getClosestWord(contentsPoint,m_MousePos))
			{

				WebCore::IntPoint wndPoint = frame->view()->contentsToWindow(m_MousePos);


				WebCore::PlatformMouseEvent pme2(wndPoint, m_MousePos, NoButton, met1,
											  false, false, false, false, false, 0);


				bReturn = frame->eventHandler()->sendContextMenuEventForWordSelection(pme2, flagGranularity);
			}
		}

		return bReturn;
// ADVANCED TEXT SELECTION - SAMSUNG -
	}
	/*else
	{
		WebCore::MouseEventType met = WebCore::MouseEventPressed;
		WebCore::PlatformMouseEvent pme(wndPoint, contentsPoint, LeftButton, met, 3, false, false, false, false, 0);
		return frame->eventHandler()->handleMousePressEvent(pme);
	}*/
}
// SAMSUNG CHANGE : ADVANCED_TEXT_SELECTION <<

void WebViewCore::scrollRenderLayer(int layer, const SkRect& rect)
{
#if USE(ACCELERATED_COMPOSITING)
    GraphicsLayerAndroid* root = graphicsRootLayer();
    if (!root)
        return;

    LayerAndroid* layerAndroid = root->platformLayer();
    if (!layerAndroid)
        return;

    LayerAndroid* target = layerAndroid->findById(layer);
    if (!target)
        return;

    RenderLayer* owner = target->owningLayer();
    if (!owner)
        return;

    if (owner->stackingContext())
        owner->scrollToOffset(rect.fLeft, rect.fTop);
#endif
}

//SAMSUNG CHANGE +
void WebViewCore::storeAnimationTimer(WebCore::Image* image)
{
    if(animatedImage){
	animatedImage->deref();
	animatedImage = NULL;
    }
    animatedImage = image;
    animatedImage->ref();	
}
// SAMSUNG CHANGE -

//SAMSUNG CHANGE +
void WebViewCore::getWebFeedLinks ( Vector<WebFeedLink*>& out )
{
    WTF::String typeRss ( "application/rss+xml" );
    WTF::String typeRdf ( "application/rdf+xml" );
    WTF::String typeAtom ( "application/atom+xml" );
    WTF::String relAlternate ( "alternate" );

    WebCore::Frame* frame = m_mainFrame ;

    LOGV ( "WebViewCore::getWebFeedLinks()" );

    while ( frame != NULL )
    {

        Document* doc = frame->document() ;

        RefPtr<WebCore::NodeList> links = doc->getElementsByTagName ( "link" ) ;
        int length = links->length() ;

        for ( int i = 0; i < length; i ++ )
        {

            WebCore::HTMLLinkElement *linkElement = static_cast<WebCore::HTMLLinkElement *> ( links->item ( i ) );
            String type = linkElement->type() ;
            String rel = linkElement->rel() ;

            if ( ( ( type == typeRss ) || ( type == typeRdf ) || ( type == typeAtom ) ) && ( rel == relAlternate ) )
            {
                String url = linkElement->href() ;
                String title = linkElement->getAttribute ( WebCore::HTMLNames::titleAttr ) ;

                LOGV ( "WebViewCore::getWebFeedLinks() type=%s, url=%s, title = %s", type.latin1().data(), url.latin1().data(), title.latin1().data() );

                out.append ( new WebFeedLink ( url, title, type) ) ;
            }

        }

        frame = frame->tree()->traverseNext() ;
    }
}
//SAMSUNG CHANGE -

//SAMSUNG CHANGES- EMAIL APP CUSTOMIZATION >>
#define FILTERED_IMAGE_HEIGHT 50
#define FILTERED_IMAGE_WIDTH  50 
bool WebViewCore::getCountImages(int *out_Count)const
{
    Frame* frame = m_mainFrame;
    int count = 0;
    while(frame) {
        PassRefPtr<HTMLCollection> imageCollection = frame->document()->images();

        CachedImage* cachedImage;
        RenderImage *renderImage;
        Node * tempNode;

        for(int i=0; i< imageCollection->length();i++)
        {
            tempNode = imageCollection->item(i) ;
            if(!tempNode)
                continue;
            renderImage = static_cast<RenderImage *>(tempNode->renderer());
            if(!renderImage)
            {
                HTMLImageElement *imageElement = static_cast<HTMLImageElement *>(tempNode);
                cachedImage = imageElement->cachedImage();
                if(!cachedImage)
                    continue;
            }
            else
                cachedImage = renderImage->cachedImage();
            if(!cachedImage)
                continue;

            WebCore::Image* image = cachedImage->image();
            CachedResource *cachedBuffer = static_cast<CachedResource *>(cachedImage);
            if((image && image->data())|| cachedBuffer->data())
            {
                count++;
            }
        }
        frame = frame->tree()->traverseNext();
    }

    *out_Count = count;
    return true;
}

bool WebViewCore::requiresSmartFit()
{
    __android_log_print(ANDROID_LOG_DEBUG,"GNANA"," requiresSmartFit called");
    int imgCount = 0;
    bool bMemFull = false;
    Frame* pFrame = NULL;
    bool m_bSmartFit = false;

    pFrame = m_mainFrame; 
    if(!pFrame){
        __android_log_print(ANDROID_LOG_DEBUG,"GNANA"," requiresSmartFit end1");
        return m_bSmartFit;
    }

    Document* doc = pFrame->document();
    if(!doc){
        __android_log_print(ANDROID_LOG_DEBUG,"GNANA"," requiresSmartFit end2");
        return m_bSmartFit;
    }

    RefPtr<WebCore::NodeList> tableList = doc->getElementsByTagName( "tr" );
    int length = tableList->length();
    if(length > 0) {
        tableList = 0;
        __android_log_print(ANDROID_LOG_DEBUG,"GNANA"," requiresSmartFit end3");
        return m_bSmartFit;
    }
  
    m_bSmartFit=true; 
    __android_log_print(ANDROID_LOG_DEBUG,"GNANA"," requiresSmartFit setting true 1");
    
    //Number of Images in html files
    getCountImages(&imgCount);
    __android_log_print(ANDROID_LOG_DEBUG,"GNANA"," requiresSmartFit images count %d", imgCount );
    
    //In case of image count less than zero smart fit should be applied
    if(imgCount <= 0){
        __android_log_print(ANDROID_LOG_DEBUG,"GNANA"," requiresSmartFit end4");
        return m_bSmartFit;
    }

    //Images collection for all frames in a page
    while(pFrame) {
        //list of images in current DOM
        PassRefPtr<HTMLCollection> listImages = pFrame->document()->images();
        FrameView* view = pFrame->view();
        CachedImage* cImage = NULL;
        RenderImage *rImage = NULL;
        String srcPath;
        Node* ImageNode = NULL;
        int i=0;
        bool isRendered = false ;

        //For every images in a particular frame, finding dimensions
        for (; i< listImages->length() ;i++)
        {
            ImageNode = listImages->item(i);
            if(!ImageNode)
                continue;

            rImage = static_cast<RenderImage *>(ImageNode->renderer());
            if(!rImage) {
                continue;
            }else{
                cImage = rImage->cachedImage();
            }

            if(!cImage) { 
                continue;
            }

            Element* imgElement = static_cast<Element*>(ImageNode);
            const AtomicString& value = imgElement->getAttribute("src");

            srcPath = value.string();
            if( (srcPath.isNull()) || (srcPath.isEmpty()) || (srcPath.length() == 0) ) {
                continue;
            }

            WebCore::Image* pImage = cImage->image();
            IntRect imgRect ;
            isRendered = false ;
            CachedResource *cBuffer = static_cast<CachedResource *>(cImage) ;

            const char *imgData;
            unsigned long imgSize;
            unsigned long imgHeight =0, imgWidth=0;
            if(pImage && pImage->data()) {  
                imgHeight = pImage->height();
                imgWidth  = pImage->width();
                imgData = pImage->data()->data();
                imgSize = pImage->data()->size();
            }else if (cBuffer->data()) {
                imgData = cBuffer->data()->data();
                imgSize = cBuffer->data()->size();
            }else {
                continue;
            }

            //This is actual check to find smart fit or not
            if(pImage && ((pImage->height() < FILTERED_IMAGE_HEIGHT) ||(pImage->width() < FILTERED_IMAGE_WIDTH))) {
                m_bSmartFit = true;
                __android_log_print(ANDROID_LOG_DEBUG,"GNANA"," requiresSmartFit setting true 2");
                continue;
            }else {
                m_bSmartFit = false;
                __android_log_print(ANDROID_LOG_DEBUG,"GNANA"," requiresSmartFit setting false 1");
            }
             
            /* to add later
            SFBalImgCodecType imgCodec = SFBalImgGetDataCodecType( (unsigned char*)(imgData ), imgSize);
            char szExt[5];                 
            GetExtensionFromCodec(imgCodec , szExt);
            if ((SFBalStricmp(szExt, "dat") == 0)) {
                continue;
            }
            */
            if(!m_bSmartFit){
                __android_log_print(ANDROID_LOG_DEBUG,"GNANA"," requiresSmartFit end5");
                return m_bSmartFit;
            }
        }
        pFrame = pFrame->tree()->traverseNext();
      }
      __android_log_print(ANDROID_LOG_DEBUG,"GNANA"," requiresSmartFit end6");
      return m_bSmartFit;
}
//SAMSUGN CHANGES <<

//----------------------------------------------------------------------
// Native JNI methods
//----------------------------------------------------------------------
static void RevealSelection(JNIEnv *env, jobject obj)
{
    GET_NATIVE_VIEW(env, obj)->revealSelection();
}

static jstring RequestLabel(JNIEnv *env, jobject obj, int framePointer,
        int nodePointer)
{
    return wtfStringToJstring(env, GET_NATIVE_VIEW(env, obj)->requestLabel(
            (WebCore::Frame*) framePointer, (WebCore::Node*) nodePointer));
}
//SISO_HTMLCOMPOSER begin
/*
It gets the hittestresult from the main frame , find the innernoe and the renderer.
If the rendered node is an image then it get the Renderblockbound rect which is basically the image region.
 */

WebCore::IntRect WebViewCore::GetHitImageSize( int anchorX , int anchorY)
{
    WebCore::Node* node = 0;
    WebCore::IntPoint anchorPoint;
    anchorPoint = WebCore::IntPoint(anchorX, anchorY);
    WebCore::HitTestResult hitTestResult = m_mainFrame->eventHandler()-> hitTestResultAtPoint(anchorPoint, false, true);
    node = hitTestResult.innerNode();
    WebCore::IntRect webRect ;

    webRect.setX(-1);
    webRect.setY(-1);
    webRect.setWidth(-1);
    webRect.setHeight(-1);

    WebCore::RenderObject *renderer = NULL ;
    if (!node) {
        DBG_NAV_LOG("GetHitImageSize: HitTest Result Node is NULL!");
        return webRect;
    }
    WebCore::RenderObject* nodeRenderer = node->renderer();

    if ( nodeRenderer != NULL && nodeRenderer->isRenderImage())
    {
        webRect=    getBlockBounds(node);
        DBG_NAV_LOGD("getRenderBlockBounds  1  : node=%p result(%d, %d, %d, %d)", node, webRect.x(), webRect.y(), webRect.width(), webRect.height());

        /*Create markup of this node - The markup is used to insert at new position in case of image move -28-9-11*/
        LOGV ( "WebViewCore::GetHitImageSize() - setting image markup datata and size " );
        String markupString = createMarkup(node);
        setImageNodeMarkup( markupString);
        m_SelectedImageNode = node;//ananta

    }
    else
    {
        DBG_NAV_LOG("getRenderBlockBounds: No render block found!      ");
    }

    //test code 
    IntPoint pt = IntPoint(webRect.right(), webRect.bottom());
    imgVisibleSelectionToReInsert = m_mainFrame->visiblePositionForPoint(pt);
    //end 

    return webRect;

}
//HTML Composer Start
WebCore::IntRect WebViewCore::GetSelectedImageSize()
{
	WebCore::Node* node = getSelectedImageNode();
    WebCore::IntRect webRect ;
	webRect.setX(-1);
	webRect.setY(-1);
	webRect.setWidth(-1);
	webRect.setHeight(-1);
    if(!node)
	 return webRect;	
    WebCore::RenderObject* nodeRenderer = node->renderer();
	if(node&&nodeRenderer&&nodeRenderer->isRenderImage())
		return  getBlockBounds(node);		
	return webRect;

}
//HTML Composer End

// SAMSUNG CHANGE ++
// Patch for MPSG100004567 - GA0100507167
bool WebViewCore::IsNodeInIFrame()
{
	WebCore::Node* node = currentFocus();
	//WebCore::RenderObject* renderer;

	if(!node){
	    return false;
	}

	if(mCurrentNode == NULL || mCurrentNode != node){
	    mCurrentNode = node;
	    while(node){
	        if(node->hasTagName(HTMLNames::iframeTag)/*renderer && renderer->isRenderIFrame()*/){
		    mIsNodeInIFrame = true;  
		    return mIsNodeInIFrame;
	    	}
	        if(!(node-> parentNode()) && node->isDocumentNode()/*node->document()*/){
		    node = (WebCore::Node*)node->document()->ownerElement();
	    	}else{
	            node = node->parentNode();
	    	}
	    }
	    mIsNodeInIFrame = false;
	}
	return mIsNodeInIFrame;
}
// SAMSUNG CHANGE --

/*
*ResizeImage - this is the native call initiated from htmlcomposerview.java 
*/

void WebViewCore:: resizeImage(int width  ,int height)
{
    WebCore::RenderObject *renderer = NULL ;
    WebCore::IntRect webRect ;
    
    if (!m_SelectedImageNode) {
        DBG_NAV_LOG("resizeImage: HitTest Result Node is NULL!");
        return ;
    }
    WebCore::RenderObject* nodeRenderer = m_SelectedImageNode->renderer();

    if ( nodeRenderer != NULL && nodeRenderer->isRenderImage())
    {

    __android_log_print(ANDROID_LOG_DEBUG,"WebviewCore","resizeImage called - width =%d  , height = %d ", width ,height);

    ((HTMLImageElement*)m_SelectedImageNode)->setWidth(width);
    ((HTMLImageElement*)m_SelectedImageNode)->setHeight(height);

    }
}

//SISO_HTMLCOMPOSER begin
//getCurrentFontSize - api to get the font size of current selection.
int WebViewCore::getCurrentFontSize()
{
    return m_mainFrame->editor()->getCurrentFontSize(); 
}

//getCurrentFontValue - api to get the font size of current selection in pixels
int WebViewCore::getCurrentFontValue()
{
	return m_mainFrame->editor()->getCurrentFontValue();	
}
//SISO_HTMLCOMPOSER end

//Added for image move feature 
void WebViewCore::saveImageSelectionController(  VisibleSelection imageSelection )
{
    __android_log_print(ANDROID_LOG_DEBUG,"WebviewCore","saveImageSelectionController called ");

    //if(m_VisibleImageSelection == NULL)
    {
        //__android_log_print(ANDROID_LOG_DEBUG,"WebviewCore","saveImageSelectionController m_VisibleImageSelection == NULL so initializing   ");
        m_VisibleImageSelection = VisibleSelection();
    }       
    m_VisibleImageSelection = imageSelection;
}

void WebViewCore::releaseImageSelectionController()
{
    __android_log_print(ANDROID_LOG_DEBUG,"WebviewCore","initImageSelectionController called ");
    m_VisibleImageSelection = VisibleSelection(); // it assign values fro no selection 

}

VisibleSelection  WebViewCore::getImageSelectionController()
{
    __android_log_print(ANDROID_LOG_DEBUG,"WebviewCore","getImageSelectionController called ");
    return m_VisibleImageSelection;
}
WebCore::Node* WebViewCore::getSelectedImageNode()
{
    __android_log_print(ANDROID_LOG_DEBUG,"WebviewCore","getSelectedImageNode called ");
    return m_SelectedImageNode;
}
void WebViewCore::removeSelectedImageNode()
{
    __android_log_print(ANDROID_LOG_DEBUG,"WebviewCore","removeSelectedImageNode called ");
    ExceptionCode e ;

    //imageNode->detach();  
    applyCommand(RemoveNodeCommand::create(m_SelectedImageNode));
    m_SelectedImageNode->remove(e);
    __android_log_print(ANDROID_LOG_DEBUG,"WebviewCore","removeSelectedImageNode - e = %d ", e);


}

void WebViewCore::setImageNodeMarkup(String imagenodemarkup)
{
    imageNodeMarkup = imagenodemarkup;
}
WTF::String WebViewCore::getImageNodeMarkup()
{
    return imageNodeMarkup;
}

void WebViewCore::SetSelectionPreviousImage(VisibleSelection imageSelection )
{
    __android_log_print(ANDROID_LOG_DEBUG,"WebviewCore","SetSelectionPreviousImage called ");

    //if(imageSelection==NULL)
    if(imageSelection.isNone())
        __android_log_print(ANDROID_LOG_DEBUG,"WebviewCore","SetSelectionPreviousImage - imageSelection is NULL     ");
    

    SelectionController* selectionContrler = m_mainFrame->selection();
    if(selectionContrler != NULL && !(selectionContrler->isNone()))
    {
        __android_log_print(ANDROID_LOG_DEBUG,"WebviewCore","selectionContrler->setSelection(imageSelection) called  ");
        selectionContrler->setSelection(imageSelection);
    }   
}


void WebViewCore:: setSelectionToMoveImage(bool isNewPosition){
    
    SelectionController* frameSelectionContrler = m_mainFrame->selection();
    if(frameSelectionContrler == NULL)
        return ;
    VisibleSelection newSelection;
    if(isNewPosition)
        newSelection = VisibleSelection(imageVisiblePosition);
    else
        newSelection = VisibleSelection(imgVisibleSelectionToReInsert);

    //if(!newSelection.isNone())
    //{
     frameSelectionContrler->setSelection(newSelection/*(newSelectionControler.selection()*/); 

    //}
    /*else
    {
        __android_log_print(ANDROID_LOG_DEBUG,"WebviewCore","setSelectionToMoveImage - No selection found to insert image   ");

    }*/
}

//check  the selection- if image can be moved 
bool WebViewCore:: checkSelectionToMoveImage(){

/*  if(imageCanMove){
            __android_log_print(ANDROID_LOG_DEBUG,"WebviewCore","imageCanMove return true   ");
                return true ;
        }
    else
        {
            __android_log_print(ANDROID_LOG_DEBUG,"WebviewCore","imageCanMove return false  ");
            return false;       
        }*/


    SelectionController* frameSelectionContrler = m_mainFrame->selection();
    if(frameSelectionContrler == NULL)
        return false;
    VisibleSelection newSelection;
    newSelection = VisibleSelection(imageVisiblePosition);

    //if(newSelection.isCaret()/*start().isCandidate()*/)
    if(frameSelectionContrler->selection().isCaret())
    {
        __android_log_print(ANDROID_LOG_DEBUG,"WebviewCore","checkSelectionToMoveImage - isCaret -  return true     ");
        return true ;

    }       
    else
    {
        __android_log_print(ANDROID_LOG_DEBUG,"WebviewCore","checkSelectionToMoveImage -isCaret -return false   ");
        return false;
    }
    
}
// HTML Composer start
// Validating the New position while moving image to insert in new position 
bool WebViewCore:: isValidePositionToMoveImage()
{
    VisibleSelection newSelection;
    newSelection = VisibleSelection(imageVisiblePosition);
    Node* anchorNode = newSelection.base().anchorNode();
    Node* imgNode = getSelectedImageNode();
    bool isSameNode=false;
    bool isCaret=checkSelectionToMoveImage();
    if(!anchorNode || !imgNode)
    return isSameNode;
    isSameNode = anchorNode->isSameNode(imgNode);
   return (!isSameNode && isCaret);
}
// HTML Composer end

//SAMSUNG CHANGES >>>
void WebViewCore::setWebTextViewOnOffStatus(bool status) {
	DBG_NAV_LOGD("setWebTextViewOnOffStatus : Setting WebTextView status = %d", status);
	WebCore::Settings* settings = mainFrame()->page()->settings();
	settings->setWebTextViewOnOffStatus(status);
}
//SAMSUNG CHANGES <<<

// image move feature - it deletes the image at ols location and insert into new location -
static void insertImageContent(JNIEnv *env, jobject obj, jstring command )
{
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);

    //Save the selection to display cursor after insertion of image 
    //viewImpl->saveSelectionController();

    if( false==viewImpl->isValidePositionToMoveImage())
    {

        __android_log_print(ANDROID_LOG_DEBUG,"WebviewCore","checkSelectionToMoveImage fails - Image can not be moved   ");
        return;
    }

    //delete using priviously save selecton controller 
    /*
    VisibleSelection imageSelection = viewImpl->getImageSelectionController();
    viewImpl->SetSelectionPreviousImage(imageSelection);
    WTF::String commandToDel = "Delete";
    WTF::String value = "null";
    viewImpl->execCommand(commandToDel,value);
       */
       
    //delete the Selected image node  - using node pointer
    
    viewImpl->removeSelectedImageNode();
    


    if( false==viewImpl->checkSelectionToMoveImage())
    {
        __android_log_print(ANDROID_LOG_DEBUG,"WebviewCore","checkSelectionToMoveImage fails - Image can not be moved viewImpl->setSelectionToMoveImage(false); ");
        viewImpl->setSelectionToMoveImage(false);
    }
    else 
    {
        //Set the selection at previously saved x,y position 
            __android_log_print(ANDROID_LOG_DEBUG,"WebviewCore","checkSelectionToMoveImage fails - Image can be moved viewImpl->setSelectionToMoveImage(true);  ");
        viewImpl->setSelectionToMoveImage(true);
   }

    //insert image at new position
    WTF::String imageNodeMarkup = viewImpl->getImageNodeMarkup();
    WTF::String commandName = jstringToWtfString(env, command); 
    viewImpl->execCommand(commandName , imageNodeMarkup);
//+HTMLCOMPOSERVIEW
//HTML Composer Start
// New Line not required after inserting the image 
/*
    WTF::String value("\n");
    WTF::String commandInsertName("InsertText"); 
    viewImpl->execCommand(commandInsertName , value);
*/
//HTML Composer End
//-HTMLCOMPOSERVIEW
}
//SISO_HTMLCOMPOSER end

static void ClearContent(JNIEnv *env, jobject obj)
{
#ifdef ANDROID_INSTRUMENT
    TimeCounterAuto counter(TimeCounter::WebViewCoreTimeCounter);
#endif
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    viewImpl->clearContent();
}
//SAMSUNG changes - Reader
static jstring applyreadability(JNIEnv *env, jobject obj,jstring flags)
{
    return wtfStringToJstring(env, GET_NATIVE_VIEW(env, obj)->applyreadability(jstringToWtfString(env, flags)));
}
static jstring loadinitialJs(JNIEnv *env, jobject obj,jstring flags)
{
    return wtfStringToJstring(env, GET_NATIVE_VIEW(env, obj)->loadinitialJs(jstringToWtfString(env, flags)));
}
//SAMSUNG changes - Reader

static void UpdateFrameCacheIfLoading(JNIEnv *env, jobject obj)
{
    GET_NATIVE_VIEW(env, obj)->updateFrameCacheIfLoading();
}

static void SetSize(JNIEnv *env, jobject obj, jint width, jint height,
        jint textWrapWidth, jfloat scale, jint screenWidth, jint screenHeight,
        jint anchorX, jint anchorY, jboolean ignoreHeight)
{
#ifdef ANDROID_INSTRUMENT
    TimeCounterAuto counter(TimeCounter::WebViewCoreTimeCounter);
#endif
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    LOGV("webviewcore::nativeSetSize(%u %u)\n viewImpl: %p", (unsigned)width, (unsigned)height, viewImpl);
    LOG_ASSERT(viewImpl, "viewImpl not set in nativeSetSize");
    viewImpl->setSizeScreenWidthAndScale(width, height, textWrapWidth, scale,
            screenWidth, screenHeight, anchorX, anchorY, ignoreHeight);
}

static void SetScrollOffset(JNIEnv *env, jobject obj, jint gen, jboolean sendScrollEvent, jint x, jint y)
{
#ifdef ANDROID_INSTRUMENT
    TimeCounterAuto counter(TimeCounter::WebViewCoreTimeCounter);
#endif
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    LOG_ASSERT(viewImpl, "need viewImpl");

    viewImpl->setScrollOffset(gen, sendScrollEvent, x, y);
}

static void SetGlobalBounds(JNIEnv *env, jobject obj, jint x, jint y, jint h,
                            jint v)
{
#ifdef ANDROID_INSTRUMENT
    TimeCounterAuto counter(TimeCounter::WebViewCoreTimeCounter);
#endif
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    LOG_ASSERT(viewImpl, "need viewImpl");

    viewImpl->setGlobalBounds(x, y, h, v);
}

static jboolean Key(JNIEnv *env, jobject obj, jint keyCode, jint unichar,
        jint repeatCount, jboolean isShift, jboolean isAlt, jboolean isSym,
        jboolean isDown)
{
#ifdef ANDROID_INSTRUMENT
    TimeCounterAuto counter(TimeCounter::WebViewCoreTimeCounter);
#endif
    return GET_NATIVE_VIEW(env, obj)->key(PlatformKeyboardEvent(keyCode,
        unichar, repeatCount, isDown, isShift, isAlt, isSym));
}

static void Click(JNIEnv *env, jobject obj, int framePtr, int nodePtr, jboolean fake)
{
#ifdef ANDROID_INSTRUMENT
    TimeCounterAuto counter(TimeCounter::WebViewCoreTimeCounter);
#endif
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    LOG_ASSERT(viewImpl, "viewImpl not set in Click");

    viewImpl->click(reinterpret_cast<WebCore::Frame*>(framePtr),
        reinterpret_cast<WebCore::Node*>(nodePtr), fake);
}
//SAMSUNG CHANGE: <MPSG100003899> xhtml page zoom scale change issue on orientation change and back key press >>
static void RecalcWidthAndForceLayout(JNIEnv *env, jobject obj)
{
    GET_NATIVE_VIEW(env, obj)->recalcWidthAndForceLayout();
}
//SAMSUNG CHANGE: <MPSG100003899> xhtml page zoom scale change issue on orientation change and back key press <<
static void ContentInvalidateAll(JNIEnv *env, jobject obj)
{
    GET_NATIVE_VIEW(env, obj)->contentInvalidateAll();
}

static void DeleteSelection(JNIEnv *env, jobject obj, jint start, jint end,
        jint textGeneration)
{
#ifdef ANDROID_INSTRUMENT
    TimeCounterAuto counter(TimeCounter::WebViewCoreTimeCounter);
#endif
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    viewImpl->deleteSelection(start, end, textGeneration);
}

static void SetSelection(JNIEnv *env, jobject obj, jint start, jint end)
{
#ifdef ANDROID_INSTRUMENT
    TimeCounterAuto counter(TimeCounter::WebViewCoreTimeCounter);
#endif
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    viewImpl->setSelection(start, end);
}

static jstring ModifySelection(JNIEnv *env, jobject obj, jint direction, jint granularity)
{
#ifdef ANDROID_INSTRUMENT
    TimeCounterAuto counter(TimeCounter::WebViewCoreTimeCounter);
#endif
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    String selectionString = viewImpl->modifySelection(direction, granularity);
    return wtfStringToJstring(env, selectionString);
}

static void ReplaceTextfieldText(JNIEnv *env, jobject obj,
    jint oldStart, jint oldEnd, jstring replace, jint start, jint end,
    jint textGeneration)
{
#ifdef ANDROID_INSTRUMENT
    TimeCounterAuto counter(TimeCounter::WebViewCoreTimeCounter);
#endif
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    WTF::String webcoreString = jstringToWtfString(env, replace);
    viewImpl->replaceTextfieldText(oldStart,
            oldEnd, webcoreString, start, end, textGeneration);
}

static void PassToJs(JNIEnv *env, jobject obj,
    jint generation, jstring currentText, jint keyCode,
    jint keyValue, jboolean down, jboolean cap, jboolean fn, jboolean sym)
{
#ifdef ANDROID_INSTRUMENT
    TimeCounterAuto counter(TimeCounter::WebViewCoreTimeCounter);
#endif
    WTF::String current = jstringToWtfString(env, currentText);
    GET_NATIVE_VIEW(env, obj)->passToJs(generation, current,
        PlatformKeyboardEvent(keyCode, keyValue, 0, down, cap, fn, sym));
}

static void ScrollFocusedTextInput(JNIEnv *env, jobject obj, jfloat xPercent,
    jint y)
{
#ifdef ANDROID_INSTRUMENT
    TimeCounterAuto counter(TimeCounter::WebViewCoreTimeCounter);
#endif
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    viewImpl->scrollFocusedTextInput(xPercent, y);
}

static void SetFocusControllerActive(JNIEnv *env, jobject obj, jboolean active)
{
#ifdef ANDROID_INSTRUMENT
    TimeCounterAuto counter(TimeCounter::WebViewCoreTimeCounter);
#endif
    LOGV("webviewcore::nativeSetFocusControllerActive()\n");
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    LOG_ASSERT(viewImpl, "viewImpl not set in nativeSetFocusControllerActive");
    viewImpl->setFocusControllerActive(active);
}

static void SaveDocumentState(JNIEnv *env, jobject obj, jint frame)
{
#ifdef ANDROID_INSTRUMENT
    TimeCounterAuto counter(TimeCounter::WebViewCoreTimeCounter);
#endif
    LOGV("webviewcore::nativeSaveDocumentState()\n");
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    LOG_ASSERT(viewImpl, "viewImpl not set in nativeSaveDocumentState");
    viewImpl->saveDocumentState((WebCore::Frame*) frame);
}

void WebViewCore::addVisitedLink(const UChar* string, int length)
{
    if (m_groupForVisitedLinks)
        m_groupForVisitedLinks->addVisitedLink(string, length);
}

static bool UpdateLayers(JNIEnv *env, jobject obj, jint nativeClass, jint jbaseLayer)
{
    WebViewCore* viewImpl = (WebViewCore*) nativeClass;
    BaseLayerAndroid* baseLayer = (BaseLayerAndroid*)  jbaseLayer;
    if (baseLayer) {
        LayerAndroid* root = static_cast<LayerAndroid*>(baseLayer->getChild(0));
        if (root)
            return viewImpl->updateLayers(root);
    }
    return true;
}

static void NotifyAnimationStarted(JNIEnv *env, jobject obj, jint nativeClass)
{
    WebViewCore* viewImpl = (WebViewCore*) nativeClass;
    viewImpl->notifyAnimationStarted();
}

static jint RecordContent(JNIEnv *env, jobject obj, jobject region, jobject pt)
{
#ifdef ANDROID_INSTRUMENT
    TimeCounterAuto counter(TimeCounter::WebViewCoreTimeCounter);
#endif
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    SkRegion* nativeRegion = GraphicsJNI::getNativeRegion(env, region);
    SkIPoint nativePt;
    BaseLayerAndroid* result = viewImpl->recordContent(nativeRegion, &nativePt);
    GraphicsJNI::ipoint_to_jpoint(nativePt, env, pt);
    return reinterpret_cast<jint>(result);
}

static void SplitContent(JNIEnv *env, jobject obj, jint content)
{
#ifdef ANDROID_INSTRUMENT
    TimeCounterAuto counter(TimeCounter::WebViewCoreTimeCounter);
#endif
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    viewImpl->splitContent(reinterpret_cast<PictureSet*>(content));
}

static void SendListBoxChoice(JNIEnv* env, jobject obj, jint choice)
{
#ifdef ANDROID_INSTRUMENT
    TimeCounterAuto counter(TimeCounter::WebViewCoreTimeCounter);
#endif
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    LOG_ASSERT(viewImpl, "viewImpl not set in nativeSendListBoxChoice");
    viewImpl->popupReply(choice);
}

// Set aside a predetermined amount of space in which to place the listbox
// choices, to avoid unnecessary allocations.
// The size here is arbitrary.  We want the size to be at least as great as the
// number of items in the average multiple-select listbox.
#define PREPARED_LISTBOX_STORAGE 10

static void SendListBoxChoices(JNIEnv* env, jobject obj, jbooleanArray jArray,
        jint size)
{
#ifdef ANDROID_INSTRUMENT
    TimeCounterAuto counter(TimeCounter::WebViewCoreTimeCounter);
#endif
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    LOG_ASSERT(viewImpl, "viewImpl not set in nativeSendListBoxChoices");
    jboolean* ptrArray = env->GetBooleanArrayElements(jArray, 0);
    SkAutoSTMalloc<PREPARED_LISTBOX_STORAGE, int> storage(size);
    int* array = storage.get();
    int count = 0;
    for (int i = 0; i < size; i++) {
        if (ptrArray[i]) {
            array[count++] = i;
        }
    }
    env->ReleaseBooleanArrayElements(jArray, ptrArray, JNI_ABORT);
    viewImpl->popupReply(array, count);
}

static jstring FindAddress(JNIEnv *env, jobject obj, jstring addr,
    jboolean caseInsensitive)
{
#ifdef ANDROID_INSTRUMENT
    TimeCounterAuto counter(TimeCounter::WebViewCoreTimeCounter);
#endif
    if (!addr)
        return 0;
    int length = env->GetStringLength(addr);
    if (!length)
        return 0;
    const jchar* addrChars = env->GetStringChars(addr, 0);
    int start, end;
    bool success = CacheBuilder::FindAddress(addrChars, length,
        &start, &end, caseInsensitive) == CacheBuilder::FOUND_COMPLETE;
    jstring ret = 0;
    if (success)
        ret = env->NewString(addrChars + start, end - start);
    env->ReleaseStringChars(addr, addrChars);
    return ret;
}

static jboolean HandleTouchEvent(JNIEnv *env, jobject obj, jint action, jintArray idArray,
                                 jintArray xArray, jintArray yArray,
                                 jint count, jint actionIndex, jint metaState)
{
#ifdef ANDROID_INSTRUMENT
    TimeCounterAuto counter(TimeCounter::WebViewCoreTimeCounter);
#endif
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    LOG_ASSERT(viewImpl, "viewImpl not set in %s", __FUNCTION__);
    jint* ptrIdArray = env->GetIntArrayElements(idArray, 0);
    jint* ptrXArray = env->GetIntArrayElements(xArray, 0);
    jint* ptrYArray = env->GetIntArrayElements(yArray, 0);
    Vector<int> ids(count);
    Vector<IntPoint> points(count);
    for (int c = 0; c < count; c++) {
        ids[c] = ptrIdArray[c];
        points[c].setX(ptrXArray[c]);
        points[c].setY(ptrYArray[c]);
    }
    env->ReleaseIntArrayElements(idArray, ptrIdArray, JNI_ABORT);
    env->ReleaseIntArrayElements(xArray, ptrXArray, JNI_ABORT);
    env->ReleaseIntArrayElements(yArray, ptrYArray, JNI_ABORT);

    return viewImpl->handleTouchEvent(action, ids, points, actionIndex, metaState);
}

static void TouchUp(JNIEnv *env, jobject obj, jint touchGeneration,
        jint frame, jint node, jint x, jint y, 
//LIGHT TOUCH
        jboolean useLT)
//LIGHT TOUCH        
{
#ifdef ANDROID_INSTRUMENT
    TimeCounterAuto counter(TimeCounter::WebViewCoreTimeCounter);
#endif
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    LOG_ASSERT(viewImpl, "viewImpl not set in %s", __FUNCTION__);
    viewImpl->touchUp(touchGeneration,
        (WebCore::Frame*) frame, (WebCore::Node*) node, x, y
//LIGHT TOUCH
        , useLT);
//LIGHT TOUCH    
}

static jstring RetrieveHref(JNIEnv *env, jobject obj, jint x, jint y)
{
#ifdef ANDROID_INSTRUMENT
    TimeCounterAuto counter(TimeCounter::WebViewCoreTimeCounter);
#endif
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    LOG_ASSERT(viewImpl, "viewImpl not set in %s", __FUNCTION__);
    WTF::String result = viewImpl->retrieveHref(x, y);
    if (!result.isEmpty())
        return wtfStringToJstring(env, result);
    return 0;
}

static jstring RetrieveAnchorText(JNIEnv *env, jobject obj, jint x, jint y)
{
#ifdef ANDROID_INSTRUMENT
    TimeCounterAuto counter(TimeCounter::WebViewCoreTimeCounter);
#endif
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    LOG_ASSERT(viewImpl, "viewImpl not set in %s", __FUNCTION__);
    WTF::String result = viewImpl->retrieveAnchorText(x, y);
    if (!result.isEmpty())
        return wtfStringToJstring(env, result);
    return 0;
}

static jstring RetrieveImageSource(JNIEnv *env, jobject obj, jint x, jint y)
{
    WTF::String result = GET_NATIVE_VIEW(env, obj)->retrieveImageSource(x, y);
    return !result.isEmpty() ? wtfStringToJstring(env, result) : 0;
}

static void StopPaintingCaret(JNIEnv *env, jobject obj)
{
    GET_NATIVE_VIEW(env, obj)->setShouldPaintCaret(false);
}

static void MoveFocus(JNIEnv *env, jobject obj, jint framePtr, jint nodePtr)
{
#ifdef ANDROID_INSTRUMENT
    TimeCounterAuto counter(TimeCounter::WebViewCoreTimeCounter);
#endif
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    LOG_ASSERT(viewImpl, "viewImpl not set in %s", __FUNCTION__);
    viewImpl->moveFocus((WebCore::Frame*) framePtr, (WebCore::Node*) nodePtr);
}

static void MoveMouse(JNIEnv *env, jobject obj, jint frame,
        jint x, jint y)
{
#ifdef ANDROID_INSTRUMENT
    TimeCounterAuto counter(TimeCounter::WebViewCoreTimeCounter);
#endif
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    LOG_ASSERT(viewImpl, "viewImpl not set in %s", __FUNCTION__);
    viewImpl->moveMouse((WebCore::Frame*) frame, x, y);
}

static void MoveMouseIfLatest(JNIEnv *env, jobject obj, jint moveGeneration,
        jint frame, jint x, jint y)
{
#ifdef ANDROID_INSTRUMENT
    TimeCounterAuto counter(TimeCounter::WebViewCoreTimeCounter);
#endif
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    LOG_ASSERT(viewImpl, "viewImpl not set in %s", __FUNCTION__);
    viewImpl->moveMouseIfLatest(moveGeneration,
        (WebCore::Frame*) frame, x, y);
}

static void UpdateFrameCache(JNIEnv *env, jobject obj)
{
#ifdef ANDROID_INSTRUMENT
    TimeCounterAuto counter(TimeCounter::WebViewCoreTimeCounter);
#endif
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    LOG_ASSERT(viewImpl, "viewImpl not set in %s", __FUNCTION__);
    viewImpl->updateFrameCache();
}

static jint GetContentMinPrefWidth(JNIEnv *env, jobject obj)
{
#ifdef ANDROID_INSTRUMENT
    TimeCounterAuto counter(TimeCounter::WebViewCoreTimeCounter);
#endif
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    LOG_ASSERT(viewImpl, "viewImpl not set in %s", __FUNCTION__);

    WebCore::Frame* frame = viewImpl->mainFrame();
    if (frame) {
        WebCore::Document* document = frame->document();
        if (document) {
            WebCore::RenderObject* renderer = document->renderer();
            if (renderer && renderer->isRenderView()) {
                return renderer->minPreferredLogicalWidth();
            }
        }
    }
    return 0;
}

static void SetViewportSettingsFromNative(JNIEnv *env, jobject obj)
{
#ifdef ANDROID_INSTRUMENT
    TimeCounterAuto counter(TimeCounter::WebViewCoreTimeCounter);
#endif
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    LOG_ASSERT(viewImpl, "viewImpl not set in %s", __FUNCTION__);

    WebCore::Settings* s = viewImpl->mainFrame()->page()->settings();
    if (!s)
        return;

#ifdef ANDROID_META_SUPPORT
    env->SetIntField(obj, gWebViewCoreFields.m_viewportWidth, s->viewportWidth());
    env->SetIntField(obj, gWebViewCoreFields.m_viewportHeight, s->viewportHeight());
    env->SetIntField(obj, gWebViewCoreFields.m_viewportInitialScale, s->viewportInitialScale());
    env->SetIntField(obj, gWebViewCoreFields.m_viewportMinimumScale, s->viewportMinimumScale());
    env->SetIntField(obj, gWebViewCoreFields.m_viewportMaximumScale, s->viewportMaximumScale());
    env->SetBooleanField(obj, gWebViewCoreFields.m_viewportUserScalable, s->viewportUserScalable());
    env->SetIntField(obj, gWebViewCoreFields.m_viewportDensityDpi, s->viewportTargetDensityDpi());
#endif
}

static void SetBackgroundColor(JNIEnv *env, jobject obj, jint color)
{
#ifdef ANDROID_INSTRUMENT
    TimeCounterAuto counter(TimeCounter::WebViewCoreTimeCounter);
#endif
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    LOG_ASSERT(viewImpl, "viewImpl not set in %s", __FUNCTION__);

    viewImpl->setBackgroundColor((SkColor) color);
}

// SAMSUNG CHANGE : ADVANCED TEXT SELECTION >>
static jstring GetSelectedText(JNIEnv *env, jobject obj)
{
#ifdef ANDROID_INSTRUMENT
	TimeCounterAuto counter(TimeCounter::WebViewCoreTimeCounter);
#endif
	WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
	LOG_ASSERT(viewImpl, "viewImpl not set in %s", __FUNCTION__);
	WTF::String result = viewImpl->getSelectedText();
	if (!result.isEmpty())
		return wtfStringToJstring(env, result);
	return 0;
}
//SISO_HTMLCOMPOSER begin
static void SimulateDelKeyForCount(JNIEnv *env, jobject obj, jint count)
{
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    viewImpl->simulateDelKeyForCount(count);
}

static jstring GetTextAroundCursor(JNIEnv *env, jobject obj, jint count , jboolean isBefore)
{
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    WTF::String result = viewImpl->getTextAroundCursor(count, isBefore);
    if (!result.isEmpty())
        return wtfStringToJstring(env, result);
    return 0;
}


static void DeleteSurroundingText(JNIEnv *env, jobject obj, jint left , jint right)
{
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    viewImpl->deleteSurroundingText(left , right);
}


static jobject InsertContent(JNIEnv *env, jobject obj,jstring content,jint newcursor, jboolean composing , jobject spanObj)
{
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    WTF::String valueString = jstringToWtfString(env, content);
    int startOffset = -1;
    int endOffset = -1;

    Vector<CompositionUnderline> spanDataVec;
    jclass vector_clazz = 0;
    jmethodID vector_sizeID = 0;
    jmethodID vector_elementAtID = 0;
    vector_clazz = env->FindClass ( "java/util/Vector" );
    vector_sizeID = env->GetMethodID(vector_clazz, "size", "()I");
    vector_elementAtID = env->GetMethodID(vector_clazz, "elementAt", "(I)Ljava/lang/Object;");

    jint vecSize = env->CallIntMethod(spanObj , vector_sizeID);
    for(int dataCnt = 0 ; dataCnt < vecSize; dataCnt++)
    {
        jobject spanData = env->CallObjectMethod(spanObj , vector_elementAtID , dataCnt);
        jclass spanInfo_clazz = 0;
        spanInfo_clazz = env->FindClass ( "android/webkit/HtmlComposerInputConnection$SpanDataInfo");
        jfieldID startOffsetField = env->GetFieldID(spanInfo_clazz , "startOffset", "I");
        jfieldID endOffsetField = env->GetFieldID(spanInfo_clazz , "endOffset", "I");
        jfieldID highLightColorField = env->GetFieldID(spanInfo_clazz , "highLightColor", "I");
        jfieldID isHighlightColorField = env->GetFieldID(spanInfo_clazz , "isHighlightColor", "Z");
        jfieldID underlineColorField = env->GetFieldID(spanInfo_clazz , "underlineColor", "I");
        jfieldID underlineThicknessField = env->GetFieldID(spanInfo_clazz , "underlineThickness", "F");


        CompositionUnderline compositionDeco;
        compositionDeco.startOffset = env->GetIntField(spanData , startOffsetField);
        compositionDeco.endOffset = env->GetIntField(spanData , endOffsetField);
        compositionDeco.isHighlightColor = env->GetBooleanField(spanData , isHighlightColorField);
        compositionDeco.thickness = env->GetFloatField(spanData, underlineThicknessField);

        int color = env->GetIntField(spanData , highLightColorField);


        jclass integer_clazz = 0;
        jmethodID integer_toHexString = 0;

        integer_clazz = env->FindClass( "java/lang/Integer" );
        integer_toHexString = env->GetStaticMethodID(integer_clazz, "toHexString", "(I)Ljava/lang/String;");

        jstring highlightColor = (jstring) env->CallStaticObjectMethod(integer_clazz , integer_toHexString , color);
        WTF::String strHighlightColor = jstringToWtfString(env, highlightColor);

        while(strHighlightColor.length() < 6)
        {
            WTF::String zero("0");
            strHighlightColor.insert(zero , 0);
        }
        WTF::String rStr = strHighlightColor.substring(0 , 2);
        WTF::String gStr = strHighlightColor.substring(2 , 2);
        WTF::String bStr = strHighlightColor.substring(4 , 2);

        LOGD("strHighlightColor rStr : %s gStr : %s bStr : %s " , rStr.utf8().data() , gStr.utf8().data() , bStr.utf8().data());
        int rVal = rStr.toIntStrict(0 , 16);
        int gVal = gStr.toIntStrict(0 , 16);
        int bVal = bStr.toIntStrict(0 , 16);
        WebCore::Color webcoreHLColor = Color(rVal , gVal , bVal );
        LOGD("webcoreHLColor rVal : %d gVal : %d bVal : %d " , rVal , gVal , bVal);
        LOGD("strHighlightColor %s " , strHighlightColor.utf8().data());
        compositionDeco.highLightColor = webcoreHLColor;

        color = env->GetIntField(spanData , underlineColorField);

        jstring underlineColor = (jstring) env->CallStaticObjectMethod(integer_clazz , integer_toHexString , color);
        WTF::String strUnderlineColor = jstringToWtfString(env, underlineColor);

        while(strUnderlineColor.length() < 6)
        {
            WTF::String zero("0");
            strUnderlineColor.insert(zero , 0);
        }
        rStr = strUnderlineColor.substring(0 , 2);
        gStr = strUnderlineColor.substring(2 , 2);
        bStr = strUnderlineColor.substring(4 , 2);

        LOGD("strUnderlineColor rStr : %s gStr : %s bStr : %s " , rStr.utf8().data() , gStr.utf8().data() , bStr.utf8().data());
        rVal = rStr.toIntStrict(0 , 16);
        gVal = gStr.toIntStrict(0 , 16);
        bVal = bStr.toIntStrict(0 , 16);
        WebCore::Color webcoreULColor = Color(rVal , gVal , bVal );
        LOGD("webcoreULColor rVal : %d gVal : %d bVal : %d " , rVal , gVal , bVal);
        LOGD("strUnderlineColor %s " , strUnderlineColor.utf8().data());
        compositionDeco.color = webcoreULColor;

        spanDataVec.append(compositionDeco);

        //highLightColor

        //int startOffset;
        //        int endOffset;
        //      String highLightColor;
        //  boolean isHighlightColor;

    }




    viewImpl->insertContent(valueString,newcursor, composing,spanDataVec,startOffset,endOffset);
    jclass selectionOffset_clazz = 0;
    jmethodID selectionOffset_initID = 0;

    selectionOffset_clazz = env->FindClass ( "android/graphics/Point" );
    selectionOffset_initID = env->GetMethodID ( selectionOffset_clazz, "<init>", "(II)V" );
    jobject jselectionOffset_Object ;
    jselectionOffset_Object = env->NewObject ( selectionOffset_clazz, selectionOffset_initID , startOffset , endOffset);
    env->DeleteLocalRef(selectionOffset_clazz);
    return jselectionOffset_Object;
}
//annata.k
static void  GetSelectionOffsetImage(JNIEnv *env, jobject obj,    jint left , jint top , jint right , jint bottom)
{
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    viewImpl->getSelectionOffsetImage( left ,  top ,  right , bottom);
    return ;
}

static jobject GetSelectionOffset(JNIEnv *env, jobject obj)
{
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    int startOffset = -1;
    int endOffset = -1;
    viewImpl->getSelectionOffset(startOffset , endOffset);

    jclass selectionOffset_clazz = 0;
    jmethodID selectionOffset_initID = 0;

    selectionOffset_clazz = env->FindClass ( "android/graphics/Point" );
    selectionOffset_initID = env->GetMethodID ( selectionOffset_clazz, "<init>", "(II)V" );
    jobject jselectionOffset_Object ;
    jselectionOffset_Object = env->NewObject ( selectionOffset_clazz, selectionOffset_initID , startOffset , endOffset);
    env->DeleteLocalRef(selectionOffset_clazz);
    return jselectionOffset_Object;
}

static jstring GetBodyText(JNIEnv *env, jobject obj)
{
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    WTF::String result = viewImpl->getBodyText();
    if (!result.isEmpty())
        return wtfStringToJstring(env, result);
    return 0;
}

static jstring GetBodyHTML(JNIEnv *env, jobject obj)
{
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    WTF::String result = viewImpl->getBodyHTML();
    if (!result.isEmpty())
        return wtfStringToJstring(env, result);
    return 0;
}

static bool GetBodyEmpty(JNIEnv *env, jobject obj)
{
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    return viewImpl->getBodyEmpty();
}



static int ContentSelectionType(JNIEnv *env, jobject obj)
{
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    return viewImpl->contentSelectionType();
}

static void UpdateIMSelection(JNIEnv *env, jobject obj,jint curStr , jint curEnd)
{
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    viewImpl->updateIMSelection(curStr  , curEnd);
}

static bool ExecCommand(JNIEnv *env, jobject obj, jstring command , jstring value)
{
    WTF::String commandString = jstringToWtfString(env, command);
    WTF::String valueString = jstringToWtfString(env, value);
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    return viewImpl->execCommand(commandString , valueString);

}

static bool CanUndo(JNIEnv *env, jobject obj)
{
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    return viewImpl->canUndo();
}

static bool CanRedo(JNIEnv *env, jobject obj)
{
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    return viewImpl->canRedo();
}

static void UndoRedoStateReset(JNIEnv *env, jobject obj)
{
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    viewImpl->undoRedoStateReset();
}

static void ResizeImage(JNIEnv *env, jobject obj,jint width , jint height)
{
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    viewImpl->resizeImage(width  , height);
}

static int GetCurrentFontSize(JNIEnv *env, jobject obj)
{
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    return viewImpl->getCurrentFontSize();
}

static int GetCurrentFontValue(JNIEnv *env, jobject obj)
{
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    return viewImpl->getCurrentFontValue();
}

static bool CopyAndSaveImage(JNIEnv *env, jobject obj, jstring url)
{
    WTF::String urlString = jstringToWtfString(env, url);
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    return viewImpl->copyAndSaveImage(urlString);

}

// SAMSUNG CHANGE >>
static bool SaveCachedImageToFile(JNIEnv *env, jobject obj, jstring url, jstring filePath)
{
    WTF::String strUrl = jstringToWtfString(env, url);
    WTF::String strPath = jstringToWtfString(env, filePath);

    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    return viewImpl->saveCachedImageToFile( strUrl, strPath );
}
// SAMSUNG CHANGE <<

static jobject GetFullMarkupData(JNIEnv* env, jobject obj )
{
    jclass markupData_clazz = 0;
    jmethodID markupData_initID = 0;
    jobject jmarkup_Object ;

    jclass subPart_clazz = 0;
    jmethodID subPart_initID = 0;

    jclass vector_clazz = 0;
    jmethodID vector_initID = 0;
    jobject vector_Object;

    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    WebHTMLMarkupData* markupData = viewImpl->getFullMarkupData();

    //ANDROID_LOG_PRINT(ANDROID_LOG_DEBUG, "HTML_EDIT", " WebViewCore::nativeGetFullMarkupData() markupData = %p", markupData);

    //Create MarkupData Object
    markupData_clazz = env->FindClass ( "android/webkit/WebHTMLMarkupData" );
    markupData_initID = env->GetMethodID ( markupData_clazz, "<init>", "()V" );
    jmarkup_Object = env->NewObject ( markupData_clazz, markupData_initID);
    LOGV ( "WebViewCore::nativeGetFullMarkupData() MarkupData Object Created ");

    //Create Vector Class
    vector_clazz = env->FindClass ( "java/util/Vector" );
    vector_initID = env->GetMethodID ( vector_clazz, "<init>", "()V" );
    vector_Object = env->NewObject ( vector_clazz, vector_initID);
    LOGV ( "WebViewCore::nativeGetFullMarkupData() Vector Object Created ");

    //Find Class and Method ID for SubPart
    subPart_clazz = env->FindClass ( "android/webkit/WebSubPart" );
    subPart_initID = env->GetMethodID ( subPart_clazz, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;J)V" ); //String,String,String,String,String,long

    if(markupData != 0){

        //Fill the Vector Object with the SupPart Data
        Vector<WebSubPart> supPartList = markupData->subPartList();
        for(int i =0;i<supPartList.size();i++){
            jobject subPart_Object;
            WebSubPart subPart = supPartList[i];

            //Create SubPart Object
            jstring j_cid = env->NewString( subPart.cid().characters(), subPart.cid().length());
            jstring j_file =  env->NewString( subPart.fileName().characters(), subPart.fileName().length());
            jstring j_mime = env->NewString( subPart.mimeType().characters(), subPart.mimeType().length());
            jstring j_path = env->NewString( subPart.path().characters(), subPart.path().length());
            jstring j_uri = env->NewString( subPart.contentUri().characters(), subPart.contentUri().length());

            subPart_Object = env->NewObject ( subPart_clazz, subPart_initID, j_cid, j_file, j_mime, j_path, j_uri, subPart.fileSize());

            //Add SubPart Object to the Vector,  Adds the specified component to the end of the vector, increasing its size by one.
            env->CallVoidMethod(vector_Object, env->GetMethodID(vector_clazz, "addElement", "(Ljava/lang/Object;)V"), subPart_Object);

            env->DeleteLocalRef(j_cid);
            env->DeleteLocalRef(j_file);
            env->DeleteLocalRef(j_mime);
            env->DeleteLocalRef(j_path);
            env->DeleteLocalRef(j_uri);
            env->DeleteLocalRef(subPart_Object);
        }

        //Set the Vector Object to the WebHTMLMarkupData
        env->CallVoidMethod(jmarkup_Object, env->GetMethodID(markupData_clazz, "setSubPartList", "(Ljava/util/Vector;)V"), vector_Object);
        env->DeleteLocalRef(vector_Object);

        //Set the HTMLFragment to the WebHTMLMarkupData
        jstring j_html = env->NewString(  markupData->htmlFragment().characters(), markupData->htmlFragment().length());
        env->CallVoidMethod(jmarkup_Object, env->GetMethodID(markupData_clazz, "setHTMLFragment", "(Ljava/lang/String;)V"), j_html);
        env->DeleteLocalRef(j_html);

        jstring j_plain= env->NewString(  markupData->plainText().characters(), markupData->plainText().length());
        env->CallVoidMethod(jmarkup_Object, env->GetMethodID(markupData_clazz, "setPlainText", "(Ljava/lang/String;)V"), j_plain);
        env->DeleteLocalRef(j_plain);

        env->DeleteLocalRef(markupData_clazz);
        env->DeleteLocalRef(vector_clazz);
        env->DeleteLocalRef(subPart_clazz);

        //Currently CharSet is ignored, will be added after confirmation
        delete markupData;
    }

    return jmarkup_Object;

}

static jobject GetCursorRect(JNIEnv *env, jobject obj , jboolean giveContentRect)
{
#ifdef ANDROID_INSTRUMENT
    TimeCounterAuto counter(TimeCounter::WebViewCoreTimeCounter);
#endif
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    LOG_ASSERT(viewImpl, "viewImpl not set in nativeSetSize");

    WebCore::IntRect webRect = viewImpl->getCursorRect(giveContentRect);

    jclass rectClass = env->FindClass("android/graphics/Rect");
    LOG_ASSERT(rectClass, "Could not find Rect class!");

    jmethodID init = env->GetMethodID(rectClass, "<init>", "(IIII)V");
    LOG_ASSERT(init, "Could not find constructor for Rect");

    jobject rect = env->NewObject(rectClass, init, webRect.x(),
            webRect.y(), webRect.right(), webRect.bottom());
    return rect ;
}


static void SetSelectionNone(JNIEnv *env, jobject obj)
{
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    viewImpl->setSelectionNone();
}

static bool GetSelectionNone(JNIEnv *env, jobject obj)
{
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    return viewImpl->getSelectionNone();
}





static void SetComposingSelectionNone(JNIEnv *env, jobject obj)
{
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    viewImpl->setComposingSelectionNone();
}
static void SetEditable(JNIEnv *env, jobject obj , jboolean isEditable)
{
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    viewImpl->setEditable(isEditable);
}

static void SetSelectionEditable(JNIEnv *env, jobject obj, jint start , jint end)
{
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    viewImpl->setSelectionEditable(start , end);
}
static void SetSelectionEditableImage(JNIEnv *env, jobject obj, jint start , jint end)
{
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    viewImpl->setSelectionEditableImage(start , end);
}
static void SetComposingRegion(JNIEnv *env, jobject obj, jint start , jint end)
{
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    viewImpl->setComposingRegion(start , end);
}
static void MoveSingleCursorHandler(JNIEnv *env, jobject obj, jint x, jint y )
{
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    viewImpl->moveSingleCursorHandler(x,y );
}

static int CheckSelectionAtBoundry(JNIEnv *env, jobject obj)
{
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    return viewImpl->checkSelectionAtBoundry();
}

static void SaveSelectionController(JNIEnv *env, jobject obj)
{
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    viewImpl->saveSelectionController();
}


static void RestorePreviousSelectionController(JNIEnv *env, jobject obj)
{
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    viewImpl->restorePreviousSelectionController();
}

static void CheckSelectedClosestWord( JNIEnv *env, jobject obj)
{
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    viewImpl->checkSelectedClosestWord();
}

static int GetStateInRichlyEditableText(JNIEnv *env, jobject obj)
{
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    return viewImpl->getStateInRichlyEditableText();    
}

static int CheckEndOfWordAtPosition( JNIEnv *env, jobject obj,  jint x, jint y )
{
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    return viewImpl->checkEndofWordAtPosition(x, y);
}
//SISO_HTMLCOMPOSER end

static void DumpDomTree(JNIEnv *env, jobject obj, jboolean useFile)
{
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    LOG_ASSERT(viewImpl, "viewImpl not set in %s", __FUNCTION__);

    viewImpl->dumpDomTree(useFile);
}

static void DumpRenderTree(JNIEnv *env, jobject obj, jboolean useFile)
{
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    LOG_ASSERT(viewImpl, "viewImpl not set in %s", __FUNCTION__);

    viewImpl->dumpRenderTree(useFile);
}

static void DumpNavTree(JNIEnv *env, jobject obj)
{
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    LOG_ASSERT(viewImpl, "viewImpl not set in %s", __FUNCTION__);

    viewImpl->dumpNavTree();
}

static void DumpV8Counters(JNIEnv*, jobject)
{
#if USE(V8)
#ifdef ANDROID_INSTRUMENT
    V8Counters::dumpCounters();
#endif
#endif
}

static void SetJsFlags(JNIEnv *env, jobject obj, jstring flags)
{
#if USE(V8)
    WTF::String flagsString = jstringToWtfString(env, flags);
    WTF::CString utf8String = flagsString.utf8();
    WebCore::ScriptController::setFlags(utf8String.data(), utf8String.length());
#endif
}


// Called from the Java side to set a new quota for the origin or new appcache
// max size in response to a notification that the original quota was exceeded or
// that the appcache has reached its maximum size.
static void SetNewStorageLimit(JNIEnv* env, jobject obj, jlong quota) {
#if ENABLE(DATABASE) || ENABLE(OFFLINE_WEB_APPLICATIONS)
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    Frame* frame = viewImpl->mainFrame();

    // The main thread is blocked awaiting this response, so now we can wake it
    // up.
    ChromeClientAndroid* chromeC = static_cast<ChromeClientAndroid*>(frame->page()->chrome()->client());
    chromeC->wakeUpMainThreadWithNewQuota(quota);
#endif
}

// Called from Java to provide a Geolocation permission state for the specified origin.
static void GeolocationPermissionsProvide(JNIEnv* env, jobject obj, jstring origin, jboolean allow, jboolean remember) {
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    Frame* frame = viewImpl->mainFrame();

    ChromeClientAndroid* chromeClient = static_cast<ChromeClientAndroid*>(frame->page()->chrome()->client());
    chromeClient->provideGeolocationPermissions(jstringToWtfString(env, origin), allow, remember);
}

// Samsung Change - HTML5 Web Notification	>>
// Called from Java to provide a Notification permission state for the specified origin.
static void NotificationPermissionsProvide(JNIEnv* env, jobject obj, jstring origin, jboolean allow) {
#if ENABLE(NOTIFICATIONS)
    String orgiginstr = jstringToWtfString(env, origin);
    LOGV("NotificationPermissionsProvide origin = %s, allow = %d", orgiginstr.utf8().data(),allow);
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    Frame* frame = viewImpl->mainFrame();

    ChromeClientAndroid* chromeClient = static_cast<ChromeClientAndroid*>(frame->page()->chrome()->client());
    chromeClient->provideNotificationPermissions(jstringToWtfString(env, origin), allow);
#endif
}

// Called from Java to send Notification event back for the specified origin.
    static void NotificationResponseback(JNIEnv* env, jobject obj ,jstring eventName, jint pointer) {
#if ENABLE(NOTIFICATIONS)
    String event = jstringToWtfString(env, eventName);
    
    LOGV("Inside NotificationResponseback COUNTER %d" ,pointer );
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    Frame* frame = viewImpl->mainFrame();
    ChromeClientAndroid* chromeClient = static_cast<ChromeClientAndroid*>(frame->page()->chrome()->client());
    chromeClient->dispatchNotificationEvents(event,pointer);
 #endif
     
}

//Called from Java to send Noification ID back for specified origin
static void NotificationIDback(JNIEnv* env, jobject obj, jint notificationID, jint counter) {
    
 #if ENABLE(NOTIFICATIONS)  
    LOGV("Inside NotificationIDback NOTIFICATIONID %d COUNTER %d" ,notificationID, counter );
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    Frame* frame = viewImpl->mainFrame();
    ChromeClientAndroid* chromeClient = static_cast<ChromeClientAndroid*>(frame->page()->chrome()->client());
    chromeClient->recordNotificationID(notificationID, counter);
#endif
}
// Samsung Change - HTML5 Web Notification	<<

static void RegisterURLSchemeAsLocal(JNIEnv* env, jobject obj, jstring scheme) {
#ifdef ANDROID_INSTRUMENT
    TimeCounterAuto counter(TimeCounter::WebViewCoreTimeCounter);
#endif
    WebCore::SchemeRegistry::registerURLSchemeAsLocal(jstringToWtfString(env, scheme));
}

static bool FocusBoundsChanged(JNIEnv* env, jobject obj)
{
    return GET_NATIVE_VIEW(env, obj)->focusBoundsChanged();
}

static void SetIsPaused(JNIEnv* env, jobject obj, jboolean isPaused)
{
    // tell the webcore thread to stop thinking while we do other work
    // (selection and scrolling). This has nothing to do with the lifecycle
    // pause and resume.
    GET_NATIVE_VIEW(env, obj)->setIsPaused(isPaused);
}

static void Pause(JNIEnv* env, jobject obj)
{
    // This is called for the foreground tab when the browser is put to the
    // background (and also for any tab when it is put to the background of the
    // browser). The browser can only be killed by the system when it is in the
    // background, so saving the Geolocation permission state now ensures that
    // is maintained when the browser is killed.
    ChromeClient* chromeClient = GET_NATIVE_VIEW(env, obj)->mainFrame()->page()->chrome()->client();
    ChromeClientAndroid* chromeClientAndroid = static_cast<ChromeClientAndroid*>(chromeClient);
    chromeClientAndroid->storeGeolocationPermissions();
// Samsung Change - HTML5 Web Notification	>>
    chromeClientAndroid->storeNotificationPermissions();
// Samsung Change - HTML5 Web Notification	<<
    Frame* mainFrame = GET_NATIVE_VIEW(env, obj)->mainFrame();
    for (Frame* frame = mainFrame; frame; frame = frame->tree()->traverseNext()) {
        Geolocation* geolocation = frame->domWindow()->navigator()->optionalGeolocation();
        if (geolocation)
            geolocation->suspend();
    }

    GET_NATIVE_VIEW(env, obj)->deviceMotionAndOrientationManager()->maybeSuspendClients();

    ANPEvent event;
    SkANP::InitEvent(&event, kLifecycle_ANPEventType);
    event.data.lifecycle.action = kPause_ANPLifecycleAction;
    GET_NATIVE_VIEW(env, obj)->sendPluginEvent(event);

    GET_NATIVE_VIEW(env, obj)->setIsPaused(true);
}

static void Resume(JNIEnv* env, jobject obj)
{
    Frame* mainFrame = GET_NATIVE_VIEW(env, obj)->mainFrame();
    for (Frame* frame = mainFrame; frame; frame = frame->tree()->traverseNext()) {
        Geolocation* geolocation = frame->domWindow()->navigator()->optionalGeolocation();
        if (geolocation)
            geolocation->resume();
    }

    GET_NATIVE_VIEW(env, obj)->deviceMotionAndOrientationManager()->maybeResumeClients();

    ANPEvent event;
    SkANP::InitEvent(&event, kLifecycle_ANPEventType);
    event.data.lifecycle.action = kResume_ANPLifecycleAction;
    GET_NATIVE_VIEW(env, obj)->sendPluginEvent(event);

    GET_NATIVE_VIEW(env, obj)->setIsPaused(false);
}

static void FreeMemory(JNIEnv* env, jobject obj)
{
    ANPEvent event;
    SkANP::InitEvent(&event, kLifecycle_ANPEventType);
    event.data.lifecycle.action = kFreeMemory_ANPLifecycleAction;
    GET_NATIVE_VIEW(env, obj)->sendPluginEvent(event);
}

static void ProvideVisitedHistory(JNIEnv *env, jobject obj, jobject hist)
{
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    LOG_ASSERT(viewImpl, "viewImpl not set in %s", __FUNCTION__);

    jobjectArray array = static_cast<jobjectArray>(hist);

    jsize len = env->GetArrayLength(array);
    for (jsize i = 0; i < len; i++) {
        jstring item = static_cast<jstring>(env->GetObjectArrayElement(array, i));
        const UChar* str = static_cast<const UChar*>(env->GetStringChars(item, 0));
        jsize len = env->GetStringLength(item);
        viewImpl->addVisitedLink(str, len);
        env->ReleaseStringChars(item, str);
        env->DeleteLocalRef(item);
    }
}

static void PluginSurfaceReady(JNIEnv* env, jobject obj)
{
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    if (viewImpl)
        viewImpl->sendPluginSurfaceReady();
}

// Notification from the UI thread that the plugin's full-screen surface has been discarded
static void FullScreenPluginHidden(JNIEnv* env, jobject obj, jint npp)
{
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    PluginWidgetAndroid* plugin = viewImpl->getPluginWidget((NPP)npp);
    if (plugin)
        plugin->exitFullScreen(false);
}

static WebCore::IntRect jrect_to_webrect(JNIEnv* env, jobject obj)
{
    int L, T, R, B;
    GraphicsJNI::get_jrect(env, obj, &L, &T, &R, &B);
    return WebCore::IntRect(L, T, R - L, B - T);
}

static bool ValidNodeAndBounds(JNIEnv *env, jobject obj, int frame, int node,
    jobject rect)
{
    IntRect nativeRect = jrect_to_webrect(env, rect);
    return GET_NATIVE_VIEW(env, obj)->validNodeAndBounds(
            reinterpret_cast<Frame*>(frame),
            reinterpret_cast<Node*>(node), nativeRect);
}

static jobject GetTouchHighlightRects(JNIEnv* env, jobject obj, jint x, jint y, jint slop)
{
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    if (!viewImpl)
        return 0;
    Vector<IntRect> rects = viewImpl->getTouchHighlightRects(x, y, slop);
    if (rects.isEmpty())
        return 0;

    jclass arrayClass = env->FindClass("java/util/ArrayList");
    LOG_ASSERT(arrayClass, "Could not find java/util/ArrayList");
    jmethodID init = env->GetMethodID(arrayClass, "<init>", "(I)V");
    LOG_ASSERT(init, "Could not find constructor for ArrayList");
    jobject array = env->NewObject(arrayClass, init, rects.size());
    LOG_ASSERT(array, "Could not create a new ArrayList");
    jmethodID add = env->GetMethodID(arrayClass, "add", "(Ljava/lang/Object;)Z");
    LOG_ASSERT(add, "Could not find add method on ArrayList");
    jclass rectClass = env->FindClass("android/graphics/Rect");
    LOG_ASSERT(rectClass, "Could not find android/graphics/Rect");
    jmethodID rectinit = env->GetMethodID(rectClass, "<init>", "(IIII)V");
    LOG_ASSERT(rectinit, "Could not find init method on Rect");

    for (size_t i = 0; i < rects.size(); i++) {
        jobject rect = env->NewObject(rectClass, rectinit, rects[i].x(),
                rects[i].y(), rects[i].maxX(), rects[i].maxY());
        if (rect) {
            env->CallBooleanMethod(array, add, rect);
            env->DeleteLocalRef(rect);
        }
    }

    env->DeleteLocalRef(rectClass);
    env->DeleteLocalRef(arrayClass);
    return array;
}

static void AutoFillForm(JNIEnv* env, jobject obj, jint queryId)
{
#if ENABLE(WEB_AUTOFILL)
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    if (!viewImpl)
        return;

    WebCore::Frame* frame = viewImpl->mainFrame();
    if (frame) {
        EditorClientAndroid* editorC = static_cast<EditorClientAndroid*>(frame->page()->editorClient());
        WebAutofill* autoFill = editorC->getAutofill();
        autoFill->fillFormFields(queryId);
    }
#endif
}




// SAMSUNG CHANGE : ADVANCED_TEXT_SELECTION >>
static void WebTextSelectionAll(JNIEnv *env, jobject obj, int x1, int y1, int x2, int y2)
{
	WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
	LOG_ASSERT(viewImpl, "view not set in %s", __FUNCTION__);
	viewImpl->webTextSelectionAll(x1, y1, x2, y2);
}

static void CopyMoveSelection(JNIEnv *env, jobject obj, int x, int y, int controller,
		bool ex, bool selectionMove, float zoomLevel, int granularity)
{
	WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
	LOG_ASSERT(viewImpl, "view not set in %s", __FUNCTION__);
	viewImpl->copyMoveSelection(x, y, controller, ex, selectionMove, zoomLevel, granularity);
}

static void ClearTextSelection(JNIEnv *env, jobject obj, int contentX, int contentY)
{
	WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
	LOG_ASSERT(viewImpl, "view not set in %s", __FUNCTION__);
	viewImpl->clearTextSelection(contentX, contentY);
}

//arvind.maan RTL selection fix
static bool RecordSelectionCopiedData(JNIEnv *env, jobject obj, jobject region,jobject cRegion,  jobject sRect,jobject eRect, jint value)
{
#ifdef ANDROID_INSTRUMENT
	TimeCounterAuto counter(TimeCounter::WebViewCoreTimeCounter);
#endif
	WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
	LOG_ASSERT(viewImpl, "viewImpl not set in RecordSelectionCopiedData");

	SkRegion* nativeRegion = GraphicsJNI::getNativeRegion(env, region);
    SkRegion* cNativeRegion = GraphicsJNI::getNativeRegion(env, cRegion);//arvind.maan RTL selection fix
	SkIRect nativeSRect;
	SkIRect nativeERect;

//arvind.maan RTL selection fix
    bool result = viewImpl->recordSelectionCopiedData(nativeRegion,cNativeRegion, &nativeSRect,&nativeERect,value);
	GraphicsJNI::set_jrect(env,sRect,nativeSRect.fLeft, nativeSRect.fTop,nativeSRect.fRight, nativeSRect.fBottom );
	GraphicsJNI::set_jrect(env,eRect,nativeERect.fLeft, nativeERect.fTop,nativeERect.fRight, nativeERect.fBottom );


	//GraphicsJNI::iRect_to_jRect(&nativeSRect, env,sRect);
	//GraphicsJNI::iRect_to_jRect(&nativeERect, env,eRect);
	return result;
}

static jint GetSelectionGranularity(JNIEnv *env, jobject obj)
{
	WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
	LOG_ASSERT(viewImpl, "viewImpl not set in GetSelectionGranularity");
	return viewImpl->getSelectionGranularity();
}

//Adding for MultiColumn Selection - Begin
static jint GetSelectionMultiColInfo(JNIEnv *env, jobject obj)
{
	WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
	LOG_ASSERT(viewImpl, "viewImpl not set in GetSelectionMultiColInfo");
	return viewImpl->getSelectionMultiColInfo();
}
//Adding for MultiColumn Selection - End

static bool SelectClosestWord(JNIEnv *env, jobject obj,int x, int y, float zoomLevel, bool flag)
{
	WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
	LOG_ASSERT(viewImpl, "viewImpl not set in SelectClosestWord");
	return viewImpl->selectClosestWord(x,y, zoomLevel, flag);

}
// SAMSUNG CHANGE : ADVANCED_TEXT_SELECTION <<

//SAMSUNG CHANGES >>
static jobjectArray nativeGetWebFeedLinks ( JNIEnv* env, jobject obj )
{
	jclass fi_clazz = 0;
	jmethodID initID = 0;
	jobjectArray infos ;
	int start = 0;
	int limit = 0;
	jobject urlobj,titleobj, typeobj;//, pathobj ;
	Vector<WebFeedLink*> feedInfoList ;

	WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);

	viewImpl->getWebFeedLinks ( feedInfoList ) ;

	LOGV ( "WebViewCore::nativeGetWebFeedLinks() links count = %d", feedInfoList.size() );

	fi_clazz = env->FindClass ( "android/webkit/WebFeedLink" );
	initID = env->GetMethodID ( fi_clazz, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V" );

	infos = env->NewObjectArray ( feedInfoList.size(), fi_clazz, NULL );

        for ( int i = 0; i <  feedInfoList.size(); i++ )
	{
		urlobj =env->NewString ( feedInfoList[i]->url().characters(), feedInfoList[i]->url().length() );
		titleobj = env->NewString ( feedInfoList[i]->title().characters(), feedInfoList[i]->title().length() );
		typeobj = env->NewString ( feedInfoList[i]->type().characters(), feedInfoList[i]->type().length() );
		//pathobj = env->NewString(feedInfoList[i]->path().characters(), feedInfoList[i]->path().length());

		jobject fi = env->NewObject ( fi_clazz, initID, urlobj, titleobj, typeobj );

		env->SetObjectArrayElement ( infos, i, fi );

		delete feedInfoList[i] ;
		env->DeleteLocalRef(urlobj);
    	env->DeleteLocalRef(titleobj);
    	env->DeleteLocalRef(typeobj);
    	env->DeleteLocalRef(fi);

		start = limit;
	}

	feedInfoList.clear() ;

	return infos ;
}
//SAMSUNG CHANGES <<
//SAMSUNG CHANGES+
// Spell Check
#if ENABLE(SPELLCHECK)
long nextWordStartIndex(char* sentence)
{
	long wordstartindex = -1;
	long i;
	
	for(i = 0;sentence[i]!='\0';i++)
	{
		if((sentence[i]>='a' && sentence[i]<='z') || (sentence[i]>='A' &&sentence[i]<='Z'))
		{	
			break;
		}
	}
	
	if(sentence[i]!='\0')
		wordstartindex = i;
		
	return wordstartindex;
}
#endif

static bool nativeCheckSpellingOfWordAtPosition(JNIEnv *env, jobject obj, jint x, jint y) {
#if ENABLE(SPELLCHECK)
    jint location = -1, length = 0;
	long wordstartindex = 0;
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    viewImpl->selectClosestWord(x,y, 1.0f, true);
    WTF::String result = viewImpl->getSelectedText();
	android_printLog(ANDROID_LOG_DEBUG, "WebViewCore","WebViewCore:nativeCheckSpellingOfWordAtPosition Text to select %s %d",result.utf8().data(),result.length());
	
	wordstartindex = nextWordStartIndex(const_cast<char*>(result.ascii().data()));
	
	if((result.length() == 0 )|| (wordstartindex == -1))
		return false;

    if (result != NULL) {
        DBG_NAV_LOGD("selected word at position: %s", result.utf8().data());
    }
    else {
        DBG_NAV_LOG("No word selected");
    }
  

    WebCore::Frame* frame = viewImpl->mainFrame();
    if (frame) {
        EditorClientAndroid* editorC = static_cast<EditorClientAndroid*>(frame->page()->editorClient());
        editorC->checkSpellingOfString(result.characters(), result.length(), &location, &length);//SAMSUNG CHANGES
    }
    
    if ((location == -1) && (length == 0)) {
        return false;
    } else {
        return true;
    }
#else
    DBG_NAV_LOG("Spell Check feature not enabled: define the macro ENABLE_SPELLCHECK");
    return false;
#endif
}

static void nativeUnmarkWord(JNIEnv *env, jobject obj, jstring word)
{
#if ENABLE(SPELLCHECK)
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    DBG_NAV_LOG("nativeUnmarkWord");
    WebCore::Frame* frame = viewImpl->mainFrame();
    if (frame) {
    EditorClientAndroid* editorC = static_cast<EditorClientAndroid*>(frame->page()->editorClient());
    editorC->unmarkwordlist(jstringToWtfString(env, word));//SAMSUNG CHANGES
    }
#else
    DBG_NAV_LOG("Spell Check feature not enabled: define the macro ENABLE_SPELLCHECK");
#endif
}
//SAMSUNG CHANGES-
//SAMSUNG changes - Reader
//Called from browser activity and webview on click of reader button. Evaluates the reader javascript
WTF ::String WebViewCore::applyreadability(WTF::String  str)
{
#if USE(V8)
	LOGV("in apply readability");
        m_mainFrame->script()->executeScript(str);
        LOGV("after evaluation of script");
			
	Element* divElement = m_mainFrame-> document()->getElementById(WTF::String("reader_div"));
	HTMLElement* HTMLdivElement= static_cast<HTMLElement *>(divElement);
	return HTMLdivElement->innerHTML();
#endif
}
//Evaluates the initial recognizearticle.js
WTF::String WebViewCore::loadinitialJs( WTF::String  str)
{
#if USE(V8)
	LOGV("in load initial JS");
        m_mainFrame->script()->executeScript(str);
	LOGV("after evaluation of script");
	Element* divElement = m_mainFrame-> document()->getElementById(WTF::String("recog_div"));
	HTMLElement* HTMLdivElement= static_cast<HTMLElement *>(divElement);
	return HTMLdivElement->innerHTML().utf8().data() ;
#endif
}
//SAMSUNG changes - Reader			 

//SAMSUNG CHANGES- EMAIL APP CUSTOMIZATION >>
static bool nativeRequiresSmartFit(JNIEnv *env, jobject obj)
{
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    return viewImpl->requiresSmartFit();
}
//SAMSUNG CHANGES <<

static void CloseIdleConnections(JNIEnv* env, jobject obj)
{
#if USE(CHROME_NETWORK_STACK)
    WebCache::get(true)->closeIdleConnections();
    WebCache::get(false)->closeIdleConnections();
#endif
}

static void ScrollRenderLayer(JNIEnv* env, jobject obj, jint layer, jobject jRect)
{
    SkRect rect;
    GraphicsJNI::jrect_to_rect(env, jRect, &rect);
    GET_NATIVE_VIEW(env, obj)->scrollRenderLayer(layer, rect);
}
//SAMSUNG CHANGES >>>
static void setWebTextViewOnOffStatus(JNIEnv* env, jobject obj, jboolean enable)
{
	GET_NATIVE_VIEW(env, obj)->setWebTextViewOnOffStatus(enable);
}
//SAMSUNG CHANGES <<<

//SAMSUNG CHANGE HTML5 COLOR <<
static void SendColorPickerChoice(JNIEnv* env, jobject obj, jint choice)
{
#ifdef ANDROID_INSTRUMENT
    TimeCounterAuto counter(TimeCounter::WebViewCoreTimeCounter);
#endif
    WebViewCore* viewImpl = GET_NATIVE_VIEW(env, obj);
    LOG_ASSERT(viewImpl, "viewImpl not set in nativeSendColorPickerChoice");
    viewImpl->ColorChooserReply(choice);
}
//SAMSUNG CHANGE HTML5 COLOR >>

// ----------------------------------------------------------------------------

/*
 * JNI registration.
 */
static JNINativeMethod gJavaWebViewCoreMethods[] = {
    { "nativeClearContent", "()V",
            (void*) ClearContent },
    { "nativeFocusBoundsChanged", "()Z",
        (void*) FocusBoundsChanged } ,
    { "nativeKey", "(IIIZZZZ)Z",
        (void*) Key },
    { "nativeClick", "(IIZ)V",
        (void*) Click },
    { "nativeContentInvalidateAll", "()V",
        (void*) ContentInvalidateAll },
    { "nativeSendListBoxChoices", "([ZI)V",
        (void*) SendListBoxChoices },
    { "nativeSendListBoxChoice", "(I)V",
        (void*) SendListBoxChoice },
    { "nativeSetSize", "(IIIFIIIIZ)V",
        (void*) SetSize },
    { "nativeSetScrollOffset", "(IZII)V",
        (void*) SetScrollOffset },
#if ENABLE(SPELLCHECK)
    { "nativeUnmarkWord", "(Ljava/lang/String;)V",
        (void*) nativeUnmarkWord }, // Spell Check
#endif
    { "nativeSetGlobalBounds", "(IIII)V",
        (void*) SetGlobalBounds },
    { "nativeSetSelection", "(II)V",
        (void*) SetSelection } ,
    { "nativeModifySelection", "(II)Ljava/lang/String;",
        (void*) ModifySelection },
    { "nativeDeleteSelection", "(III)V",
        (void*) DeleteSelection } ,
    { "nativeReplaceTextfieldText", "(IILjava/lang/String;III)V",
        (void*) ReplaceTextfieldText } ,
    { "nativeMoveFocus", "(II)V",
        (void*) MoveFocus },
    { "nativeMoveMouse", "(III)V",
        (void*) MoveMouse },
    { "nativeMoveMouseIfLatest", "(IIII)V",
        (void*) MoveMouseIfLatest },
    { "passToJs", "(ILjava/lang/String;IIZZZZ)V",
        (void*) PassToJs },
    { "nativeScrollFocusedTextInput", "(FI)V",
        (void*) ScrollFocusedTextInput },
    { "nativeSetFocusControllerActive", "(Z)V",
        (void*) SetFocusControllerActive },
    { "nativeSaveDocumentState", "(I)V",
        (void*) SaveDocumentState },
    { "nativeFindAddress", "(Ljava/lang/String;Z)Ljava/lang/String;",
        (void*) FindAddress },
    { "nativeHandleTouchEvent", "(I[I[I[IIII)Z",
            (void*) HandleTouchEvent },
//LIGHT TOUCH            
    { "nativeTouchUp", "(IIIIIZ)V",
        (void*) TouchUp },
//LIGHT TOUCH        
    { "nativeRetrieveHref", "(II)Ljava/lang/String;",
        (void*) RetrieveHref },
    { "nativeRetrieveAnchorText", "(II)Ljava/lang/String;",
        (void*) RetrieveAnchorText },
    { "nativeRetrieveImageSource", "(II)Ljava/lang/String;",
        (void*) RetrieveImageSource },
    { "nativeStopPaintingCaret", "()V",
        (void*) StopPaintingCaret },
    { "nativeUpdateFrameCache", "()V",
        (void*) UpdateFrameCache },
    { "nativeGetContentMinPrefWidth", "()I",
        (void*) GetContentMinPrefWidth },
    { "nativeUpdateLayers", "(II)Z",
        (void*) UpdateLayers },
    { "nativeNotifyAnimationStarted", "(I)V",
        (void*) NotifyAnimationStarted },
    { "nativeRecordContent", "(Landroid/graphics/Region;Landroid/graphics/Point;)I",
        (void*) RecordContent },
    { "setViewportSettingsFromNative", "()V",
        (void*) SetViewportSettingsFromNative },
    { "nativeSplitContent", "(I)V",
        (void*) SplitContent },
    { "nativeSetBackgroundColor", "(I)V",
        (void*) SetBackgroundColor },
//SISO_HTMLCOMPOSER begin
    { "nativeInsertContent","(Ljava/lang/String;IZLjava/util/Vector;)Landroid/graphics/Point;",
        (void*) InsertContent},
    { "nativeSimulateDelKeyForCount", "(I)V",
     (void*) SimulateDelKeyForCount },

    { "nativeGetTextAroundCursor", "(IZ)Ljava/lang/String;",
     (void*) GetTextAroundCursor },

    { "nativeDeleteSurroundingText", "(II)V",
        (void*) DeleteSurroundingText },


    { "nativeGetSelectionOffset", "()Landroid/graphics/Point;",
        (void*) GetSelectionOffset },

    { "nativeGetSelectionOffsetImage", "(IIII)V",
        (void*) GetSelectionOffsetImage },

    { "nativeGetBodyText", "()Ljava/lang/String;",
        (void*) GetBodyText },

    { "nativeExecCommand", "(Ljava/lang/String;Ljava/lang/String;)Z",
     (void*) ExecCommand },

    { "nativeCanUndo", "()Z",
     (void*) CanUndo },

    { "nativeCanRedo", "()Z",
     (void*) CanRedo },

    { "nativeUndoRedoStateReset", "()V",
     (void*) UndoRedoStateReset },


    { "nativeCopyAndSaveImage", "(Ljava/lang/String;)Z",
     (void*) CopyAndSaveImage },

    { "nativeSaveCachedImageToFile", "(Ljava/lang/String;Ljava/lang/String;)Z",
     (void*) SaveCachedImageToFile },


    { "nativeGetBodyHTML", "()Ljava/lang/String;",
        (void*) GetBodyHTML },

    { "nativeGetFullMarkupData", "()Landroid/webkit/WebHTMLMarkupData;",
        (void*) GetFullMarkupData },

    { "nativeSetEditable", "(Z)V",
        (void*) SetEditable },

    { "nativeSetSelectionEditable", "(II)V",
        (void*) SetSelectionEditable },

    { "nativeMoveSingleCursorHandler", "(II)V",
        (void*) MoveSingleCursorHandler },

    { "nativeSetComposingRegion", "(II)V",
        (void*) SetComposingRegion },

    { "nativeGetCursorRect", "(Z)Landroid/graphics/Rect;",
        (void*) GetCursorRect },

    { "nativeSetSelectionNone", "()V",
        (void*) SetSelectionNone },

    { "nativeGetSelectionNone", "()Z",
        (void*) GetSelectionNone },

    { "nativeSetComposingSelectionNone", "()V",
        (void*) SetComposingSelectionNone },

    { "nativeGetBodyEmpty", "()Z",
     (void*) GetBodyEmpty },


    { "nativeContentSelectionType", "()I",
     (void*) ContentSelectionType },

    { "nativeUpdateIMSelection", "(II)V",
     (void*) UpdateIMSelection },

    { "nativeRestorePreviousSelectionController", "()V",
     (void*) RestorePreviousSelectionController },


    { "nativeSaveSelectionController", "()V",
     (void*) SaveSelectionController },

    { "nativeCheckSelectionAtBoundry", "()I",
     (void*) CheckSelectionAtBoundry },

    { "nativeCheckSelectedClosestWord", "()V", 
     (void*) CheckSelectedClosestWord },

    { "nativeGetStateInRichlyEditableText", "()I",
     (void*) GetStateInRichlyEditableText },    

    { "nativeinsertImageContent", "(Ljava/lang/String;)V",
     (void*) insertImageContent },

    { "nativeresizeImage", "(II)V",
     (void*) ResizeImage },

    { "nativegetCurrentFontSize", "()I",
     (void*) GetCurrentFontSize },


    { "nativegetCurrentFontValue", "()I",
     (void*) GetCurrentFontValue },


    { "nativeCheckEndOfWordAtPosition", "(II)Z",
     (void*) CheckEndOfWordAtPosition },
//SISO_HTMLCOMPOSER end
    { "nativeRegisterURLSchemeAsLocal", "(Ljava/lang/String;)V",
        (void*) RegisterURLSchemeAsLocal },
    { "nativeDumpDomTree", "(Z)V",
        (void*) DumpDomTree },
    { "nativeDumpRenderTree", "(Z)V",
        (void*) DumpRenderTree },
    { "nativeDumpNavTree", "()V",
        (void*) DumpNavTree },
    { "nativeDumpV8Counters", "()V",
        (void*) DumpV8Counters },
    { "nativeSetNewStorageLimit", "(J)V",
        (void*) SetNewStorageLimit },
    { "nativeGeolocationPermissionsProvide", "(Ljava/lang/String;ZZ)V",
        (void*) GeolocationPermissionsProvide },
    { "nativeSetIsPaused", "(Z)V", (void*) SetIsPaused },
    { "nativePause", "()V", (void*) Pause },
    { "nativeResume", "()V", (void*) Resume },
    { "nativeFreeMemory", "()V", (void*) FreeMemory },
    { "nativeSetJsFlags", "(Ljava/lang/String;)V", (void*) SetJsFlags },
    { "nativeRequestLabel", "(II)Ljava/lang/String;",
        (void*) RequestLabel },
    { "nativeRevealSelection", "()V", (void*) RevealSelection },
    { "nativeUpdateFrameCacheIfLoading", "()V",
        (void*) UpdateFrameCacheIfLoading },
    { "nativeProvideVisitedHistory", "([Ljava/lang/String;)V",
        (void*) ProvideVisitedHistory },
    { "nativeFullScreenPluginHidden", "(I)V",
        (void*) FullScreenPluginHidden },
    { "nativePluginSurfaceReady", "()V",
        (void*) PluginSurfaceReady },
    { "nativeValidNodeAndBounds", "(IILandroid/graphics/Rect;)Z",
        (void*) ValidNodeAndBounds },
    { "nativeGetTouchHighlightRects", "(III)Ljava/util/ArrayList;",
        (void*) GetTouchHighlightRects },
    { "nativeAutoFillForm", "(I)V",
        (void*) AutoFillForm },
    { "nativeScrollLayer", "(ILandroid/graphics/Rect;)V",
        (void*) ScrollRenderLayer },
//SAMSUNG changes - Reader
	{ "applyreadability", "(Ljava/lang/String;)Ljava/lang/String;",
        (void*) applyreadability },
    { "loadinitialJs", "(Ljava/lang/String;)Ljava/lang/String;",
        (void*) loadinitialJs },
//SAMSUNG changes - Reader
    { "nativeCloseIdleConnections", "()V",
        (void*) CloseIdleConnections },
    //SAMSUNG CHANGE : ADVANCED_TEXT_SELECTION >>
    { "nativeWebTextSelectionAll", "(IIII)V",
     (void*) WebTextSelectionAll },
    { "nativeCopyMoveSelection", "(IIIZZFI)V",
     (void*) CopyMoveSelection },
    { "nativeClearTextSelection", "(II)V",
     (void*) ClearTextSelection },
    { "nativeRecordSelectionCopiedData", "(Landroid/graphics/Region;Landroid/graphics/Region;Landroid/graphics/Rect;Landroid/graphics/Rect;I)Z",/*arvind.maan RTL selection fix*/
     (void*) RecordSelectionCopiedData },
    { "nativeGetSelectionGranularity", "()I",
     (void*) GetSelectionGranularity },
    { "nativeSelectClosestWord", "(IIFZ)Z",
     (void*) SelectClosestWord },
    { "nativeGetSelectedText", "()Ljava/lang/String;",
        (void*) GetSelectedText },
//SAMSUNG CHANGE: <MPSG100003899> xhtml page zoom scale change issue on orientation change and back key press >>
    { "nativeRecalcWidthAndForceLayout", "()V",
     (void*) RecalcWidthAndForceLayout },
//SAMSUNG CHANGE: <MPSG100003899> xhtml page zoom scale change issue on orientation change and back key press <<
// Adding for Multicolumn text selection - Begin     
    { "nativeGetSelectionMultiColInfo", "()Z",
     (void*) GetSelectionMultiColInfo },
// Adding for Multicolumn text selection - End
// SAMSUNG CHANGE <<
//SAMSUNG CHANGES >>
    { "nativeGetWebFeedLinks", "()[Landroid/webkit/WebFeedLink;",
        (void*) nativeGetWebFeedLinks },
// Spell Check        
    { "nativeCheckSpellingOfWordAtPosition", "(II)Z",
        (void*) nativeCheckSpellingOfWordAtPosition },
//SAMSUNG CHANGES <<
    //SAMSUNG CHANGES- EMAIL APP CUSTOMIZATION >>
    { "nativeRequiresSmartFit", "()Z",  
        (void*) nativeRequiresSmartFit },
    //SAMSUNG CHANGES <<
// Samsung Change - HTML5 Web Notification	>>
    { "nativeNotificationPermissionsProvide", "(Ljava/lang/String;Z)V",
        (void*) NotificationPermissionsProvide },
    { "nativeNotificationResponseback", "(Ljava/lang/String;I)V",
        (void*) NotificationResponseback },
    { "nativeNotificationIDback", "(II)V",
        (void*) NotificationIDback },
// Samsung Change - HTML5 Web Notification	<<
//SAMSUNG CHANGES >>>
    { "nativeSetWebTextViewOnOffStatus", "(Z)V",  
        (void*) setWebTextViewOnOffStatus },
//SAMSUNG CHANGES <<<
//SAMSUNG CHANGE HTML5 COLOR <<
    { "nativeSendColorPickerChoice", "(I)V",
        (void*) SendColorPickerChoice }
//SAMSUNG CHANGE HTML5 COLOR <<
};

int registerWebViewCore(JNIEnv* env)
{
    jclass widget = env->FindClass("android/webkit/WebViewCore");
    LOG_ASSERT(widget,
            "Unable to find class android/webkit/WebViewCore");
    gWebViewCoreFields.m_nativeClass = env->GetFieldID(widget, "mNativeClass",
            "I");
    LOG_ASSERT(gWebViewCoreFields.m_nativeClass,
            "Unable to find android/webkit/WebViewCore.mNativeClass");
    gWebViewCoreFields.m_viewportWidth = env->GetFieldID(widget,
            "mViewportWidth", "I");
    LOG_ASSERT(gWebViewCoreFields.m_viewportWidth,
            "Unable to find android/webkit/WebViewCore.mViewportWidth");
    gWebViewCoreFields.m_viewportHeight = env->GetFieldID(widget,
            "mViewportHeight", "I");
    LOG_ASSERT(gWebViewCoreFields.m_viewportHeight,
            "Unable to find android/webkit/WebViewCore.mViewportHeight");
    gWebViewCoreFields.m_viewportInitialScale = env->GetFieldID(widget,
            "mViewportInitialScale", "I");
    LOG_ASSERT(gWebViewCoreFields.m_viewportInitialScale,
            "Unable to find android/webkit/WebViewCore.mViewportInitialScale");
    gWebViewCoreFields.m_viewportMinimumScale = env->GetFieldID(widget,
            "mViewportMinimumScale", "I");
    LOG_ASSERT(gWebViewCoreFields.m_viewportMinimumScale,
            "Unable to find android/webkit/WebViewCore.mViewportMinimumScale");
    gWebViewCoreFields.m_viewportMaximumScale = env->GetFieldID(widget,
            "mViewportMaximumScale", "I");
    LOG_ASSERT(gWebViewCoreFields.m_viewportMaximumScale,
            "Unable to find android/webkit/WebViewCore.mViewportMaximumScale");
    gWebViewCoreFields.m_viewportUserScalable = env->GetFieldID(widget,
            "mViewportUserScalable", "Z");
    LOG_ASSERT(gWebViewCoreFields.m_viewportUserScalable,
            "Unable to find android/webkit/WebViewCore.mViewportUserScalable");
    gWebViewCoreFields.m_viewportDensityDpi = env->GetFieldID(widget,
            "mViewportDensityDpi", "I");
    LOG_ASSERT(gWebViewCoreFields.m_viewportDensityDpi,
            "Unable to find android/webkit/WebViewCore.mViewportDensityDpi");
    gWebViewCoreFields.m_webView = env->GetFieldID(widget,
            "mWebView", "Landroid/webkit/WebView;");
    LOG_ASSERT(gWebViewCoreFields.m_webView,
            "Unable to find android/webkit/WebViewCore.mWebView");
    gWebViewCoreFields.m_drawIsPaused = env->GetFieldID(widget,
            "mDrawIsPaused", "Z");
    LOG_ASSERT(gWebViewCoreFields.m_drawIsPaused,
            "Unable to find android/webkit/WebViewCore.mDrawIsPaused");
    gWebViewCoreFields.m_lowMemoryUsageMb = env->GetFieldID(widget, "mLowMemoryUsageThresholdMb", "I");
    gWebViewCoreFields.m_highMemoryUsageMb = env->GetFieldID(widget, "mHighMemoryUsageThresholdMb", "I");
    gWebViewCoreFields.m_highUsageDeltaMb = env->GetFieldID(widget, "mHighUsageDeltaMb", "I");

    gWebViewCoreStaticMethods.m_isSupportedMediaMimeType =
        env->GetStaticMethodID(widget, "isSupportedMediaMimeType", "(Ljava/lang/String;)Z");
    LOG_FATAL_IF(!gWebViewCoreStaticMethods.m_isSupportedMediaMimeType,
        "Could not find static method isSupportedMediaMimeType from WebViewCore");

    env->DeleteLocalRef(widget);

    return jniRegisterNativeMethods(env, "android/webkit/WebViewCore",
            gJavaWebViewCoreMethods, NELEM(gJavaWebViewCoreMethods));
}

} /* namespace android */
