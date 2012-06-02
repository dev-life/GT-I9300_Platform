/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2009, 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "MarkupAccumulator.h"

#include "CDATASection.h"
#include "Comment.h"
#include "DocumentFragment.h"
#include "DocumentType.h"
#include "Editor.h"
#include "HTMLElement.h"
#include "HTMLNames.h"
#include "KURL.h"
#include "ProcessingInstruction.h"
#include "XMLNSNames.h"
#include <wtf/unicode/CharacterNames.h>
////////SISO_HTMLCOMPOSER begin
#include "android/log.h" 
//#include "DocLoader.h"
#include "FileSystem.h"
#include <wtf/text/CString.h>
#include "MIMETypeRegistry.h"
#include "HTTPParsers.h"
#include "RenderImage.h"
#include "Base64.h"
#include "WebViewCore.h"
#include <SharedBuffer.h>
#include "KURL.h"

// SAMSUNG CHANGE : CopyImage >>
// To print debug
#undef LOG
#include <utils/Log.h>

#undef LOG_TAG
#define LOG_TAG "Markup"
// SAMSUNG CHANGE <<


#define HTML_COMPOSER_CID "cid:"
#define HTML_COMPOSER_DOMAIN "@sec.galaxytab"
#define HTML_COMPOSER_FILE "file://"
#define HTML_COMPOSER_CONTENT "content://"
#define HTML_COMPOSER_TRIPLE_SLASH ":///"
#define HTML_COMPOSER_DOUBLE_SLASH "://"
/*SAMSUNG_HTML_EDIT_EXTENSION END*/

using namespace android;
////////SISO_HTMLCOMPOSER end
using namespace std;

namespace WebCore {

using namespace HTMLNames;
/*SAMSUNG_HTML_EDIT_EXTENSION - BEGIN*/
static String getCIDData(const String& url, CachedResource* resource, WebHTMLMarkupData* markupData );
static String getUniqueFileNameFromUrl(const String& url, String& urlAbsolutePath, String& actualFileName);
static String getFileNameFromUrl(CachedResource* cachedResource , String url);
////////SISO_HTMLCOMPOSER begin
String getCIDData(const String& url, CachedResource* resource, WebHTMLMarkupData* markupData){
    //ANDROID_LOG_PRINT(ANDROID_LOG_DEBUG, "HTML_EDIT", "Markup.cpp,getCIDData  ");

    if(!(url.startsWith(HTML_COMPOSER_FILE) || url.startsWith(HTML_COMPOSER_CONTENT))){
        ////ANDROID_LOG_PRINT(ANDROID_LOG_DEBUG, "HTML_EDIT", "Markup.cpp,getCIDData  , URL not in file/content scheme");
        return String();
    }

    if(url.startsWith("data:")){
        ////ANDROID_LOG_PRINT(ANDROID_LOG_DEBUG, "HTML_EDIT", "Markup.cpp,getCIDData  Resource is in Data Scheme, skipping, data size  ",url.length());
        return String();
    }	
    
    String urlAbsolutePath;
    String actualFileName;

	int queryIndex = url.reverseFind('?');
	String correctUri;
    if(queryIndex > 0){
        correctUri = url.substring(0, queryIndex);
    }
    else{
        correctUri = url;
    }    

    String uniqueFileName =  getUniqueFileNameFromUrl(correctUri,urlAbsolutePath,actualFileName);
    String cid = HTML_COMPOSER_CID + uniqueFileName +HTML_COMPOSER_DOMAIN;
    String mime;

	if(resource){
        RefPtr<SharedBuffer> imageBuffer = resource->data();
        if(imageBuffer){
            mime = resource->response().mimeType();
            ////ANDROID_LOG_PRINT(ANDROID_LOG_DEBUG, "HTML_EDIT", "Markup.cpp,getCIDData  , mime from buffer: %s",mime.utf8().data());
        }
    }

	if(mime.isEmpty()){
		if(actualFileName.contains('.')){
			int extIndex = actualFileName.reverseFind('.');
			String extension = actualFileName.right(actualFileName.length() - (extIndex +1));
			////ANDROID_LOG_PRINT(ANDROID_LOG_DEBUG, "HTML_EDIT", "Markup.cpp,getCIDData  , extension: %s",extension.utf8().data());
			mime = MIMETypeRegistry::getMIMETypeForExtension(extension);
			////ANDROID_LOG_PRINT(ANDROID_LOG_DEBUG, "HTML_EDIT", "Markup.cpp,getCIDData  , mime from extension: %s",mime.utf8().data());
		}
    }

    long long resultSize = 0;
    //Find Out the File Size
    String pathFile = urlAbsolutePath + actualFileName;
    String decodedPathFile = decodeURLEscapeSequences( pathFile );
    ////ANDROID_LOG_PRINT(ANDROID_LOG_DEBUG, "HTML_EDIT", "Markup.cpp,getCIDData pathFile::%s", decodedPathFile.utf8().data());
    bool s = getFileSize(decodedPathFile, resultSize);
    ////ANDROID_LOG_PRINT(ANDROID_LOG_DEBUG, "HTML_EDIT", "Markup.cpp,getCIDData getFileSize ret::%d",s);
    
    ////ANDROID_LOG_PRINT(ANDROID_LOG_DEBUG, "HTML_EDIT", "Markup.cpp,getCIDData cid::%s, fileName:%s, mime:%s, path:%s resultSize = %lld,correctUri = %s",cid.utf8().data(),actualFileName.utf8().data(),mime.utf8().data(),urlAbsolutePath.utf8().data(),resultSize,correctUri.utf8().data());

    Vector<WebSubPart> existingSubPartList;
    existingSubPartList = markupData->subPartList();
    for( size_t i = 0; i<existingSubPartList.size(); i++ ) {
        if( existingSubPartList.at(i).contentUri() == correctUri ) return cid; // It is not required to save a new cid.
    }

    android::WebSubPart subPart = android::WebSubPart(cid,actualFileName,mime,urlAbsolutePath,resultSize,correctUri);
    markupData->subPartList().append(subPart);
    return cid;    
}

String getUniqueFileNameFromUrl(const String& url, String& urlAbsolutePath, String& actualFileName){
    ////ANDROID_LOG_PRINT(ANDROID_LOG_DEBUG, "HTML_EDIT", "Markup.cpp,getUniqueFileNameFromUrl  URL Encoding Original : %s, length:%d",url.utf8().data(),url.length());

    String compPath;
	int queryIndex = url.reverseFind('?');
    if(queryIndex > 0){
        compPath = url.substring(0, queryIndex);
    }
    else{
        compPath = url;
    }

    String contentUri = compPath;
    ////ANDROID_LOG_PRINT(ANDROID_LOG_DEBUG, "HTML_EDIT", "Markup.cpp,getUniqueFileNameFromUrl  compPath 1 : %s",compPath.utf8().data());

    //Remove the protocol specifier
	int protoIndex = compPath.find(HTML_COMPOSER_DOUBLE_SLASH);
	String proto;
    if(protoIndex > 0){
		compPath = compPath.right((compPath.length() - (protoIndex + 2))); 
    }
	
    //Take the FileName before replacing .
    actualFileName = pathGetFileName(compPath);
    ////ANDROID_LOG_PRINT(ANDROID_LOG_DEBUG, "HTML_EDIT", "Markup.cpp,getUniqueFileNameFromUrl  compPath 2 : %s, actualFileName: %s",compPath.utf8().data(),actualFileName.utf8().data());

	String uniqueFileName = compPath;
    urlAbsolutePath = compPath.left(compPath.length() -actualFileName.length());

    ////ANDROID_LOG_PRINT(ANDROID_LOG_DEBUG, "HTML_EDIT", "Markup.cpp,getUniqueFileNameFromUrl Only urlAbsolutePath %s",urlAbsolutePath.utf8().data());

    
    uniqueFileName.replace('/', '_');
    uniqueFileName.replace('.', '_');
    ////ANDROID_LOG_PRINT(ANDROID_LOG_DEBUG, "HTML_EDIT", "Markup.cpp,getUniqueFileNameFromUrl  uniqueFileName = %s",uniqueFileName.utf8().data());

	return uniqueFileName;
}

String getFileNameFromUrl(CachedResource* cachedResource , String url)
{
    String filename;	
    if (cachedResource && cachedResource->isImage())
    {
	////ANDROID_LOG_PRINT(ANDROID_LOG_DEBUG,"HTML_EDIT","markup.cpp getFileNameFromUrl, url :%s",url.utf8().data());
	String contentDisposition = cachedResource->response().httpHeaderField("Content-Disposition");
	////ANDROID_LOG_PRINT(ANDROID_LOG_DEBUG,"HTML_EDIT","markup.cpp getFileNameFromUrl, contentDisposition :%s",contentDisposition.utf8().data());
	filename = filenameFromHTTPContentDisposition(contentDisposition);
	if(filename.isEmpty())
	{
	    String contentLocation = cachedResource->response().httpHeaderField("Content-Location");
	    ////ANDROID_LOG_PRINT(ANDROID_LOG_DEBUG,"HTML_EDIT","markup.cpp , contentLocation :%s",contentLocation.utf8().data());				   
	    if (!contentLocation.isEmpty() && !contentLocation.endsWith("/") && contentLocation.reverseFind('?') < 0) 
	    {
                
	        int index = contentLocation.reverseFind('/') + 1;
              	if (index > 0) 
		{
                    filename = contentLocation.substring(index);
              	} 
		else
		{
                    filename = contentLocation;
              	}
            }
	   else 
	   {
	       	int queryIndex = url.reverseFind('?');
                // If there is a query string strip it, same as desktop browsers
                if (queryIndex > 0) 
		{
                    url = url.substring(0, queryIndex);
                }
		filename =  pathGetFileName(url);
                /*if (!url.endsWith("/")) 
		    {
                    	int index = url.reverseFind('/') + 1;
                    	if (index > 0) 
			{
                            filename = url.substring(index);
                    	}
                    }*/
            }

	}
		
    }
    return filename;			
}

static CachedResource* getCachedResource(const Element* element)
{
    // Attempt to pull CachedImage from element
    ASSERT(element);
    RenderObject* renderer = element->renderer();
    if (!renderer || !renderer->isImage())
        return 0;

    RenderImage* image = toRenderImage(renderer);
    if (image->cachedImage() && !image->cachedImage()->errorOccurred()){
        return (CachedResource*)image->cachedImage();
    }

    return 0;
}

static bool checkResourceAvailablity(const Element* element, Attribute* attr){
	
    //Check the Resource Availablity
    CachedResource* resource = getCachedResource(element);

    return resource && resource->isImage();
}

static String createLocalResource(const Element* element, Attribute* attr, const String& basePath){

    CachedResource* resource = getCachedResource(element);

    return createLocalResource(resource , basePath);      		
}

String createLocalResource(CachedResource* resource, const String& basePath,WebHTMLMarkupData* markupData ){

    if(basePath.isNull() || basePath.isEmpty()){
         return String(); 
    }

    if(resource){
        RefPtr<SharedBuffer> imageBuffer = resource->data();
        if(imageBuffer){
            String mime = resource->response().mimeType();
            ////ANDROID_LOG_PRINT(ANDROID_LOG_DEBUG, "HTML_EDIT", "Markup.cpp,createLocalResource  mime: %s",mime.utf8().data());	
            String extension = MIMETypeRegistry::getPreferredExtensionForMIMEType(mime);


            //Check path for the local file path
            String url = resource->url();
    		if(url.startsWith("data:")){
    			////ANDROID_LOG_PRINT(ANDROID_LOG_DEBUG, "HTML_EDIT", "Markup.cpp,createLocalResource  Resource is in Data Scheme, skipping, data size  ",url.length());
    			return String();
    		}

            //Check for local resource						
    	    if(url.startsWith(HTML_COMPOSER_FILE) || url.startsWith(HTML_COMPOSER_CONTENT)){
                ////ANDROID_LOG_PRINT(ANDROID_LOG_DEBUG, "HTML_EDIT", "Markup.cpp,createLocalResource  Resource is Local, skipping creating local resource: URL: %s",url.utf8().data());

                //Check wether to create CID for this
                if(markupData != 0){
                    return getCIDData(url, resource, markupData);
                }
                else {
                    return url;
                }
    	    }
    
            String uniqueURLPath;
            String actualFileName;
            String cid = HTML_COMPOSER_CID + getUniqueFileNameFromUrl(url, uniqueURLPath,actualFileName) + HTML_COMPOSER_DOMAIN;
            uniqueURLPath = basePath +  (uniqueURLPath.startsWith("/")?uniqueURLPath:("/" + uniqueURLPath));
    	    String uniqueFilePath = uniqueURLPath + actualFileName;
    	    String contentUri = HTML_COMPOSER_FILE + uniqueFilePath;
    	    contentUri.replace(HTML_COMPOSER_TRIPLE_SLASH,HTML_COMPOSER_DOUBLE_SLASH);
    	    ////ANDROID_LOG_PRINT(ANDROID_LOG_DEBUG, "HTML_EDIT", "Markup.cpp,createLocalResource  uniqueURLPath: %s uniqueFilePath: %s,contentUri:%s",uniqueURLPath.utf8().data(), uniqueFilePath.utf8().data(),contentUri.utf8().data());
    	    
    	    //Check for the availablity of File
    	    if(fileExists(uniqueFilePath)){
                ////ANDROID_LOG_PRINT(ANDROID_LOG_DEBUG, "HTML_EDIT", "Markup.cpp,createLocalResource  File Available: %s",uniqueFilePath.utf8().data());
                if(markupData != 0){
                    long long fileSize = 0;
                    getFileSize(uniqueFilePath,fileSize);
                    WebSubPart subPart = WebSubPart(cid,actualFileName ,mime,uniqueURLPath,fileSize,contentUri); 
                    markupData->subPartList().append(subPart);
                    return cid;
    		    }
    		    else{
                    return contentUri;
                }
            }

            ////ANDROID_LOG_PRINT(ANDROID_LOG_DEBUG, "HTML_EDIT", "Markup.cpp, Checking Path Exists uniqueURLPath: %s ",uniqueURLPath.utf8().data());			
            if(!fileExists(uniqueURLPath)){
                ////ANDROID_LOG_PRINT(ANDROID_LOG_DEBUG, "HTML_EDIT", "Markup.cpp, Path not available, creating uniqueURLPath: %s ",uniqueURLPath.utf8().data());
                bool ret = makeAllDirectories(uniqueURLPath);
            }    
    	    
            PlatformFileHandle fileHandle;
            CString completeFileName = openLocalFile(uniqueFilePath, extension,  fileHandle);
    	    ////ANDROID_LOG_PRINT(ANDROID_LOG_DEBUG, "HTML_EDIT", "Markup.cpp, createLocalResource completeFileName  : %s",completeFileName.data());
    
    	    if(!completeFileName.isNull()){
                const char* segment = NULL;
                unsigned pos = 0;
    		    while (unsigned length = imageBuffer->getSomeData(segment, pos)) {
    		        writeToFile(fileHandle, segment, length);
    		        pos += length;
    	        }
    	        closeFile(fileHandle);
    
    	        if(markupData != 0){
    		        //Append WebSubPart and return CID
    		        ////ANDROID_LOG_PRINT(ANDROID_LOG_DEBUG, "HTML_EDIT", "Markup.cpp, createLocalResource cid: %s", cid.utf8().data());
    		        long long fileSize = 0;
    		        getFileSize(uniqueFilePath, fileSize);
    		        WebSubPart subPart = WebSubPart(cid,actualFileName ,mime,uniqueURLPath,fileSize,contentUri);
    		        markupData->subPartList().append(subPart);
    		        return cid;
    	        }
    	        else{
    		        return contentUri;
    	        }
    	    }
        } 
    }

    ////ANDROID_LOG_PRINT(ANDROID_LOG_DEBUG, "HTML_EDIT", "Markup.cpp, No Local Resource Created");
    return String();    		
}

////////SISO_HTMLCOMPOSER end

// SAMSUNG CHANGE : CopyImage >>
bool saveCachedImageToFile(CachedResource* resource, const String& imageUrl, const String& filePath)
{
	RefPtr<SharedBuffer> imageBuffer = resource->data();
	if ( NULL == imageBuffer ) {
		//LOGE("imageBuffer is NULL");
		return false;
	}

	if ( 0 == imageBuffer->size() ) {
		//LOGE("imageBuffer size is 0");
		return false;
	}

	PlatformFileHandle fileHandle = open(filePath.utf8().data(), O_WRONLY | O_CREAT | O_TRUNC /*| O_EXCL*/, S_IRUSR | S_IWUSR | S_IROTH);
	if ( -1 == fileHandle ) {
		//LOGE("fileHandle is -1 with filepath = %s", filePath.utf8().data());
		return false;
	}

	const char* segment = NULL;
	unsigned pos = 0;
	while (unsigned length = imageBuffer->getSomeData(segment, pos)) {
		writeToFile(fileHandle, segment, length);
		pos += length;
	}
	closeFile(fileHandle);
	
	return true;
}
// SAMSUNG CHANGE : CopyImage <<

void appendCharactersReplacingEntities(Vector<UChar>& out, const UChar* content, size_t length, EntityMask entityMask)
{
    DEFINE_STATIC_LOCAL(const String, ampReference, ("&amp;"));
    DEFINE_STATIC_LOCAL(const String, ltReference, ("&lt;"));
    DEFINE_STATIC_LOCAL(const String, gtReference, ("&gt;"));
    DEFINE_STATIC_LOCAL(const String, quotReference, ("&quot;"));
    DEFINE_STATIC_LOCAL(const String, nbspReference, ("&nbsp;"));

    static const EntityDescription entityMaps[] = {
        { '&', ampReference, EntityAmp },
        { '<', ltReference, EntityLt },
        { '>', gtReference, EntityGt },
        { '"', quotReference, EntityQuot },
        { noBreakSpace, nbspReference, EntityNbsp },
    };

    size_t positionAfterLastEntity = 0;
    for (size_t i = 0; i < length; ++i) {
        for (size_t m = 0; m < WTF_ARRAY_LENGTH(entityMaps); ++m) {
            if (content[i] == entityMaps[m].entity && entityMaps[m].mask & entityMask) {
                out.append(content + positionAfterLastEntity, i - positionAfterLastEntity);
                append(out, entityMaps[m].reference);
                positionAfterLastEntity = i + 1;
                break;
            }
        }
    }
    out.append(content + positionAfterLastEntity, length - positionAfterLastEntity);
}

MarkupAccumulator::MarkupAccumulator(Vector<Node*>* nodes, EAbsoluteURLs shouldResolveURLs, const Range* range)
    : m_nodes(nodes)
    , m_range(range)
    , m_shouldResolveURLs(shouldResolveURLs)
{
}

MarkupAccumulator::~MarkupAccumulator()
{
}

String MarkupAccumulator::serializeNodes(Node* node, Node* nodeToSkip, EChildrenOnly childrenOnly,const String& basePath  /*SAMSUNG_HTML_EDIT_EXTENSION*/, WebHTMLMarkupData* markupData )
{
    Vector<UChar> out;
    serializeNodesWithNamespaces(node, nodeToSkip, childrenOnly, 0,basePath  /*SAMSUNG_HTML_EDIT_EXTENSION*/, markupData );
    out.reserveInitialCapacity(length());
    concatenateMarkup(out);
    return String::adopt(out);
}

void MarkupAccumulator::serializeNodesWithNamespaces(Node* node, Node* nodeToSkip, EChildrenOnly childrenOnly, const Namespaces* namespaces,const String& basePath  /*SAMSUNG_HTML_EDIT_EXTENSION*/, WebHTMLMarkupData* markupData )
{
    if (node == nodeToSkip)
        return;

    Namespaces namespaceHash;
    if (namespaces)
        namespaceHash = *namespaces;

    if (!childrenOnly)
        appendStartTag(node, &namespaceHash,basePath  /*SAMSUNG_HTML_EDIT_EXTENSION*/, markupData );

    if (!(node->document()->isHTMLDocument() && elementCannotHaveEndTag(node))) {
        for (Node* current = node->firstChild(); current; current = current->nextSibling())
            serializeNodesWithNamespaces(current, nodeToSkip, IncludeNode, &namespaceHash,basePath  /*SAMSUNG_HTML_EDIT_EXTENSION*/, markupData);
    }

    if (!childrenOnly)
        appendEndTag(node);
}

void MarkupAccumulator::appendString(const String& string)
{
    m_succeedingMarkup.append(string);
}

void MarkupAccumulator::appendStartTag(Node* node, Namespaces* namespaces,const String& basePath  /*SAMSUNG_HTML_EDIT_EXTENSION*/, WebHTMLMarkupData* markupData )
{
    Vector<UChar> markup;
    appendStartMarkup(markup, node, namespaces,basePath  /*SAMSUNG_HTML_EDIT_EXTENSION*/,markupData);
    appendString(String::adopt(markup));
    if (m_nodes)
        m_nodes->append(node);
}

void MarkupAccumulator::appendEndTag(Node* node)
{
    Vector<UChar> markup;
    appendEndMarkup(markup, node);
    appendString(String::adopt(markup));
}

size_t MarkupAccumulator::totalLength(const Vector<String>& strings)
{
    size_t length = 0;
    for (size_t i = 0; i < strings.size(); ++i)
        length += strings[i].length();
    return length;
}

// FIXME: This is a very inefficient way of accumulating the markup.
// We're converting results of appendStartMarkup and appendEndMarkup from Vector<UChar> to String
// and then back to Vector<UChar> and again to String here.
void MarkupAccumulator::concatenateMarkup(Vector<UChar>& out)
{
    for (size_t i = 0; i < m_succeedingMarkup.size(); ++i)
        append(out, m_succeedingMarkup[i]);
}

void MarkupAccumulator::appendAttributeValue(Vector<UChar>& result, const String& attribute, bool documentIsHTML)
{
    appendCharactersReplacingEntities(result, attribute.characters(), attribute.length(),
        documentIsHTML ? EntityMaskInHTMLAttributeValue : EntityMaskInAttributeValue);
}

void MarkupAccumulator::appendQuotedURLAttributeValue(Vector<UChar>& result, const String& urlString)
{
    UChar quoteChar = '\"';
    String strippedURLString = urlString.stripWhiteSpace();
    if (protocolIsJavaScript(strippedURLString)) {
        // minimal escaping for javascript urls
        if (strippedURLString.contains('"')) {
            if (strippedURLString.contains('\''))
                strippedURLString.replace('\"', "&quot;");
            else
                quoteChar = '\'';
        }
        result.append(quoteChar);
        append(result, strippedURLString);
        result.append(quoteChar);
        return;
    }

    // FIXME: This does not fully match other browsers. Firefox percent-escapes non-ASCII characters for innerHTML.
    result.append(quoteChar);
    appendAttributeValue(result, urlString, false);
    result.append(quoteChar);
}

void MarkupAccumulator::appendNodeValue(Vector<UChar>& out, const Node* node, const Range* range, EntityMask entityMask)
{
    String str = node->nodeValue();
    const UChar* characters = str.characters();
    size_t length = str.length();

    if (range) {
        ExceptionCode ec;
        if (node == range->endContainer(ec))
            length = range->endOffset(ec);
        if (node == range->startContainer(ec)) {
            size_t start = range->startOffset(ec);
            characters += start;
            length -= start;
        }
    }

    appendCharactersReplacingEntities(out, characters, length, entityMask);
}

bool MarkupAccumulator::shouldAddNamespaceElement(const Element* element)
{
    // Don't add namespace attribute if it is already defined for this elem.
    const AtomicString& prefix = element->prefix();
    AtomicString attr = !prefix.isEmpty() ? "xmlns:" + prefix : "xmlns";
    return !element->hasAttribute(attr);
}

bool MarkupAccumulator::shouldAddNamespaceAttribute(const Attribute& attribute, Namespaces& namespaces)
{
    namespaces.checkConsistency();

    // Don't add namespace attributes twice
    if (attribute.name() == XMLNSNames::xmlnsAttr) {
        namespaces.set(emptyAtom.impl(), attribute.value().impl());
        return false;
    }
    
    QualifiedName xmlnsPrefixAttr(xmlnsAtom, attribute.localName(), XMLNSNames::xmlnsNamespaceURI);
    if (attribute.name() == xmlnsPrefixAttr) {
        namespaces.set(attribute.localName().impl(), attribute.value().impl());
        return false;
    }
    
    return true;
}

void MarkupAccumulator::appendNamespace(Vector<UChar>& result, const AtomicString& prefix, const AtomicString& namespaceURI, Namespaces& namespaces)
{
    namespaces.checkConsistency();
    if (namespaceURI.isEmpty())
        return;
        
    // Use emptyAtoms's impl() for both null and empty strings since the HashMap can't handle 0 as a key
    AtomicStringImpl* pre = prefix.isEmpty() ? emptyAtom.impl() : prefix.impl();
    AtomicStringImpl* foundNS = namespaces.get(pre);
    if (foundNS != namespaceURI.impl()) {
        namespaces.set(pre, namespaceURI.impl());
        result.append(' ');
        append(result, xmlnsAtom.string());
        if (!prefix.isEmpty()) {
            result.append(':');
            append(result, prefix);
        }

        result.append('=');
        result.append('"');
        appendAttributeValue(result, namespaceURI, false);
        result.append('"');
    }
}

EntityMask MarkupAccumulator::entityMaskForText(Text* text) const
{
    const QualifiedName* parentName = 0;
    if (text->parentElement())
        parentName = &static_cast<Element*>(text->parentElement())->tagQName();
    
    if (parentName && (*parentName == scriptTag || *parentName == styleTag || *parentName == xmpTag))
        return EntityMaskInCDATA;

    return text->document()->isHTMLDocument() ? EntityMaskInHTMLPCDATA : EntityMaskInPCDATA;
}

void MarkupAccumulator::appendText(Vector<UChar>& out, Text* text)
{
    appendNodeValue(out, text, m_range, entityMaskForText(text));
}

void MarkupAccumulator::appendComment(Vector<UChar>& out, const String& comment)
{
    // FIXME: Comment content is not escaped, but XMLSerializer (and possibly other callers) should raise an exception if it includes "-->".
    append(out, "<!--");
    append(out, comment);
    append(out, "-->");
}

void MarkupAccumulator::appendDocumentType(Vector<UChar>& result, const DocumentType* n)
{
    if (n->name().isEmpty())
        return;

    append(result, "<!DOCTYPE ");
    append(result, n->name());
    if (!n->publicId().isEmpty()) {
        append(result, " PUBLIC \"");
        append(result, n->publicId());
        append(result, "\"");
        if (!n->systemId().isEmpty()) {
            append(result, " \"");
            append(result, n->systemId());
            append(result, "\"");
        }
    } else if (!n->systemId().isEmpty()) {
        append(result, " SYSTEM \"");
        append(result, n->systemId());
        append(result, "\"");
    }
    if (!n->internalSubset().isEmpty()) {
        append(result, " [");
        append(result, n->internalSubset());
        append(result, "]");
    }
    append(result, ">");
}

void MarkupAccumulator::appendProcessingInstruction(Vector<UChar>& out, const String& target, const String& data)
{
    // FIXME: PI data is not escaped, but XMLSerializer (and possibly other callers) this should raise an exception if it includes "?>".
    append(out, "<?");
    append(out, target);
    append(out, " ");
    append(out, data);
    append(out, "?>");
}

void MarkupAccumulator::appendElement(Vector<UChar>& out, Element* element, Namespaces* namespaces,const String& basePath  /*SAMSUNG_HTML_EDIT_EXTENSION*/, WebHTMLMarkupData* markupData )
{
    appendOpenTag(out, element, namespaces);

    NamedNodeMap* attributes = element->attributes();
    unsigned length = attributes->length();
    for (unsigned int i = 0; i < length; i++)
        appendAttribute(out, element, *attributes->attributeItem(i), namespaces,basePath  /*SAMSUNG_HTML_EDIT_EXTENSION*/,markupData );

    appendCloseTag(out, element);
}

void MarkupAccumulator::appendOpenTag(Vector<UChar>& out, Element* element, Namespaces* namespaces)
{
    out.append('<');
    append(out, element->nodeNamePreservingCase());
    if (!element->document()->isHTMLDocument() && namespaces && shouldAddNamespaceElement(element))
        appendNamespace(out, element->prefix(), element->namespaceURI(), *namespaces);    
}

void MarkupAccumulator::appendCloseTag(Vector<UChar>& out, Element* element)
{
    if (shouldSelfClose(element)) {
        if (element->isHTMLElement())
            out.append(' '); // XHTML 1.0 <-> HTML compatibility.
        out.append('/');
    }
    out.append('>');
}

void MarkupAccumulator::appendAttribute(Vector<UChar>& out, Element* element, const Attribute& attribute, Namespaces* namespaces,const String& basePath  /*SAMSUNG_HTML_EDIT_EXTENSION*/, WebHTMLMarkupData* markupData )
{
    bool documentIsHTML = element->document()->isHTMLDocument();

    out.append(' ');

    if (documentIsHTML)
        append(out, attribute.name().localName());
    else
        append(out, attribute.name().toString());

    out.append('=');

    if (element->isURLAttribute(const_cast<Attribute*>(&attribute))) {
//SISO_HTMLCOMPOSER begin
                    String urlValue;
                    if( (!basePath.isEmpty()) 
			            && checkResourceAvailablity(element, const_cast<Attribute*>(&attribute)) 
			            && !(urlValue = createLocalResource(getCachedResource(element), basePath,markupData)).isEmpty()){
			            ////ANDROID_LOG_PRINT(ANDROID_LOG_DEBUG, "HTML_EDIT", " entered Local Storage Case appendQuotedURLAttributeValue: %s",urlValue.utf8().data());
			            appendQuotedURLAttributeValue(out, urlValue);
                    }
                    else if(markupData != 0
                        && checkResourceAvailablity(element, const_cast<Attribute*>(&attribute))
                        &&  !(urlValue = getCIDData(attribute.value(), getCachedResource(element),markupData)).isEmpty()){
                        ////ANDROID_LOG_PRINT(ANDROID_LOG_DEBUG, "HTML_EDIT", " entered CID Generation case appendQuotedURLAttributeValue: %s",urlValue.utf8().data());
                        appendQuotedURLAttributeValue(out, urlValue);
                    }
//SISO_HTMLCOMPOSER end
        // We don't want to complete file:/// URLs because it may contain sensitive information
        // about the user's system.
//SISO_HTMLCOMPOSER begin
        else if (shouldResolveURLs() && !element->document()->url().isLocalFile())
//SISO_HTMLCOMPOSER end
            appendQuotedURLAttributeValue(out, element->document()->completeURL(attribute.value()).string());
        else
            appendQuotedURLAttributeValue(out, attribute.value()); 
    } else {
        out.append('\"');
        appendAttributeValue(out, attribute.value(), documentIsHTML);
        out.append('\"');
    }

    if (!documentIsHTML && namespaces && shouldAddNamespaceAttribute(attribute, *namespaces))
        appendNamespace(out, attribute.prefix(), attribute.namespaceURI(), *namespaces);
}

void MarkupAccumulator::appendCDATASection(Vector<UChar>& out, const String& section)
{
    // FIXME: CDATA content is not escaped, but XMLSerializer (and possibly other callers) should raise an exception if it includes "]]>".
    append(out, "<![CDATA[");
    append(out, section);
    append(out, "]]>");
}

void MarkupAccumulator::appendStartMarkup(Vector<UChar>& result, const Node* node, Namespaces* namespaces,const String& basePath  /*SAMSUNG_HTML_EDIT_EXTENSION*/, WebHTMLMarkupData* markupData )
{
    if (namespaces)
        namespaces->checkConsistency();

    switch (node->nodeType()) {
    case Node::TEXT_NODE:
        appendText(result, static_cast<Text*>(const_cast<Node*>(node)));
        break;
    case Node::COMMENT_NODE:
        appendComment(result, static_cast<const Comment*>(node)->data());
        break;
    case Node::DOCUMENT_NODE:
    case Node::DOCUMENT_FRAGMENT_NODE:
        break;
    case Node::DOCUMENT_TYPE_NODE:
        appendDocumentType(result, static_cast<const DocumentType*>(node));
        break;
    case Node::PROCESSING_INSTRUCTION_NODE:
        appendProcessingInstruction(result, static_cast<const ProcessingInstruction*>(node)->target(), static_cast<const ProcessingInstruction*>(node)->data());
        break;
    case Node::ELEMENT_NODE:
        appendElement(result, static_cast<Element*>(const_cast<Node*>(node)), namespaces,basePath  /*SAMSUNG_HTML_EDIT_EXTENSION*/, markupData );
        break;
    case Node::CDATA_SECTION_NODE:
        appendCDATASection(result, static_cast<const CDATASection*>(node)->data());
        break;
    case Node::ATTRIBUTE_NODE:
    case Node::ENTITY_NODE:
    case Node::ENTITY_REFERENCE_NODE:
    case Node::NOTATION_NODE:
    case Node::XPATH_NAMESPACE_NODE:
        ASSERT_NOT_REACHED();
        break;
    }
}

// Rules of self-closure
// 1. No elements in HTML documents use the self-closing syntax.
// 2. Elements w/ children never self-close because they use a separate end tag.
// 3. HTML elements which do not have a "forbidden" end tag will close with a separate end tag.
// 4. Other elements self-close.
bool MarkupAccumulator::shouldSelfClose(const Node* node)
{
    if (node->document()->isHTMLDocument())
        return false;
    if (node->hasChildNodes())
        return false;
    if (node->isHTMLElement() && !elementCannotHaveEndTag(node))
        return false;
    return true;
}

bool MarkupAccumulator::elementCannotHaveEndTag(const Node* node)
{
    if (!node->isHTMLElement())
        return false;
    
    // FIXME: ieForbidsInsertHTML may not be the right function to call here
    // ieForbidsInsertHTML is used to disallow setting innerHTML/outerHTML
    // or createContextualFragment.  It does not necessarily align with
    // which elements should be serialized w/o end tags.
    return static_cast<const HTMLElement*>(node)->ieForbidsInsertHTML();
}

void MarkupAccumulator::appendEndMarkup(Vector<UChar>& result, const Node* node)
{
    if (!node->isElementNode() || shouldSelfClose(node) || (!node->hasChildNodes() && elementCannotHaveEndTag(node)))
        return;

    result.append('<');
    result.append('/');
    append(result, static_cast<const Element*>(node)->nodeNamePreservingCase());
    result.append('>');
}

}
