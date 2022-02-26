// Copyright (c) 2022 Sultim Tsyrendashiev
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


#if !defined(EFFECT_SOURCE_IS_PING) && !defined(EFFECT_SOURCE_IS_PONG) 
    #error Define EFFECT_SOURCE_IS_PING or EFFECT_SOURCE_IS_PONG to boolean value
#endif


vec2 getInverseEffectSourceSize()
{
    ivec2 sz = imageSize(framebufUpscaledPing); // framebufUpscaledPong has the same size
    return vec2(1.0 / float(sz.x), 1.0 / float(sz.y));
}


// get UV coords in [0..1] range
vec2 getEffectSourceUV(ivec2 pix)
{
    return (vec2(pix) + 0.5) * getInverseEffectSourceSize();
}


vec3 loadFromEffectSource(ivec2 pix)
{
    if (EFFECT_SOURCE_IS_PING)
    {
        return imageLoad(framebufUpscaledPing, pix).rgb;
    }
    else
    {
        return imageLoad(framebufUpscaledPong, pix).rgb;
    }
}


void storeToEffectTarget(const vec3 value, ivec2 pix)
{
    if (EFFECT_SOURCE_IS_PING)
    {
        imageStore(framebufUpscaledPong, pix, vec4(value, 0.0));
    }
    else
    {
        imageStore(framebufUpscaledPing, pix, vec4(value, 0.0));
    }
}


#ifdef DESC_SET_RANDOM
vec4 getEffectRandomSample(ivec2 pix, uint frameIndex, ivec2 imgSize)
{
    return getRandomSample(getRandomSeed(pix, frameIndex, imgSize.x, imgSize.y), 0);
}
#endif