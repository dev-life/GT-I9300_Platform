/*-----------------------------------------------------------------------------
 * Copyright (c) 2005-2011, Cambridge Silicon Radio Limited All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *	  * Redistributions of source code must retain the above copyright
 *		notice, this list of conditions and the following disclaimer.
 *	  * Redistributions in binary form must reproduce the above copyright
 *		notice, this list of conditions and the following disclaimer in the
 *		documentation and/or other materials provided with the distribution.
 *	  * Neither the name of the Cambridge Silicon Radio Limited nor the
 *		names of its contributors may be used to endorse or promote products
 *		derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL CAMBRIDGE SILICON RADIO LIMITED BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *----------------------------------------------------------------------------*/
 /*
 ###############################################################
 # $CSR Belfast Revision: 001, Android 4.0.x/BlueZ 4.93$
 # $Date: 2012-02-02 09:21:47 +0100 (Thu, 02 Feb 2012) $
 # $Last modified by Ashish Meena@CSR Belfast$
 ################################################################
*/
#if !defined (APTX_ENC_H)
#define APTX_ENC_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _DLLEXPORT
#define APTXBTENCEXPORT __declspec(dllexport)
#else
#define APTXBTENCEXPORT 
#endif

/* StereoEncode will take 8 audio samples (16-bit per sample)
 * and generate one 32-bit codeword with autosync inserted.
 * The bitstream is compatible with be BC05 implementation.
 */

int APTXBTENCEXPORT aptxbtenc_encodestereo(void* _state, void* _pcmL,
		void* _pcmR, void* _buffer);

/* aptxbtenc_version can be used to extract the version number
 * of the apt-X encoder
 */

const char* APTXBTENCEXPORT aptxbtenc_version(void);

/* aptxbtenc_build can be used to extract the build number
 * of the apt-X encoder
 */

const char* APTXBTENCEXPORT aptxbtenc_build(void);

/* NewAptxEnc will create the required structure to run the 
 * aptx encoder. The function will return a pointer to that structure.
 * endian: type boolean : 0 = output is little endian, 
 *                        1 = output is big endian
 */
void APTXBTENCEXPORT * NewAptxEnc(short endian);

/*  aptxbtenc_init is used to initialise the encoder structure.
 * _state should be a pointer to the encoder structure (stereo).
 * endian represent the endianness of the output data 
 * (0=little endian. Big endian otherwise)
 * The function returns 1 if an error occured during the initialisation.
 * The function returns 0 if no error occured during the initialisation.
 */

int APTXBTENCEXPORT aptxbtenc_init(void* _state, short endian);

/* SizeofAptxbtenc returns the size (in byte) of the memory
 * allocation required to store the state of the encoder
 */
int APTXBTENCEXPORT SizeofAptxbtenc(void);

typedef void* aptX_creator(short);
typedef void aptX_disposer(void*);
typedef int aptX_StereoEncode(void*, void*, void*, void*);
#ifdef __cplusplus
}extern "C"
#endif

#endif  // APTX_ENC_H

