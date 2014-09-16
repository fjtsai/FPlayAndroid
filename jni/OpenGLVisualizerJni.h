//
// FPlayAndroid is distributed under the FreeBSD License
//
// Copyright (c) 2013-2014, Carlos Rafael Gimenes das Neves
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// The views and conclusions contained in the software and documentation are those
// of the authors and should not be interpreted as representing official policies,
// either expressed or implied, of the FreeBSD Project.
//
// https://github.com/carlosrafaelgn/FPlayAndroid
//

//https://www.khronos.org/opengles/sdk/docs/man3/docbook4/xhtml/glDeleteBuffers.xml
//https://www.khronos.org/opengles/sdk/docs/man3/docbook4/xhtml/glVertexAttribPointer.xml
//https://www.khronos.org/opengles/sdk/docs/man3/docbook4/xhtml/glTexImage2D.xml
//https://www.khronos.org/opengles/sdk/docs/man3/docbook4/xhtml/glGenBuffers.xml

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

static unsigned int glProgram, glVShader, glFShader, glTex[2], glBuf[2], glColorIndex;
static float glNew;

void JNICALL glChangeColorIndex(JNIEnv* env, jclass clazz, int jcolorIndex) {
	glColorIndex = jcolorIndex;
}

void JNICALL glChangeSpeed(JNIEnv* env, jclass clazz, int speed) {
	switch (speed) {
	case 1:
		glNew = 0.09375f / 16.0f; //0.09375 @ 60fps (~16ms)
		break;
	case 2:
		glNew = 0.140625f / 16.0f;
		break;
	default:
		glNew = 0.0625f / 16.0f;
		break;
	}
}

int JNICALL glOnSurfaceCreated(JNIEnv* env, jclass clazz, int bgColor) {
	const float bgR = (float)((bgColor >> 16) & 0xff) / 255.0f;
	const float bgG = (float)((bgColor >> 8) & 0xff) / 255.0f;
	const float bgB = (float)(bgColor & 0xff) / 255.0f;
	int l;
	glProgram = glCreateProgram();
	if (glGetError() || !glProgram) return -1;
	glVShader = glCreateShader(GL_VERTEX_SHADER);
	if (glGetError() || !glVShader) return -2;
	glFShader = glCreateShader(GL_FRAGMENT_SHADER);
	if (glGetError() || !glFShader) return -3;
	const char* vertexShader = "attribute vec4 inPosition; attribute vec2 inTexCoord; varying vec2 vTexCoord; void main() { gl_Position = inPosition; vTexCoord = inTexCoord; }";
	l = strlen(vertexShader);
	glShaderSource(glVShader, 1, &vertexShader, &l);
	if (glGetError()) return -4;
	//memory reuse FTW! :)
	sprintf((char*)floatBuffer, "precision mediump float; uniform sampler2D texAmplitude; uniform sampler2D texColor; varying vec2 vTexCoord; void main() {" \
	"vec4 c = texture2D(texAmplitude, vec2(" \
	"vTexCoord.x" \
	/*"(vTexCoord.x < 0.25) ? (vTexCoord.x * 0.5) : (((vTexCoord.x - 0.25) * 1.167) + 0.125)"*/ \
	", 0.0)); if (vTexCoord.y <= c.a && vTexCoord.y >= -c.a) {" \
	"gl_FragColor = texture2D(texColor, c.ar);" \
	"} else {" \
	"gl_FragColor = vec4(%.8f, %.8f, %.8f, 1.0);" \
	"} }", bgR, bgG, bgB);
	l = strlen((char*)floatBuffer);
	const char* fragmentShader = (char*)floatBuffer;
	glShaderSource(glFShader, 1, &fragmentShader, &l);
	if (glGetError()) return -5;
	glCompileShader(glVShader);
	if (glGetError()) return -6;
	glCompileShader(glFShader);
	if (glGetError()) return -7;
	glAttachShader(glProgram, glVShader);
	if (glGetError()) return -8;
	glAttachShader(glProgram, glFShader);
	if (glGetError()) return -9;
	glBindAttribLocation(glProgram, 0, "inPosition");
	if (glGetError()) return -10;
	glBindAttribLocation(glProgram, 1, "inTexCoord");
	if (glGetError()) return -11;
	glLinkProgram(glProgram);
	if (glGetError()) return -12;

	//create a rectangle that occupies the entire screen
	#define left -1.0f
	#define top 1.0f
	#define right 1.0f
	#define bottom -1.0f
	#define z 0.0f
	const float vertices[] = {
		left, bottom, z, 1.0f,
		right, bottom, z, 1.0f,
		left, top, z, 1.0f,
		right, top, z, 1.0f
	};
	#undef left
	#undef top
	#undef right
	#undef bottom
	#undef z
	#define leftTex 0.0f
	#define topTex 1.0f
	#define rightTex 1.0f
	#define bottomTex -1.0f
	const float texCoords[] = {
		leftTex, bottomTex,
		rightTex, bottomTex,
		leftTex, topTex,
		rightTex, topTex
	};
	#undef leftTex
	#undef topTex
	#undef rightTex
	#undef bottomTex
	glBuf[0] = 0;
	glBuf[1] = 0;
	glGenBuffers(2, glBuf);
	if (glGetError() || !glBuf[0] || !glBuf[1]) return -13;
	glBindBuffer(GL_ARRAY_BUFFER, glBuf[0]);
	glBufferData(GL_ARRAY_BUFFER, (4 << 2) * sizeof(float), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, glBuf[1]);
	glBufferData(GL_ARRAY_BUFFER, (4 << 1) * sizeof(float), texCoords, GL_STATIC_DRAW);
	if (glGetError()) return -14;
	
	glClearColor(bgR, bgG, bgB, 1.0f);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DITHER);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_BLEND);
	//glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);
	glGetError(); //clear any eventual error flags
	
	glTex[0] = 0;
	glTex[1] = 0;
	glGenTextures(2, glTex);
	if (glGetError() || !glTex[0] || !glTex[1]) return -15;
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, glTex[0]);
	if (glGetError()) return -16;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	memset(floatBuffer + 512, 0, 256);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 256, 1, 0, GL_ALPHA, GL_UNSIGNED_BYTE, (unsigned char*)(floatBuffer + 512));
	if (glGetError()) return -17;

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, glTex[1]);
	if (glGetError()) return -18;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	if (glGetError()) return -19;
	
	glUseProgram(glProgram);
	glUniform1i(glGetUniformLocation(glProgram, "texAmplitude"), 0);
	glUniform1i(glGetUniformLocation(glProgram, "texColor"), 1);
	if (glGetError()) return -20;
	
	//leave everything prepared for fast drawing :)
	glActiveTexture(GL_TEXTURE0);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, glBuf[0]);
	glVertexAttribPointer(0, 4, GL_FLOAT, false, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, glBuf[1]);
	glVertexAttribPointer(1, 2, GL_FLOAT, false, 0, 0);
	
	glColorIndex = 257;
	colorIndex = 0;
	glChangeSpeed(env, clazz, 2);
	updateMultiplier(env, clazz, 0);
	
	return 0;
}

void JNICALL glOnSurfaceChanged(JNIEnv* env, jclass clazz, int width, int height) {
	glViewport(0, 0, width, height);
}

void JNICALL glProcess(JNIEnv* env, jclass clazz, jbyteArray jbfft, int deltaMillis) {
	float* const fft = floatBuffer;
	const float* const multiplier = fft + 256;
	//fft format:
	//index  0   1    2  3  4  5  ..... n-2        n-1
	//       Rdc Rnyq R1 I1 R2 I2       R(n-1)/2  I(n-1)/2
	signed char* const bfft = (signed char*)env->GetPrimitiveArrayCritical(jbfft, 0);
	if (!bfft)
		return;
	const float coefNew = glNew * (float)deltaMillis;
	const float coefOld = 1.0f - coefNew;
	//*** we are not drawing/analyzing the last bin (Nyquist) ;) ***
	bfft[1] = bfft[0];
	for (int i = 0; i < 256; i++) {
		//bfft[i] stores values from 0 to -128/127 (inclusive)
		const int re = (int)bfft[i << 1];
		const int im = (int)bfft[(i << 1) + 1];
		
		float m = multiplier[i] * sqrtf((float)((re * re) + (im * im)));
		//2048/32768 = 0.0625
		//float m = (((multiplier[i] * sqrtf((float)((re * re) + (im * im)))) - 2048.0f) * 1.0625f) + 2048.0f;
		//if (m < 0)
		//	m = 0;
		
		const float old = fft[i];
		if (m < old)
			m = (coefNew * m) + (coefOld * old);
			//m = (0.28125f * m) + (0.71875f * old);
		fft[i] = m;
		//v goes from 0 to 32768 (inclusive)
		const int v = (int)m;
		((unsigned char*)(floatBuffer + 512))[i] = ((v >= (255 << 7)) ? 255 : ((m <= 0) ? 0 : (unsigned char)(v >> 7)));
	}
	env->ReleasePrimitiveArrayCritical(jbfft, bfft, JNI_ABORT);
}

void JNICALL glDrawFrame(JNIEnv* env, jclass clazz) {
	if (colorIndex != glColorIndex) {
		colorIndex = glColorIndex;
		glActiveTexture(GL_TEXTURE1);
		for (int i = 0, idx = colorIndex; i < 256; i++, idx++) {
			const unsigned int c = (unsigned int)COLORS[idx];
			unsigned int r = (c >> 11) & 0x1f;
			unsigned int g = (c >> 5) & 0x3f;
			unsigned int b = c & 0x1f;
			r = ((r << 3) | (r >> 2));
			g = ((g << 2) | (g >> 4)) << 8;
			b = ((b << 3) | (b >> 2)) << 16;
			((unsigned int*)floatBuffer)[i] = 0xff000000 | r | g | b;
		}
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, (unsigned char*)floatBuffer);
		updateMultiplier(env, clazz, 0);
		glActiveTexture(GL_TEXTURE0);
	}
	//glUseProgram(glProgram);
	//glClear(GL_COLOR_BUFFER_BIT);
	//glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, glTex[0]);
	//glActiveTexture(GL_TEXTURE1);
	//glBindTexture(GL_TEXTURE_2D, glTex[1]);
	//glEnableVertexAttribArray(0);
	//glEnableVertexAttribArray(1);
	//refresh the texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 256, 1, 0, GL_ALPHA, GL_UNSIGNED_BYTE, (unsigned char*)(floatBuffer + 512));
	//draw the rectangle (4 vertices)
	//glBindBuffer(GL_ARRAY_BUFFER, glBuf[0]);
	//glVertexAttribPointer(0, 4, GL_FLOAT, false, 0, 0);
	//glBindBuffer(GL_ARRAY_BUFFER, glBuf[1]);
	//glVertexAttribPointer(1, 2, GL_FLOAT, false, 0, 0);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
