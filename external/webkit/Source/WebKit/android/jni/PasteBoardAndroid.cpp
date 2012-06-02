#include "config.h"

///////////////
#undef LOG
#define LOG_TAG "WML_SISO"
#include <utils/Log.h>
////////////////////
#include "Pasteboard.h"
#include "NotImplemented.h"
#include "Frame.h"
#include "Node.h"
#include "Range.h"
#include "KURL.h"
#include "DocumentFragment.h"
#include "markup.h"
#include "RenderImage.h"
#include "Image.h"
#include "SharedBuffer.h"
#include "CachedResource.h"
#include "CachedResourceLoader.h"
#include "WebFrameView.h"
#include "WebViewCore.h"
#include "FrameView.h"
#include <JNIHelp.h>  // For jniRegisterNativeMethods
#include <JNIUtility.h>
#include <jni.h>
#include "WebCoreJni.h"  // For to_string

namespace android {
	
static jmethodID GetJMethod(JNIEnv* env, jclass clazz, const char name[], const char signature[])
{
    jmethodID m = env->GetMethodID(clazz, name, signature);
    LOG_ASSERT(m, "Could not find method %s", name);
    return m;
}

struct JavaGlueForPasteBoard {
    //jweak       m_obj;
    jmethodID   m_getText;
    jmethodID   m_getHTML;
    jmethodID   m_setDataToClipBoard;
    AutoJObject object(JNIEnv* env , jweak m_obj) {
        return getRealObject(env, m_obj);
    }
} m_javaGlueForPasteBoard;

static void InitPasteboardJni(JNIEnv *env, jobject obj)
{
    LOGD("InitPasteboardJni");
    jclass clazz = env->FindClass("android/webkit/WebClipboard");
   //m_javaGlueForPasteBoard.m_obj = env->NewWeakGlobalRef(obj);

    m_javaGlueForPasteBoard.m_getText = GetJMethod(env, clazz, "getText", "()Ljava/lang/String;");
    m_javaGlueForPasteBoard.m_getHTML = GetJMethod(env, clazz, "getHTML", "()Ljava/lang/String;");

    m_javaGlueForPasteBoard.m_setDataToClipBoard = GetJMethod(env, clazz, "setDataToClipboard", "(Ljava/lang/String;Ljava/lang/String;)V");
}

WTF::String getText(jweak m_obj){
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    jstring returnVal = (jstring) env->CallObjectMethod(m_javaGlueForPasteBoard.object(env , m_obj).get(),
        m_javaGlueForPasteBoard.m_getText);
	
    WTF::String result = jstringToWtfString(env, returnVal);	
    checkException(env);
    return result;	
}

WTF::String getHTML(jweak m_obj){
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    jstring returnVal = (jstring) env->CallObjectMethod(m_javaGlueForPasteBoard.object(env, m_obj).get(),
        m_javaGlueForPasteBoard.m_getHTML);
	
    WTF::String result = jstringToWtfString(env, returnVal);	
    checkException(env);
    return result;
}

void setDataToClipBoard(WTF::String& format , WTF::String& data, jweak m_obj){

    LOGD("   setDataToClipBoard :  Enter");	
	
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    if(env == NULL || format.isEmpty() || data.isEmpty())
    	return;


    jstring jFormat = env->NewString((jchar*) format.characters(), format.length());	
    jstring jData = env->NewString((jchar*) data.characters(), data.length());

    if((m_javaGlueForPasteBoard.object(env , m_obj).get() == NULL )||(m_javaGlueForPasteBoard.m_setDataToClipBoard  ==  0 ))
    {
        if(m_javaGlueForPasteBoard.object(env , m_obj).get() == NULL) {
                LOGD("   setDataToClipBoard no object");
        }
        if(m_javaGlueForPasteBoard.m_setDataToClipBoard  ==  0) {
                LOGD("   setDataToClipBoard no m_setDataToClipBoard");
        }
        LOGD("   setDataToClipBoard :  return with out copy");	
	return;
    }
    env->CallVoidMethod(m_javaGlueForPasteBoard.object(env , m_obj).get(),
        	 m_javaGlueForPasteBoard.m_setDataToClipBoard , jFormat , jData);


    env->DeleteLocalRef(jFormat); 
    env->DeleteLocalRef(jData);
    checkException(env);
}

static JNINativeMethod gPasteboardMethods[] = {
    { "nativeInitPasteboardJni", "()V", (void*) InitPasteboardJni }
};

int register_pasteboard(JNIEnv* env)
{
    const char* kPasteBoardClass = "android/webkit/WebClipboard";
    jclass pasteBoardClass = env->FindClass(kPasteBoardClass);
    LOG_ASSERT(geolocationPermissions, "Unable to find class");

    return jniRegisterNativeMethods(env, kPasteBoardClass ,
            gPasteboardMethods, NELEM(gPasteboardMethods));
}
		

jweak getCurrentClipboardObj(WebCore::Frame* frame)
{
    if(frame != 0)
    {
        WebCore::FrameView* frameView = 	frame->view();
        WebFrameView* webFrameView = static_cast<WebFrameView*>(frameView->platformWidget());
        if (!webFrameView){
            LOGD("writeSelection failed to get webFrameView from frame passed in parameter"); 
            return NULL;
        }

        WebViewCore* webViewCoreLocal = webFrameView->webViewCore();
        if(!webViewCoreLocal)
        {
            LOGD("writeSelection failed to get webViewCore from webFrameView");           
            return NULL;
        }

        jobject obj =  webViewCoreLocal->getWebViewJavaObject();

        JNIEnv* env = JSC::Bindings::getJNIEnv();
        jclass contextClass = env->GetObjectClass(obj);
        jmethodID appContextMethod = env->GetMethodID(contextClass, "getWebClipboard", "()Landroid/webkit/WebClipboard;");
        env->DeleteLocalRef(contextClass);
        jobject clipObj = env->CallObjectMethod(obj, appContextMethod);
        checkException(env);

        return env->NewWeakGlobalRef(clipObj);
    }

    return NULL;
}

}

namespace WebCore {

const String savePath =  "/sdcard/temp";

void replaceNBSPWithSpace(String& str)
{
    static const UChar NonBreakingSpaceCharacter = 0xA0;
    static const UChar SpaceCharacter = ' ';
    str.replace(NonBreakingSpaceCharacter, SpaceCharacter);
}

void copyImagePathToClipboard(const String& imagePath)
{
    String dataFormat = "HTML";
    String htmlImageFragment = "<img src=";
    htmlImageFragment.append(imagePath);
    htmlImageFragment.append(" />");			

    jweak clipObj = android::getCurrentClipboardObj(NULL);
    if(clipObj)
        android::setDataToClipBoard(dataFormat , htmlImageFragment, clipObj);	
}

String createLocalResource(Frame* frame , String url)
{
    String filename;	
    CachedResource* cachedResource = frame->document()->cachedResourceLoader()->cachedResource(url);
    //if (!cachedResource)
    //    cachedResource = cache()->resourceForURL(url);
    if (cachedResource && cachedResource->isImage())
    {
	return createLocalResource(cachedResource ,savePath); 
    }
    return String();	
}

// SAMSUNG CHANGE : CopyImage >>
// Copy Image Issue on Nested Frame
CachedResource *retrieveCachedResource(Frame* frame,  String imageUrl)
{
    if (NULL == frame || imageUrl.isEmpty())
        return NULL;

    FrameTree *frameTree = frame->tree();
    if (!frameTree)
        return frame->document()->cachedResourceLoader()->cachedResource(imageUrl);

    WebCore::Frame *childFrame = frameTree->firstChild();
    Document *childFrameDocument = NULL;
    CachedResource* cachedResource = NULL;
    while (childFrame)
    {
        childFrameDocument = childFrame->document();
        if (childFrameDocument)
        {
            cachedResource = childFrameDocument->cachedResourceLoader()->cachedResource(imageUrl);
            if (cachedResource)
                break;
        }
        cachedResource = retrieveCachedResource(childFrame, imageUrl);
        if (cachedResource) break;
        childFrame = childFrame->tree()->nextSibling();
    }

    if (cachedResource)
        return cachedResource;
    else
        return frame->document()->cachedResourceLoader()->cachedResource(imageUrl);
}

bool saveCachedImageToFile(Frame* frame, String imageUrl, String filePath)
{
    // Copy Image Issue on Nested Frame
    CachedResource* cachedResource = retrieveCachedResource(frame, imageUrl);
    if (cachedResource && cachedResource->isImage())
        return saveCachedImageToFile(cachedResource, imageUrl, filePath);

    return false;
}
// SAMSUNG CHANGE : CopyImage <<

// FIXME, no support for spelling yet.
Pasteboard* Pasteboard::generalPasteboard()
{
    LOGD("generalPasteboard");	
    static Pasteboard* pasteboard = new Pasteboard();
    return pasteboard;
}

void Pasteboard::writeSelection(Range* selectedRange, bool canSmartCopyOrDelete, Frame* frame)
{
    LOGD("writeSelection");	
    String html = createMarkup(selectedRange, 0 , AnnotateForInterchange , false );
    String dataFormat = "HTML";
    ExceptionCode ec = 0;

    jweak clipObj = android::getCurrentClipboardObj(frame);
    if(clipObj)
        android::setDataToClipBoard(dataFormat , html , clipObj);
}

void Pasteboard::writePlainText(const String& plainText)
{
    LOGD("writePlainText");
    String dataFormat = "HTML";
    jweak clipObj = android::getCurrentClipboardObj(NULL);
    if(clipObj)
    android::setDataToClipBoard(dataFormat , (String&)plainText , clipObj);
}

void Pasteboard::writeURL(const KURL&, const String&, Frame*)
{
    LOGD("writeURL");
    notImplemented();
}

void Pasteboard::clear()
{
    LOGD("clear");
    notImplemented();
}

bool Pasteboard::canSmartReplace()
{
    notImplemented();
    return false;
}

PassRefPtr<DocumentFragment> Pasteboard::documentFragment(Frame* frame, PassRefPtr<Range> context, bool allowPlainText, bool& chosePlainText)
{
    LOGD("documentFragment");
    chosePlainText = false;
    String dataFormat = "HTML";
    String imageFormat = "IMAGE";
    String markup;
    KURL srcURL;

    jweak clipObj = android::getCurrentClipboardObj(frame);
    if(clipObj)
        markup = android::getHTML(clipObj);
    if(!markup.isEmpty()) {	
        RefPtr<DocumentFragment> fragment =
        createFragmentFromMarkup(frame->document(), markup, srcURL, FragmentScriptingNotAllowed);
        if (fragment)
            return fragment.release();
    } else if (allowPlainText) {
        WTF::String markup = android::getText(clipObj);//ChromiumBridge::clipboardReadPlainText(buffer);
        if (!markup.isEmpty()) {
            chosePlainText = true;

            RefPtr<DocumentFragment> fragment = createFragmentFromText(context.get(), markup);
            if (fragment)
                return fragment.release();
        }
    }

    return 0;
}

String Pasteboard::plainText(Frame* frame)
{
    LOGD("plainText");
    jweak clipObj = android::getCurrentClipboardObj(frame);
    if(clipObj)
        return android::getText(clipObj);
    else
        return "";
}

Pasteboard::Pasteboard()
{
    LOGD("Pasteboard");
    notImplemented();
}

/*
Pasteboard::~Pasteboard()
{
    LOGD("~Pasteboard");
    notImplemented();
}
*/

void Pasteboard::writeImage(WebCore::Node* node, WebCore::KURL const& , WTF::String const& title)
{
    /*ASSERT(node);
    ASSERT(node->renderer());
    ASSERT(node->renderer()->isImage());
    RenderImage* renderer = toRenderImage(node->renderer());
    CachedImage* cachedImage = renderer->cachedImage();
    ASSERT(cachedImage);
    Image* image = cachedImage->image();
    ASSERT(image);

    // If the image is wrapped in a link, |url| points to the target of the
    // link.  This isn't useful to us, so get the actual image URL.
    AtomicString urlString;
    if (node->hasTagName(HTMLNames::imgTag) || node->hasTagName(HTMLNames::inputTag))
        urlString = static_cast<Element*>(node)->getAttribute(HTMLNames::srcAttr);
#if ENABLE(SVG)
    else if (node->hasTagName(SVGNames::imageTag))
        urlString = static_cast<Element*>(node)->getAttribute(XLinkNames::hrefAttr);
#endif
    else if (node->hasTagName(HTMLNames::embedTag) || node->hasTagName(HTMLNames::objectTag)) {
        Element* element = static_cast<Element*>(node);
        urlString = element->getAttribute(element->imageSourceAttributeName());
    }
    KURL url = urlString.isEmpty() ? KURL() : node->document()->completeURL(deprecatedParseURL(urlString));

    const char* imageEncodedBuffer = image->data()->data();
    	 		
    //NativeImagePtr bitmap = image->nativeImageForCurrentFrame();
    //ChromiumBridge::clipboardWriteImage(bitmap, url, title);*/
}

}
