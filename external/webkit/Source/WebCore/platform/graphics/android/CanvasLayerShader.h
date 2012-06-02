/*
Copyright (c) 2011, Code Aurora Forum. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of Code Aurora Forum, Inc. nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef CanvasLayerShader_h
#define CanvasLayerShader_h

#if USE(ACCELERATED_COMPOSITING)

#include "FloatRect.h"
#include "IntRect.h"
#include "SkRect.h"
#include "TransformationMatrix.h"
#include <GLES2/gl2.h>
#include <vector>
#include <map>

namespace WebCore{

class CanvasLayerShader {

public:
    CanvasLayerShader();
    void initialize();
    int getAlpha()  { return m_suAlpha;   }
    int getSampler() { return m_suSampler; }
    int getProgram()    { return m_program; }

    // Drawing
    void setViewport(SkRect& viewport);
    void setTitleBarHeight(int& height) {   m_titleBarHeight = height; }
    void setViewRect(IntRect& rect)     {   m_viewRect = rect;  }
    void setWebViewRect(IntRect& rect)  {   m_webViewRect = rect;   }
    void setClipRect(FloatRect& rect)   {   m_clipRect = rect;  }
    void setScreenClip(IntRect& rect)   {   m_screenClip = rect;    }
    void setDocumentViewport(FloatRect& rect)   {   m_documentViewport = rect;  }
    void setAlphaLayer(bool& val)       {   m_alphaLayer = val; }
    void setScale(float& val)           {   m_currentScale = val;   }
    void setRepositionMatrix(TransformationMatrix& matrix)  {   m_repositionMatrix = matrix;    }
    void setWebViewMatrix(TransformationMatrix& matrix)     {   m_webViewMatrix = matrix;   }

    bool drawPrimitives(std::vector<SkRect>& primitives, std::vector<FloatRect>& texturecoords,
                            std::vector<int>& primScaleX, std::vector<int>& primScaleY,
                            int textureId, TransformationMatrix& matrix, float opacity);
    void cleanupData(int textureId);
    void resetBlending();

private:
   //Shader stuff
   GLuint loadShader(GLenum shaderType, const char* shaderSource);
   GLuint createProgram(const char* vSource, const char* fSource);

   //helpers
   void setBlendingState(bool enableBlending);

private:
   //Data
   TransformationMatrix m_projectionMatrix;

   GLuint m_vertexBuffer[2];
   int m_numVertices;

   //multiple bitmap framework
   std::map<int, GLuint> m_gvertex_buffer_map;
   std::map<int, GLuint> m_gtexture_buffer_map;
   std::map<int, GLfloat*> m_vertex_buffer_data_map;
   std::map<int, GLfloat*> m_texture_buffer_data_map;
   std::map<int, int> m_numVertices_map;

   int m_program;
   bool m_blendingEnabled;

   //uniforms
   int m_suAlpha;
   int m_suSampler;

   //attribs
   GLint m_saPos;
   GLint m_saTexCoords;

   SkRect m_viewport;
   int m_titleBarHeight;
   IntRect m_viewRect;
   IntRect m_webViewRect;
   FloatRect m_documentViewport;
   bool m_alphaLayer;
   TransformationMatrix m_webViewMatrix;
   float m_currentScale;
   TransformationMatrix m_repositionMatrix;
   FloatRect m_clipRect;
   IntRect m_screenClip;
};   // class CanvasLayerShader

}   // namespace WebCore

#endif // if USE(ACCELERATED_COMPOSITING)

#endif // define CanvasLayerShader_h
